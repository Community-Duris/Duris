#!/usr/bin/env python3
"""The disabled account-erasure policy cannot leave menu clients in a dead state."""

from _paths import SRC
from contract_text import contains


ACCOUNT = (SRC / "account.c").read_text(encoding="utf-8", errors="replace")


def body(signature: str) -> str:
    start = ACCOUNT.index(signature)
    opening = ACCOUNT.index("{", start)
    depth = 0
    for position in range(opening, len(ACCOUNT)):
        if ACCOUNT[position] == "{":
            depth += 1
        elif ACCOUNT[position] == "}":
            depth -= 1
            if depth == 0:
                return ACCOUNT[start : position + 1]
    raise AssertionError(f"unterminated function: {signature}")


menu = body("void display_account_menu(")
delete = body("void delete_account(")
verify = body("void verify_delete_account(")

assert "Delete this account" not in menu
assert "case 7:" in menu
assert "Account deletion is not available" in menu
assert contains(delete, "Account deletion is not available")
assert contains(delete, "STATE(d) = CON_DISPLAY_ACCT_MENU;")
assert contains(delete, "display_account_menu(d, NULL);")
assert contains(verify, "delete_account(d, arg);")

print("disabled account deletion returns every menu state to a usable prompt")
