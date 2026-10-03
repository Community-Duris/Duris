#!/usr/bin/env python3
"""Compile and optionally run the production deletion guard against disposable SQL."""
from pathlib import Path
import os
import shlex
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
SQL_PLAYER = (ROOT / "src/sql/sql_player.c").read_text(encoding="utf-8")


def function_body(source: str, signature: str, *, last: bool = False) -> str:
    start = source.rindex(signature) if last else source.index(signature)
    brace = source.index("{", start)
    depth = 0
    for position in range(brace, len(source)):
        if source[position] == "{":
            depth += 1
        elif source[position] == "}":
            depth -= 1
            if depth == 0:
                return source[start : position + 1]
    raise AssertionError(f"unterminated function: {signature}")


# Compile and invoke the exact production definitions while supplying only the
# connection/transaction utilities that normally live elsewhere in sql_player.c.
PRODUCTION = "\n".join(
    (
        function_body(SQL_PLAYER, "bool sql_player_deletion_guard(", last=True),
        function_body(SQL_PLAYER, "bool sql_delete_player(", last=True),
    )
)
PRELUDE = r'''
#include <mysql.h>
#include "persistence/economic_sql_lifecycle_guard.h"
#include "sql/sql_player_deletion.h"
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <type_traits>

MYSQL *DB = nullptr;
bool player_save_journal_pid_quarantined(int) { return false; }
static bool transaction_active = false;
static int character_deletion_guard_pid = 0;
static unsigned revision_forgets = 0;
bool sql_in_transaction(void);
bool sql_begin_transaction(void);
bool sql_commit(void);
bool sql_rollback(void);
bool sql_run_query(const char *query);
MYSQL_RES *db_query(const char *format, ...);
void sql_player_error(const char *site);
void player_revision_forget(int pid);
'''
HARNESS = r'''
namespace {
void require(bool condition, const std::string &message) {
    if (!condition) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
const char *env(const char *name) {
    const char *value = std::getenv(name);
    require(value && *value, std::string("missing environment: ") + name);
    return value;
}
void execute(const std::string &statement) {
    require(mysql_real_query(DB, statement.data(), statement.size()) == 0,
            "SQL failed: " + statement.substr(0, 120) + " error=" +
            std::to_string(mysql_errno(DB)));
    MYSQL_RES *rows = mysql_store_result(DB);
    if (rows) mysql_free_result(rows);
    else require(mysql_field_count(DB) == 0, "unexpected missing SQL result");
}
std::string scalar(const std::string &statement) {
    require(mysql_real_query(DB, statement.data(), statement.size()) == 0,
            "scalar query failed: " + statement);
    std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> rows(mysql_store_result(DB), mysql_free_result);
    require(bool(rows) && mysql_num_rows(rows.get()) == 1, "scalar result cardinality");
    MYSQL_ROW row = mysql_fetch_row(rows.get());
    require(row && row[0], "scalar value missing");
    return row[0];
}
void seed_player() {
    execute("INSERT INTO player_data(pid) VALUES(1)");
    execute("INSERT INTO deletion_guard_cleanup(pid,marker) VALUES(1,1)");
}
}

bool sql_in_transaction(void) { return transaction_active; }
bool sql_begin_transaction(void) {
    if (!DB || transaction_active || mysql_query(DB, "START TRANSACTION")) return false;
    transaction_active = true;
    character_deletion_guard_pid = 0;
    return true;
}
bool sql_commit(void) {
    if (!DB || !transaction_active || mysql_query(DB, "COMMIT")) return false;
    transaction_active = false;
    character_deletion_guard_pid = 0;
    return true;
}
bool sql_rollback(void) {
    if (!DB || !transaction_active || mysql_query(DB, "ROLLBACK")) return false;
    transaction_active = false;
    character_deletion_guard_pid = 0;
    return true;
}
MYSQL_RES *db_query(const char *format, ...) {
    char query[512];
    va_list args;
    va_start(args, format);
    const int length = std::vsnprintf(query, sizeof(query), format, args);
    va_end(args);
    if (!DB || length < 0 || static_cast<size_t>(length) >= sizeof(query) ||
        mysql_real_query(DB, query, static_cast<unsigned long>(length))) return nullptr;
    return mysql_store_result(DB);
}
bool sql_run_query(const char *query) {
    if (!DB || !query || mysql_real_query(DB, query, std::strlen(query))) return false;
    MYSQL_RES *rows = mysql_store_result(DB);
    if (rows) mysql_free_result(rows);
    return mysql_field_count(DB) == 0;
}
void sql_player_error(const char *) {}
void player_revision_forget(int) { ++revision_forgets; }

int main() {
    require(std::string(env("TEST_DB_DISPOSABLE")) == "1" &&
            std::string(env("DB_HOST")) == "127.0.0.1" &&
            std::string(env("DB_NAME")).starts_with("death_delete_gate_test_"),
            "refusing non-dedicated/non-loopback DB target");
    DB = mysql_init(nullptr);
    require(DB != nullptr, "mysql_init");
    using reconnect_flag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;
    reconnect_flag reconnect = false;
    require(mysql_options(DB, MYSQL_OPT_RECONNECT, &reconnect) == 0, "disable reconnect");
    require(mysql_real_connect(DB, env("DB_HOST"), env("DB_USER"), env("DB_PASSWD"),
                               env("DB_NAME"), std::stoul(env("DB_PORT")), nullptr, 0) != nullptr,
            "connect to disposable fixture");
    execute("CREATE TEMPORARY TABLE player_data(pid INT NOT NULL PRIMARY KEY) ENGINE=InnoDB");
    execute("CREATE TEMPORARY TABLE player_death_conflict_evidence(operation_id BINARY(16) NOT NULL, pid INT NOT NULL, PRIMARY KEY(pid,operation_id)) ENGINE=InnoDB");
    execute("CREATE TEMPORARY TABLE deletion_guard_cleanup(pid INT NOT NULL PRIMARY KEY, marker INT NOT NULL) ENGINE=InnoDB");
    execute("CREATE TEMPORARY TABLE economic_sql_lifecycle_installation(phase TINYINT NOT NULL)");
    execute("CREATE TEMPORARY TABLE economic_lineage_state(active_epoch BINARY(16) NULL)");
    seed_player();

    {
    economic_sql_currency_writer_guard writer;
    require(!writer.is_valid_for(DB), "unowned lease must not validate");
    require(economic_sql_currency_writer_guard::acquire(DB, &writer) == 0, "legacy deletion lease");
    require(writer.is_valid_for(DB) && !writer.is_valid_for(nullptr), "lease must bind exact live session");
    using reconnect_flag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;
    reconnect_flag allow_reconnect = true;
    require(mysql_options(DB, MYSQL_OPT_RECONNECT, &allow_reconnect) == 0, "enable reconnect fault");
    require(!writer.is_valid_for(DB), "reconnect-enabled session must invalidate admission");
    allow_reconnect = false;
    require(mysql_options(DB, MYSQL_OPT_RECONNECT, &allow_reconnect) == 0 && writer.is_valid_for(DB), "restore reconnect-disabled lease");
    require(!sql_player_deletion_guard(1, writer), "guard must require an active outer transaction");
    require(sql_begin_transaction(), "begin no-evidence guard transaction");
    require(!sql_player_deletion_guard(2, writer), "missing player row must fail closed");
    require(sql_player_deletion_guard(1, writer), "clean player row should pass guard");
    require(sql_rollback(), "rollback clean guard transaction");
    }

    execute("INSERT INTO player_death_conflict_evidence(operation_id,pid) VALUES(UNHEX('00000000000000000000000000000001'),1)");
    require(!sql_delete_player(1, false), "sql_delete_player must refuse unresolved evidence");
    require(scalar("SELECT COUNT(*) FROM player_data WHERE pid=1") == "1", "refusal deleted player row");
    require(scalar("SELECT COUNT(*) FROM player_death_conflict_evidence WHERE pid=1") == "1", "refusal deleted recovery evidence");

    {
    economic_sql_currency_writer_guard writer;
    require(economic_sql_currency_writer_guard::acquire(DB, &writer) == 0, "outer cleanup deletion lease");
    require(sql_begin_transaction(), "begin cleanup rollback probe");
    execute("UPDATE deletion_guard_cleanup SET marker=2 WHERE pid=1");
    require(!sql_player_deletion_guard(1, writer), "outer guard must refuse unresolved evidence");
    require(!sql_delete_player(1, false, &writer), "outer-transaction deletion must refuse unresolved evidence");
    require(transaction_active, "guard unexpectedly ended caller-owned transaction");
    require(sql_rollback(), "caller rollback after deletion refusal");
    require(scalar("SELECT marker FROM deletion_guard_cleanup WHERE pid=1") == "1", "caller rollback did not preserve cleanup row");
    require(scalar("SELECT COUNT(*) FROM player_data WHERE pid=1") == "1" &&
            scalar("SELECT COUNT(*) FROM player_death_conflict_evidence WHERE pid=1") == "1",
            "caller rollback did not preserve identity/evidence rows");

    }
    execute("DROP TEMPORARY TABLE player_death_conflict_evidence");
    require(!sql_delete_player(1, false), "missing evidence table/read failure must fail closed");
    require(scalar("SELECT COUNT(*) FROM player_data WHERE pid=1") == "1", "read failure deleted player row");
    require(revision_forgets == 0, "refusal evicted player revision");

    execute("CREATE TEMPORARY TABLE player_death_conflict_evidence(operation_id BINARY(16) NOT NULL, pid INT NOT NULL, PRIMARY KEY(pid,operation_id)) ENGINE=InnoDB");
    // Native SQL authority refuses active/staged/read-error cuts before BEGIN.
    const auto unchanged = [&] {
        require(!transaction_active && revision_forgets == 0, "refusal started a transaction or evicted revision");
        require(scalar("SELECT COUNT(*) FROM player_data WHERE pid=1") == "1" &&
                scalar("SELECT marker FROM deletion_guard_cleanup WHERE pid=1") == "1",
                "accounting refusal changed native identity/cleanup");
    };
    execute("INSERT INTO economic_lineage_state(active_epoch) VALUES(UNHEX('00000000000000000000000000000001'))");
    require(!sql_delete_player(1, false), "active accounting admitted physical player deletion");
    unchanged();
    execute("DELETE FROM economic_lineage_state");
    for (int phase : {1, 2}) {
        execute("INSERT INTO economic_sql_lifecycle_installation(phase) VALUES(" + std::to_string(phase) + ")");
        require(!sql_delete_player(1, false), "staged installation admitted physical player deletion");
        unchanged();
        execute("DELETE FROM economic_sql_lifecycle_installation");
    }
    execute("DROP TEMPORARY TABLE economic_sql_lifecycle_installation");
    require(!sql_delete_player(1, false), "missing lifecycle schema admitted deletion");
    unchanged();
    execute("CREATE TEMPORARY TABLE economic_sql_lifecycle_installation(phase TINYINT NOT NULL)");
    {
        economic_sql_currency_writer_guard writer;
        require(economic_sql_currency_writer_guard::acquire(DB, &writer) == 0, "lease-loss deletion admission");
        require(sql_begin_transaction() && sql_player_deletion_guard(1, writer), "guarded outer deletion admission");
        require(!sql_delete_player(1, false), "cached PID admitted deletion without the held lease");
        execute("SELECT RELEASE_LOCK('duris:economic_sql_currency_writers')");
        require(!writer.is_valid_for(DB), "lost named lease still validates");
        require(!sql_player_deletion_guard(1, writer), "cached PID bypassed lease loss");
        require(!sql_delete_player(1, false, &writer), "physical deletion bypassed lease loss");
        require(sql_rollback(), "rollback lost-lease transaction");
    }
    unchanged();
    std::cout << "PASS: active/staged/missing-schema and absent/lost-lease deletion refusals preserve identity\n";
    require(sql_delete_player(1, false), "clean player deletion should commit");
    require(scalar("SELECT COUNT(*) FROM player_data WHERE pid=1") == "0", "successful deletion retained player row");
    require(revision_forgets == 0, "transaction owner unexpectedly evicted player revision");
    mysql_close(DB);
    DB = nullptr;
    std::cout << "PASS: production deletion guard and sql_delete_player on disposable InnoDB fixture\n";
}
'''


class DeleteGateSQL(unittest.TestCase):
    def test_production_guard_compiles_and_optional_real_sql(self):
        with tempfile.TemporaryDirectory(prefix="death-delete-gate-") as directory:
            directory = Path(directory)
            source = directory / "harness.cpp"
            source.write_text(PRELUDE + "\n" + PRODUCTION + "\n" + HARNESS, encoding="utf-8")
            binary = directory / "harness"
            cflags = shlex.split(subprocess.check_output(["mysql_config", "--cflags"], text=True))
            libs = shlex.split(subprocess.check_output(["mysql_config", "--libs"], text=True))
            subprocess.run(
                [os.environ.get("CXX", "g++"), "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                 "-pthread", "-Isrc", *cflags, str(source),
                 "src/persistence/economic_sql_lifecycle_guard.c", *libs, "-o", str(binary)],
                cwd=ROOT, check=True,
            )
            if os.environ.get("TEST_DB_DISPOSABLE") != "1":
                self.skipTest("production SQL guard compiled; runtime NOT run without the dedicated disposable fixture")
            subprocess.run([str(binary)], cwd=ROOT, check=True)


if __name__ == "__main__":
    unittest.main()
