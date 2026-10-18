#!/usr/bin/env python3
"""Booting more ships than the ship-room pool holds leaves no broken ship behind.

Ship interiors come from a fixed pool of rooms, 60003..64999.  Once SQL boot
loaded every ship row instead of the first 512, a large enough `ships` table
could run the pool dry: 700 frigates need 6,300 rooms and the pool has 4,997.
Every ship past that point failed load_ship(), and three things went wrong:

- set_ship_physical_layout() kept the rooms it had claimed before running out,
  so they were lost to every other ship for the life of the process;
- sql_load_all_ships() logged the failure but left the ship registered;
- the ship still carried LOADED from its saved flags, with no rooms, so an
  orderly shutdown read world[real_room(-1)] in shutdown_ships().

This runs the real sql_load_all_ships(), load_ship(), set_ship_physical_layout()
and shutdown_ships(), with the real ship registry, over 700 persisted frigates
and two sloops, under AddressSanitizer.  Rooms are now claimed all or nothing,
a ship that cannot be placed is destroyed (its row is kept), LOADED is cleared
before placement, and shutdown skips rooms a ship never got.  The first sloop
fits in the two rooms the frigates leave, which it could not when a failed
frigate kept them.
"""

from pathlib import Path
import os
import re
import subprocess
import tempfile

from _paths import ROOT, extract_function, source

FUNCTIONS = "\n\n".join(
    [
        extract_function("ship_utils.c", signature)
        for signature in (
            "ShipObjHash::ShipObjHash()",
            "bool ShipObjHash::add(P_ship ship)",
            "bool ShipObjHash::erase(P_ship ship)\n",
            "bool ShipObjHash::erase(P_ship ship, unsigned t_index)",
            "bool ShipObjHash::get_first(visitor &vs)",
            "bool ShipObjHash::get_next(visitor &vs)",
        )
    ]
    + [
        extract_function("ship_base.c", signature)
        for signature in (
            "void init_ship_layout(P_ship ship)",
            "void set_ship_layout(P_ship ship, int m_class)",
            "int find_free_ship_room()",
            "static void release_ship_rooms(P_ship ship)",
            "bool set_ship_physical_layout(P_ship ship)",
            "int load_ship(P_ship ship, int to_room)",
            "void shutdown_ships()",
        )
    ]
    + [extract_function("sql_player.c", "bool sql_load_all_ships()")]
)

