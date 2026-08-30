#!/usr/bin/env python3
"""Regression contract for displaying news during account login."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
ACCOUNT = (ROOT / "src/account.c").read_text(encoding="utf-8")


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

news = 'SEND_TO_Q(news.c_str(), d);'
motd = 'SEND_TO_Q(motd.c_str(), d);'
prompt = 'SEND_TO_Q("\\r\\n*** PRESS RETURN: ", d);'

assert news in pages
assert motd in pages
assert prompt in pages
assert pages.index(news) < pages.index(motd) < pages.index(prompt)
assert password.count("display_account_login_pages(d);") == 2
assert new_account.count("display_account_login_pages(d);") == 1

print("account login news regression passed")
