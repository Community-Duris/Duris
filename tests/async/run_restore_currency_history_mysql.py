#!/usr/bin/env python3
"""SELECT-only currency restore checks against an owned disposable SQL schema."""
import os
from pathlib import Path
import struct
import sys
import uuid

import pymysql

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
from qualify_database_restore import require_currency_revision_history

if (os.environ.get("TEST_DB_DISPOSABLE") != "1" or
        os.environ.get("DB_HOST") != "127.0.0.1" or os.environ.get("DB_SOCKET")):
    raise SystemExit("explicit disposable loopback database required")
settings = {"host": "127.0.0.1", "port": int(os.environ["DB_PORT"]),
            "user": os.environ["DB_USER"], "password": os.environ["DB_PASSWD"],
            "autocommit": True, "connect_timeout": 5, "read_timeout": 20}
schema = "restore_currency_test_" + uuid.uuid4().hex
connection = pymysql.connect(**settings)
reader = None
lineage = bytes.fromhex("11" * 16)
operation = (4).to_bytes(16, "big")


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
    require_currency_revision_history(Executor())


def refused():
    try:
        admitted()
    except RuntimeError as error:
        assert str(error) == "restore_currency_revision_history_mismatch", error
    else:
        raise AssertionError("corrupt currency revision history admitted")


