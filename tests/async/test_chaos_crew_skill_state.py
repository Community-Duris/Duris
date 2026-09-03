#!/usr/bin/env python3
"""Regression contracts for authoritative crew-skill mutation."""

from __future__ import annotations

from pathlib import Path

from _paths import SRC


CHAOS = (SRC / "combat/chaos.c").read_text(encoding="utf-8")
ACTSET = (SRC / "cmd/actset.c").read_text(encoding="utf-8")
SHIP_UTILS = (SRC / "ships/ship_utils.c").read_text(encoding="utf-8")
SHIPS = (SRC / "ships/ships.h").read_text(encoding="utf-8")


def section(text: str, start_marker: str, end_marker: str) -> str:
    start = text.index(start_marker)
    end = text.index(end_marker, start)
    return text[start:end]


chaos_branch = section(
    CHAOS,
    'if (is_abbrev(buff, "crewexperience"))',
    'if (is_abbrev(buff, "portal"))',
)
assert "set_crew_skills(ship, s, g, r," in chaos_branch
assert "ship->crew.sail_skill = s" not in chaos_branch
assert "ship->crew.guns_skill = g" not in chaos_branch
assert "ship->crew.rpar_skill = r" not in chaos_branch

for skill_flag, arguments in (
    ("sailskill", "atoi(val), ship->crew.guns_skill, ship->crew.rpar_skill"),
    ("gunskill", "ship->crew.sail_skill, atoi(val), ship->crew.rpar_skill"),
    ("repairskill", "ship->crew.sail_skill, ship->crew.guns_skill, atoi(val)"),
):
    start = ACTSET.index(f'if (SAME_STRING(flag, "{skill_flag}"))')
    end = ACTSET.index("\n\t}", start) + 3
    branch = ACTSET[start:end]
    assert f"set_crew_skills(ship, {arguments}," in branch

helper = section(SHIP_UTILS, "void set_crew_skills(", "void reset_crew_stamina(")
for assignment in (
    "ship->crew.sail_skill = sail_skill;",
    "ship->crew.guns_skill = guns_skill;",
    "ship->crew.rpar_skill = rpar_skill;",
):
    assert assignment in helper
assert "update_crew(ship);" in helper
assert "update_ship_status(ship);" in helper
assert "queue_ship_save(ship, reason);" in helper
assert "void set_crew_skills(P_ship" in SHIPS

# The original Chaos validation and staff command surface remain present.
assert 'if (s < 0 || g < 0 || r < 0)' in CHAOS
assert 'if (s > max || g > max || r > max)' in CHAOS
assert 'CMD_CHAOS' in (SRC / "cmd/interp.c").read_text(encoding="utf-8")

print("Chaos crew-skill derived-state/save contracts passed")
