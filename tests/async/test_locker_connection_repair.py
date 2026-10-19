#!/usr/bin/env python3
"""Regression contracts for pooled locker connection failure hygiene."""
from _paths import SRC, extract_function
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
source = (SRC / "locker_async.c").read_text(encoding="utf-8", errors="replace")

checks = {
    "failed connection has a repair helper": "static int repair_failed_connection(MYSQL **conn_io)" in source,
    "pending results are drained before rollback": source.find("sql_clear_results_on(conn);") < source.find("mysql_rollback(conn)"),
    "failed transactions are rolled back": "mysql_rollback(conn)" in source,
    "poisoned pooled connections are replaced": "sql_pool_replace_connection(conn)" in source,
    "replacement is returned to the worker release path": "apply_sql_script(&conn, job.sql)" in source,
    "multi-statement failure does not split and replay the batch": "Fallback: split on" not in source,
    "multi-result failure also repairs the connection": source.count("repair_failed_connection(conn_io)") >= 2,
}

for name, ok in checks.items():
    print(("PASS" if ok else "FAIL") + ": " + name)

assert all(checks.values())

harness = r'''
#include <cassert>
#include <cstdio>
struct MYSQL {};
static MYSQL fresh;
static bool reconnect, rollback_fails;
static int phase;
#define LOG_FILE "test"
void logit(const char *, const char *, ...) {}
void sql_clear_results_on(MYSQL *) { assert(phase++ == 0); }
int mysql_rollback(MYSQL *) { assert(phase++ == 1); return rollback_fails; }
unsigned int mysql_errno(MYSQL *) { return 2013; }
const char *mysql_sqlstate(MYSQL *) { return "HY000"; }
MYSQL *sql_pool_replace_connection(MYSQL *old) {
    assert(phase++ == 2);
    delete old; // A relinquished handle must never escape back to the caller.
    return reconnect ? &fresh : nullptr;
}
''' + extract_function("locker_async.c", "static int repair_failed_connection(MYSQL **conn_io)") + r'''
int main() {
    assert(!repair_failed_connection(nullptr));
    MYSQL *empty = nullptr;
    assert(!repair_failed_connection(&empty) && phase == 0);
    for (int scenario = 0; scenario < 4; ++scenario) {
        phase = 0; reconnect = scenario & 1; rollback_fails = scenario & 2;
        MYSQL *connection = new MYSQL;
        assert(!repair_failed_connection(&connection));
        assert(phase == 3 && connection == (reconnect ? &fresh : nullptr));
    }
    puts("locker repair drains and rolls back, then relinquishes failed handles");
}
'''
with tempfile.TemporaryDirectory(prefix="locker-repair-") as temporary:
    test, binary = Path(temporary) / "test.cpp", Path(temporary) / "test"
    test.write_text(harness)
    subprocess.run(["g++", "-std=c++20", "-fsanitize=address,undefined", "-fno-pie", "-no-pie",
                    str(test), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
