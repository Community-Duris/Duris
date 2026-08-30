#!/usr/bin/env python3
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "src" / "nanny.c").read_text()

keepchar = source[
    source.index("void select_keepchar(") : source.index("void display_stats(")
]
discard = keepchar[keepchar.index("case 'n':") : keepchar.index("case 'q':")]

assert "#ifdef USE_ACCOUNT" in discard
assert "free_char(d->character);" in discard
assert "d->character = NULL;" in discard
assert "STATE(d) = CON_DISPLAY_ACCT_MENU;" in discard
assert "display_account_menu(d, NULL);" in discard
assert discard.index("free_char(d->character);") < discard.index("display_account_menu(d, NULL);")

print("[PASS] discarding an account character returns to the account menu")
