#!/usr/bin/env python3
"""SELECT-only epic restore checks against an owned disposable SQL schema."""
import os
from pathlib import Path
import sys
import uuid

import pymysql

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
from qualify_database_restore import require_epic_revision_history

if (os.environ.get("TEST_DB_DISPOSABLE") != "1" or
        os.environ.get("DB_HOST") != "127.0.0.1" or os.environ.get("DB_SOCKET")):
    raise SystemExit("explicit disposable loopback database required")
settings = {"host": "127.0.0.1", "port": int(os.environ["DB_PORT"]),
            "user": os.environ["DB_USER"], "password": os.environ["DB_PASSWD"],
            "autocommit": True, "connect_timeout": 5, "read_timeout": 20}
schema = "restore_epic_test_" + uuid.uuid4().hex
connection = pymysql.connect(**settings)
reader = None

class Executor:
    def sql(self, query):
        assert query.startswith("SELECT ")
        with reader.cursor() as cursor:
            cursor.execute(query)
            return str(cursor.fetchone()[0])


def execute(query, args=None):
    with connection.cursor() as cursor:
        cursor.execute(query, args)


def admitted():
    require_epic_revision_history(Executor())


def refused():
    try:
        admitted()
    except RuntimeError as error:
        assert str(error) == "restore_epic_revision_history_mismatch", error
    else:
        raise AssertionError("corrupt epic revision history admitted")


try:
    execute("CREATE DATABASE " + schema)
    connection.select_db(schema)
    execute("CREATE TABLE player_data(pid BIGINT UNSIGNED PRIMARY KEY,epic_revision BIGINT UNSIGNED) ENGINE=InnoDB")
    execute("CREATE TABLE epic_balance_baseline(pid BIGINT UNSIGNED PRIMARY KEY,opening_revision BIGINT UNSIGNED) ENGINE=InnoDB")
    execute("CREATE TABLE epic_ledger(operation_id BINARY(16) PRIMARY KEY,pid BIGINT UNSIGNED,epic_revision BIGINT UNSIGNED,UNIQUE(pid,epic_revision)) ENGINE=InnoDB")
    # A separate read-only transaction executes only the production SELECT.
    reader = pymysql.connect(**settings, database=schema)
    with reader.cursor() as cursor:
        cursor.execute("SET SESSION TRANSACTION READ ONLY")
    execute("INSERT INTO player_data VALUES(42,0)")
    execute("INSERT INTO epic_balance_baseline VALUES(42,0)")
    admitted()
    execute("UPDATE player_data SET epic_revision=9")
    refused()
    execute("UPDATE player_data SET epic_revision=0")
    admitted()
    for revision in range(1, 5):
        execute("INSERT INTO epic_ledger VALUES(%s,42,%s)", (revision.to_bytes(16, "big"), revision))
    execute("UPDATE player_data SET epic_revision=4")
    admitted()
    # Removing a cancelling pair can conserve value and retain the last event.
    execute("DELETE FROM epic_ledger WHERE epic_revision IN (2,3)")
    refused()
    for revision in (2, 3):
        execute("INSERT INTO epic_ledger VALUES(%s,42,%s)", (revision.to_bytes(16, "big"), revision))
    admitted()
    execute("UPDATE player_data SET epic_revision=3")
    refused()
    execute("UPDATE player_data SET epic_revision=4")
    admitted()
    execute("UPDATE epic_balance_baseline SET opening_revision=2")
    admitted()
    execute("UPDATE epic_balance_baseline SET opening_revision=5")
    refused()
    # Full unsigned range must not overflow signed/unsigned subtraction.
    execute("DELETE FROM epic_ledger")
    execute("UPDATE player_data SET epic_revision=18446744073709551615")
    execute("UPDATE epic_balance_baseline SET opening_revision=18446744073709551615")
    admitted()
    execute("UPDATE player_data SET epic_revision=18446744073709551614")
    refused()
    print("Epic restore revision history: exact cut, gaps, recovery and uint64 bounds passed")
finally:
    if reader is not None:
        reader.close()
    execute("DROP DATABASE IF EXISTS " + schema)
    connection.close()
