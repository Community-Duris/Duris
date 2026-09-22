#!/usr/bin/env python3
"""god_list names the staff who pass god_check(); its "\\0" entry ends the scan."""
import re

from _paths import SRC
from _source_contract import strip_comments

src = strip_comments((SRC / "constant.c").read_text(encoding="utf-8"))
match = re.search(r"const\s+char\s*\*\s*god_list\[\]\s*=\s*\{(.*?)\};", src, re.S)
assert match, "god_list definition not found"
names = re.findall(r'"((?:[^"\\]|\\.)*)"', match.group(1))
assert "Zusuk" in names, names
assert names[-1] == "\\0", names
assert "\\0" not in names[:-1], names
print("god list contract passed")
