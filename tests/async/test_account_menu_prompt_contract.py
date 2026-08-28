#!/usr/bin/env python3
"""Regression contracts for account-menu prompting and selection rendering."""

from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[2]
ACCOUNT = (ROOT / "src/account.c").read_text(encoding="utf-8", errors="replace")
COMM = (ROOT / "src/comm.c").read_text(encoding="utf-8", errors="replace")


def function_body(source, signature):
    start = source.index(signature)
    brace = source.index("{", start)
    depth = 0
    for pos in range(brace, len(source)):
        if source[pos] == "{":
            depth += 1
        elif source[pos] == "}":
            depth -= 1
            if depth == 0:
                return source[start : pos + 1]
    raise AssertionError(f"unterminated function: {signature}")


def check(name, condition):
    print(("OK: " if condition else "FAIL: ") + name)
    return bool(condition)


menu = function_body(ACCOUNT, "void display_account_menu(")
select_char = function_body(ACCOUNT, "void account_select_char(")
character_list = function_body(ACCOUNT, "void display_character_list(P_desc")
process_output = function_body(COMM, "int process_output(")

ok = True
ok &= check(
    "account menu marks its question as a prompt",
    re.search(r'Please select an option:.*?prompt_mode = TRUE;', menu, re.S) is not None,
)
ok &= check(
    "character list marks its question as a prompt",
    re.search(r'Which character would you like to play\?.*?prompt_mode = TRUE;', character_list, re.S)
    is not None,
)
ok &= check(
    "character confirmation marks its question as a prompt",
    re.search(r'Play as .*?prompt_mode = TRUE;', select_char, re.S) is not None,
)
ok &= check(
    "output sends GA for prompts in connected account states",
    "if (had_prompt)" in process_output and "had_prompt && !t->connected" not in process_output,
)
ok &= check(
    "account menu selection requires a complete numeric token",
    "strtol(arg, &end, 10)" in menu and "end == arg" in menu and "*end" in menu and "atoi(" not in menu,
)
ok &= check(
    "character number selection requires a complete numeric token",
    "strtol(arg, &end, 10)" in select_char
    and "end == arg" in select_char
    and "*end" in select_char
    and "atoi(" not in select_char,
)
ok &= check(
    "race-switch feedback uses the configured timer",
    select_char.count("+ racewarSwitchTimer) - current_time") == 2
    and "+ 3600) - current_time" not in select_char,
)
ok &= check(
    "character list preserves the negotiated terminal type",
    "term_type =" not in character_list,
)
ok &= check(
    "character list validates class-table indexes",
    character_list.count("<= CLASS_COUNT") >= 2,
)
ok &= check(
    "room-name color copying reserves its terminator",
    "dst_idx + color_length >= sizeof room_display" in character_list
    and "dst_idx + 1 < sizeof room_display" in character_list,
)

if not ok:
    raise SystemExit(1)

print("\nAll account menu prompt contracts passed.")
