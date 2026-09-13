#!/usr/bin/env python3
"""Issue #259: execute the production SQL status writer with a deterministic clock."""

from pathlib import Path
import shlex
import subprocess
import tempfile

from _paths import SRC
from contract_text import index

ROOT = Path(__file__).resolve().parents[2]


def body(text: str, signature: str) -> str:
    """Extract one production definition, skipping declarations/stubs."""
    start = index(text, signature)
    while ";" in text[start + len(signature) : text.index("{", start)]:
        start = index(text, signature, start + len(signature))
    opening = text.index("{", start)
    depth = 0
    for position in range(opening, len(text)):
        if text[position] == "{":
            depth += 1
        elif text[position] == "}":
            depth -= 1
            if depth == 0:
                return text[start : position + 1]
    raise AssertionError(f"unterminated definition: {signature}")


source_text = (SRC / "sql_player.c").read_text(encoding="utf-8", errors="replace")
# The first definition is the __NO_MYSQL__ stub.  The production implementation
# is in the MySQL half of this source file, just as in the existing SQL harness.
mysql_source_text = source_text[source_text.index("\n#else\n") :]
status_save = body(mysql_source_text, "bool sql_save_player_status(P_char ch, int type, int room)")

HARNESS = r'''
#include <mysql/mysql.h>

#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "core/files.h"
#include "account/account.h"
#include "guild/assocs.h"
#include "player/player_playtime.h"
#include "net/output_preference_codec.h"
#include "player/player_revision_state.h"
#include "sql/sql.h"

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

/* The extracted function only needs a non-null service handle. */
MYSQL *DB = reinterpret_cast<MYSQL *>(static_cast<uintptr_t>(1));
room_data test_rooms[1] = {};
P_room world = test_rooms;

static time_t controlled_now = 10000;
static int lookup_pid = 0;
static my_ulonglong generated_insert_id = 100;
static bool transaction_open = false;
static std::vector<std::string> queries;

/* Linker-wrapped clock used by both time(nullptr) and time(0) in production. */
extern "C" time_t __wrap_time(time_t *out)
{
    if (out)
        *out = controlled_now;
    return controlled_now;
}

/* Minimal SQL service stubs: capture production query text, do not connect. */
bool sql_begin_transaction(void)
{
    assert(!transaction_open);
    transaction_open = true;
    return true;
}

bool sql_commit(void)
{
    assert(transaction_open);
    transaction_open = false;
    return true;
}

bool sql_rollback(void)
{
    transaction_open = false;
    return true;
}

bool sql_in_transaction(void)
{
    return transaction_open;
}

bool sql_run_query(const char *query)
{
    assert(query);
    queries.emplace_back(query);
    if (queries.back().rfind("INSERT INTO player_data (", 0) == 0)
        generated_insert_id++;
    return true;
}

static bool sql_try_get_player_pid(const char *, int *pid_out)
{
    assert(pid_out);
    *pid_out = lookup_pid;
    return true;
}

static bool sql_delete_player_subtable(int pid, const char *table_name)
{
    (void)pid;
    (void)table_name;
    return true;
}

static int batch_append(char *, int, size_t, const char *, ...)
{
    std::abort();
}

static void sql_queue_account_character_cache_sync(P_char ch, int room)
{
    (void)ch;
    (void)room;
}

char *sql_escape_string(const char *value)
{
    assert(value);
    const size_t length = std::strlen(value);
    char *copy = static_cast<char *>(std::malloc(length + 1));
    assert(copy);
    std::memcpy(copy, value, length + 1);
    return copy;
}

MYSQL_RES *db_query_at(struct persistence_query_site site, const char *query, ...)
{
    (void)site;
    (void)query;
    return nullptr;
}

void sql_player_error(const char *site)
{
    (void)site;
}

bool player_revision_hydrate(int pid, player_revision_t durable_revision)
{
    (void)pid;
    (void)durable_revision;
    return true;
}

void logit(const char *, const char *, ...)
{
}

/* The body uses these client calls only for the extracted insert/update paths. */
extern "C" my_ulonglong mysql_insert_id(MYSQL *)
{
    return generated_insert_id;
}

extern "C" my_ulonglong mysql_affected_rows(MYSQL *)
{
    return 1;
}

extern "C" MYSQL_ROW mysql_fetch_row(MYSQL_RES *)
{
    return nullptr;
}

extern "C" void mysql_free_result(MYSQL_RES *)
{
}

__STATUS_BODY__

static std::string save_and_check(P_char ch, int expected, int expected_pid,
                                  bool insert_path)
{
    const size_t query_start = queries.size();
    const unsigned int before_played = ch->player.time.played;
    const time_t before_logon = ch->player.time.logon;
    const time_t before_saved = ch->player.time.saved;

    assert(sql_save_player_status(ch, 0, 1));
    assert(ch->player.time.played == before_played);
    assert(ch->player.time.logon == before_logon);
    assert(ch->player.time.saved == before_saved);

    std::string status_query;
    for (size_t i = query_start; i < queries.size(); ++i)
    {
        if (queries[i].find("played_time=") != std::string::npos ||
            (insert_path && queries[i].rfind("INSERT INTO player_data (", 0) == 0))
        {
            status_query = queries[i];
            break;
        }
    }
    assert(!status_query.empty());
    if (insert_path)
        assert(status_query.rfind("INSERT INTO player_data (", 0) == 0);
    else
        assert(status_query.rfind("UPDATE player_data SET ", 0) == 0);

    if (insert_path)
    {
        // INSERT has a column list, so prove the value in the corresponding
        // birth_time, played_time, last_save value sequence.
        const std::string insert_played_marker =
            "FROM_UNIXTIME(NULLIF(0,0)), " + std::to_string(expected) +
            ", FROM_UNIXTIME(NULLIF(" + std::to_string(controlled_now) + ",0))";
        assert(status_query.find(insert_played_marker) != std::string::npos);
    }
    else
    {
        const std::string played_marker = "played_time=" + std::to_string(expected);
        assert(status_query.find(played_marker) != std::string::npos);
    }
    if (!insert_path)
    {
        const std::string save_marker =
            "last_save=FROM_UNIXTIME(NULLIF(" + std::to_string(controlled_now) + ",0))";
        assert(status_query.find(save_marker) != std::string::npos);
    }
    if (!insert_path)
        assert(ch->only.pc->pid == expected_pid);
    return status_query;
}

int main()
{
    char_data modern = {};
    pc_only_data modern_pc = {};
    char modern_name[] = "ModernSqlHero";
    modern.only.pc = &modern_pc;
    modern.player.name = modern_name;
    modern.player.time.played = 3600;
    modern.player.time.logon = 9400;
    modern.player.time.saved = 9000;
    modern_pc.pid = 0;

    // The new INSERT path must persist baseline 3600 plus 600 active seconds.
    lookup_pid = 0;
    save_and_check(&modern, 4200, 101, true);
    assert(modern_pc.pid == 101);

    // Reusing the unchanged live baseline/logon must be idempotent, not compound.
    lookup_pid = 101;
    const std::string first_update = save_and_check(&modern, 4200, 101, false);
    const std::string repeated_update = save_and_check(&modern, 4200, 101, false);
    assert(first_update.find("played_time=4200") != std::string::npos);
    assert(repeated_update.find("played_time=4200") != std::string::npos);

    // A reloaded legacy total (4200) with a new-session logon is equivalent.
    char_data legacy = {};
    pc_only_data legacy_pc = {};
    char legacy_name[] = "LegacySqlHero";
    legacy.only.pc = &legacy_pc;
    legacy.player.name = legacy_name;
    legacy.player.time.played = 4200;
    legacy.player.time.logon = 10000;
    legacy.player.time.saved = 10000;
    legacy_pc.pid = 202;
    lookup_pid = 202;
    const std::string legacy_same = save_and_check(&legacy, 4200, 202, false);

    // Advance one controlled ten-minute interval: both representations reach 4800.
    controlled_now = 10600;
    const std::string modern_later = save_and_check(&modern, 4800, 101, false);
    const std::string modern_later_repeat = save_and_check(&modern, 4800, 101, false);
    const std::string legacy_later = save_and_check(&legacy, 4800, 202, false);
    const std::string legacy_later_repeat = save_and_check(&legacy, 4800, 202, false);
    assert(legacy_same.find("played_time=4200") != std::string::npos);
    assert(modern_later.find("played_time=4800") != std::string::npos);
    assert(modern_later_repeat.find("played_time=4800") != std::string::npos);
    assert(legacy_later.find("played_time=4800") != std::string::npos);
    assert(legacy_later_repeat.find("played_time=4800") != std::string::npos);

    assert(!transaction_open);
    std::cout << "[PASS] production SQL status INSERT/UPDATE: 3600+600=4200; repeated modern/legacy totals stay idempotent; live baseline/logon unchanged\n";
    return 0;
}
'''.replace("__STATUS_BODY__", status_save)

with tempfile.TemporaryDirectory(prefix="duris-playtime-sql-") as temporary:
    directory = Path(temporary)
    source = directory / "playtime_legacy_sql.cpp"
    binary = directory / "playtime_legacy_sql"
    source.write_text(HARNESS, encoding="utf-8")
    cflags = shlex.split(subprocess.check_output(["mysql_config", "--cflags"], text=True))
    libs = shlex.split(subprocess.check_output(["mysql_config", "--libs"], text=True))
    compile_result = subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-O1",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-Isrc",
            *cflags,
            str(source),
            *libs,
            "-Wl,--wrap=time",
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        capture_output=True,
        text=True,
        check=False,
    )
    assert compile_result.returncode == 0, compile_result.stdout + compile_result.stderr
    subprocess.run([str(binary)], cwd=ROOT, check=True)
