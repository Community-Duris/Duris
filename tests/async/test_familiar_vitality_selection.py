#!/usr/bin/env python3
"""Regression contract for NPC vitality spell selection."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SOURCE = (ROOT / "src/mobact.c").read_text()

start = SOURCE.index("bool CastClericSpell(")
end = SOURCE.index("bool CastPaladinSpell(", start)
body = SOURCE[start:end]

selection = body[body.index("npc_has_spell_slot(ch, SPELL_VITALITY)") :]
selection = selection[: selection.index("spl = SPELL_VITALITY;")]

for spell in (
    "SPELL_VITALITY",
    "SPELL_MIELIKKI_VITALITY",
    "SPELL_FALUZURES_VITALITY",
    "SPELL_ESHABALAS_VITALITY",
):
    assert f"!affected_by_spell(target, {spell})" in selection

print("NPC vitality selection contract passed")
