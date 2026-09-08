#!/usr/bin/env python3
"""Source contracts for account password recovery by email.

Pure source checks, no server, no database, no compiler.  Pins are made against
CODE: comments are stripped before any body is searched (a comment mentioning a
call is not a call), and where a pin is about the arguments of a log call the
string literals are stripped too, so the prose in a format string can neither
satisfy nor trip an identifier pin.

Each pin guards an invariant the design review named as a defect class:

  * the one player-visible answer to a reset request is one macro, and the code
    rejection is one text -- no email-on-file oracle, no attempts-remaining
    count, no "capped" variant on telnet;
  * nothing logs the code, the address, the host or the account name, and the
    mail worker logs nothing at all;
  * the code is RAND_bytes + SHA256 + CRYPTO_memcmp, never number() or CRYPT2();
  * libcurl is the only mail path (no shell), every send is bounded, TLS is
    never weakened, credentials have no compiled default;
  * the worker exits on the stop flag whatever its backlog;
  * the per-descriptor attempt counter is zeroed only by close_socket and a
    completed reset;
  * the apply helper never dereferences its acct_name argument after
    write_account, whose re-read loop frees and reallocates it;
  * the boot / pulse / shutdown / close_socket wiring, the CON_ table, the
    Makefile, the packaging, the example environment and the documentation all
    carry the feature.

Run alone with `python3 tests/async/test_account_recovery_contract.py`; exits 1
on any FAIL line.
"""

import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from _paths import ROOT, source  # noqa: E402
from _source_contract import block_start, function_body, strip_comments  # noqa: E402

SRC_ROOT = ROOT / "src"

failures = []


def check(ok: bool, label: str, extra: str = "") -> None:
    """Print one OK/FAIL line and record a failure for the exit status."""
    if ok:
        print(f"OK: {label}")
    else:
        failures.append(label)
        print(f"FAIL: {label}" + (f"\n      {extra}" if extra else ""))


def read_repo(rel: str) -> str:
    return (ROOT / rel).read_text(encoding="latin-1")


def read_source(name: str) -> str:
    return source(name).read_text(encoding="latin-1")


def statement_present(text: str, call: str) -> bool:
    """True when `call` (a regex for the call expression, no trailing ';')
    begins a statement: start of line, optional whitespace, the call, then
    a ';' closing it (possibly on a later line, as long as no '{' or another
    ';' intervenes). Lines that begin with '*' or '//' are comment lines and
    never count, and the text is comment-stripped anyway."""
    code = strip_comments(text)
    pattern = re.compile(r"^[ \t]*(" + call + r")", re.M)
    for m in pattern.finditer(code):
        line_start = code.rfind("\n", 0, m.start()) + 1
        line = code[line_start : code.find("\n", m.start())].lstrip()
        if line.startswith("*") or line.startswith("//"):
            continue
        tail = code[m.end() : m.end() + 400]
        semi = tail.find(";")
        if semi < 0:
            continue
        if "{" in tail[:semi]:
            continue
        return True
    return False


def body_of(text: str, signature: str, label: str) -> str:
    """The comment-stripped body of the first DEFINITION matching `signature`;
    a missing definition is a failure and yields "" so later pins fail too."""
    body = function_body(text, signature)
    check(body is not None, f"{label} is defined")
    return body or ""


def ordered(text: str, *needles: str) -> bool:
    """Every needle present, first occurrences in strictly increasing order."""
    indices = [text.find(needle) for needle in needles]
    return all(index >= 0 for index in indices) and indices == sorted(indices) and len(
        set(indices)
    ) == len(indices)


def strip_strings(code: str) -> str:
    """Blank string literals so identifier pins never match prose in a format."""
    return re.sub(r'"(?:[^"\\\n]|\\.)*"', '""', code)


def call_arguments(code: str, name: str):
    """The argument text of every `name(` call in already comment-stripped code,
    parentheses balanced."""
    for match in re.finditer(r"(?<![A-Za-z0-9_])" + re.escape(name) + r"\s*\(", code):
        depth = 0
        start = match.end()
        for index in range(start - 1, len(code)):
            char = code[index]
            if char == "(":
                depth += 1
            elif char == ")":
                depth -= 1
                if depth == 0:
                    yield code[start:index]
                    break


def enclosing_function(code: str, pos: int) -> str:
    """The body of the innermost brace block containing `pos` whose head ends in a
    parameter list -- the function `pos` is in -- or "" when there is none."""
    while True:
        start = block_start(code, pos)
        if start <= 0:
            return ""
        head = code[: start - 1].rstrip()
        if re.search(r"\)\s*(const|noexcept|override)?$", head):
            depth = 0
            for index in range(start - 1, len(code)):
                if code[index] == "{":
                    depth += 1
                elif code[index] == "}":
                    depth -= 1
                    if depth == 0:
                        return code[start - 1 : index + 1]
            return ""
        pos = start - 1


def enclosing_block_heads(code: str, pos: int) -> list:
    """The text just before each '{' that encloses `pos`, innermost first."""
    heads = []
    while True:
        start = block_start(code, pos)
        if start <= 0:
            return heads
        heads.append(code[: start - 1].rstrip()[-80:])
        pos = start - 1


def ifdef_regions(text: str, macro: str) -> list:
    """(start, end) character ranges in which `#ifdef macro` is the active arm."""
    regions = []
    stack = []
    for match in re.finditer(r"^[ \t]*#[ \t]*(ifdef|ifndef|if|elif|else|endif)\b(.*)$", text, re.M):
        directive, rest = match.group(1), match.group(2)
        if directive in ("ifdef", "ifndef", "if"):
            target = directive == "ifdef" and rest.strip() == macro
            stack.append([target, match.start() if target else None])
        elif directive in ("else", "elif"):
            if stack and stack[-1][0] and stack[-1][1] is not None:
                regions.append((stack[-1][1], match.start()))
                stack[-1][1] = None
        elif directive == "endif" and stack:
            target, start = stack.pop()
            if target and start is not None:
                regions.append((start, match.end()))
    return regions


