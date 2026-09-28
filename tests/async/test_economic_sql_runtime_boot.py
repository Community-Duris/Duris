#!/usr/bin/env python3
"""Exercise real SQL boot/shutdown ordering around lifecycle runtime authority.

Connection factories, schema probes and the lifecycle owner are controlled here;
real-SQL guard/connection tests separately establish their storage semantics.
"""
from pathlib import Path
import os
import subprocess
import tempfile
from _paths import ROOT, source

PREFIX = r'''
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <string>
#include <vector>
struct MYSQL { bool closed = false; } connection, fallback;
MYSQL *DB = nullptr, *persistenceDB = nullptr;
constexpr int LOG_STATUS = 0, CLIENT_MULTI_STATEMENTS = 1;
constexpr int ITEM_UID_BOOT_RESERVATION = 32, SQL_POOL_DEFAULT_SIZE = 2;
enum Failure { none, open_failed, exclusion_failed, runtime_failed,
               schema_failed, schema_closed_connection, season_failed,
               lookup_failed, allocator_failed, pool_failed } failure;
bool runtime_held = false;
int starts = 0, stops = 0, writes = 0, closes = 0;
std::vector<std::string> events;
void reset(Failure value) {
    assert(!runtime_held);
    connection = {}; fallback = {}; DB = nullptr; persistenceDB = nullptr;
    starts = stops = writes = closes = 0; events.clear(); failure = value;
}
void logit(int, const char *, ...) {}
MYSQL *sql_open_configured_connection(unsigned long) {
    events.push_back("open"); return failure == open_failed ? nullptr : &connection;
}
void mysql_close(MYSQL *conn) {
    assert(conn && !conn->closed); conn->closed = true; ++closes;
    if (conn == &connection) assert(!runtime_held);
    events.push_back("close");
}
bool duris_sql_exclusion_guard_acquire(MYSQL *conn) {
    assert(conn == DB && !conn->closed); events.push_back("exclusion_start");
    return failure != exclusion_failed;
}
void duris_sql_exclusion_guard_release() {
    assert(!runtime_held); events.push_back("exclusion_stop");
}
bool sql_economic_runtime_start() noexcept {
    assert(DB && !DB->closed && !runtime_held); ++starts;
    events.push_back("runtime_start");
    runtime_held = failure != runtime_failed;
    return runtime_held;
}
void sql_economic_runtime_shutdown() noexcept {
    ++stops; events.push_back("runtime_stop"); runtime_held = false;
}
void sql_resetConnectTimes() {
    assert(runtime_held && "boot write before economic runtime authority");
    ++writes; events.push_back("reset_connections");
}
bool sql_verify_boot_database() {
    assert(runtime_held); events.push_back("schema");
    if (failure == schema_closed_connection) {
        // A verifier/transport failure can leave the main handle unavailable.
        connection.closed = true; ++closes; DB = nullptr; return false;
    }
    return failure != schema_failed;
}
bool sql_load_active_season_state() {
    assert(runtime_held); return failure != season_failed;
}
bool sql_populate_lookup_tables() {
    assert(runtime_held); ++writes; return failure != lookup_failed;
}
bool item_uid_allocator_reserve(MYSQL *, int) {
    assert(runtime_held); ++writes; return failure != allocator_failed;
}
int sql_pool_init(int) { assert(runtime_held); return failure == pool_failed ? -1 : 0; }
void sql_pool_shutdown() { events.push_back("pool_stop"); }
'''

SUFFIX = r'''
int main() {
    reset(none);
    assert(initialize_mysql() == 1 && starts == 1 && runtime_held && DB);
    persistenceDB = &fallback;
    shutdown_mysql();
    assert(!runtime_held && stops == 1 && closes == 2 && !DB && !persistenceDB);
    assert(events[events.size()-1] == "close");

    reset(open_failed);
    assert(initialize_mysql() == -1 && starts == 0 && !runtime_held && !DB);
    reset(exclusion_failed);
    assert(initialize_mysql() == -1 && starts == 0 && !runtime_held && !DB && closes == 1);
    reset(runtime_failed);
    assert(initialize_mysql() == -1 && starts == 1 && !runtime_held && !DB);
    assert(writes == 0 && closes == 1);

    for (Failure fault : {schema_failed, schema_closed_connection, season_failed,
                          lookup_failed, allocator_failed}) {
        reset(fault);
        assert(initialize_mysql() == -1 && starts == 1 && stops == 1);
        assert(!runtime_held && !DB && closes == 1);
    }
    reset(pool_failed);
    assert(initialize_mysql() == 1 && runtime_held);
    shutdown_mysql();
    assert(!runtime_held && !DB && starts == 1 && stops == 1 && closes == 1);
    std::puts("PASS: SQL boot owns runtime authority before writes and releases on every exit");
}
'''


def extract_last(text, signature):
    start = text.rindex(signature)
    depth = 0
    for end in range(text.index('{', start), len(text)):
        depth += (text[end] == '{') - (text[end] == '}')
        if depth == 0:
            return text[start:end + 1]
    raise AssertionError(signature)


def main():
    text = source('sql/sql.c').read_text()
    code = PREFIX + extract_last(text, 'int initialize_mysql()')
    code += extract_last(text, 'void shutdown_mysql(void)') + SUFFIX
    with tempfile.TemporaryDirectory(prefix='duris-sql-runtime-boot-') as directory:
        cpp = Path(directory) / 'boot.cpp'; exe = Path(directory) / 'boot'
        cpp.write_text(code)
        subprocess.run([os.environ.get('CXX', 'g++'), '-std=c++20', '-Wall', '-Wextra',
                        '-Werror', '-g', '-O0', '-fsanitize=address,undefined',
                        '-fno-pie', '-no-pie', str(cpp), '-o', str(exe)], check=True,
                       cwd=ROOT, timeout=60)
        subprocess.run([str(exe)], check=True, timeout=30,
                       env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=1:halt_on_error=1'})


if __name__ == '__main__':
    main()
