#!/usr/bin/env python3
"""Source-contract checks for INNATE_ARCANE_RUDIMENTS mechanic."""
from pathlib import Path
import sys, re
from contract_text import contains

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"

def check(name, ok):
	if not ok:
		print(f"FAIL: {name}")
	return bool(ok)

all_ok = True

# 1. structs.h: define exists
structs = (SRC / "structs.h").read_text()
all_ok &= check("INNATE_ARCANE_RUDIMENTS #define",
	contains(structs, "#define INNATE_ARCANE_RUDIMENTS       177"))
all_ok &= check("LAST_INNATE updated",
	contains(structs, "#define LAST_INNATE INNATE_ARCANE_RUDIMENTS"))

# 2. innates_data array
innates = (SRC / "innates.c").read_text()
all_ok &= check("\"arcane rudiments\" in innates_data",
	contains(innates, "{\"arcane rudiments\", NULL},"))

# 3. No class assignments yet (0 ADD_CLASS_INNATE for it)
add_class_count = len(re.findall(r'ADD_CLASS_INNATE\(INNATE_ARCANE_RUDIMENTS,', innates))
all_ok &= check("no class assignments (user wants 0 for now)",
	add_class_count == 0)

# 4. Memorize.c references
mem = (SRC / "memorize.c").read_text()
has_innate_refs = [(m.start(), m.group()) for m in re.finditer(r'has_innate\(ch,\s*INNATE_ARCANE_RUDIMENTS\)', mem)]
all_ok &= check("has_innate used in memorize.c (expected 4 sites)",
	len(has_innate_refs) == 4)

# 5. No class has this innate inherited accidentally - check class_innates size
# class_innates[LAST_INNATE+2][CLASS_COUNT][5] uses LAST_INNATE
class_innates_line = [l for l in innates.splitlines() if "class_innates" in l and "[" in l]
all_ok &= check("class_innates array sized for LAST_INNATE",
	any("LAST_INNATE" in l for l in class_innates_line))

if all_ok:
	print("All arcane rudiments checks passed.")
	sys.exit(0)
else:
	print("FAILURES DETECTED")
	sys.exit(1)