def literal_reaches(body: str, file_text: str, literal: str) -> bool:
    """True when `literal` is written in `body` directly, or `body` uses a macro
    that `file_text` #defines to exactly that literal (a hoisted format string)."""
    if literal in body:
        return True
    for match in re.finditer(r"#define\s+(\w+)\s*\\?\s*\"" + re.escape(literal) + r"\"", file_text):
        if re.search(r"\b" + match.group(1) + r"\b", body):
            return True
    return False


def source_files():
    """Every .c/.h under src/, comment-stripped, keyed by repo-relative path."""
    files = {}
    for path in sorted(SRC_ROOT.rglob("*")):
        if path.is_file() and path.suffix in (".c", ".h"):
            files[path.relative_to(ROOT).as_posix()] = strip_comments(
                path.read_text(encoding="latin-1")
            )
    return files


# Identifiers that must never appear in the argument list of a log-like call.
FORBIDDEN_LOG_ARGUMENT_TOKENS = (
    r"\bacct_email\b",
    r"\bemail\b",
    r"->to\b",
    r"\.to\b",
    r"\bcode\b",
    r"\bnormalized\b",
    r"\bhost\b",
    r"\bacct_name\b",
    r"\bname\b",
    r"\bmessage\b",
    r"\bbody\b",
    r"\bpassword\b",
    r"pending_hash",
    r"account_recovery_code",
    r"curl_easy_strerror",
)
LOG_LIKE_CALLS = ("logit", "statuslog", "wizlog", "persistence_alert", "fprintf", "printf")


header_raw = read_source("account_recovery.h")
header = strip_comments(header_raw)
core_raw = read_source("account_recovery.c")
core = strip_comments(core_raw)
nanny_raw = read_source("account_recovery_nanny.c")
nanny = strip_comments(nanny_raw)
mailer_raw = read_source("mail_sender.c")
mailer = strip_comments(mailer_raw)
mail_header = strip_comments(read_source("mail_sender.h"))
account = strip_comments(read_source("account.c"))
account_h = strip_comments(read_source("account.h"))
ws = strip_comments(read_source("ws_handlers.c"))
comm = strip_comments(read_source("comm.c"))
structs = strip_comments(read_source("structs.h"))
constant = strip_comments(read_source("constant.c"))
nanny_c = strip_comments(read_source("nanny.c"))
all_sources = source_files()


# --------------------------------------------------------------------- *
# (1) one uniform text, one place, used by the telnet entry
# --------------------------------------------------------------------- *


def test_uniform_text() -> None:
    first_line = "If that account has an email address on file, a reset code has been sent to it."
    carriers = [path for path, text in all_sources.items() if first_line in text]
    check(
        carriers == ["src/account/account_recovery.h"],
        "the uniform reset text is defined exactly once under src (account_recovery.h)",
        f"carriers: {carriers}",
    )
    check(
        "#define ACCOUNT_RECOVERY_UNIFORM_TEXT" in header,
        "ACCOUNT_RECOVERY_UNIFORM_TEXT is a macro in account_recovery.h",
    )
    begin = body_of(
        nanny,
        r"\bvoid\s+account_recovery_begin_from_password_prompt\s*\(",
        "account_recovery_begin_from_password_prompt",
    )
    check(
        "ACCOUNT_RECOVERY_UNIFORM_TEXT" in begin,
        "the telnet '?' entry prints ACCOUNT_RECOVERY_UNIFORM_TEXT (code, not a comment)",
    )
    check(
        "queued" in begin and "suppressed" in begin and "capacity" in begin
        and "invalid_name" in begin,
        "the telnet entry routes queued/suppressed/capacity/invalid_name through one path",
    )
    check(
        "Wait 10 minutes" in begin and "host_limited" in begin,
        "the host-limited answer is the only outcome-specific text on telnet",
    )
    check(
        "account_recovery_credential_fingerprint(" in begin,
        "the telnet entry captures the credential fingerprint at request time",
    )
    check(
        "account_recovery_attempts" not in begin,
        "the '?' entry does not touch the per-descriptor attempt counter",
    )


# --------------------------------------------------------------------- *
# (2) one rejection text; capped is invisible; WS ordering
# --------------------------------------------------------------------- *


def test_code_entry_texts() -> None:
    enter = body_of(
        nanny, r"\bvoid\s+account_recovery_enter_code\s*\(", "account_recovery_enter_code"
    )
    check(
        enter.count("not accepted") == 1,
        "account_recovery_enter_code carries exactly one 'not accepted' literal",
    )
    check(
        "\\r\\nThat code was not accepted.\\r\\nEnter the reset code (or CANCEL): " in enter,
        "the S2 rejection is one literal: rejection text plus re-prompt",
    )
    check(
        "Too many incorrect codes" not in enter,
        "telnet renders a capped token exactly like a mismatch (no 'Too many incorrect codes')",
    )
    check("no active" not in enter, "no 'no active code' variant (token-exists oracle)")
    check(
        re.search(r"%\d*[dulli].{0,30}attempt|attempt.{0,30}%\d*[dulli]", enter) is None
        and "remain" not in enter,
        "no attempts-remaining count in the code prompt",
    )
    check(
        enter.count("Too many attempts. Disconnecting.") == 1,
        "the per-descriptor cap is the only terminal exit from S2",
    )
    check(
        ordered(enter, "account_recovery_attempts >=", "++d->account_recovery_attempts",
                "account_recovery_check("),
        "the per-descriptor cap is checked before the increment, before the code check",
    )
    check(
        "account_recovery_descriptor_closed(" not in enter,
        "the S2 cap exit cleanses; only close_socket zeroes the counter",
    )
    check(
        "account_recovery_descriptor_cleanse(" in enter or "return_to_password_prompt(" in enter
        or "disconnect_from_recovery(" in enter,
        "S2 exits cleanse the descriptor's code buffer",
    )
    # never echo what was typed, never print a code
    for arguments in call_arguments(strip_strings(nanny), "SEND_TO_Q"):
        for token in (r"\barg\b", r"account_recovery_code", r"acct_email", r"acct_name",
                      r"\bhost\b", r"pending_hash"):
            check(
                re.search(token, arguments) is None,
                f"no SEND_TO_Q in account_recovery_nanny.c prints {token}",
                arguments.strip()[:80],
            )
    check("sprintf(" not in nanny, "account_recovery_nanny.c formats nothing into a buffer")

    complete = body_of(ws, r"\bvoid\s+ws_cmd_complete_reset\s*\(", "ws_cmd_complete_reset")
    check(
        "Invalid or expired reset code" in complete and "Missing reset fields" in complete,
        "ws_cmd_complete_reset carries the one code-failure text and the missing-fields text",
    )
    check(
        ordered(complete, "strlen(new_password) < 6", "account_recovery_attempts >=",
                "account_recovery_check(", "bcrypt_hash_password(", "account_recovery_complete("),
        "WS order: password length -> descriptor cap -> code check -> bcrypt -> apply",
    )
    check(
        "Too many incorrect" not in complete and "remain" not in complete,
        "WS complete_reset has no capped or attempts-remaining variant",
    )
    request = body_of(ws, r"\bvoid\s+ws_cmd_request_reset\s*\(", "ws_cmd_request_reset")
    reply = body_of(ws, r"\bstatic\s+void\s+ws_request_reset_reply\s*\(", "ws_request_reset_reply")
    decoy = body_of(ws, r"\bstatic\s+void\s+ws_request_reset_decoy\s*\(", "ws_request_reset_decoy")
    check(
        request.count('"reset_requested"') >= 2 and '"reset_requested"' in reply,
        "WS request_reset answers reset_requested for junk, unknown, unreadable and real names",
    )
    check(
        request.count("ws_request_reset_decoy(") >= 3 and "account_recovery_request(" in decoy
        and "NULL, 0," in decoy,
        "WS request_reset charges the host window for unknown/unloadable names via the decoy",
    )
    check(
        "wait 10 minutes" in reply and "ws_request_reset_reply(" in request,
        "WS request_reset carries the host-limited 'wait 10 minutes' text",
    )
    check(
        "account_recovery_credential_fingerprint(" in request,
        "WS request_reset captures the credential fingerprint from the scratch account",
    )


