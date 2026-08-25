#!/usr/bin/env python3
"""Regression test for wear all crash investigation fixes.

Verifies:
1. affect_modify() in src/affects.c does not perform out-of-bounds bitv[5] read.
2. calculate_hitpoints2() in src/affects.c bounds-checks race and object modifiers before indexing stat_factor[].
3. apply_affs() and affect_total() in src/affects.c bound race indices for stat_factor[] and combat_by_race[].
4. free_obj() in src/db.c guards obj->R_num >= 0 before indexing obj_index[].
5. wear() case 13 (HOLD) in src/actobj.c rejects equipping to weapon slots when HOLD is occupied.
6. wear() case 12 (WIELD) in src/actobj.c guards secondary and fourth weapon slot capacities.
7. do_score() in src/actinf.c bounds-checks race and modifiers before indexing stat_factor[].
8. OBJ_VNUM and GET_OBJ_PROC macros in src/utils.h guard obj->R_num >= 0.
"""

from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]

affects = (ROOT / "src" / "affects.c").read_text(encoding="utf-8", errors="replace")
structs = (ROOT / "src" / "structs.h").read_text(encoding="utf-8", errors="replace")
db = (ROOT / "src" / "db.c").read_text(encoding="utf-8", errors="replace")
actobj = (ROOT / "src" / "actobj.c").read_text(encoding="utf-8", errors="replace")
actinf = (ROOT / "src" / "actinf.c").read_text(encoding="utf-8", errors="replace")
utils = (ROOT / "src" / "utils.h").read_text(encoding="utf-8", errors="replace")

checks = []

# 1. affect_modify out-of-bounds bitv check
checks.append((
    "affect_modify does not read bitv[5]",
    "bitv[5]" not in affects and "SET_BIT(TmpAffs.BV_6" not in affects
))
checks.append((
    "hold_data struct does not define BV_6",
    "BV_6" not in structs
))

# 2. calculate_hitpoints2 bounds checks
hp2_match = re.search(r"int calculate_hitpoints2\(P_char ch\)\s*\{.*?\n\}", affects, re.S)
if hp2_match:
    hp2_body = hp2_match.group(0)
    checks.append((
        "calculate_hitpoints2 guards GET_RACE bounds",
        "race >= 0 && race <= LAST_RACE" in hp2_body
    ))
    checks.append((
        "calculate_hitpoints2 guards APPLY_CON_RACE modifier bounds",
        "obj->affected[j].modifier >= 0 &&" in hp2_body and "obj->affected[j].modifier <= LAST_RACE" in hp2_body
    ))
else:
    checks.append(("calculate_hitpoints2 function present", False))

# 3. apply_affs race bounding
apply_affs_match = re.search(r"void apply_affs\(P_char ch, int mode\)\s*\{.*?\n\}", affects, re.S)
if apply_affs_match:
    apply_body = apply_affs_match.group(0)
    checks.append((
        "apply_affs bounds racial stat indices",
        "t1            = BOUNDED(0, t1, LAST_RACE);" in apply_body
    ))
    checks.append((
        "apply_affs bounds character race for stat_factor and combat_by_race",
        "int ch_race = BOUNDED(0, (int)GET_RACE(ch), LAST_RACE);" in apply_body and
        "combat_by_race[ch_race]" in apply_body and
        "stat_factor[ch_race]" in apply_body
    ))
else:
    checks.append(("apply_affs function present", False))

# 4. free_obj R_num guard
free_obj_match = re.search(r"void free_obj\(P_obj obj\)\s*\{.*?\n\}", db, re.S)
if free_obj_match:
    free_body = free_obj_match.group(0)
    checks.append((
        "free_obj guards obj->R_num >= 0 before obj_index lookup",
        "obj->R_num >= 0 && obj_index[obj->R_num].func.obj == barb" in free_body
    ))
else:
    checks.append(("free_obj function present", False))

# 5. wear() case 13 (HOLD) slot enforcement
hold_case_match = re.search(r"case 13:\s*/\* Hold \*/.*?\n\t\tcase 14:", actobj, re.S)
if hold_case_match:
    hold_body = hold_case_match.group(0)
    checks.append((
        "wear() case 13 does not fall back to WIELD or WIELD3/4",
        "WIELD" not in hold_body and "WIELD3" not in hold_body and "WIELD4" not in hold_body
    ))
    checks.append((
        "wear() case 13 guards already holding item",
        "if (ch->equipment[HOLD])" in hold_body and "execute_wear(ch, obj_object, HOLD" in hold_body
    ))
else:
    checks.append(("wear() case 13 present", False))

# 6. wear() case 12 (WIELD) slot capacity guard
wield_case_match = re.search(r"case 12:\s*/\* Wield \*/.*?\n\t\tcase 13:", actobj, re.S)
if wield_case_match:
    wield_body = wield_case_match.group(0)
    checks.append((
        "wear() case 12 guards secondary weapon occupied",
        "if (ch->equipment[SECONDARY_WEAPON])" in wield_body
    ))
    checks.append((
        "wear() case 12 guards fourth weapon occupied for 4-handed",
        "!ch->equipment[FOURTH_WEAPON]" in wield_body
    ))
else:
    checks.append(("wear() case 12 present", False))

# 7. actinf.c do_score bounds checks
score_match = re.search(r"i\s*=\s*BOUNDED\(0,\s*\(int\)GET_RACE\(ch\),\s*LAST_RACE\);", actinf)
checks.append((
    "do_score bounds GET_RACE before indexing stat_factor",
    score_match is not None
))
checks.append((
    "do_score bounds modifier before indexing stat_factor",
    "mod <= RACE_NONE || mod > LAST_RACE" in actinf
))

# 8. utils.h macro safety
checks.append((
    "OBJ_VNUM macro guards R_num >= 0",
    "(obj)->R_num >= 0" in utils and "obj_index[(obj)->R_num].virtual_number" in utils
))
checks.append((
    "GET_OBJ_PROC macro guards R_num >= 0",
    "(obj)->R_num >= 0" in utils and "obj_index[(obj)->R_num].func.obj" in utils
))

failed = [name for name, ok in checks if not ok]
for name, ok in checks:
    print(f"[{'PASS' if ok else 'FAIL'}] {name}")

if failed:
    print("\nFailed regression checks:")
    for name in failed:
        print(f"- {name}")
    sys.exit(1)

print("\nAll wear all regression checks passed successfully.")
