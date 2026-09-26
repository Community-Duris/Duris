#!/usr/bin/env python3
"""Racewar admission policy, entry ordering, and persistence regressions."""

from _paths import ROOT, SRC, rel
import subprocess
import tempfile


def function_body(source: str, signature: str) -> str:
    start = source.index(signature)
    opening = source.index("{", start)
    depth = 0
    for position in range(opening, len(source)):
        if source[position] == "{":
            depth += 1
        elif source[position] == "}":
            depth -= 1
            if not depth:
                return source[start : position + 1]
    raise AssertionError(f"unterminated function: {signature}")


account = (SRC / "account.c").read_text(encoding="utf-8")
nanny = (SRC / "nanny.c").read_text(encoding="utf-8")
websocket = (SRC / "ws_handlers.c").read_text(encoding="utf-8")
sql_player = (SRC / "sql_player.c").read_text(encoding="utf-8")
flatfile = (SRC / "flatfile_account_adapter.c").read_text(encoding="utf-8")
makefile = (ROOT / "src/Makefile").read_text(encoding="utf-8")

check_at = function_body(account, "account_check_racewar_admission_at(P_desc")
check = function_body(account, "account_racewar_admission account_check_racewar_admission(")
commit = function_body(account, "bool account_commit_character_admission(")
can_connect = function_body(account, "int can_connect(")
select_character = function_body(account, "void account_select_char(")
confirm_character = function_body(account, "void account_confirm_char(")
reconnect = function_body(account, "int is_char_in_game(")
native_side_check = function_body(nanny, "static bool account_creation_side_allowed(")
ws_login = function_body(websocket, "void ws_finish_login(")
ws_enter = function_body(websocket, "void ws_cmd_enter(")
ws_create = function_body(websocket, "void ws_cmd_create_character(")

assert 'get_property("account.timer.racewarSwitch", 3600)' in account
assert "account_racewar_evaluate" in check_at
assert "account.timer.racewarSwitch" in check_at
assert "account_check_racewar_admission_at" in check
assert "account_check_racewar_admission" in can_connect
assert "account_check_racewar_admission" in select_character
assert "account_format_racewar_denial" in select_character
assert "account_check_racewar_admission" in confirm_character
assert "+ 3600" not in select_character and "+ 3600" not in confirm_character

assert "account_check_racewar_admission_at" in commit
assert "acct_good" in commit and "acct_evil" in commit
assert commit.index("write_account(d->account)") < commit.index("return true;", commit.index("write_account"))
assert "ACCOUNT_RACEWAR_DENIAL_PERSISTENCE" in commit
assert confirm_character.index("account_commit_character_admission") < confirm_character.index(
    "STATE(d) = CON_PLAYING"
)
assert reconnect.index("account_check_racewar_admission") < reconnect.index("close_socket(")
assert reconnect.index("account_commit_character_admission") < reconnect.index(
    "STATE(d) = CON_PLAYING"
)

assert "account_check_racewar_admission" in native_side_check
assert nanny.count("if (!account_creation_side_allowed(d))") == 2
rmotd = nanny[nanny.index("case CON_RMOTD:") : nanny.index("case CON_MAIN_MENU:", nanny.index("case CON_RMOTD:"))]
assert rmotd.index("account_commit_character_admission") < rmotd.index("STATE(d) = CON_PLAYING")
assert rmotd.index("account_commit_character_admission") < rmotd.index("enter_game(d)")

assert ws_login.index("account_commit_character_admission") < ws_login.index(
    "/* kick any duplicate sessions"
)
assert ws_enter.index("account_check_racewar_admission") < ws_enter.index(
    "/* duplicate session check"
)
assert "ws_resolve_racewar_side(race_id, alignment)" in ws_create
assert ws_create.index("const account_racewar_admission preflight") < ws_create.index(
    "/* allocate character structure"
)
assert ws_create.index("account_commit_character_admission") < ws_create.index(
    "STATE(d) = CON_PLAYING"
)
assert ws_create.index("account_commit_character_admission") < ws_create.index(
    'cJSON_AddStringToObject(response, "status", "created")'
)
assert "item_creation_grant_cancel_batch_before_entry(ch)" in ws_create
assert "account/racewar_admission.o" in makefile

# Both maintained account backends already round-trip the timestamps that the
# admission commit now writes before entry.
assert "acc->acct_good" in sql_player and "acc->acct_evil" in sql_player
assert "record->last_good = account->acct_good" in flatfile
assert "record->last_evil = account->acct_evil" in flatfile
assert "account->acct_good = record.last_good" in flatfile
assert "account->acct_evil = record.last_evil" in flatfile

with tempfile.TemporaryDirectory(prefix="duris-racewar-admission-") as temporary:
    binary = f"{temporary}/racewar_admission_policy_test"
    compile_result = subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Wpedantic",
            "-Werror",
            "-Isrc",
            "tests/async/racewar_admission_policy_harness.cpp",
            rel("racewar_admission.c"),
            "-o",
            binary,
        ],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if compile_result.returncode:
        raise SystemExit(compile_result.stdout)
    run_result = subprocess.run(
        [binary],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if run_result.returncode:
        raise SystemExit(run_result.stdout)
    print(run_result.stdout.strip())

print("racewar admission source contracts passed")