# --------------------------------------------------------------------- *
# (3) logging hygiene
# --------------------------------------------------------------------- *


def test_log_hygiene() -> None:
    apply_body = function_body(
        account, r"account_recovery_apply_outcome\s+account_apply_recovered_password\s*\("
    ) or ""
    for label, text in (
        ("account_recovery.c", core),
        ("account_recovery_nanny.c", nanny),
        ("mail_sender.c", mailer),
        ("account_apply_recovered_password", apply_body),
    ):
        stringless = strip_strings(text)
        for call in LOG_LIKE_CALLS:
            for arguments in call_arguments(stringless, call):
                for token in FORBIDDEN_LOG_ARGUMENT_TOKENS:
                    check(
                        re.search(token, arguments) is None,
                        f"{label}: no {call}( argument mentions {token}",
                        arguments.strip()[:80],
                    )
        # C14: uint64_t is printed as %llu with an explicit cast, never PRIu64
        check("PRIu64" not in text, f"{label}: no PRIu64 (logit is format(printf))")
        for call in ("logit", "statuslog"):
            for arguments in call_arguments(text, call):
                if "%llu" in arguments:
                    check(
                        "unsigned long long" in arguments,
                        f"{label}: every %llu in a {call} is fed an (unsigned long long) cast",
                        arguments.strip()[:80],
                    )
    for call in ("logit(", "statuslog(", "wizlog(", "persistence_alert(", "printf("):
        check(call not in mailer, f"mail_sender.c never calls {call}")
    check(
        "P_desc" not in mailer and "descriptor_data" not in mailer,
        "mail_sender.c knows no descriptor",
    )
    config_body = body_of(
        mailer, r"\bbool\s+mail_sender_config_from_env\s*\(", "mail_sender_config_from_env"
    )
    check(
        mailer.count("getenv(") == config_body.count("getenv(") and config_body.count("getenv(") >= 7,
        "getenv( appears only inside mail_sender_config_from_env, once per MAIL_* key",
    )
    engine_includes = [
        line for line in re.findall(r'#include\s+"([^"]+)"', mailer)
        if line != "net/mail_sender.h"
    ]
    check(engine_includes == [], "mail_sender.c includes no engine header", str(engine_includes))
    for module, text in (("account_recovery.c", core), ("account_recovery_nanny.c", nanny),
                         ("mail_sender.c", mailer)):
        check("persistence_alert(" not in text, f"{module}: no persistence_alert for mail failures")
    check(
        "SEND_TO_Q" not in core and "ws_send" not in core and "descriptor_list" not in core,
        "the core emits nothing to a player and walks no descriptor",
    )
    check(
        "(account=redacted)" in core,
        "core log lines carry the literal (account=redacted) marker",
    )


# --------------------------------------------------------------------- *
# (4)-(7) crypto, no shell, bounded libcurl, worker exit rule
# --------------------------------------------------------------------- *