HARNESS = r'''
#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "ships/ships.h"
#include "sql/sql.h"
#include "sql/sql_player.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <set>
#include <string>
#include <vector>

static const int FRIGATES = 700, SLOOPS = 2, ANCHOR_VNUM = 43220;
static const int POOL_FIRST = SHIPZONE * 100 + 3; // past the three reserved rooms

// The anchor room at index 0, then every pool room in vnum order.
P_room world;
int top_of_world;
int shiperror = 0;
char buf[MAX_STRING_LENGTH];
const ShipTypeData ship_type_data[MAXSHIPCLASS] = {};
ShipObjHash shipObjHash;

int real_room(const int vnum)
{
	if (vnum == ANCHOR_VNUM)
		return 0;
	if (vnum < VROOM_SHIPS_START || vnum > VROOM_SHIPS_END)
		return NOWHERE;
	return 1 + vnum - VROOM_SHIPS_START;
}
int real_room0(const int vnum) { return real_room(vnum) == NOWHERE ? 0 : real_room(vnum); }
int ship_room_proc(int, P_char, int, char *) { return 0; }

void *__malloc(size_t size, const char *, const char *, int) { return calloc(1, size); }
void __free(void *p, const char *, int) { free(p); }
int panic_corruption_int(const char *, const char *, ...) { abort(); }
void panic_corruption(const char *, const char *, ...) { abort(); }
void fatal_boot_error(const char *, const char *, ...) { abort(); }
void logit(const char *, const char *, ...) {}
int BOUNDED(int low, int value, int high) { return value < low ? low : value > high ? high : value; }

void obj_to_room(P_obj, int) {}
void obj_from_room(P_obj) {}
void char_from_room(P_char) {}
bool char_to_room(P_char, int, int) { return true; }
void name_ship(const char *, P_ship) {}
void name_ship_rooms(P_ship) {}
void update_crew(P_ship) {}
void reset_crew_stamina(P_ship) {}
void set_ship_armor(P_ship, bool) {}
void update_ship_status(P_ship, P_ship) {}

bool sql_begin_transaction(void) { return true; }
bool sql_commit(void) { return true; }
bool sql_rollback(void) { return true; }
static int writes = 0;
int write_ship(P_ship ship) // like the real one, it refuses an unloaded ship
{
	if (!SHIP_LOADED(ship))
		return FALSE;
	++writes;
	return TRUE;
}

// A ship that could not be placed is destroyed without deleting its row, after
// it has left the registry, and holds no rooms and no LOADED flag.
static int destroyed = 0;
void delete_ship(P_ship ship, bool keep_row)
{
	assert(keep_row && !ship->next && !SHIP_LOADED(ship));
	for (int i = 0; i < MAX_SHIP_ROOM; i++)
		assert(SHIP_ROOM_NUM(ship, i) == -1);
	ShipVisitor svs;
	for (bool fn = shipObjHash.get_first(svs); fn; fn = shipObjHash.get_next(svs))
		assert(svs.get_value() != ship);
	++destroyed;
	delete ship->shipobj;
	delete ship->panel;
	delete ship;
}

// The `ships` table: owner names, then one ship per row as sql_load_ship()
// builds it, with the flags it was saved with.
static int handle = 0, result_handle = 0;
MYSQL *DB = reinterpret_cast<MYSQL *>(&handle);
static std::vector<std::string> owners;
static std::vector<char *> rows;
static size_t next_row = 0;

MYSQL_RES *db_query_at(struct persistence_query_site, const char *, ...)
{
	next_row = 0;
	return reinterpret_cast<MYSQL_RES *>(&result_handle);
}
MYSQL_ROW mysql_fetch_row(MYSQL_RES *) { return next_row < rows.size() ? &rows[next_row++] : nullptr; }
void mysql_free_result(MYSQL_RES *) {}

P_ship sql_load_ship(const char *owner)
{
	P_ship ship = new ShipData{};
	ship->shipobj = new obj_data{};
	ship->panel = new obj_data{};
	ship->db_id = 1 + static_cast<int>(next_row);
	ship->m_class = strncmp(owner, "Sloop", 5) ? SH_FRIGATE : SH_SLOOP;
	ship->anchor = ANCHOR_VNUM;
	ship->flags = LOADED | DOCKED; // every saved ship was loaded when saved
	init_ship_layout(ship);
	set_ship_layout(ship, ship->m_class);
	shipObjHash.add(ship);
	return ship;
}

''' + FUNCTIONS + r'''

static int rooms_in_use()
{
	int used = 0;
	for (int i = 0; i <= top_of_world; i++)
		used += world[i].funct == ship_room_proc;
	return used;
}

int main()
{
	top_of_world = VROOM_SHIPS_END - VROOM_SHIPS_START + 1;
	world = new room_data[top_of_world + 1]{};
	world[0].number = ANCHOR_VNUM;
	for (int vnum = VROOM_SHIPS_START; vnum <= VROOM_SHIPS_END; vnum++)
		world[real_room(vnum)].number = vnum;
	const int pool = VROOM_SHIPS_END - POOL_FIRST + 1;

	for (int i = 0; i < FRIGATES; i++)
		owners.push_back("Frigate" + std::to_string(i));
	for (int i = 0; i < SLOOPS; i++)
		owners.push_back("Sloop" + std::to_string(i));
	for (auto &owner : owners)
		rows.push_back(owner.data());

	assert(sql_load_all_ships());

	// Every frigate that fits, and the sloop that fits in what they leave.
	const int frigates_placed = pool / 9;
	assert(frigates_placed == 555 && pool - frigates_placed * 9 == 2);
	assert(shipObjHash.size() == frigates_placed + 1);
	assert(destroyed == FRIGATES - frigates_placed + 1);

	// What stays registered is loaded, and owns distinct rooms from the pool,
	// which are exactly the rooms in use.
	std::set<int> owned;
	ShipVisitor svs;
	for (bool fn = shipObjHash.get_first(svs); fn; fn = shipObjHash.get_next(svs))
	{
		P_ship ship = svs;
		assert(SHIP_LOADED(ship) && ship->room_count > 0);
		assert(ship->bridge == SHIP_ROOM_NUM(ship, 0));
		for (int i = 0; i < ship->room_count; i++)
		{
			const int vnum = SHIP_ROOM_NUM(ship, i);
			assert(vnum >= POOL_FIRST && vnum <= VROOM_SHIPS_END);
			assert(world[real_room(vnum)].funct == ship_room_proc);
			assert(owned.insert(vnum).second);
		}
	}
	assert((int)owned.size() == pool && rooms_in_use() == pool);

	// A frigate that cannot be placed gives back what it claimed, and keeps
	// its room graph for a later try.
	P_ship late = sql_load_ship("Frigate-late");
	shipObjHash.erase(late);
	assert(!set_ship_physical_layout(late));
	assert(rooms_in_use() == pool && late->room_count == 9);
	for (int i = 0; i < MAX_SHIP_ROOM; i++)
		assert(SHIP_ROOM_NUM(late, i) == -1);
	assert(!load_ship(late, 0) && !SHIP_LOADED(late));
	delete_ship(late, true);

	// A ship registered without rooms, as an unplaced ship once was, does not
	// send shutdown outside the world.
	P_ship unplaced = new ShipData{};
	unplaced->shipobj = new obj_data{};
	init_ship_layout(unplaced);
	set_ship_layout(unplaced, SH_FRIGATE);
	shipObjHash.add(unplaced);

	shutdown_ships();
	assert(writes == frigates_placed + 1);

	puts("a full ship-room pool leaves every registered ship placed, and shuts down cleanly");
	return 0;
}
'''


