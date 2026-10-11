#!/usr/bin/env python3
"""A hull purchase checks the room pool, and never sells a second ship.

Two gaps around a full ship-room pool:

- A hull upgrade freed the ship's rooms in reset_ship() and then ignored a
  failed set_ship_physical_layout(), so upgrading into a nearly full pool left
  a loaded ship with no rooms.
- A ship the pool could not hold at boot stayed stored but out of the world,
  so its owner could buy another.  That ship's first save then collided with
  the stored one on UNIQUE(owner_name) and kept failing, and the purchase was
  lost at the next reboot.

This runs the real ship_hull_purchase_committed() and refuse_for_stored_ship().
The room check (ship_rooms_fit_class()) and stored-ship placement
(place_stored_ship()) are stubbed here; test_ship_boot_room_pool_full.py runs
them for real.  An upgrade that does not fit is refunded and changes nothing.
A first-ship purchase by an owner with a stored ship brings that ship back, or
is refused while there is no room, and is refunded either way.  buy_hull()
refuses both before any epic transaction starts.
"""

from pathlib import Path
import re
import subprocess
import tempfile

from _paths import ROOT, extract_function, source

SHOP = source("ship_shop.c").read_text(encoding="utf-8")
CONTEXT = SHOP[SHOP.index("struct ship_hull_purchase_context\n{") :]
CONTEXT = CONTEXT[: CONTEXT.index("};") + 2]
FUNCTIONS = "\n\n".join(
    extract_function("ship_shop.c", signature)
    for signature in (
        "static bool refuse_for_stored_ship(P_char ch)",
        "void ship_hull_purchase_committed(P_char ch, bool committed,",
    )
)

HARNESS = r'''
#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "magic/spells.h"
#include "ships/ships.h"
#include "world/epic_command.h"

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <string>

''' + CONTEXT + r'''

static room_data rooms[4] = {};
P_room world = rooms;
const ShipTypeData ship_type_data[MAXSHIPCLASS] = {};
ShipObjHash shipObjHash;
ShipObjHash::ShipObjHash() {}
bool ShipObjHash::erase(P_ship) { return true; }
int ShipData::slot_weight(int) const { return 0; }

static ShipData owned_ship, bought_ship;
static bool owns_ship = false, fits = true;
static stored_ship_state stored = stored_ship_state::none;
static int refunds = 0, resets = 0, created = 0, saves = 0;
static std::string told;

P_ship get_ship_from_owner(char *) { return owns_ship ? &owned_ship : nullptr; }
bool ship_rooms_fit_class(P_ship ship, int m_class)
{
	assert(ship == &owned_ship && m_class == SH_FRIGATE);
	return fits;
}
stored_ship_state place_stored_ship(const char *owner)
{
	assert(!strcmp(owner, "Owner"));
	return stored;
}
void refund_ship_epics(P_char, const ship_hull_purchase_context &) { ++refunds; }
struct ShipData *new_ship(int, bool)
{
	++created;
	return &bought_ship;
}
int load_ship(P_ship, int) { return TRUE; }
void name_ship(const char *, P_ship) {}
void delete_ship(P_ship, bool) {}
void reset_ship(P_ship ship, bool)
{
	assert(ship == &owned_ship && ship->m_class == SH_FRIGATE);
	++resets;
}
void kick_everyone_off(P_ship) {}
void update_ship_status(P_ship, P_ship) {}
void queue_ship_save(P_ship, const char *) { ++saves; }
bool ocean_pvp_state() { return false; }
int SUB_MONEY(P_char, int, int) { return 0; }
void ADD_MONEY(P_char, int, const char *) {}
void affect_from_char(P_char, int) {}
char *str_dup(const char *text) { return strdup(text); }
void send_to_char(const char *text, P_char) { told += text; }
void send_to_char_f(P_char, const char *, ...) {}
int panic_corruption_int(const char *, const char *, ...) { abort(); }

''' + FUNCTIONS + r'''

static char_data buyer;

static void reset(bool owner_has_ship)
{
	owned_ship = ShipData{};
	owned_ship.m_class = SH_SLOOP;
	bought_ship = ShipData{};
	owns_ship = owner_has_ship;
	fits = true;
	stored = stored_ship_state::none;
	refunds = resets = created = saves = 0;
	told.clear();
	buyer = char_data{};
	buyer.player.name = const_cast<char *>("Owner");
	buyer.player.level = 50;
	buyer.points.cash[3] = 1000;
}

static void buy(bool owned)
{
	ship_hull_purchase_context context = {};
	context.hull_type = SH_FRIGATE;
	context.old_hull = owned ? SH_SLOOP : -1;
	context.coin_delta = 1000;
	context.epic_cost = 10;
	context.room = 1;
	context.owned = owned;
	strcpy(context.name, "Brine");
	ship_hull_purchase_committed(&buyer, true, epic_command_result{}, 0,
				     reinterpret_cast<const uint8_t *>(&context), sizeof(context));
}

int main()
{
	// An upgrade that fits rebuilds the hull.
	reset(true);
	buy(true);
	assert(resets == 1 && refunds == 0 && saves == 1);

	// One the pool cannot hold is refunded and leaves the ship alone.
	reset(true);
	fits = false;
	buy(true);
	assert(resets == 0 && refunds == 1 && saves == 0);
	assert(owned_ship.m_class == SH_SLOOP);
	assert(told.find("no room to build that hull") != std::string::npos);

	// A first ship for someone with no stored ship is built.
	reset(false);
	buy(false);
	assert(created == 1 && refunds == 0 && saves == 1);

	// An owner whose stored ship is out of the world gets it back, or is told
	// to wait, and is refunded either way: no second ship is made.
	for (auto state : { stored_ship_state::placed, stored_ship_state::no_room,
			    stored_ship_state::unreadable })
	{
		reset(false);
		stored = state;
		buy(false);
		assert(created == 0 && refunds == 1 && saves == 0);
		assert(told.find(state == stored_ship_state::unreadable ? "could not be read" :
									 "You already own a ship") !=
		       std::string::npos);
	}

	puts("hull purchases check the room pool and never sell a second ship");
	return 0;
}
'''

# buy_hull() applies the same two checks before it starts an epic transaction.
buy_hull = extract_function("ship_shop.c", "int buy_hull(P_char ch, P_ship ship, int owned")
owned_branch = buy_hull[: buy_hull.index("\telse\n\t{\n\t\tif (refuse_for_stored_ship(ch))")]
assert "if (!ship_rooms_fit_class(ship, hull_type))" in owned_branch
assert owned_branch.index("ship_rooms_fit_class") < owned_branch.index("submit_ship_hull_purchase")
new_branch = buy_hull[buy_hull.index("\telse\n\t{\n\t\tif (refuse_for_stored_ship(ch))") :]
assert new_branch.index("refuse_for_stored_ship") < new_branch.index("submit_ship_hull_purchase")

with tempfile.TemporaryDirectory(prefix="duris-ship-purchase-") as directory:
    path = Path(directory)
    (path / "test.cpp").write_text(HARNESS, encoding="utf-8")
    subprocess.run(
        ["g++", "-std=c++20", "-g", "-fsanitize=address,undefined", "-fno-pie", "-no-pie", "-fno-omit-frame-pointer",
         "-Isrc", "-Itests/async", "-I/usr/include/mysql",
         str(path / "test.cpp"), "-o", str(path / "test")],
        cwd=ROOT, check=True)
    subprocess.run([str(path / "test")], check=True, env={"ASAN_OPTIONS": "detect_leaks=0"})
