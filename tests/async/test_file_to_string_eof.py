#!/usr/bin/env python3
"""Regression contract for file_to_string()'s end-of-file handling.

file_to_string() reads a whole file a line at a time and uses end of file as its
loop's exit condition.  It was converted to REQUIRED_FGETS, which calls
fatal_boot_error() when fgets() returns NULL -- which is exactly what happens on
the final, expected iteration.  Every motd/wizlist/news file is read through
this function during "Reading files from lib directory", so the game could not
boot at all:

    file_io: db.c:4239: required line could not be read

This pins the loop back to a plain fgets() and keeps the file handle closed on
the over-long-file path.
"""
from pathlib import Path
import re

root = Path(__file__).resolve().parents[2]
db = (root / "src/db.c").read_text()

failures = []


def check(name, ok):
    print(("[PASS] " if ok else "[FAIL] ") + name)
    if not ok:
        failures.append(name)


start = db.index("char *file_to_string(const char *name)")
body = db[start:db.index("\n}\n", start)]

check("file_to_string reads with plain fgets", "while (fgets(tmp, 255, fl))" in body)
check("file_to_string does not fatal on the expected EOF",
      "REQUIRED_FGETS" not in body)
check("file_to_string closes the file before the too-long bailout",
      re.search(r'file \(%s\) too long.*?\n\s*fclose\(fl\);\s*\n\s*return \(NULL\);',
                body, re.S) is not None)
check("file_to_string still closes the file on the success path", "fclose(fl);" in body)

# The same shape must not come back anywhere else: a REQUIRED_* read whose
# enclosing loop is terminated by feof() is the bug this test exists for.
offenders = []
for path in sorted((root / "src").rglob("*.c")):
    if path.name == "safe_io.c":
        continue
    text = path.read_text(errors="replace")
    for m in re.finditer(r'^[ \t]*REQUIRED_F(?:GETS|READ)\b.*$', text, re.M):
        line_no = text.count("\n", 0, m.start()) + 1
        window = text[max(0, m.start() - 400):m.end() + 400]
        if "#if 0" in text[max(0, m.start() - 4000):m.start()]:
            continue
        if re.search(r'while\s*\(\s*!\s*feof', window):
            offenders.append("%s:%d" % (path.relative_to(root), line_no))
check("no REQUIRED_* read sits in a while(!feof(...)) loop", not offenders)
for o in offenders:
    print("       " + o)

if failures:
    print("\nFailed regression checks:")
    for f in failures:
        print("- " + f)
    raise SystemExit(1)
print("file_to_string EOF contract passed")