def test_crypto_and_transport() -> None:
    for token in ("RAND_bytes(", "SHA256(", "CRYPTO_memcmp(", "OPENSSL_cleanse("):
        check(token in core, f"account_recovery.c uses {token}")
    for pattern, label in (
        (r"(?<![A-Za-z0-9_])number\s*\(", "number("),
        (r"(?<![A-Za-z0-9_])CRYPT2\s*\(", "CRYPT2("),
        (r"(?<![A-Za-z0-9_])bcrypt_hash_password\s*\(", "bcrypt_hash_password("),
        (r"strcmp\s*\(\s*\w*digest", "strcmp(digest"),
        (r"(?<![A-Za-z0-9_])memcmp\s*\(", "plain memcmp("),
    ):
        check(re.search(pattern, core) is None, f"account_recovery.c never uses {label}")
    check("inet_pton(AF_INET6" in core, "the host window keys IPv6 on its /64 (inet_pton)")
    check("host_slots_recycled" in core, "live host-slot recycles are counted")
    for module, text in (("account_recovery.c", core), ("account_recovery_nanny.c", nanny),
                         ("mail_sender.c", mailer)):
        check(
            re.search(r"(?<![A-Za-z0-9_])(system|popen|fork|execl|execv)\s*\(", text) is None,
            f"{module}: no shell or process spawn",
        )
    for token in (
        "CURLOPT_TIMEOUT_MS", "CURLOPT_CONNECTTIMEOUT_MS", "CURLOPT_NOSIGNAL",
        "CURLOPT_PROTOCOLS_STR", "CURLOPT_MAIL_RCPT", "CURLOPT_MAIL_FROM", "CURLOPT_USE_SSL",
        "CURLUSESSL_ALL", "curl_url_set", "std::call_once", "CURLOPT_UPLOAD",
        "CURLOPT_READFUNCTION",
    ):
        check(token in mailer, f"mail_sender.c uses {token}")
    for token in ("CURLOPT_VERBOSE", "CURLOPT_ERRORBUFFER", "CURLOPT_SSL_VERIFYPEER",
                  "CURLOPT_SSL_VERIFYHOST", "CURLUSESSL_TRY", "curl_easy_strerror",
                  "CURLOPT_URL,"):
        check(token not in mailer, f"mail_sender.c never uses {token}")
    check("Content-Transfer-Encoding: 7bit" in mailer, "the message declares 7bit encoding")
    worker = body_of(mailer, r"\bvoid\s+worker_main\s*\(", "worker_main")
    for token in ("logit", "statuslog", "add_event", "nevent_"):
        check(token not in worker, f"worker_main has no {token} token")
    check(
        "if (stop_requested)" in worker and "&& jobs.empty()" not in worker,
        "worker_main exits on the stop flag regardless of backlog (C3)",
    )
    check("OPENSSL_cleanse(job.message" in worker, "the worker cleanses each sent message")
    shutdown = body_of(mailer, r"\bvoid\s+mail_sender_shutdown\s*\(", "mail_sender_shutdown")
    check(
        ordered(shutdown, "stop_requested = true", "dropped_at_shutdown += jobs.size()",
                "OPENSSL_cleanse(", "jobs.clear()", "notify_all()", ".join()"),
        "shutdown counts, cleanses and drops the backlog under the mutex before waking and joining",
    )
    classify = body_of(mailer, r"\bmail_outcome\s+mail_sender_classify\s*\(", "mail_sender_classify")
    curl_switch = min(
        index for index in (classify.find("switch"), classify.find("curl_code ==")) if index >= 0
    ) if any(index >= 0 for index in (classify.find("switch"), classify.find("curl_code =="))) else -1
    check(
        0 <= classify.find("smtp_code") < curl_switch,
        "mail_sender_classify decides on the SMTP class before the curl code (C10)",
    )
    for function, signature in (
        ("mail_sender_init", r"\bbool\s+mail_sender_init\s*\("),
        ("mail_sender_send_libcurl", r"\bmail_result\s+mail_sender_send_libcurl\s*\("),
    ):
        body = body_of(mailer, signature, function)
        first_statement = body.lstrip("{ \t\r\n").split(";", 1)[0]
        check(
            "call_once" in first_statement,
            f"{function} runs std::call_once(curl_global_init) as its first statement (C11)",
        )
    config_body = body_of(
        mailer, r"\bbool\s+mail_sender_config_from_env\s*\(", "mail_sender_config_from_env"
    )
    check("call_once" in config_body, "mail_sender_config_from_env runs the call_once first")
    check(
        "curl_global_cleanup" not in mailer,
        "curl_global_cleanup is intentionally never called",
    )


# --------------------------------------------------------------------- *
# C12 enable switch, C19 subject ownership, C20 capacity erases
# --------------------------------------------------------------------- *


def test_configuration_and_ownership() -> None:
    config_body = body_of(
        mailer, r"\bbool\s+mail_sender_config_from_env\s*\(", "mail_sender_config_from_env"
    )
    for token in ('getenv("MAIL_ENABLED")', '"not configured"', '"MAIL_ENABLED must be TRUE or FALSE"',
                  '"MAIL_HOST missing"', '"MAIL_HOST invalid"', '"MAIL_PORT invalid"',
                  '"MAIL_TLS must be TRUE or FALSE"', '"MAIL_TLS=FALSE requires a loopback MAIL_HOST"',
                  '"MAIL_USERNAME and MAIL_PASSWORD must be set together"',
                  '"credentials require MAIL_TLS=TRUE"', '"MAIL_FROM missing"', '"MAIL_FROM invalid"'):
        check(token in config_body, f"mail_sender_config_from_env carries {token}")
    check(
        ordered(config_body, 'getenv("MAIL_ENABLED")', 'getenv("MAIL_HOST")'),
        "MAIL_ENABLED is examined before MAIL_HOST (the switch, not the host, enables)",
    )
    init = body_of(core, r"\bbool\s+account_recovery_init\s*\(", "account_recovery_init")
    check(
        literal_reaches(
            init, core,
            "Account recovery: MAIL_ENABLED is not TRUE; password reset by email disabled.",
        ),
        "the not-configured boot line names MAIL_ENABLED",
    )
    check(
        literal_reaches(init, core, "Account recovery enabled (smtp port=%ld tls=%d)."),
        "the success boot line carries port and tls only",
    )
    check(
        literal_reaches(
            init, core,
            "Account recovery configuration rejected: %s; password reset by email disabled.",
        ),
        "the rejected boot line names the category and never a value",
    )
    check("is_valid_email(" in init, "account_recovery_init re-validates MAIL_FROM with is_valid_email")
    check(
        "mail_sender_config_from_env(" in init and "mail_sender_init(" in init,
        "account_recovery_init wires config_from_env into mail_sender_init",
    )
    check(
        '"Duris account password reset"' in core and "Duris account password reset" not in mailer,
        "the subject literal lives in account_recovery.c, not in the mailer (C19)",
    )
    check(
        re.search(r"mail_sender_submit\(uint64_t request_id, const std::string &to,\s*"
                  r"const std::string &subject,", mail_header) is not None,
        "mail_sender_submit carries the subject (C19)",
    )
    check(
        re.search(r"constexpr size_t MAIL_SENDER_MAX_PENDING = 256;", mail_header) is not None,
        "the mail queue cap is 256",
    )
    submit_at = core.find("mail_sender_submit(")
    requester = enclosing_function(core, submit_at) if submit_at >= 0 else ""
    check(
        submit_at >= 0 and "entries.erase(" in requester[requester.find("mail_sender_submit(") :],
        "a refused submit erases the entry (capacity never leaves a tombstone)",
    )
    check("mail_sender_address_is_safe(" in mailer, "mail_sender.c owns the structural address check")
    for token in ("MAIL_CA_CERT", "MAIL_RECOVERY_CONTACT"):
        for label, text in (("mail_sender.c", mailer), ("account_recovery.c", core),
                            (".env.example", read_repo(".env.example")),
                            ("CONFIGURATION.md", read_repo("docs/operations/CONFIGURATION.md"))):
            check(token not in text, f"{label} does not adopt {token}")
    # ENVIRONMENT=local is a real repo setting elsewhere; the mail path must not key on it.
    for label, text in (("mail_sender.c", mailer), ("account_recovery.c", core)):
        check("ENVIRONMENT" not in text, f"{label} grants no ENVIRONMENT=local plaintext exemption")


