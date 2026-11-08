#!/usr/bin/env python3
"""Drive the real pool through stale probes, failed replacement and confirmed lock loss.

By default the MySQL boundary is deterministic fault injection. Set
DURIS_TEST_MYSQL_SOCKET or DURIS_TEST_MYSQL_HOST to a disposable MariaDB to exercise the same cases
against real idle timeouts and a killed lock-owner session. No application DB is used.
"""
import os
from pathlib import Path
import re
import subprocess
import tempfile
from _paths import ROOT, SRC

initialize = (SRC / "sql.c").read_text().split("int initialize_mysql()", 1)[1]
keepalive = re.search(r'"(SET SESSION wait_timeout=\d+)"', initialize).group(1)
assert initialize.index(keepalive) < initialize.index("duris_sql_exclusion_guard_acquire(DB)")

HARNESS = r'''
#include "sql/sql.h"
#include "sql/sql_pool.h"
#include "sql/sql_exclusion_guard.h"
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <thread>
#include <chrono>

static bool refuse_connections = false;
static int opens = 0, lock_alerts = 0;
static MYSQL *owner = nullptr;
#ifdef LIVE_MYSQL
static MYSQL *admin = nullptr;
static MYSQL *open_connection(bool database = true) {
    MYSQL *conn = mysql_init(nullptr);
    const char *host = getenv("DURIS_TEST_MYSQL_HOST");
    assert(conn && mysql_real_connect(conn, host ? host : "localhost", "root", "",
        database ? "duris_pr717_pool_test" : nullptr, 0,
        getenv("DURIS_TEST_MYSQL_SOCKET"), CLIENT_MULTI_STATEMENTS));
    return conn;
}
static void execute(MYSQL *conn, const std::string &query) {
    if (mysql_real_query(conn, query.data(), query.size())) {
        fprintf(stderr, "%s: %s\n", query.c_str(), mysql_error(conn)); abort();
    }
}
#else
struct connection_state { bool alive; unsigned long id; };
static std::map<MYSQL *, connection_state> connections;
static unsigned long next_id = 1;
static bool lock_owned = true;
static char scalar[4] = "1";
static MYSQL *open_connection() {
    MYSQL *conn = new MYSQL{};
    connections[conn] = {true, next_id++}; return conn;
}
unsigned long mysql_thread_id(MYSQL *conn) { return connections.at(conn).id; }
int mysql_real_query(MYSQL *conn, const char *, unsigned long) {
    if (!connections.at(conn).alive) return 1;
    strcpy(scalar, lock_owned ? "1" : "0"); return 0;
}
MYSQL_RES *mysql_store_result(MYSQL *) { static MYSQL_RES result; return &result; }
unsigned int mysql_num_fields(MYSQL_RES *) { return 1; }
my_ulonglong mysql_num_rows(MYSQL_RES *) { return 1; }
MYSQL_ROW mysql_fetch_row(MYSQL_RES *) { static char *row[] = {scalar}; return row; }
unsigned long *mysql_fetch_lengths(MYSQL_RES *) { static unsigned long length = 1; return &length; }
void mysql_free_result(MYSQL_RES *) {}
int mysql_next_result(MYSQL *) { return -1; }
void mysql_close(MYSQL *conn) { assert(connections.erase(conn) == 1); delete conn; }
#endif

MYSQL *sql_open_configured_connection(unsigned long) {
    ++opens;
    if (refuse_connections) return nullptr;
    MYSQL *conn = open_connection();
#ifdef LIVE_MYSQL
    execute(conn, "SET SESSION wait_timeout=1");
#endif
    if (duris_sql_exclusion_guard_allows(conn)) return conn;
    mysql_close(conn); return nullptr;
}
void logit(const char *, const char *format, ...) {
    if (strstr(format, "runtime database lock is lost")) ++lock_alerts;
}

int main() {
#ifdef LIVE_MYSQL
    assert(!mysql_library_init(0, nullptr, nullptr));
    admin = open_connection(false);
    execute(admin, "CREATE DATABASE duris_pr717_pool_test");
#endif
    owner = open_connection();
#ifdef LIVE_MYSQL
    execute(owner, "SET SESSION wait_timeout=1");
    execute(owner, KEEPALIVE_SQL);
#endif
    assert(duris_sql_exclusion_guard_acquire(owner));
    assert(sql_pool_init(1) == 0 && sql_pool_is_active());
    MYSQL *borrowed = sql_pool_acquire();
    assert(borrowed && sql_pool_in_use() == 1);
    sql_pool_release(borrowed);
#ifdef LIVE_MYSQL
    std::this_thread::sleep_for(std::chrono::seconds(2));
#else
    connections.at(borrowed).alive = false;
#endif
    // The borrower is dead; the runtime owner is still alive and owns the lock.
    assert(duris_sql_exclusion_guard_allows(owner));
    refuse_connections = true;
    int was_active = 0;
    assert(!sql_pool_acquire_with_status(&was_active) && was_active);
    assert(!duris_sql_exclusion_guard_state_ref().lost && sql_pool_is_active());
    assert(sql_pool_total() == 1 && sql_pool_available() == 1 && sql_pool_in_use() == 0);
    refuse_connections = false;
    MYSQL *replacement = sql_pool_acquire();
    assert(replacement && duris_sql_exclusion_guard_allows(replacement));
    assert(sql_pool_in_use() == 1 && sql_pool_available() == 0);

    // Direct callers also relinquish ownership when replacement fails.
    refuse_connections = true;
    assert(!sql_pool_replace_connection(replacement));
    assert(sql_pool_in_use() == 0 && sql_pool_available() == 1);
    refuse_connections = false;
    const int before_retry = opens;
    replacement = sql_pool_acquire();
    assert(replacement && opens == before_retry + 1); sql_pool_release(replacement);

#ifdef LIVE_MYSQL
    execute(admin, "KILL " + std::to_string(mysql_thread_id(owner)));
#else
    lock_owned = false;
#endif
    const int opened_before_loss = opens;
    assert(!sql_pool_acquire());
    assert(duris_sql_exclusion_guard_state_ref().lost && !sql_pool_is_active());
    assert(!sql_pool_acquire() && lock_alerts == 1 && opens == opened_before_loss);
    assert(sql_pool_in_use() == 0);
    sql_pool_shutdown();
    duris_sql_exclusion_guard_release();
    mysql_close(owner);
#ifdef LIVE_MYSQL
    execute(admin, "DROP DATABASE duris_pr717_pool_test");
    mysql_close(admin); mysql_library_end();
#else
    assert(connections.empty());
#endif
    puts("SQL pool stale connection, refused reconnect, ownership and real lock loss passed");
}
'''

with tempfile.TemporaryDirectory(prefix="sql-pool-recovery-") as tmp:
    source, binary = Path(tmp) / "test.cpp", Path(tmp) / "test"
    source.write_text(HARNESS)
    args = ["g++", "-std=c++20", "-g", "-Wall", "-Wextra", "-Werror", "-Isrc",
            "-I/usr/include/mysql", "-I/usr/include/libxml2", str(source),
            str(SRC / "sql_pool.c"), "-pthread", "-o", str(binary)]
    if os.environ.get("DURIS_TEST_MYSQL_SOCKET") or os.environ.get("DURIS_TEST_MYSQL_HOST"):
        args += ["-DLIVE_MYSQL", f'-DKEEPALIVE_SQL="{keepalive}"', "-lmysqlclient"]
    else:
        args += ["-fsanitize=address,undefined", "-fno-pie", "-no-pie"]
    subprocess.run(args, cwd=ROOT, check=True)
    subprocess.run([str(binary)], check=True, timeout=30)
