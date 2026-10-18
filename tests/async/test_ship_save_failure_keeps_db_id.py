#!/usr/bin/env python3
"""A failed ship save forgets the row id only when that save created the row.

sql_save_ship() inserts a new ship's `ships` row and then writes its armour, crew
and slot rows in one batch, all in one transaction.  When the batch or the commit
fails, the transaction is rolled back.  For a new ship that removes the row the
save just inserted, so the ship must go back to `db_id == -1` and insert again
next time.

The failure paths reset `db_id` for every ship, including one whose row already
existed.  The next save then took the insert path, collided with
`UNIQUE(owner_name)` and failed, and every later save failed the same way until
reboot (shutdown_ships() then treats that as corruption).  The commit-failure path
had the opposite defect: a new ship kept the id of a row that was rolled back, so
later saves updated a row that did not exist.

This runs the real sql_save_ship() and its batch helpers against a scripted
database that enforces `UNIQUE(owner_name)`.
"""

from pathlib import Path
import subprocess
import tempfile

from _paths import ROOT, source

# The MySQL build's batch size, the three batch helpers and sql_save_ship()
# itself.  The file also has a __NO_MYSQL__ stub of sql_save_ship() earlier on.
SQL_PLAYER = source("sql_player.c").read_text(encoding="utf-8")
START = SQL_PLAYER.index("#define SHIP_SQL_BATCH_SIZE")
SAVE_SHIP = SQL_PLAYER[START:SQL_PLAYER.index("static bool sql_load_ship_armor(", START)]
assert SAVE_SHIP.count("bool sql_save_ship(P_ship ship)") == 1

HARNESS = r'''
#include "core/structs.h"
#include "ships/ships.h"
#include "sql/sql.h"
#include "sql/sql_player.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

static int handle = 0;
MYSQL *DB = reinterpret_cast<MYSQL *>(&handle);

// A one-table database: whether the owner's `ships` row exists, and whether the
// current transaction inserted it.
static bool row_committed = false, row_pending = false;
static bool in_transaction = false;
static bool fail_batch = false, fail_commit = false;
static int inserts = 0, rollbacks = 0;
static const int ROW_ID = 7;
static char id_text[16];
static char *id_row[] = { id_text };
static int result_handle = 0;

bool sql_in_transaction(void) { return in_transaction; }
bool sql_begin_transaction(void)
{
	assert(!in_transaction);
	in_transaction = true;
	return true;
}
bool sql_commit(void)
{
	assert(in_transaction);
	if (fail_commit)
		return false;
	in_transaction = false;
	row_committed = row_committed || row_pending;
	row_pending = false;
	return true;
}
bool sql_rollback(void)
{
	assert(in_transaction);
	in_transaction = false;
	row_pending = false;
	++rollbacks;
	return true;
}

char *sql_escape_string(const char *text) { return strdup(text); }
void sql_player_error(const char *) {}
void logit(const char *, const char *, ...) {}

static bool sql_run_query(const char *query)
{
	assert(!strncmp(query, "insert into ships ", 18));
	++inserts;
	if (row_committed || row_pending)
		return false; // UNIQUE(owner_name)
	row_pending = true;
	return true;
}

MYSQL_RES *db_query_at(struct persistence_query_site, const char *, ...)
{
	return reinterpret_cast<MYSQL_RES *>(&result_handle);
}
MYSQL_ROW mysql_fetch_row(MYSQL_RES *)
{
	if (!row_committed && !row_pending)
		return nullptr;
	snprintf(id_text, sizeof id_text, "%d", ROW_ID);
	return id_row;
}
void mysql_free_result(MYSQL_RES *) {}
MYSQL_RES *mysql_store_result(MYSQL *) { return nullptr; }
bool sql_trace_exec_at(struct persistence_query_site, const char *, const char *, size_t, bool,
		       bool)
{
	return !fail_batch;
}
void sql_clear_results() {}

''' + SAVE_SHIP + r'''

static ShipData ship;
static char owner[] = "Owner", name[] = "Brine";

static void reset(bool row_exists, int db_id, bool outer_transaction = false)
{
	row_committed = row_exists;
	row_pending = false;
	in_transaction = outer_transaction;
	fail_batch = fail_commit = false;
	inserts = rollbacks = 0;
	ship = ShipData{};
	ship.ownername = owner;
	ship.name = name;
	ship.db_id = db_id;
}

int main()
{
	// An existing ship whose batch fails keeps its id, and its next save updates.
	reset(true, ROW_ID);
	fail_batch = true;
	assert(!sql_save_ship(&ship));
	assert(rollbacks == 1 && inserts == 0);
	assert(ship.db_id == ROW_ID);
	fail_batch = false;
	assert(sql_save_ship(&ship));
	assert(inserts == 0 && ship.db_id == ROW_ID && !in_transaction);

	// The same after a failed commit.
	reset(true, ROW_ID);
	fail_commit = true;
	assert(!sql_save_ship(&ship));
	assert(rollbacks == 1 && ship.db_id == ROW_ID);
	fail_commit = false;
	assert(sql_save_ship(&ship));
	assert(inserts == 0 && ship.db_id == ROW_ID);

	// Inside a caller's transaction (shutdown_ships()) nothing is rolled back
	// here, and the existing ship still keeps its id.
	reset(true, ROW_ID, true);
	fail_batch = true;
	assert(!sql_save_ship(&ship));
	assert(rollbacks == 0 && in_transaction && ship.db_id == ROW_ID);

	// A new ship whose batch fails loses the rolled-back row's id, and inserts
	// again next time.
	reset(false, -1);
	fail_batch = true;
	assert(!sql_save_ship(&ship));
	assert(rollbacks == 1 && inserts == 1 && !row_committed);
	assert(ship.db_id == -1);
	fail_batch = false;
	assert(sql_save_ship(&ship));
	assert(inserts == 2 && row_committed && ship.db_id == ROW_ID);

	// The same after a failed commit: the row was rolled back, so the id goes.
	reset(false, -1);
	fail_commit = true;
	assert(!sql_save_ship(&ship));
	assert(rollbacks == 1 && inserts == 1 && !row_committed);
	assert(ship.db_id == -1);
	fail_commit = false;
	assert(sql_save_ship(&ship));
	assert(inserts == 2 && row_committed && ship.db_id == ROW_ID);

	// A new ship inside a caller's transaction also forgets the id it inserted.
	reset(false, -1, true);
	fail_batch = true;
	assert(!sql_save_ship(&ship));
	assert(rollbacks == 0 && in_transaction && ship.db_id == -1);

	puts("ship save failure keeps an existing db_id and forgets a rolled-back insert");
	return 0;
}
'''

with tempfile.TemporaryDirectory(prefix="duris-ship-save-") as directory:
    path = Path(directory)
    (path / "test.cpp").write_text(HARNESS, encoding="utf-8")
    subprocess.run(
        ["g++", "-std=c++20", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
         "-Isrc", "-Itests/async", "-I/usr/include/mysql",
         str(path / "test.cpp"), "-o", str(path / "test")],
        cwd=ROOT, check=True)
    subprocess.run([str(path / "test")], check=True)
