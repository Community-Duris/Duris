#!/usr/bin/env python3
"""The Ship Damage Control epic skill reduces damage to its owner's ship.

SKILL_SHIP_DAMAGE_CONTROL is sold as an epic reward (80 epic points and 8,000
platinum) and taught by the Commodore in Headless, and its help entry promises
that the captain's ship "endures blows that would send lesser hulls to the
bottom".  Its only call sites, in damage_sail() and damage_hull(), had been
commented out, so the skill did nothing.

This runs the real epic_ship_damage_control(), damage_sail() and damage_hull():
with the owner aboard, a skill of 100 removes 24% of each hit; without the owner
aboard, or without the skill, the full damage lands.
"""

from pathlib import Path
import subprocess
import tempfile

from _paths import ROOT, extract_function

SOURCE = "ship_combat.c"
FUNCTIONS = "\n\n".join(
    extract_function(SOURCE, signature)
    for signature in (
        "int epic_ship_damage_control(P_char ch, int dam)",
        "int damage_sail(P_ship attacker, P_ship target, int dam)",
        "int damage_hull(P_ship attacker, P_ship target, int dam, int arc, int armor_pierce)",
    )
)

HARNESS = r'''
#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "ships/ships.h"
#include "magic/spells.h"

#include <cassert>
#include <cstdio>

static P_char aboard = nullptr;
static int skill = 0;

P_char captain_is_aboard(P_ship) { return aboard; }
int GET_CHAR_SKILL_P(P_char, int number)
{
	assert(number == SKILL_SHIP_DAMAGE_CONTROL);
	return skill;
}
int number(int low, int) { return low; }
void act_to_all_in_ship(P_ship, const char *) {}
void act_to_all_in_ship_f(P_ship, const char *, ...) {}
void act_to_outside_ships(P_ship, P_ship, int, const char *, ...) {}
const char *get_arc_name(int) { return "fore"; }
void damage_weapon(P_ship, P_ship, int, int) {}
void stun_all_in_ship(P_ship, int) {}

''' + FUNCTIONS + r'''

static int sail_after(int dam)
{
	ShipData ship = {};
	ship.mainsail = 1000;
	damage_sail(nullptr, &ship, dam);
	return 1000 - ship.mainsail;
}

static int hull_after(int dam)
{
	ShipData ship = {};
	ship.armor[SIDE_FORE] = 1000;
	ship.internal[SIDE_FORE] = 1000;
	damage_hull(nullptr, &ship, dam, SIDE_FORE, 0);
	return 1000 - ship.armor[SIDE_FORE];
}

int main()
{
	char_data captain = {};
	captain.specials.position = STAT_NORMAL;

	// No owner aboard: full damage.
	aboard = nullptr;
	skill = 100;
	assert(sail_after(100) == 100 && hull_after(100) == 100);

	// The owner is aboard but never learned the skill: full damage.
	aboard = &captain;
	skill = 0;
	assert(sail_after(100) == 100 && hull_after(100) == 100);

	// The owner is aboard with the skill mastered: 4% + 20% = 24% off.
	skill = 100;
	assert(sail_after(100) == 76 && hull_after(100) == 76);

	// Half-learned: 14% off.  Small hits keep at least one point.
	skill = 50;
	assert(sail_after(100) == 86 && hull_after(100) == 86);
	assert(sail_after(1) == 1 && hull_after(2) >= 1);

	puts("ship damage control reduces damage to its owner's ship");
	return 0;
}
'''

with tempfile.TemporaryDirectory(prefix="duris-ship-dc-") as directory:
    path = Path(directory)
    (path / "test.cpp").write_text(HARNESS, encoding="utf-8")
    subprocess.run(
        ["g++", "-std=c++20", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
         "-Isrc", "-Itests/async", "-I/usr/include/mysql",
         str(path / "test.cpp"), "-o", str(path / "test")],
        cwd=ROOT, check=True)
    subprocess.run([str(path / "test")], check=True)
