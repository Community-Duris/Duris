#!/usr/bin/env python3
"""Regression test for wear all crash investigation fixes.

Verifies:
1. affect_modify() in src/affects.c does not perform out-of-bounds bitv[5] read.
2. calculate_hitpoints2() in src/affects.c bounds-checks race and object modifiers before indexing stat_factor[].
3. apply_affs() and affect_total() in src/affects.c bound race indices for stat_factor[] and combat_by_race[].
4. free_obj() in src/db.c guards obj->R_num >= 0 before indexing obj_index[].
5. Hand placement uses unoccupied storage slots and a shared hand budget.
6. wear() case 12 (WIELD) in src/actobj.c guards secondary and fourth weapon slot capacities.
7. do_score() in src/actinf.c bounds-checks race and modifiers before indexing stat_factor[].
8. OBJ_VNUM and GET_OBJ_PROC macros in src/utils.h guard obj->R_num >= 0.
"""

from _paths import SRC
from pathlib import Path
import re
import sys
from contract_text import contains

ROOT = Path(__file__).resolve().parents[2]

affects = (SRC / "affects.c").read_text(encoding="utf-8", errors="replace")
structs = (SRC / "structs.h").read_text(encoding="utf-8", errors="replace")
db = (SRC / "db.c").read_text(encoding="utf-8", errors="replace")
actobj = (SRC / "actobj.c").read_text(encoding="utf-8", errors="replace")
actinf = (SRC / "actinf.c").read_text(encoding="utf-8", errors="replace")
utils = (SRC / "utils.h").read_text(encoding="utf-8", errors="replace")

checks = []

# 1. affect_modify out-of-bounds bitv check
checks.append((
    "affect_modify does not read bitv[5]",
    not contains(affects, "bitv[5]") and not contains(affects, "SET_BIT(TmpAffs.BV_6")
))
checks.append((
    "hold_data struct does not define BV_6",
    not contains(structs, "BV_6")
))

# 2. calculate_hitpoints2 bounds checks
hp2_match = re.search(r"int calculate_hitpoints2\(P_char ch\)\s*\{.*?\n\}", affects, re.S)
if hp2_match:
    hp2_body = hp2_match.group(0)
    checks.append((
        "calculate_hitpoints2 guards GET_RACE bounds",
        contains(hp2_body, "race >= 0 && race <= LAST_RACE")
    ))
    checks.append((
        "calculate_hitpoints2 guards APPLY_CON_RACE modifier bounds",
        contains(hp2_body, "obj->affected[j].modifier >= 0 &&") and contains(hp2_body, "obj->affected[j].modifier <= LAST_RACE")
    ))
else:
    checks.append(("calculate_hitpoints2 function present", False))

# 3. apply_affs race bounding
apply_affs_match = re.search(r"void apply_affs\(P_char ch, int mode\)\s*\{.*?\n\}", affects, re.S)
if apply_affs_match:
    apply_body = apply_affs_match.group(0)
    checks.append((
        "apply_affs bounds racial stat indices",
        contains(apply_body, "t1            = BOUNDED(0, t1, LAST_RACE);")
    ))
    checks.append((
        "apply_affs bounds character race for stat_factor and combat_by_race",
        contains(apply_body, "int ch_race = BOUNDED(0, (int)GET_RACE(ch), LAST_RACE);") and
        contains(apply_body, "combat_by_race[ch_race]") and
        contains(apply_body, "stat_factor[ch_race]")
    ))
else:
    checks.append(("apply_affs function present", False))

# 4. free_obj R_num guard
free_obj_match = re.search(r"void free_obj\(P_obj obj\)\s*\{.*?\n\}", db, re.S)
if free_obj_match:
    free_body = free_obj_match.group(0)
    checks.append((
        "free_obj guards obj->R_num >= 0 before obj_index lookup",
        contains(free_body, "obj->R_num >= 0 && obj_index[obj->R_num].func.obj == barb")
    ))
else:
    checks.append(("free_obj function present", False))

