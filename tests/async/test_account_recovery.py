#!/usr/bin/env python3
"""Execute the account-recovery core with an injected mail sender and clock.

tests/async/account_recovery_harness.cpp links src/account/account_recovery.c and
src/net/mail_sender.c against four stubs (logit, statuslog, is_valid_email and
account_apply_recovered_password) plus the real password_hash.c, and drives the
token store, the cooldown tombstones, the two-phase check/complete, the host
window, the queue capacity, the pulse outcomes and the shutdown join with no
network at all.  The log stubs render their lines into a file so the run can
prove no reset code, address, host or account name was ever formatted into one.

Compile flags are the working client-free set from test_flatfile_ip_activity.py
and test_flatfile_kingdom_repository.py, plus the libraries the two modules
need.  The harness builds its mail configuration in code and never reads MAIL_*.
"""

from _paths import SRC, rel
import pathlib
import re
import subprocess
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[2]

EXPECTED_SECTIONS = (
    "fingerprint",
    "classify",
    "render",
    "canonical_name",
    "a_request_renders_mail",
    "b_suppressed_tombstone",
    "c_canonical_keying",
    "d_check_normalises_without_consuming",
    "e_token_attempt_cap",
    "f_two_phase_complete",
    "g_fenced_and_superseded",
    "h_failures_keep_token",
    "i_expiry_and_sweep",
    "j_latest_wins",
    "k_host_window",
    "l_capacity_erases_entry",
    "m_pulse_outcomes",
    "n_invalidate_and_forget",
    "o_store_cap_and_host_slots",
    "p_shutdown_join",
    "q_log_hygiene",
)

# Belt under the harness's own hygiene section: values it is known to hand the
# core, plus the shapes a code takes (32 hex, or 8-8-8-8), must not be in the log.
FORBIDDEN_LOG_LITERALS = (
    "@",
    "duris.test",
    "203.0.113.7",
    "198.51.100.9",
    "2001:db8",
    "Fraser",
    "Web_Player14",
    "Foxtrot",
    "Juliet",
)
FORBIDDEN_LOG_PATTERNS = (
    re.compile(r"[0-9a-f]{32}"),
    re.compile(r"[0-9a-f]{8}-[0-9a-f]{8}-[0-9a-f]{8}-[0-9a-f]{8}"),
)

with tempfile.TemporaryDirectory(prefix="duris-account-recovery-") as temporary:
    temporary_path = pathlib.Path(temporary)
    binary = temporary_path / "account_recovery_test"
    log_file = temporary_path / "stub.log"
    compile_result = subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Wpedantic",
            "-Werror",
            "-D__NO_MYSQL__",
            "-ffunction-sections",
            "-fdata-sections",
            "-Isrc/no_mysql",
            "-Isrc",
            "-Isrc/ships",
            "-I/usr/include/libxml2",
            "tests/async/account_recovery_harness.cpp",
            rel("account_recovery.c"),
            rel("mail_sender.c"),
            rel("password_hash.c"),
            "-Wl,--gc-sections",
            "-lcurl",
            "-lcrypto",
            "-lssl",
            "-lcrypt",
            "-pthread",
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if compile_result.returncode:
        raise SystemExit(compile_result.stdout)
    try:
        run_result = subprocess.run(
            [str(binary), str(log_file)],
            cwd=ROOT,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            timeout=180,
        )
    except subprocess.TimeoutExpired as error:
        raise SystemExit("account recovery harness exceeded 180 s") from error
    if run_result.returncode:
        raise SystemExit(run_result.stdout)
    for section in EXPECTED_SECTIONS:
        if f"ok: {section}\n" not in run_result.stdout:
            raise SystemExit(f"harness did not report section {section}:\n{run_result.stdout}")
    for pattern in FORBIDDEN_LOG_PATTERNS:
        if pattern.search(run_result.stdout):
            raise SystemExit("harness stdout carries a code-shaped token")

    stub_log = log_file.read_text(encoding="utf-8", errors="replace")
    if "account recovery request=" not in stub_log or "(account=redacted)" not in stub_log:
        raise SystemExit("the log stubs did not render the core's request lines")
    if "outcome=queued" not in stub_log:
        raise SystemExit("no queued request reached the log")
    for literal in FORBIDDEN_LOG_LITERALS:
        if literal in stub_log:
            raise SystemExit(f"stub log carries a value it must never carry: {literal!r}")
    for pattern in FORBIDDEN_LOG_PATTERNS:
        if pattern.search(stub_log):
            raise SystemExit("stub log carries a code-shaped token")
    print(run_result.stdout.strip())

# The modules are only tested if the server actually links them.
makefile = (SRC / "Makefile").read_text()
for object_file in (
    "account/account_recovery.o",
    "account/account_recovery_nanny.o",
    "net/mail_sender.o",
):
    if object_file not in makefile:
        raise SystemExit(f"{object_file} is not linked into the server")
if not re.search(r"^GENERAL_LIBS\s*=.*-lcurl", makefile, re.MULTILINE):
    raise SystemExit("GENERAL_LIBS does not link libcurl")

print("account recovery core regression passed")
