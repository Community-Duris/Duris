#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "src/plushit.c").read_text()
header = (ROOT / "src/plushit.h").read_text()
fight = (ROOT / "src/fight.c").read_text()
makefile = (ROOT / "src/Makefile").read_text()
properties = (ROOT / "lib/duris.properties").read_text()

# the module owns the whole rule
for symbol in (
    "plushit_required",
    "plushit_available",
    "silver_required",
    "silver_available",
    "int plushit_blocks(",
    "APPLY_HITROLL",
    "ITEM2_SILVER",
    "MAT_SILVER",
    "MAT_MITHRIL",
):
    assert symbol in source, symbol

# both switches are read per call and default to off
assert 'get_property("combat.plushit.enforce", 0)' in source
assert 'get_property("combat.silver.enforce", 0)' in source

# the rule is never enforced against players or for trusted attackers
assert "IS_PC(victim) || IS_TRUSTED(ch)" in source

# fight.c carries exactly one guard, and it only ever aborts the swing
assert fight.count("plushit_blocks(") == 1
assert '#include "plushit.h"' in fight
assert "if (plushit_blocks(ch, victim, weapon))" in fight

# one object line
assert "plushit.o" in makefile

# the public surface is one predicate
assert "int plushit_blocks(P_char ch, P_char victim, P_obj weapon);" in header

# ships off: the stock properties file carries neither key
assert "combat.plushit.enforce" not in properties
assert "combat.silver.enforce" not in properties

print("plushit module contract passed")