# Occupied-slot protection is shared by wield and hold; behavior is exercised below.
checks.append((
    'hand placement guards occupied slots',
    contains(actobj, 'if (!ch->equipment[slot])') and
    contains(actobj, 'free_hand_slot(ch, false, hands_needed)') and
    contains(actobj, 'free_hand_slot(ch, true, hands_needed)')
))

# 7. actinf.c do_score bounds checks
score_match = re.search(r"i\s*=\s*BOUNDED\(0,\s*\(int\)GET_RACE\(ch\),\s*LAST_RACE\);", actinf)
checks.append((
    "do_score bounds GET_RACE before indexing stat_factor",
    score_match is not None
))
checks.append((
    "do_score bounds modifier before indexing stat_factor",
    contains(actinf, "mod <= RACE_NONE || mod > LAST_RACE")
))

# 8. utils.h macro safety
checks.append((
    "OBJ_VNUM macro guards R_num >= 0",
    contains(utils, "(obj)->R_num >= 0") and contains(utils, "obj_index[(obj)->R_num].virtual_number")
))
checks.append((
    "GET_OBJ_PROC macro guards R_num >= 0",
    contains(utils, "(obj)->R_num >= 0") and contains(utils, "obj_index[(obj)->R_num].func.obj")
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

# Compile production hand branches and scribing functions using the existing
# extraction harness. Other wear locations are outside this focused regression.
import subprocess
import tempfile
from _paths import extract_function

wear_source = extract_function('actobj.c', 'int wear(P_char ch, P_obj obj_object, int keyword, bool showit)')
def hand_wear_source(text):
    switch = re.search(r'\bswitch\s*\(\s*keyword\s*\)', text)
    wield = re.search(r'^\s*case\s+12\s*:', text, re.M)
    eyes = re.search(r'^\s*case\s+15\s*:', text, re.M)
    assert switch and wield and eyes, 'wear hand-case boundaries are missing'
    assert switch.start() < wield.start() < eyes.start(), 'wear hand cases are out of order'
    return (text[:switch.start()] + 'switch (keyword) {\n' +
            text[wield.start():eyes.start()] + '\n}\nreturn FALSE;\n}')


# Indentation, switch spacing, and case comments must not change extraction.
variant = wear_source.expandtabs(4).replace('switch (keyword)', 'switch \n ( keyword )')
variant = variant.replace('/* Wield */', '/* weapon role */').replace('/* Eyes */', '/* next role */')
def tokens(text):
    return re.sub(r'\s+', '', re.sub(r'/\*.*?\*/|//[^\n]*', '', text, flags=re.S))
assert tokens(hand_wear_source(wear_source)) == tokens(hand_wear_source(variant))

production = '\n'.join([
    extract_function('actobj.c', 'int wield_item_size('),
    extract_function('actobj.c', 'int get_numb_free_hands('),
    extract_function('actobj.c', 'static int free_hand_slot('),
    hand_wear_source(wear_source),
    extract_function('memorize.c', 'static P_obj scribing_implement('),
    extract_function('memorize.c', 'int ScriberSillyChecks('),
    extract_function('memorize.c', 'P_obj SpellBookAtHand('),
    extract_function('memorize.c', 'P_obj FindSpellBookWithSpell('),
    extract_function('memorize.c', 'void do_scribe(P_char ch, char *arg, int /*cmd*/)'),
    extract_function('memorize.c', 'void event_scribe(P_char ch, P_char /*victim*/'),
])
fixture = (ROOT / 'tests/async/wear_all_fixture.cpp').read_text()
with tempfile.TemporaryDirectory() as td:
    source = Path(td) / 'wear.cpp'
    binary = Path(td) / 'wear'
    source.write_text(fixture.replace('// PRODUCTION_FUNCTIONS', production))
    subprocess.run(['g++', '-std=c++20', '-g', '-fsanitize=address,undefined',
                    '-fno-omit-frame-pointer', '-fno-pie', '-no-pie',
                    '-I' + str(SRC), str(source), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('Production hand-equipment and scribing behavioral checks passed.')
