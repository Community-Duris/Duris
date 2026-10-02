#!/usr/bin/env python3
"""Real two-connection delete/retention lock-order regression on an empty disposable DB.

Deletion uses the actual production functions. The competing publisher executes
only retention's player-row-lock/evidence-insert boundary, not the full save path.
"""
from pathlib import Path
import importlib.util
import os
import shlex
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("deletion_guard_fixture", Path(__file__).with_name("test_player_death_recovery_delete_gate_mysql.py"))
assert spec is not None and spec.loader is not None
fixture = importlib.util.module_from_spec(spec)
spec.loader.exec_module(fixture)

HELPERS = r'''
namespace {
void query_on(MYSQL *connection, const std::string &statement) {
    require(mysql_real_query(connection, statement.data(), statement.size()) == 0,
            "contender SQL failed: " + statement + " error=" + std::to_string(mysql_errno(connection)));
    MYSQL_RES *rows = mysql_store_result(connection);
    if (rows) mysql_free_result(rows);
    else require(mysql_field_count(connection) == 0, "missing contender result");
}
std::string scalar_on(MYSQL *connection, const std::string &statement) {
    require(mysql_real_query(connection, statement.data(), statement.size()) == 0, "observer SQL failed");
    std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> rows(mysql_store_result(connection), mysql_free_result);
    require(rows && mysql_num_rows(rows.get()) == 1, "observer scalar cardinality");
    MYSQL_ROW row = mysql_fetch_row(rows.get());
    require(row && row[0], "observer scalar missing");
    return row[0];
}
MYSQL *connect_peer() {
    MYSQL *connection = mysql_init(nullptr);
    require(connection != nullptr, "peer mysql_init");
    using reconnect_flag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;
    reconnect_flag reconnect = false;
    require(mysql_options(connection, MYSQL_OPT_RECONNECT, &reconnect) == 0, "peer disable reconnect");
    require(mysql_real_connect(connection, env("DB_HOST"), env("DB_USER"), env("DB_PASSWD"),
                               env("DB_NAME"), std::stoul(env("DB_PORT")), nullptr, 0) != nullptr, "peer connect");
    query_on(connection, "SET SESSION TRANSACTION ISOLATION LEVEL REPEATABLE READ");
    query_on(connection, "SET SESSION innodb_lock_wait_timeout=10");
    return connection;
}
void await_server_lock_request(MYSQL *observer, unsigned long id) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(8);
    do {
        // MariaDB can show a const PK lookup in Statistics before exposing
        // an innodb_trx row. The other connection already owns this exact
        // player lock: observe the real server query before releasing it.
        if (scalar_on(observer, "SELECT COUNT(*) FROM information_schema.processlist WHERE ID=" +
                      std::to_string(id) + " AND COMMAND='Query' AND INFO='SELECT pid FROM player_data WHERE pid=1 FOR UPDATE'") == "1") return;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    } while (std::chrono::steady_clock::now() < deadline);
    std::cerr << "LOCK_WAIT_DIAGNOSTIC id=" << id
              << " process=" << scalar_on(observer, "SELECT CONCAT(COUNT(*),':',COALESCE(MAX(CONCAT(COMMAND,'|',STATE,'|',COALESCE(INFO,''))),'')) FROM information_schema.processlist WHERE ID=" + std::to_string(id))
              << " transaction=" << scalar_on(observer, "SELECT CONCAT(COUNT(*),':',COALESCE(MAX(CONCAT(trx_state,'|',COALESCE(trx_query,''))),'')) FROM information_schema.innodb_trx WHERE trx_mysql_thread_id=" + std::to_string(id)) << '\n';
    require(false, "contender never reached observed server player-lock request");
}
}
'''
RACES = r'''
    execute("DELETE FROM deletion_guard_cleanup");
    seed_player();
    execute("SET SESSION TRANSACTION ISOLATION LEVEL REPEATABLE READ");
    execute("SET SESSION innodb_lock_wait_timeout=10");
    MYSQL *publisher = connect_peer();
    MYSQL *observer = connect_peer();
    const unsigned long deletion_id = mysql_thread_id(DB);
    const unsigned long publisher_id = mysql_thread_id(publisher);

    // Retention owns the row first. A pre-lock repeatable-read view would miss
    // its later commit; observe the server's locking query before releasing it.
    query_on(publisher, "START TRANSACTION");
    query_on(publisher, "SELECT pid FROM player_data WHERE pid=1 FOR UPDATE");
    query_on(publisher, "INSERT INTO player_death_conflict_evidence(operation_id,pid) VALUES(UNHEX('0123456789abcdef0123456789abcdef'),1)");
    bool deletion_succeeded = true;
    std::thread deleting([&] {
        require(mysql_thread_init() == 0, "deleter thread init");
        deletion_succeeded = sql_delete_player(1, false);
        mysql_thread_end();
    });
    await_server_lock_request(observer, deletion_id);
    query_on(publisher, "COMMIT");
    deleting.join();
    require(!deletion_succeeded, "delete missed evidence committed while waiting for player lock");
    require(scalar("SELECT COUNT(*) FROM player_data WHERE pid=1") == "1", "race refusal deleted identity");
    require(scalar("SELECT HEX(operation_id) FROM player_death_conflict_evidence WHERE pid=1") ==
            "0123456789ABCDEF0123456789ABCDEF", "race refusal changed exact evidence bytes");
    require(scalar("SELECT marker FROM deletion_guard_cleanup WHERE pid=1") == "1", "race refusal changed cleanup row");
    std::cout << "PASS: retention-first commit remains visible after observed deletion lock request\n";

    // Deletion owns the row first. A publisher must wait for that same lock,
    // then refuse to publish evidence against the deleted identity.
    execute("DELETE FROM player_death_conflict_evidence WHERE pid=1");
    require(sql_begin_transaction() && sql_player_deletion_guard(1), "begin deletion-first guard");
    require(sql_delete_player(1, false), "guarded outer transaction deletion");
    bool publisher_saw_player = true;
    std::thread publishing([&] {
        require(mysql_thread_init() == 0, "publisher thread init");
        query_on(publisher, "START TRANSACTION");
        require(mysql_query(publisher, "SELECT pid FROM player_data WHERE pid=1 FOR UPDATE") == 0, "publisher row lock");
        MYSQL_RES *rows = mysql_store_result(publisher);
        require(rows != nullptr, "publisher row result");
        publisher_saw_player = mysql_num_rows(rows) != 0;
        mysql_free_result(rows);
        if (publisher_saw_player)
            query_on(publisher, "INSERT INTO player_death_conflict_evidence(operation_id,pid) VALUES(UNHEX('fedcba9876543210fedcba9876543210'),1)");
        query_on(publisher, "COMMIT");
        mysql_thread_end();
    });
    await_server_lock_request(observer, publisher_id);
    require(sql_commit(), "commit deletion-first transaction");
    publishing.join();
    require(!publisher_saw_player, "publisher admitted evidence for deleted identity");
    require(scalar("SELECT COUNT(*) FROM player_data") == "0" &&
            scalar("SELECT COUNT(*) FROM player_death_conflict_evidence") == "0", "deletion-first durable state mismatch");
    mysql_close(publisher);
    mysql_close(observer);
    std::cout << "PASS: deletion-first commit prevents a later publisher from finding the identity\n";
'''


