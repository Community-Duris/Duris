#!/usr/bin/env python3
from _paths import SRC
from pathlib import Path
from contract_text import contains

ROOT = Path(__file__).resolve().parents[2]
source = (SRC / "plushit.c").read_text()
header = (SRC / "plushit.h").read_text()
fight = (SRC / "fight.c").read_text()
makefile = (SRC / "Makefile").read_text()
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
assert contains(source, 'get_property("combat.plushit.enforce", 0)')
assert contains(source, 'get_property("combat.silver.enforce", 0)')

# the rule is never enforced against players or for trusted attackers
assert contains(source, "IS_PC(victim) || IS_TRUSTED(ch)")

# fight.c carries exactly one guard, and it only ever aborts the swing
assert fight.count("plushit_blocks(") == 1
assert contains(fight, '#include "item/plushit.h"')
assert contains(fight, "if (plushit_blocks(ch, victim, weapon))")

# one object line
assert "plushit.o" in makefile

# the public surface is one predicate
assert contains(header, "int plushit_blocks(P_char ch, P_char victim, P_obj weapon);")

# ships off: the stock properties file carries neither key
assert "combat.plushit.enforce" not in properties
assert "combat.silver.enforce" not in properties

print("plushit module contract passed")