# --------------------------------------------------------------------- *
# (8) comm.c wiring
# --------------------------------------------------------------------- *


def test_comm_wiring() -> None:
    check('#include "account/account_recovery.h"' in comm, "comm.c includes account_recovery.h")
    # run_the_game() owns boot, the game_loop() call and the shutdown chain (main() delegates).
    run_the_game = body_of(comm, r"\bvoid\s+run_the_game\s*\(", "comm.c run_the_game")
    check(
        ordered(run_the_game, "player_load_pipeline_init()", "account_recovery_init()",
                "game_booted = TRUE"),
        "account_recovery_init runs after player_load_pipeline_init and before game_booted",
    )
    # The coordinator is also shut down on an early-exit path before game_loop(); the
    # chain that matters is the one that runs after game_loop() returns.
    loop_at = run_the_game.find("game_loop(port, sslport);")
    after_loop = run_the_game[loop_at:] if loop_at >= 0 else ""
    check(
        ordered(after_loop, "game_loop(port, sslport);", "player_load_pipeline_shutdown();",
                "account_recovery_shutdown();", "critical_command_coordinator_shutdown();"),
        "account_recovery_shutdown follows player_load_pipeline_shutdown after game_loop returns",
    )
    check(statement_present(comm, r"account_recovery_pulse\(\)"), "account_recovery_pulse(); is a statement")
    pulse_at = comm.find("account_recovery_pulse();")
    heads = enclosing_block_heads(comm, pulse_at) if pulse_at >= 0 else []
    check(
        any(head.endswith("if (!(pulse % 2))") for head in heads),
        "account_recovery_pulse(); sits inside the `if (!(pulse % 2))` block",
    )
    check(
        ordered(comm, "account_recovery_pulse();", "redis_world_recovery_pulse();"),
        "account_recovery_pulse precedes redis_world_recovery_pulse in the 500 ms block",
    )
    close_socket = body_of(comm, r"\bvoid\s+close_socket\s*\(", "close_socket")
    check(
        "account_recovery_descriptor_closed(d);" in close_socket,
        "close_socket calls account_recovery_descriptor_closed(d)",
    )
    check(
        re.search(r"(mail_sender|account_recovery)_\w*cancel\s*\(", comm) is None,
        "comm.c cancels nothing on the mailer (best-effort, name-keyed store)",
    )
    # the pulse cadence the core documents (C18)
    pulse_body = body_of(core, r"\bsize_t\s+account_recovery_pulse\s*\(", "account_recovery_pulse")
    check("% 4" in pulse_body, "the expire sweep runs every 4th pulse call")
    check(
        "500 ms" in core_raw and "4th call" in core_raw,
        "account_recovery.c states the 500 ms cadence and the every-4th-call sweep",
    )


# --------------------------------------------------------------------- *
# (9) account.c: prompt, intercept, hooks, apply helper (C1, C2, C8)
# --------------------------------------------------------------------- *