try:
    execute("CREATE DATABASE " + schema)
    connection.select_db(schema)
    statements = (
        "CREATE TABLE player_data(pid BIGINT UNSIGNED PRIMARY KEY,wallet_revision BIGINT UNSIGNED)",
        "CREATE TABLE account_banks(id BIGINT UNSIGNED PRIMARY KEY,bank_revision BIGINT UNSIGNED)",
        "CREATE TABLE currency_wallet_baseline(pid BIGINT UNSIGNED PRIMARY KEY,opening_revision BIGINT UNSIGNED)",
        "CREATE TABLE currency_bank_baseline(bank_id BIGINT UNSIGNED PRIMARY KEY,opening_revision BIGINT UNSIGNED)",
        "CREATE TABLE currency_ledger(operation_id BINARY(16) PRIMARY KEY,pid BIGINT UNSIGNED,bank_id BIGINT UNSIGNED,"
        "wallet_revision BIGINT UNSIGNED,bank_revision BIGINT UNSIGNED)",
        "CREATE TABLE critical_operation_inbox(operation_id BINARY(16) PRIMARY KEY,status INT,result_code INT,"
        "failure_stage INT,committed_at TIMESTAMP NULL)",
        "CREATE TABLE economic_account_mapping(mapping_id BIGINT UNSIGNED PRIMARY KEY,lineage BINARY(16),"
        "context_id BIGINT UNSIGNED,native_id BIGINT UNSIGNED,active_native_id BIGINT UNSIGNED,"
        "retiring_operation_id BINARY(16),account_kind INT,backend_kind INT)",
        "CREATE TABLE economic_accounting_operation(operation_id BINARY(16) PRIMARY KEY,lineage BINARY(16),"
        "outcome INT,result_code INT,reason INT)",
        "CREATE TABLE economic_accounting_account_effect(operation_id BINARY(16),account_key BINARY(40),"
        "before_revision BIGINT UNSIGNED,after_revision BIGINT UNSIGNED)",
    )
    for statement in statements:
        execute(statement + " ENGINE=InnoDB")
    reader = pymysql.connect(**settings, database=schema)
    with reader.cursor() as cursor:
        cursor.execute("SET SESSION TRANSACTION READ ONLY")
    execute("INSERT INTO player_data VALUES(42,0)")
    execute("INSERT INTO account_banks VALUES(1,0)")
    execute("INSERT INTO currency_wallet_baseline VALUES(42,0)")
    execute("INSERT INTO currency_bank_baseline VALUES(1,0)")
    admitted()
    for table, revision in (("player_data", "wallet_revision"), ("account_banks", "bank_revision")):
        execute(f"UPDATE {table} SET {revision}=9")
        refused()
        execute(f"UPDATE {table} SET {revision}=0")
        admitted()
    for revision in range(1, 5):
        identity = revision.to_bytes(16, "big")
        execute("INSERT INTO currency_ledger VALUES(%s,42,1,%s,%s)", (identity, revision, revision))
        execute("INSERT INTO critical_operation_inbox VALUES(%s,1,0,0,CURRENT_TIMESTAMP)", (identity,))
    execute("UPDATE player_data SET wallet_revision=4")
    execute("UPDATE account_banks SET bank_revision=4")
    admitted()
    execute("DELETE FROM currency_ledger WHERE wallet_revision IN(2,3)")
    refused()
    for revision in (2, 3):
        execute("INSERT INTO currency_ledger VALUES(%s,42,1,%s,%s)",
                (revision.to_bytes(16, "big"), revision, revision))
    admitted()
    # Economic and legacy witnesses for the same native revision count once.
    execute("INSERT INTO economic_account_mapping VALUES(7,%s,0,42,42,NULL,1,1),(9,%s,0,1,1,NULL,2,1)",
            (lineage, lineage))
    execute("INSERT INTO economic_accounting_operation VALUES(%s,%s,1,0,32)", (operation, lineage))
    for kind, lifetime in ((1, 7), (2, 9)):
        key = lineage + struct.pack("<HHQQ4x", 1, kind, lifetime, 0)
        execute("INSERT INTO economic_accounting_account_effect VALUES(%s,%s,3,4)", (operation, key))
    admitted()
    execute("DELETE FROM currency_ledger WHERE wallet_revision=4")
    admitted()
    execute("UPDATE economic_accounting_operation SET outcome=2,result_code=1")
    refused()
    execute("UPDATE economic_accounting_operation SET outcome=1,result_code=0")
    admitted()
    execute("UPDATE critical_operation_inbox SET status=0 WHERE operation_id=%s", (operation,))
    refused()
    execute("UPDATE critical_operation_inbox SET status=1 WHERE operation_id=%s", (operation,))
    admitted()
    execute("UPDATE economic_account_mapping SET context_id=1 WHERE account_kind=2")
    refused()
    execute("UPDATE economic_account_mapping SET context_id=0 WHERE account_kind=2")
    admitted()
    execute("UPDATE player_data SET wallet_revision=3")
    refused()
    execute("UPDATE player_data SET wallet_revision=4")
    admitted()
    execute("UPDATE currency_wallet_baseline SET opening_revision=2")
    execute("UPDATE currency_bank_baseline SET opening_revision=2")
    admitted()
    execute("UPDATE currency_bank_baseline SET opening_revision=5")
    refused()
    # Empty histories at UINT64_MAX and a final native step must not overflow.
    execute("DELETE FROM currency_ledger")
    execute("DELETE FROM economic_accounting_account_effect")
    for table, revision in (("player_data", "wallet_revision"), ("account_banks", "bank_revision"),
                            ("currency_wallet_baseline", "opening_revision"),
                            ("currency_bank_baseline", "opening_revision")):
        execute(f"UPDATE {table} SET {revision}=18446744073709551615")
    admitted()
    execute("UPDATE currency_wallet_baseline SET opening_revision=18446744073709551614")
    execute("UPDATE currency_bank_baseline SET opening_revision=18446744073709551614")
    execute("INSERT INTO currency_ledger VALUES(%s,42,1,18446744073709551615,18446744073709551615)", (operation,))
    admitted()
    execute("DELETE FROM currency_ledger")
    refused()
    print("Currency restore history: exact cuts, native/economic bridge, gaps, recovery and uint64 bounds passed")
finally:
    if reader is not None:
        reader.close()
    execute("DROP DATABASE IF EXISTS " + schema)
    connection.close()