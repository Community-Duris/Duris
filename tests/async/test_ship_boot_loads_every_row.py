#!/usr/bin/env python3
"""SQL boot loads every ship row, not just the first 512.

sql_load_all_ships() collects the owner names first and then loads each ship, so
the load never nests a query inside the open result.  It collected them into a
fixed `char owner_names[512][64]` and stopped reading at 512, silently leaving
every further ship unloaded, although MAXSHIPS is 2000 and the flat-file backend
has no such cap.  Names were also cut to 63 bytes.

This runs the real sql_load_all_ships() against a result of 700 rows, one of
them with a NULL owner and one with a long owner name.
"""

from pathlib import Path
import subprocess
import tempfile

from _paths import ROOT, extract_function

HARNESS = r'''
#include "core/prototypes.h"
#include "core/structs.h"
#include "ships/ships.h"
#include "sql/sql.h"
#include "sql/sql_player.h"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

static const int ROWS = 700, NULL_ROW = 300, LONG_ROW = 650;
static const std::string LONG_NAME(80, 'q');

static int handle = 0, result_handle = 0;
MYSQL *DB = reinterpret_cast<MYSQL *>(&handle);
const ShipTypeData ship_type_data[MAXSHIPCLASS] = {};

static std::vector<std::string> names;
static std::vector<char *> rows;
static size_t next_row = 0;
static bool result_open = false;
static std::vector<std::string> loaded;
static ShipData ships[ROWS];
ShipObjHash shipObjHash;
ShipObjHash::ShipObjHash() {}
bool ShipObjHash::erase(P_ship) { return true; } // for a ship load_ship() refused
void delete_ship(P_ship, bool) {}

MYSQL_RES *db_query_at(struct persistence_query_site, const char *, ...)
{
	next_row = 0;
	result_open = true;
	return reinterpret_cast<MYSQL_RES *>(&result_handle);
}
MYSQL_ROW mysql_fetch_row(MYSQL_RES *)
{
	return next_row < rows.size() ? &rows[next_row++] : nullptr;
}
void mysql_free_result(MYSQL_RES *) { result_open = false; }

P_ship sql_load_ship(const char *owner_name)
{
	assert(!result_open); // no query while the owner list is still open
	loaded.emplace_back(owner_name);
	return &ships[loaded.size() - 1];
}
void name_ship(const char *, P_ship) {}
int real_room0(const int) { return 1; }
int load_ship(P_ship, int) { return TRUE; }
void update_crew(P_ship) {}
void reset_crew_stamina(P_ship) {}
void set_ship_armor(P_ship, bool) {}
void update_ship_status(P_ship, P_ship) {}
void logit(const char *, const char *, ...) {}
int BOUNDED(int low, int value, int high) { return value < low ? low : value > high ? high : value; }

''' + extract_function("sql_player.c", "bool sql_load_all_ships()") + r'''

int main()
{
	names.reserve(ROWS);
	for (int i = 0; i < ROWS; i++)
		names.push_back(i == LONG_ROW ? LONG_NAME : "Owner" + std::to_string(i));
	for (int i = 0; i < ROWS; i++)
		rows.push_back(i == NULL_ROW ? nullptr : names[i].data());

	assert(sql_load_all_ships());
	assert(loaded.size() == ROWS - 1);
	assert(loaded.front() == "Owner0" && loaded.back() == "Owner699");
	assert(loaded[LONG_ROW - 1] == LONG_NAME);

	puts("sql_load_all_ships loads every ship row");
	return 0;
}
'''

with tempfile.TemporaryDirectory(prefix="duris-ship-boot-") as directory:
    path = Path(directory)
    (path / "test.cpp").write_text(HARNESS, encoding="utf-8")
    subprocess.run(
        ["g++", "-std=c++20", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
         "-Isrc", "-Itests/async", "-I/usr/include/mysql",
         str(path / "test.cpp"), "-o", str(path / "test")],
        cwd=ROOT, check=True)
    subprocess.run([str(path / "test")], check=True)