def test_account_c() -> None:
    check(re.search(r"\bP_desc\b", account_h) is None, "account.h never names P_desc (C1)")
    check(
        '#include "account/account_recovery.h"' in account_h,
        "account.h includes account_recovery.h",
    )
    password = body_of(account, r"\bvoid\s+get_account_password\s*\(", "get_account_password")
    check(
        "arg[0] == '?'" in password and ordered(password, "strspn(", "account_password_matches"),
        "the '?' intercept tolerates trailing whitespace and precedes the password check (C8)",
    )
    check(
        ordered(password, "account_recovery_begin_from_password_prompt(d)", "account_password_matches"),
        "the intercept hands off to account_recovery_begin_from_password_prompt before matching",
    )
    check(
        password.count("display_account_login_pages(d);") == 2,
        "get_account_password still shows the login pages exactly twice",
    )
    select = body_of(account, r"\bvoid\s+select_accountname\s*\(", "select_accountname")
    check("send_account_password_prompt(d)" in select, "select_accountname uses the prompt helper")
    prompt = "Please enter your password (or ? to reset it by email): "
    check(
        account.count(prompt) == 1 and "enter your password" in prompt,
        "the enabled prompt appears exactly once and keeps 'enter your password'",
    )
    prompt_helper = body_of(
        account, r"\bvoid\s+send_account_password_prompt\s*\(", "send_account_password_prompt"
    )
    check(
        '"Please enter your password: "' in prompt_helper and "account_recovery_enabled()" in prompt_helper
        and "CON_GET_ACCT_PASSWD" in prompt_helper,
        "the disabled prompt is byte-identical to the old one and the helper sets the state",
    )
    for function in ("verify_new_account_email", "verify_new_account_password"):
        body = body_of(account, r"\bvoid\s+" + function + r"\s*\(", function)
        check(
            ordered(body, "write_account(", "account_recovery_invalidate("),
            f"{function} invalidates the outstanding token after its write_account",
        )
    delete = body_of(account, r"\bvoid\s+verify_delete_account\s*\(", "verify_delete_account")
    check("account_recovery_forget(" in delete, "verify_delete_account forgets the entry")
    for function in ("ws_cmd_change_email", "ws_cmd_change_password"):
        body = body_of(ws, r"\bvoid\s+" + function + r"\s*\(", function)
        check(
            ordered(body, "write_account(", "account_recovery_invalidate("),
            f"{function} invalidates the outstanding token after its write_account",
        )
    wrapper = body_of(
        account, r"\bvoid\s+close_other_account_sessions\s*\(", "close_other_account_sessions"
    )
    check(
        "close_account_sessions_named(" in wrapper,
        "close_other_account_sessions is a wrapper over close_account_sessions_named",
    )
    apply_body = body_of(
        account,
        r"account_recovery_apply_outcome\s+account_apply_recovered_password\s*\(",
        "account_apply_recovered_password",
    )
    check(
        ordered(apply_body, "read_account(", "acct_blocked != 0",
                "account_recovery_credential_fingerprint(", "CRYPTO_memcmp(", "write_account(",
                "close_account_sessions_named("),
        "apply: fresh read -> fence -> fingerprint -> write -> kick sessions, in that order",
    )
    check("superseded" in apply_body, "apply reports superseded on a fingerprint mismatch (C7)")
    flattened = re.sub(r"\s+", " ", apply_body)
    check(
        'persistence_alert(AVATAR, "account", "redacted", "none", "none", "write_failed", NULL)'
        in flattened,
        "apply raises the 7-argument persistence_alert on a write failure",
    )
    write_at = apply_body.find("write_account(")
    check(
        write_at >= 0 and re.search(r"\bacct_name\b", apply_body[write_at:]) is None,
        "apply never touches acct_name after write_account( (C2 use-after-free)",
    )
    complete = body_of(
        core, r"account_recovery_complete_outcome\s+account_recovery_complete\s*\(",
        "account_recovery_complete",
    )
    apply_at = complete.find("account_apply_recovered_password(")
    check(
        ordered(complete, "account_recovery_canonical_name(", "account_apply_recovered_password(")
        and apply_at >= 0 and re.search(r"\bacct_name\b", complete[apply_at + 40 :]) is None,
        "complete canonicalises first and never touches acct_name after the apply call (C2)",
    )
    check(
        "superseded" in complete and "fingerprint" in complete,
        "complete passes the stored fingerprint and relays superseded (C7)",
    )


# --------------------------------------------------------------------- *
# C5: the attempt counter is zeroed in exactly three places
# --------------------------------------------------------------------- *


def test_attempt_counter_ownership() -> None:
    literal = "account_recovery_attempts = 0"
    sites = [(path, text.count(literal)) for path, text in all_sources.items() if literal in text]
    check(
        sorted(sites) == [("src/account/account_recovery_nanny.c", 2), ("src/net/ws_handlers.c", 1)],
        "'account_recovery_attempts = 0' appears exactly 3 times: nanny x2, ws_handlers x1",
        str(sites),
    )
    closed = body_of(
        nanny, r"\bvoid\s+account_recovery_descriptor_closed\s*\(", "account_recovery_descriptor_closed"
    )
    verify = body_of(
        nanny, r"\bvoid\s+account_recovery_verify_new_password\s*\(",
        "account_recovery_verify_new_password",
    )
    cleanse = body_of(
        nanny, r"\bvoid\s+account_recovery_descriptor_cleanse\s*\(",
        "account_recovery_descriptor_cleanse",
    )
    ws_complete = function_body(ws, r"\bvoid\s+ws_cmd_complete_reset\s*\(") or ""
    check(
        closed.count(literal) == 1 and verify.count(literal) == 1 and ws_complete.count(literal) == 1,
        "descriptor_closed, verify_new_password (ok branch) and ws_cmd_complete_reset each zero it once",
    )
    check(
        literal not in cleanse and "OPENSSL_cleanse(d->account_recovery_code" in cleanse
        and ("FREE(" in cleanse or "release_pending_hash(" in cleanse)
        and "FREE(d->account_recovery_pending_hash)" in nanny,
        "descriptor_cleanse scrubs the code and frees the hash but keeps the attempt count",
    )
    check(
        ordered(verify, "account_recovery_complete(", "account_recovery_descriptor_cleanse(",
                "account_recovery_complete_outcome::ok", literal),
        "verify_new_password zeroes attempts only on the ok branch, after cleansing",
    )
    callers = {
        path: text.count("account_recovery_descriptor_closed(")
        for path, text in all_sources.items()
        if "account_recovery_descriptor_closed(" in text and path.endswith(".c")
    }
    check(
        callers == {"src/account/account_recovery_nanny.c": 1, "src/net/comm.c": 1},
        "account_recovery_descriptor_closed is defined once and called only from comm.c",
        str(callers),
    )


# --------------------------------------------------------------------- *
# (10) CON_ table, descriptor fields, nanny cases
# --------------------------------------------------------------------- *


def test_connection_states() -> None:
    for name, value in (("CON_ACCT_RESET_CODE", 90), ("CON_ACCT_RESET_NEWPW", 91),
                        ("CON_ACCT_RESET_NEWPW2", 92), ("TOTAL_CON", 92)):
        check(
            re.search(r"#define\s+" + name + r"\s+" + str(value) + r"\b", structs) is not None,
            f"structs.h defines {name} {value}",
        )
    for field in ("account_recovery_attempts", "account_recovery_code[33]",
                  "account_recovery_pending_hash"):
        check(field in structs, f"descriptor_data carries {field}")
    total = re.search(r"#define\s+TOTAL_CON\s+(\d+)", structs)
    total_con = int(total.group(1)) if total else -1
    table_at = constant.find("connected_types[TOTAL_CON + 2]")
    table_end = constant.find("};", table_at)
    table = constant[table_at:table_end] if table_at >= 0 else ""
    names = re.findall(r'"((?:[^"\\]|\\.)*)"', table)
    check(
        names[-1:] == ["\\n"] and len(names) - 1 == total_con + 1,
        "connected_types holds TOTAL_CON+1 names before the \\n terminator",
        f"names={len(names) - 1} TOTAL_CON+1={total_con + 1}",
    )
    check(
        names[-5:-1] == ["PLAYER_LOAD", "ACCT_RESET_CODE", "ACCT_RESET_NEWPW", "ACCT_RESET_NEWPW2"],
        "the three reset states follow PLAYER_LOAD at the end of connected_types",
    )
    check('#include "account/account_recovery.h"' in nanny_c, "nanny.c includes account_recovery.h")
    verification = ifdef_regions(nanny_c, "REQUIRE_EMAIL_VERIFICATION")
    for state, handler in (("CON_ACCT_RESET_CODE", "account_recovery_enter_code"),
                           ("CON_ACCT_RESET_NEWPW", "account_recovery_new_password"),
                           ("CON_ACCT_RESET_NEWPW2", "account_recovery_verify_new_password")):
        match = re.search(r"case " + state + r":\s*" + handler + r"\(d, arg\);", nanny_c)
        check(match is not None, f"nanny.c dispatches {state} to {handler}(d, arg)")
        check(
            match is not None and not any(start <= match.start() < end for start, end in verification),
            f"{state} is outside every REQUIRE_EMAIL_VERIFICATION region",
        )


