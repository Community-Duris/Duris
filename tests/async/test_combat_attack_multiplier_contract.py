#!/usr/bin/env python3
"""Combat-mind must raise a reduced attack multiplier to its configured floor."""

from _paths import SRC
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
fight = (SRC / "fight.c").read_text()

combat_mind = fight.split("if (affected_by_spell(ch, SPELL_COMBAT_MIND))", 1)[1]
combat_mind = combat_mind.split("// we ceil to not round off attacks", 1)[0]

assert 'get_property("attacks.combatMind.multiplier", 0.75)' in combat_mind
assert "if (attacksMultiplier < cmMulti)" in combat_mind
assert "attacksMultiplier = cmMulti;" in combat_mind
assert "cmMulti = cmMulti;" not in combat_mind

print("combat attack multiplier contract passed")
