#!/usr/bin/env python3
"""Login and registration show a brief notice; full news is read on request."""

from _paths import SRC
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
ACCOUNT = (SRC / "account.c").read_text(encoding="utf-8")
NANNY = (SRC / "nanny.c").read_text(encoding="utf-8")
ACTINF = (SRC / "actinf.c").read_text(encoding="utf-8")


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


pages = function_body(ACCOUNT, "void display_account_login_pages(")
password = function_body(ACCOUNT, "void get_account_password(")
new_account = function_body(ACCOUNT, "void verify_new_account_information(")

notice = "Type 'news' in game to read the latest updates."
motd = 'SEND_TO_Q(motd.c_str(), d);'
prompt = 'SEND_TO_Q("\\r\\n*** PRESS RETURN: ", d);'

assert notice in pages
assert "news.c_str()" not in ACCOUNT
assert "news.c_str()" not in NANNY
assert motd in pages
assert prompt in pages
assert pages.index(notice) < pages.index(motd) < pages.index(prompt)
assert password.count("display_account_login_pages(d);") == 2
assert new_account.count("display_account_login_pages(d);") == 1
assert "send_to_char(news.c_str(), ch, LOG_NONE);" in function_body(
    ACTINF, "void do_news("
)

print("account login news regression passed")