# --------------------------------------------------------------------- *
# (11) WebSocket table and handlers
# --------------------------------------------------------------------- *


def test_websocket() -> None:
    check(
        ordered(ws, '{ "register", ws_cmd_register }', '{ "request_reset", ws_cmd_request_reset }',
                '{ "complete_reset", ws_cmd_complete_reset }'),
        "request_reset and complete_reset are registered after register",
    )
    check(
        re.search(r"^void ws_cmd_request_reset\(", ws, re.M) is not None
        and re.search(r"^void ws_cmd_complete_reset\(", ws, re.M) is not None,
        "both reset handlers are non-static definitions",
    )
    check(
        ordered(ws, "void ws_cmd_change_password(", "void ws_cmd_request_reset(",
                "void ws_cmd_complete_reset("),
        "the reset handlers are defined after ws_cmd_change_password",
    )
    for function in ("ws_cmd_request_reset", "ws_cmd_complete_reset"):
        body = body_of(ws, r"\bvoid\s+" + function + r"\s*\(", function)
        check("ws_player_auth_attempt(" in body, f"{function} sits behind ws_player_auth_attempt")
        check(
            re.search(r"d->account\s*=[^=]", body) is None,
            f"{function} never assigns d->account (scratch account only)",
        )
        check("logit(" not in body and "statuslog(" not in body, f"{function} logs nothing itself")
    request = function_body(ws, r"\bvoid\s+ws_cmd_request_reset\s*\(") or ""
    core_call = request.find("account_recovery_request(")
    check(
        ordered(request, "allocate_account()", "read_account(", "account_recovery_request(")
        and core_call >= 0 and "free_account(" in request[core_call:],
        "request_reset loads a scratch account, asks the core, then frees it",
    )


# --------------------------------------------------------------------- *
# (12)-(14) build, packaging, environment, documentation, checks
# --------------------------------------------------------------------- *


def test_build_and_packaging() -> None:
    makefile = read_repo("src/Makefile")
    check(
        re.search(r"^GENERAL_LIBS\s*=.*-lcurl", makefile, re.M) is not None,
        "GENERAL_LIBS links -lcurl",
    )
    for object_file in ("account/account_recovery.o", "account/account_recovery_nanny.o",
                        "net/mail_sender.o"):
        check(object_file in makefile, f"Makefile builds {object_file}")
    equivs = read_repo("packaging/duris-build-deps.equivs")
    depends = re.search(r"^Depends:\s*(.*?)(?=^[A-Z][A-Za-z-]*:)", equivs, re.M | re.S)
    check(
        depends is not None
        and re.search(r"(^|[,|])\s*libcurl4-gnutls-dev\s*(?=[,|]|$)", depends.group(1)) is not None,
        "the build-deps metapackage depends on libcurl4-gnutls-dev",
    )
    check(re.search(r"^Suggests:", equivs, re.M) is not None, "the equivs keeps a field after Depends")
    dockerfile = read_repo("Dockerfile")
    marker = "FROM ubuntu:24.04 AS runtime"
    check(marker in dockerfile, "Dockerfile has the runtime stage marker")
    build_stage, _, runtime_stage = dockerfile.partition(marker)
    check(
        ordered(build_stage, "libcjson-dev", "libcurl4-gnutls-dev"),
        "Dockerfile build stage installs libcurl4-gnutls-dev (alphabetical, after libcjson-dev)",
    )
    check(
        ordered(runtime_stage, "libcjson1", "libcurl3t64-gnutls"),
        "Dockerfile runtime stage installs libcurl3t64-gnutls (alphabetical, after libcjson1)",
    )
    check(
        "libcurl4-gnutls-dev" in read_repo(".github/workflows/quality.yml"),
        "quality.yml's client-free apt list installs libcurl4-gnutls-dev",
    )


