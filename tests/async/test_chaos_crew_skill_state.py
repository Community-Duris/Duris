#!/usr/bin/env python3
"""Regression contracts for authoritative crew-skill mutation."""

from __future__ import annotations

import subprocess
import tempfile
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
assert "void set_crew_skills(P_ship ship, float sail_skill, float guns_skill, float rpar_skill," in SHIPS

helper_start = SHIP_UTILS.index("void set_crew_skills(")
body_start = SHIP_UTILS.index("{", helper_start)
depth = 0
body_end = None
for index in range(body_start, len(SHIP_UTILS)):
    if SHIP_UTILS[index] == "{":
        depth += 1
    elif SHIP_UTILS[index] == "}":
        depth -= 1
        if depth == 0:
            body_end = index + 1
            break
assert body_end is not None
helper_definition = SHIP_UTILS[helper_start:body_end]
harness = f"""
#include <cmath>

struct Crew {{
    float sail_skill = 0.0f;
    float guns_skill = 0.0f;
    float rpar_skill = 0.0f;
}};
struct Ship {{ Crew crew; }};
using P_ship = Ship *;
int updates = 0;
int status_updates = 0;
int queued_saves = 0;
void update_crew(P_ship) {{ ++updates; }}
void update_ship_status(P_ship) {{ ++status_updates; }}
void queue_ship_save(P_ship, const char *) {{ ++queued_saves; }}
{helper_definition}

int main() {{
    Ship ship;
    ship.crew.guns_skill = 2.25f;
    ship.crew.rpar_skill = 3.75f;
    set_crew_skills(&ship, 1.5f, ship.crew.guns_skill, ship.crew.rpar_skill, "test");
    if (std::fabs(ship.crew.sail_skill - 1.5f) > 0.001f ||
        std::fabs(ship.crew.guns_skill - 2.25f) > 0.001f ||
        std::fabs(ship.crew.rpar_skill - 3.75f) > 0.001f ||
        updates != 1 || status_updates != 1 || queued_saves != 1)
        return 1;
    return 0;
}}
"""
with tempfile.TemporaryDirectory(prefix="duris-crew-skill-") as temporary:
    temporary_path = Path(temporary)
    harness_path = temporary_path / "crew_skill.cpp"
    binary_path = temporary_path / "crew_skill"
    harness_path.write_text(harness, encoding="utf-8")
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Werror",
            str(harness_path),
            "-o",
            str(binary_path),
        ],
        check=True,
    )
    subprocess.run([str(binary_path)], check=True)

# The original Chaos validation and staff command surface remain present.
assert 'if (s < 0 || g < 0 || r < 0)' in CHAOS
assert 'if (s > max || g > max || r > max)' in CHAOS
assert 'CMD_CHAOS' in (SRC / "cmd/interp.c").read_text(encoding="utf-8")

print("Chaos crew-skill derived-state/save contracts passed")
