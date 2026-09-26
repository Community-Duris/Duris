#!/usr/bin/env python3
"""god_list names the staff who pass god_check(); its "\\0" entry ends the scan.

do_start_impl() makes an OVERLORD of any character with a god_list name, so no
character may take one, even after a wipe frees it.
"""
import re

from _paths import SRC
from _source_contract import function_bodies, strip_comments
from contract_text import contains

src = strip_comments((SRC / "constant.c").read_text(encoding="utf-8"))
match = re.search(r"const\s+char\s*\*\s*god_list\[\]\s*=\s*\{(.*?)\};", src, re.S)
assert match, "god_list definition not found"
names = re.findall(r'"((?:[^"\\]|\\.)*)"', match.group(1))
assert "Zusuk" in names, names
assert names[-1] == "\\0", names
assert "\\0" not in names[:-1], names

parse = function_bodies((SRC / "nanny.c").read_text(encoding="utf-8"),
                        r"\bbool\s+_parse_name\s*\(")
assert len(parse) == 1, "_parse_name definition not found"
assert contains(strip_comments(parse[0]), "if (character_name && god_check(name)) return TRUE;")

# Every character name is checked against god_list. Account names are not:
# accounts named after staff exist, and an account's name grants nothing.
calls = (
    ("account.c", r"\bvoid\s+select_accountname\s*\(", "_parse_name(arg, tmp_name, false)"),
    ("account.c", r"\bvoid\s+account_new_char_name\s*\(", "_parse_name(arg, tmp_name, true)"),
    ("wiz_newchar.c", r"\bvoid\s+do_newchar\s*\(", "_parse_name(arg1, name_lower, true)"),
    ("modify.c", r"\bbool\s+rename_character\s*\(", "_parse_name(new_name, new_name, true)"),
    ("nanny.c", r"\bvoid\s+select_name\s*\(", "_parse_name(arg, tmp_name, true)"),
)
for name, signature, call in calls:
    bodies = function_bodies((SRC / name).read_text(encoding="utf-8"), signature)
    assert len(bodies) == 1, (name, signature)
    assert contains(strip_comments(bodies[0]), call), (name, call)
print("god list contract passed")
