#!/usr/bin/env python3
"""Regression contract for Moonwell's stored Moonstone target."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
MAGIC = (ROOT / "src/magic.c").read_text()

start = MAGIC.index("void spell_moonwell(")
end = MAGIC.index("void spell_moonstone(", start)
moonwell = MAGIC[start:end]

normalize = moonwell.index('!str_cmp(arg, "moonstone")')
npc_rejection = moonwell.index("if (IS_NPC(victim))")
portal = moonwell.index("spell_general_portal(")

assert "affected_by_spell(ch, SPELL_MOONSTONE)" in moonwell
assert "victim = ch;" in moonwell[normalize:npc_rejection]
assert normalize < npc_rejection < portal

print("Moonwell Moonstone target contract passed")
