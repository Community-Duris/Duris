#!/usr/bin/env python3
"""Source-contract and regression tests for Dragoon class spellcasting and slot mechanics."""
from pathlib import Path
import sys, re

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"

def check(name, ok):
	if not ok:
		print(f"FAIL: {name}")
	else:
		print(f"OK: {name}")
	return bool(ok)

all_ok = True

# 1. Verify draconic apotheosis registration in skills.c
skills = (SRC / "skills.c").read_text()
all_ok &= check("draconic apotheosis spell creation in skills.c",
	'SPELL_CREATE("draconic apotheosis", SPELL_DRACONIC_APOTHEOSIS,' in skills)
all_ok &= check("draconic apotheosis added as circle 12 Dragoon spell",
	"SPELL_ADD(CLASS_DRAGOON, 12);" in skills)

# 2. Verify memorize.c does not define REVERSE_DRAGOON_COMMUNE
mem = (SRC / "memorize.c").read_text()
all_ok &= check("REVERSE_DRAGOON_COMMUNE removed from memorize.c",
	"REVERSE_DRAGOON_COMMUNE" not in mem)
all_ok &= check("handle_undead_mem loops top-down from get_max_circle",
	"for (i = get_max_circle(ch); i >= 1; i--)" in mem)

# 3. Verify do_assimilate mount guard for Dragoon commune
all_ok &= check("do_assimilate checks dragoon mount status before scheduling event_memorize",
	"if (IS_DRAGOON(ch) && !is_dragoon_mounted(ch))" in mem)

# 4. Verify actwiz.c do_restore handles USES_SPELL_SLOTS for PCs
actwiz = (SRC / "actwiz.c").read_text()
restore_matches = re.findall(r'if\s*\(USES_SPELL_SLOTS\(victim\)\)', actwiz) + re.findall(r'else\s+if\s*\(USES_SPELL_SLOTS\(victim\)\)', actwiz)
all_ok &= check("do_restore includes USES_SPELL_SLOTS handling (both all and single target)",
	len(restore_matches) >= 2)

# 5. Verify sparser.c retains slot check
sparser = (SRC / "sparser.c").read_text()
all_ok &= check("sparser.c checks undead_spell_slots under USES_SPELL_SLOTS",
	"if (circle != -1 && !ch->specials.undead_spell_slots[circle])" in sparser)

# 6. Verify mount.c triggers commune upon mounting
mount = (SRC / "mount.c").read_text()
all_ok &= check("mount.c triggers do_assimilate when Dragoon mounts",
	"if (is_dragoon_mounted(ch))" in mount and "do_assimilate(ch, \"nl\", CMD_COMMUNE);" in mount)

if all_ok:
	print("\nAll Dragoon spellcasting contract checks passed.")
	sys.exit(0)
else:
	print("\nFAILURES DETECTED in Dragoon spellcasting contract tests.")
	sys.exit(1)
