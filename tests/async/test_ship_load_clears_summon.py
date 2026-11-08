#!/usr/bin/env python3
"""A ship loaded at boot is never left waiting on a summon that no longer exists.

`summon ship` raises SUMMONED and schedules summon_ship_event(), which docks the
ship at the caller's port and clears the flag.  The flag is saved with the rest
of `flags`, but the event is not.  After a reboot or copyover the ship loaded at
its old anchor with SUMMONED still set, and every later summon was refused with
"There is already an order out on your ship" until a hull change or sinking
cleared it.

load_ship() already drops the other transient flags (sinking, flying, ramming,
NPC combat).  This runs the real load_ship() and checks that it drops SUMMONED
too, and still keeps the ship's persistent flags.
"""

from pathlib import Path
import subprocess
import tempfile

from _paths import ROOT, extract_function

HARNESS = r'''
#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "ships/ships.h"

#include <cassert>
#include <cstdio>

static room_data rooms[4] = {};
P_room world = rooms;
int shiperror = 0;

int real_room0(const int vnum) { return vnum == 60100 ? 2 : 0; }
void obj_to_room(P_obj, int) {}
bool set_ship_physical_layout(P_ship ship)
{
	ship->bridge = 60100;
	return true;
}
void update_crew(P_ship) {}
void reset_crew_stamina(P_ship) {}
void update_ship_status(P_ship, P_ship) {}

''' + extract_function("ship_base.c", "int load_ship(P_ship ship, int to_room)") + r'''

int main()
{
	obj_data hull = {}, panel = {};
	ShipData ship = {};
	ship.shipobj = &hull;
	ship.panel = &panel;
	rooms[1].number = 43220;

	// As saved while a summon was under way: in transit, summon pending.
	ship.flags = SUMMONED | AIR | SINKING;
	assert(load_ship(&ship, 1));
	assert(!IS_SET(ship.flags, SUMMONED));
	assert(!IS_SET(ship.flags, SINKING));
	assert(IS_SET(ship.flags, AIR));
	assert(IS_SET(ship.flags, LOADED) && IS_SET(ship.flags, DOCKED));
	assert(ship.anchor == 43220 && ship.location == 1);

	puts("load_ship clears a stale summon and keeps persistent flags");
	return 0;
}
'''

with tempfile.TemporaryDirectory(prefix="duris-ship-load-") as directory:
    path = Path(directory)
    (path / "test.cpp").write_text(HARNESS, encoding="utf-8")
    subprocess.run(
        ["g++", "-std=c++20", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
         "-Isrc", "-Itests/async", "-I/usr/include/mysql",
         str(path / "test.cpp"), "-o", str(path / "test")],
        cwd=ROOT, check=True)
    subprocess.run([str(path / "test")], check=True)
