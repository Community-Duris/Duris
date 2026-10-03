#!/usr/bin/env python3
"""Prove real SQL pool retirement rolls back and restores usable capacity.

The production pool is linked unchanged. Only its configured-connection factory
is supplied by this explicitly disposable fixture; normal boot is not exercised.
"""
from pathlib import Path
import os
import shlex
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
HARNESS = r'''
#include "sql/sql_pool.h"
#include <atomic>
#include <chrono>
#include <new>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <type_traits>

namespace {
bool fail_factory = false, throw_factory = false;
void require(bool ok, const std::string &message) {
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
const char *env(const char *name) {
    const char *value = std::getenv(name);
    require(value && *value, std::string("missing environment: ") + name);
    return value;
}
MYSQL *connect_fixture() {
    MYSQL *connection = mysql_init(nullptr);
    require(connection != nullptr, "mysql_init");
    using reconnect_flag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;
    reconnect_flag reconnect = false;
    unsigned int timeout = 3;
    require(mysql_options(connection, MYSQL_OPT_RECONNECT, &reconnect) == 0, "disable reconnect");
    require(mysql_options(connection, MYSQL_OPT_CONNECT_TIMEOUT, &timeout) == 0, "connect timeout");
    require(mysql_options(connection, MYSQL_OPT_READ_TIMEOUT, &timeout) == 0, "read timeout");
    require(mysql_real_connect(connection, env("DB_HOST"), env("DB_USER"), env("DB_PASSWD"),
                               env("DB_NAME"), std::stoul(env("DB_PORT")), nullptr, 0) != nullptr, "fixture connect");
    return connection;
}
void query(MYSQL *connection, const std::string &text) {
    const int result = mysql_real_query(connection, text.data(), text.size());
    require(result == 0,
            "query failed: " + text + " error=" + std::to_string(mysql_errno(connection)) +
            " " + mysql_error(connection));
    MYSQL_RES *rows = mysql_store_result(connection);
    if (rows) mysql_free_result(rows);
    else require(mysql_field_count(connection) == 0, "missing query result");
}
std::string scalar(MYSQL *connection, const std::string &text) {
    require(mysql_real_query(connection, text.data(), text.size()) == 0, "scalar SQL failed");
    std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> rows(mysql_store_result(connection), mysql_free_result);
    require(rows && mysql_num_rows(rows.get()) == 1, "scalar cardinality");
    MYSQL_ROW row = mysql_fetch_row(rows.get());
    require(row && row[0], "scalar missing");
    return row[0];
}
void assert_server_retired(MYSQL *observer, unsigned long id) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    do {
        if (scalar(observer, "SELECT COUNT(*) FROM information_schema.processlist WHERE ID=" + std::to_string(id)) == "0" &&
            scalar(observer, "SELECT COUNT(*) FROM information_schema.innodb_trx WHERE trx_mysql_thread_id=" + std::to_string(id)) == "0") return;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    } while (std::chrono::steady_clock::now() < deadline);
    require(false, "retired server session/transaction remains live");
}
}
MYSQL *sql_open_configured_connection(unsigned long) {
    if (throw_factory) throw std::bad_alloc();
    return fail_factory ? nullptr : connect_fixture();
}
void logit(const char *, const char *, ...) {}
int main() {
    require(std::string(env("TEST_DB_DISPOSABLE")) == "1" &&
            std::string(env("DB_HOST")) == "127.0.0.1" &&
            std::string(env("DB_NAME")).starts_with("death_read_pool_test_"), "non-disposable target refused");
    MYSQL *observer = connect_fixture();
    require(scalar(observer, "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE()") == "0", "fixture must start empty");
    query(observer, "CREATE TABLE read_pool_probe(marker INT PRIMARY KEY) ENGINE=InnoDB");
    require(sql_pool_init(1) == 0, "pool init");
    for (bool server_killed : {false, true, false, true}) {
        MYSQL *bad = sql_pool_acquire(); require(bad != nullptr, "borrow");
        const auto id = mysql_thread_id(bad);
        query(bad, "START TRANSACTION");
        query(bad, "INSERT INTO read_pool_probe(marker) VALUES(1)");
        require(bad->server_status & SERVER_STATUS_IN_TRANS, "missing open transaction");
        if (server_killed) {
            query(observer, "KILL CONNECTION " + std::to_string(id));
            require(mysql_query(bad, "SELECT 1") != 0, "killed session appeared usable");
        }
        sql_pool_discard_connection(bad);
        sql_pool_release(bad);
        assert_server_retired(observer, id);
        require(scalar(observer, "SELECT COUNT(*) FROM read_pool_probe") == "0", "retirement committed unacknowledged row");
        int active = 0;
        MYSQL *healthy = sql_pool_acquire_with_status(&active);
        require(healthy && active == 1, "discard permanently exhausted real pool");
        require(mysql_thread_id(healthy) != id, "old SQL session reused");
        require(!(healthy->server_status & SERVER_STATUS_IN_TRANS) &&
                (healthy->server_status & SERVER_STATUS_AUTOCOMMIT), "replacement is not clean");
        query(healthy, "INSERT INTO read_pool_probe(marker) VALUES(2)");
        sql_pool_release(healthy);
        require(scalar(observer, "SELECT marker FROM read_pool_probe") == "2", "subsequent borrow did not commit");
        query(observer, "DELETE FROM read_pool_probe");
    }
    // Replacement owns cleanup even when opening a new session fails or
    // throws: the original transaction rolls back, and capacity stays usable.
    for (int fault : {0, 1, 2}) {
        MYSQL *bad = sql_pool_acquire(); require(bad != nullptr, "replacement borrow");
        const auto id = mysql_thread_id(bad);
        query(bad, "START TRANSACTION");
        query(bad, "INSERT INTO read_pool_probe(marker) VALUES(1)");
        fail_factory = fault == 0; throw_factory = fault == 1;
        if (fault == 2) sql_pool_discard_connection(bad);
        require(!sql_pool_replace_connection(bad), "failed replacement unexpectedly succeeded");
        fail_factory = false; throw_factory = false;
        require(sql_pool_in_use() == 0 && sql_pool_available() == 0, "failed replacement leaked lease");
        assert_server_retired(observer, id);
        require(scalar(observer, "SELECT COUNT(*) FROM read_pool_probe") == "0", "failed replacement committed row");
        MYSQL *fresh = sql_pool_acquire(); require(fresh != nullptr, "replacement capacity recovery");
        require(mysql_thread_id(fresh) != id && !(fresh->server_status & SERVER_STATUS_IN_TRANS), "dirty replacement reused");
        query(fresh, "INSERT INTO read_pool_probe(marker) VALUES(2)");
        sql_pool_release(fresh);
        require(scalar(observer, "SELECT marker FROM read_pool_probe") == "2", "replacement write lost");
        query(observer, "DELETE FROM read_pool_probe");
    }

    // Actual shutdown must complete after a closing replacement consumes its
    // outstanding native session. The original pointer is never released again.
    MYSQL *closing = sql_pool_acquire(); require(closing != nullptr, "shutdown borrow");
    const auto closing_id = mysql_thread_id(closing);
    query(closing, "START TRANSACTION");
    query(closing, "INSERT INTO read_pool_probe(marker) VALUES(1)");
    std::atomic<bool> shutdown_done{false};
    std::thread closer([&] { sql_pool_shutdown(); shutdown_done = true; });
    bool closing_seen = false;
    const auto closing_deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (std::chrono::steady_clock::now() < closing_deadline) {
        int active = 1;
        require(!sql_pool_acquire_with_status(&active), "closing pool lent busy slot");
        if (!active) { closing_seen = true; break; }
    }
    require(closing_seen && !shutdown_done, "shutdown did not fence borrower");
    require(!sql_pool_replace_connection(closing), "closing replacement succeeded");
    require(sql_pool_in_use() == 0, "closing replacement retained borrower");
    closer.join(); require(shutdown_done && sql_pool_total() == 0, "shutdown remains pending");
    assert_server_retired(observer, closing_id);
    require(scalar(observer, "SELECT COUNT(*) FROM read_pool_probe") == "0", "shutdown committed row");
    sql_pool_shutdown();
    mysql_close(observer);
    std::cout << "PASS: real SQL pool retires open/killed sessions, rolls back their rows, and commits from replacement borrowers\n";
}
'''