def load_ship_failure_blocks(name):
    """Yield the body of each live `if (!load_ship(...))` failure branch in `name`."""
    text = re.sub(r"/\*.*?\*/", "", source(name).read_text(encoding="utf-8"), flags=re.S)
    for match in re.finditer(r"if \(!load_ship\([^;{]*\)\)\s*", text):
        start = match.end()
        if text[start] != "{":
            yield text[start:text.index(";", start) + 1]
            continue
        depth = 0
        for end in range(start, len(text)):
            depth += {"{": 1, "}": -1}.get(text[end], 0)
            if not depth:
                yield text[start:end + 1]
                break


# Every other caller destroys a ship that load_ship() could not place, rather
# than leaving it registered without rooms.
for name in ("ship_shop.c", "ship_npc.c", "sql_player.c", "ship_base.c"):
    blocks = list(load_ship_failure_blocks(name))
    assert blocks, name
    for block in blocks:
        assert "shipObjHash.erase(" in block and "delete_ship(" in block, (name, block)

with tempfile.TemporaryDirectory(prefix="duris-ship-pool-") as directory:
    path = Path(directory)
    (path / "test.cpp").write_text(HARNESS, encoding="utf-8")
    subprocess.run(
        ["g++", "-std=c++20", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
         "-Isrc", "-Itests/async", "-I/usr/include/mysql",
         str(path / "test.cpp"), "-o", str(path / "test")],
        cwd=ROOT, check=True)
    subprocess.run([str(path / "test")], check=True,
                   env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
