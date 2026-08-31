#!/usr/bin/env python3
"""Regression contracts for spell level bounds and combined affect flags."""
from _paths import SRC
from pathlib import Path

from contract_text import contains, index

ROOT = Path(__file__).resolve().parents[2]
bard = (SRC / "bard.c").read_text()
psionics = (SRC / "psionics.c").read_text()
magic = (SRC / "magic.c").read_text()

bard_start = index(bard, "void bard_sleep(int l,")
bard_end = index(bard, "void bard_calm(", bard_start)
bard_sleep = bard[bard_start:bard_end]
assert contains(bard_sleep, "int i, level = l;")
assert contains(bard_sleep, "af.duration = 4 + (level < 0 ? -level : level);")

agitation_start = index(psionics, "void spell_molecular_agitation(")
agitation_end = index(psionics, "void spell_adrenaline_control(", agitation_start)
agitation = psionics[agitation_start:agitation_end]
assert contains(agitation, "level = MIN(level, static_cast<int>(ARRAY_SIZE(dam_each)) - 1);")
assert not contains(agitation, "sizeof(dam_each[0] - 1)")

shadow_start = index(magic, "void spell_shadow_projection(")
shadow_end = index(magic, "void spell_concealment(", shadow_start)
shadow = magic[shadow_start:shadow_end]
assert contains(shadow, "af.bitvector = AFF_SNEAK;")
assert contains(shadow, "af.bitvector |= AFF_HIDE;")
assert not contains(shadow, "af.bitvector = AFF_HIDE;")

print("spell affect regression contracts passed")