def compile_sql(binary):
    source = Path(str(binary) + ".cpp")
    source.write_text(HARNESS)
    flags = shlex.split(subprocess.check_output(["mysql_config", "--cflags"], text=True))
    libs = shlex.split(subprocess.check_output(["mysql_config", "--libs"], text=True))
    subprocess.run([os.environ.get("CXX", "g++"), "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                    "-pthread", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
                    "-fno-pie", "-no-pie", "-Isrc", *flags, str(source), "src/sql/sql_pool.c", *libs, "-o", str(binary)], cwd=ROOT, check=True)


class PoolDiscardSQL(unittest.TestCase):
    def test_actual_pool_with_real_sessions(self):
        with tempfile.TemporaryDirectory(prefix="pool-discard-sql-") as directory:
            binary = Path(directory) / "probe"
            compile_sql(binary)
            if os.environ.get("TEST_DB_DISPOSABLE") != "1":
                self.skipTest("compiled; real SQL needs an empty dedicated disposable DB")
            self.assertEqual(os.environ.get("DB_HOST"), "127.0.0.1")
            self.assertRegex(os.environ.get("DB_NAME", ""), r"^death_read_pool_test_[0-9a-f]{12}$")
            subprocess.run([str(binary)], cwd=ROOT, check=True, timeout=60)


if __name__ == "__main__":
    unittest.main()
