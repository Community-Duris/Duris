#!/usr/bin/env python3
"""The implemented account deletion flow is reachable from the account menu."""

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
case_7 = menu[menu.index("case 7:") : menu.index("case 8:")]
delete = body("void delete_account(")
verify = body("void verify_delete_account(")

assert "Delete this account" in menu
assert "case 7:" in menu
assert "Account deletion is not available" not in menu
assert contains(case_7, "STATE(d) = CON_ACCT_DELETE_ACCT;")
assert contains(case_7, "delete_account(d, NULL);")
assert contains(delete, "account_password_matches(d->account, arg)")
assert contains(delete, "STATE(d) = CON_ACCT_VERIFY_DELETE_ACCT;")
assert contains(verify, "d->account->acct_blocked = ACCOUNT_BLOCK_DELETION")
assert contains(verify, "sql_delete_account(account_name.c_str())")

print("account deletion is reachable and guarded from the account menu")