def compile_sql(binary, *, stale_view_control=False):
    production = fixture.PRODUCTION
    if stale_view_control:
        needle = "\tchar query[256];"
        assert production.count(needle) == 1
        production = production.replace(needle, '\tMYSQL_RES *premature = db_query("SELECT pid FROM player_data WHERE pid=%d", pid);\n\tif (premature) mysql_free_result(premature);\n' + needle)
    harness = fixture.HARNESS.replace("CREATE TEMPORARY TABLE", "CREATE TABLE").replace("DROP TEMPORARY TABLE", "DROP TABLE")
    harness = harness.replace("int main() {", HELPERS + "\nint main() {")
    marker = '    execute("CREATE TABLE player_data'
    assert harness.count(marker) == 1
    harness = harness.replace(marker, '    require(scalar("SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE()") == "0", "serialization fixture must start empty");\n' + marker)
    marker = "    mysql_close(DB);"
    assert harness.count(marker) == 1
    harness = harness.replace(marker, RACES + marker)
    source = Path(str(binary) + ".cpp")
    source.write_text(fixture.PRELUDE + "\n#include <chrono>\n#include <thread>\n" + production + "\n" + harness)
    flags = shlex.split(subprocess.check_output(["mysql_config", "--cflags"], text=True))
    libs = shlex.split(subprocess.check_output(["mysql_config", "--libs"], text=True))
    subprocess.run([os.environ.get("CXX", "g++"), "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                    "-pthread", "-Isrc", *flags, str(source), *libs, "-o", str(binary)], cwd=ROOT, check=True)


class DeleteSerializationSQL(unittest.TestCase):
    def test_actual_delete_lock_order(self):
        with tempfile.TemporaryDirectory(prefix="delete-serialization-") as directory:
            binary = Path(directory) / "probe"
            compile_sql(binary)
            if os.environ.get("TEST_DB_DISPOSABLE") != "1":
                self.skipTest("compiled; real race requires an empty dedicated disposable DB")
            self.assertEqual(os.environ.get("DB_HOST"), "127.0.0.1")
            self.assertRegex(os.environ.get("DB_NAME", ""), r"^death_delete_gate_test_[0-9a-f]{12}$")
            subprocess.run([str(binary)], cwd=ROOT, check=True, timeout=60)


if __name__ == "__main__":
    unittest.main()