def test_environment_and_docs() -> None:
    env_lines = read_repo(".env.example").splitlines()
    check(
        [line for line in env_lines if line.startswith("MAIL_HOST=")] == ["MAIL_HOST="],
        ".env.example has exactly one MAIL_HOST= line and it is empty",
    )
    check(
        [line for line in env_lines if line.startswith("MAIL_ENABLED=")] == ["MAIL_ENABLED=FALSE"],
        ".env.example has exactly one MAIL_ENABLED= line reading MAIL_ENABLED=FALSE",
    )
    for line in ("MAIL_PORT=587", "MAIL_TLS=TRUE", "MAIL_USERNAME=", "MAIL_PASSWORD=put-secret-here",
                 "MAIL_FROM=noreply@yourdomain.com"):
        check(line in env_lines, f".env.example carries {line}")
    check(
        "# Account password recovery mail (libcurl SMTP). Set MAIL_ENABLED=TRUE to turn it on."
        in env_lines,
        ".env.example names MAIL_ENABLED as the switch in its comment",
    )
    env_text = "\n".join(env_lines)
    check(
        "Email Configuration (for notifications)" not in env_text,
        "the old 'Email Configuration (for notifications)' block is gone",
    )
    configuration = read_repo("docs/operations/CONFIGURATION.md")
    for name in ("MAIL_ENABLED", "MAIL_HOST", "MAIL_PORT", "MAIL_TLS", "MAIL_USERNAME",
                 "MAIL_PASSWORD", "MAIL_FROM"):
        check(f"| `{name}` |" in configuration, f"CONFIGURATION.md has a table row for {name}")
    for token in ("cycle_mud.sh", "request_reset", "complete_reset", "ACCOUNT_RECOVERY_HOST_MAX_REQUESTS"):
        check(token in configuration, f"CONFIGURATION.md mentions {token}")
    runbook = read_repo("docs/operations/RUNBOOK.md")
    check("account recovery" in runbook.lower() and "20 s" in runbook, "RUNBOOK.md covers recovery and the 20 s tail")
    check("change_password.sh" in runbook, "RUNBOOK.md keeps scripts/change_password.sh as the fallback")
    check("mail worker" in read_repo("docs/reference/ARCHITECTURE.md"), "ARCHITECTURE.md lists the mail worker")
    baseline = read_repo("docs/operations/SECURITY_BASELINE.md")
    check(
        "libcurl4-gnutls-dev added to the build dependencies (2026-09-06)" in baseline,
        "SECURITY_BASELINE.md carries the dated libcurl follow-up bullet (C13)",
    )
    check("curl" in read_repo("docs/guides/BUILDING.md"), "BUILDING.md lists curl among the link libraries")
    testing = read_repo("docs/guides/TESTING.md")
    for command in ("tests/async/test_account_recovery.py", "tests/async/test_account_recovery_smtp_live.py",
                    "tests/async/test_account_recovery_contract.py"):
        check(command in testing, f"TESTING.md documents {command}")
    security_check = read_repo("scripts/security_source_check.py")
    for token in ("MAIL_PASSWORD_DEFAULT", "CURLOPT_VERBOSE", "CURLOPT_SSL_VERIFYPEER", "CURLUSESSL_TRY",
                  "MAIL_PASSWORD[:-]", 'getenv\\s*\\(\\s*"MAIL_PASSWORD"'):
        check(token in security_check, f"security_source_check.py enforces {token}")
    documentation_contract = read_repo("tests/async/test_documentation_contract.py")
    for name in ("MAIL_ENABLED", "MAIL_HOST", "MAIL_PORT", "MAIL_TLS", "MAIL_USERNAME",
                 "MAIL_PASSWORD", "MAIL_FROM"):
        check(f'"{name}",' in documentation_contract, f"test_documentation_contract requires {name}")
    hygiene = read_repo("tests/async/test_persistence_log_hygiene.py")
    reviewed = re.search(r"REVIEWED\s*=\s*\[(.*?)\]", hygiene, re.S)
    for basename in ("account_recovery.c", "account_recovery_nanny.c", "mail_sender.c"):
        check(
            reviewed is not None and f'"{basename}"' in reviewed.group(1),
            f"test_persistence_log_hygiene reviews {basename}",
        )


# --------------------------------------------------------------------- *
# (15) the player text agrees with the constants
# --------------------------------------------------------------------- *


def test_constants_match_text() -> None:
    def constant_value(name: str) -> int:
        match = re.search(r"#define\s+" + name + r"\s+(\d+)", header)
        check(match is not None, f"account_recovery.h defines {name}")
        return int(match.group(1)) if match else -1

    ttl = constant_value("ACCOUNT_RECOVERY_TTL_SEC")
    cooldown = constant_value("ACCOUNT_RECOVERY_COOLDOWN_SEC")
    window = constant_value("ACCOUNT_RECOVERY_HOST_WINDOW_SEC")
    check(constant_value("ACCOUNT_RECOVERY_HOST_SLOTS") == 1024, "1024 host slots (C6)")
    check(constant_value("ACCOUNT_RECOVERY_NAME_MAX") == 64, "canonical names are 1..64 bytes (C9)")
    check(
        "#define ACCOUNT_RECOVERY_NAME_BUF (ACCOUNT_RECOVERY_NAME_MAX + 1)" in header,
        "the name buffer is NAME_MAX + 1",
    )
    check(constant_value("ACCOUNT_RECOVERY_FINGERPRINT_LEN") == 32, "the fingerprint is 32 bytes (C7)")
    check(constant_value("ACCOUNT_RECOVERY_MAX_TOKEN_ATTEMPTS") == 5, "5 guesses per token")
    check(constant_value("ACCOUNT_RECOVERY_MAX_DESCRIPTOR_ATTEMPTS") == 5, "5 guesses per connection")
    uniform_at = header.find("#define ACCOUNT_RECOVERY_UNIFORM_TEXT")
    uniform_end = header.find("\n\n", uniform_at)
    uniform = header[uniform_at:uniform_end]
    check(
        f"{ttl // 60} minutes" in uniform and f"{cooldown // 60} minutes" in uniform,
        "the uniform text's minute figures agree with TTL and COOLDOWN",
    )
    check(f"within {ttl // 60} minutes" in core, "the mail body's validity figure agrees with TTL")
    check(
        f"Wait {window // 60} minutes" in nanny and f"wait {window // 60} minutes" in ws,
        "the host-limited texts on both transports agree with HOST_WINDOW_SEC",
    )
    canonical = body_of(
        core, r"\bbool\s+account_recovery_canonical_name\s*\(", "account_recovery_canonical_name"
    )
    check(
        "unsigned char" in canonical and "0x21" in canonical and "0x7f" in canonical,
        "the canonical name rule is stated on unsigned char with the 0x21/0x7f bounds (C9)",
    )


for test in (
    test_uniform_text,
    test_code_entry_texts,
    test_log_hygiene,
    test_crypto_and_transport,
    test_configuration_and_ownership,
    test_comm_wiring,
    test_account_c,
    test_attempt_counter_ownership,
    test_connection_states,
    test_websocket,
    test_build_and_packaging,
    test_environment_and_docs,
    test_constants_match_text,
):
    test()

if failures:
    print(f"\n{len(failures)} contract failure(s):")
    for label in failures:
        print(f"  - {label}")
    sys.exit(1)
print("\naccount recovery source contracts passed")
