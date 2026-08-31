#!/usr/bin/env python3
"""Regression contracts for pooled locker connection failure hygiene."""
from _paths import SRC
from pathlib import Path

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

raise SystemExit(0 if all(checks.values()) else 1)
