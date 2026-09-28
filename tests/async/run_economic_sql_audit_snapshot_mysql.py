#!/usr/bin/env python3
"""Exercise partial SQL audit export on a disposable, SELECT-only database."""

import hashlib
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile
from unittest import mock
import uuid

import pymysql

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
import economic_sql_audit_snapshot as exporter  # noqa: E402
from economic_sql_audit_snapshot import capture  # noqa: E402
from reconcile_economy_accounting import Reconciler  # noqa: E402
from test_economic_sql_audit_origins import EPOCH, LINEAGE, OPENING, OP, key, witness  # noqa: E402

if (os.environ.get("ECONOMIC_ACCOUNTING_DISPOSABLE_SCHEMA") != "1" or
        os.environ.get("DB_HOST") != "127.0.0.1" or os.environ.get("DB_SOCKET")):
    raise SystemExit("explicit disposable loopback database required")

settings = {"host": "127.0.0.1", "port": int(os.environ.get("DB_PORT", "3306")),
            "user": os.environ["DB_USER"], "password": os.environ["DB_PASSWORD"],
            "autocommit": True, "cursorclass": pymysql.cursors.DictCursor,
            "connect_timeout": 5, "read_timeout": 20, "write_timeout": 5}
schema = "economic_schema_test_snapshot_" + uuid.uuid4().hex
reader = "audit_" + uuid.uuid4().hex[:12]
assert re.fullmatch(r"economic_schema_test_snapshot_[0-9a-f]{32}", schema)
assert re.fullmatch(r"audit_[0-9a-f]{12}", reader)

TABLES = (
    "CREATE TABLE economic_baseline_control (lineage BINARY(16),epoch BINARY(16),"
    "opening_account VARBINARY(40),revision BIGINT UNSIGNED,last_operation_id BINARY(16)) ENGINE=InnoDB",
    "CREATE TABLE economic_baseline_witness (operation_id BINARY(16),lineage BINARY(16),"
    "epoch BINARY(16),book_revision BIGINT UNSIGNED,holding_count INT,item_count INT,"
    "witness_digest BINARY(32),canonical_witness MEDIUMBLOB) ENGINE=InnoDB",
    "CREATE TABLE economic_accounting_operation (operation_id BINARY(16),lineage BINARY(16),"
    "epoch BINARY(16),original_operation_id BINARY(16),reason INT,outcome INT,result_code INT,"
    "source_event BINARY(48),account_count INT,posting_count INT,child_count INT,"
    "item_event_count INT) ENGINE=InnoDB",
    "CREATE TABLE critical_operation_inbox (operation_id BINARY(16),status INT,result_code INT) ENGINE=InnoDB",
    "CREATE TABLE economic_accounting_account_effect (operation_id BINARY(16),account_index INT,"
    "account_key BINARY(40),before_copper BIGINT,before_silver BIGINT,before_gold BIGINT,"
    "before_platinum BIGINT,after_copper BIGINT,after_silver BIGINT,after_gold BIGINT,"
    "after_platinum BIGINT,before_revision BIGINT,after_revision BIGINT) ENGINE=InnoDB",
    "CREATE TABLE economic_accounting_coin_posting (operation_id BINARY(16),line_index INT,"
    "account_index INT,child_index INT,delta_copper BIGINT,delta_silver BIGINT,delta_gold BIGINT,"
    "delta_platinum BIGINT,copper_value BIGINT) ENGINE=InnoDB",
    "CREATE TABLE economic_accounting_child (operation_id BINARY(16),child_index INT,"
    "child_operation_id BINARY(16),parent_index INT) ENGINE=InnoDB",
    "CREATE TABLE economic_accounting_item_reference (operation_id BINARY(16),event_index INT,"
    "child_index INT,item_uid BIGINT,before_revision BIGINT,after_revision BIGINT,"
    "legacy_operation_id BINARY(16),legacy_event_index INT) ENGINE=InnoDB",
    "CREATE TABLE economic_accounting_source_claim (lineage BINARY(16),source_event BINARY(48),"
    "operation_id BINARY(16)) ENGINE=InnoDB",
    "CREATE TABLE economic_account_mapping (mapping_id BIGINT,account_kind INT,context_id BIGINT,"
    "active_native_id BIGINT,lineage BINARY(16),backend_kind INT) ENGINE=InnoDB",
    "CREATE TABLE player_data (pid BIGINT,copper BIGINT,silver BIGINT,gold BIGINT,platinum BIGINT,"
    "wallet_revision BIGINT) ENGINE=InnoDB",
    "CREATE TABLE account_banks (id BIGINT,bank_copper BIGINT,bank_silver BIGINT,bank_gold BIGINT,"
    "bank_platinum BIGINT,bank_revision BIGINT) ENGINE=InnoDB",
    "CREATE TABLE item_current_owner (item_uid BIGINT,root_item_uid BIGINT,parent_item_uid BIGINT,"
    "owner_type INT,owner_id BIGINT,owner_context_id BIGINT,item_revision BIGINT,state INT) ENGINE=InnoDB",
    "CREATE TABLE item_ownership_ledger (operation_id BINARY(16),event_index INT,item_uid BIGINT,"
    "root_item_uid BIGINT,parent_item_uid BIGINT,to_owner_type INT,to_owner_id BIGINT,"
    "to_owner_context_id BIGINT,item_revision BIGINT,reason_type INT) ENGINE=InnoDB",
)

root = bytes.fromhex("aa" * 16)
source = bytes.fromhex("bb" * 48)
prior_root = bytes.fromhex("dd" * 16)
prior_epoch = bytes.fromhex("ee" * 16)
prior_source = bytes.fromhex("ff" * 48)
admin = pymysql.connect(**settings)
try:
    with admin.cursor() as cursor:
        cursor.execute(f"CREATE DATABASE `{schema}`")
        cursor.execute(f"CREATE USER '{reader}'@'%' IDENTIFIED BY 'disposable-audit-only'")
        cursor.execute(f"GRANT SELECT ON `{schema}`.* TO '{reader}'@'%'")
    setup = pymysql.connect(**(settings | {"database": schema}))
    try:
        with setup.cursor() as cursor:
            for statement in TABLES:
                cursor.execute(statement)
            row = witness()
            blob = bytearray(row["canonical_witness"])
            bank = key(2, 9) + struct.pack("<4qQ", 3, 0, 0, 0, 1) + bytes.fromhex("99" * 32)
            blob[304:304] = bank
            struct.pack_into("<I", blob, 8, len(blob))
            struct.pack_into("<I", blob, 184, 2)
            blob = bytes(blob)
            cursor.execute("INSERT INTO economic_baseline_control VALUES (%s,%s,%s,1,%s)",
                           (LINEAGE, EPOCH, OPENING, OP))
            cursor.execute("INSERT INTO economic_baseline_witness VALUES (%s,%s,%s,1,2,1,%s,%s)",
                           (OP, LINEAGE, EPOCH, hashlib.sha256(blob).digest(), blob))
            cursor.execute("INSERT INTO economic_accounting_operation VALUES "
                           "(%s,%s,%s,NULL,38,1,0,%s,2,0,0,0)",
                           (OP, LINEAGE, EPOCH, bytes.fromhex("cc" * 48)))
            cursor.execute("INSERT INTO economic_accounting_operation VALUES "
                           "(%s,%s,%s,NULL,3,1,0,%s,2,2,0,0)",
                           (root, LINEAGE, EPOCH, source))
            cursor.execute("INSERT INTO critical_operation_inbox VALUES (%s,1,0),(%s,1,0)",
                           (OP, root))
            for index, account, before, after, before_revision, after_revision in (
                    (0, key(1, 7), 5, 2, 4, 5), (1, key(2, 9), 3, 6, 1, 2)):
                cursor.execute("INSERT INTO economic_accounting_account_effect VALUES "
                               "(%s,%s,%s,%s,0,0,0,%s,0,0,0,%s,%s)",
                               (root, index, account, before, after, before_revision, after_revision))
            for index, delta in ((0, -3), (1, 3)):
                cursor.execute("INSERT INTO economic_accounting_coin_posting VALUES "
                               "(%s,%s,%s,0,%s,0,0,0,%s)",
                               (root, index, index, delta, delta))
            cursor.execute("INSERT INTO economic_accounting_source_claim VALUES (%s,%s,%s)",
                           (LINEAGE, source, root))
            cursor.execute("INSERT INTO economic_accounting_operation VALUES "
                           "(%s,%s,%s,NULL,3,1,0,%s,0,0,0,0)",
                           (prior_root, LINEAGE, prior_epoch, prior_source))
            cursor.execute("INSERT INTO economic_accounting_source_claim VALUES (%s,%s,%s)",
                           (LINEAGE, prior_source, prior_root))
            cursor.execute("INSERT INTO economic_account_mapping VALUES "
                           "(7,1,0,7,%s,1),(9,2,0,9,%s,1)", (LINEAGE, LINEAGE))
            cursor.execute("INSERT INTO player_data VALUES (7,2,0,0,0,5)")
            cursor.execute("INSERT INTO account_banks VALUES (9,6,0,0,0,2)")
            cursor.execute("INSERT INTO item_current_owner VALUES (81,81,NULL,1,7,0,2,1)")
        audit = pymysql.connect(**(settings | {"database": schema, "user": reader,
                                             "password": "disposable-audit-only"}))
        try:
            snapshot = capture(audit, LINEAGE, EPOCH)
            assert snapshot["complete"] is False and snapshot["quiescent"] is True
            assert len(snapshot["operations"]) == 1
            assert len(snapshot["effects"]) == len(snapshot["postings"]) == 2
            assert len(snapshot["native"]["holdings"]) == 2
            assert snapshot["native_mapping_coverage"]["wallet_rows"] == 1
            assert snapshot["native_mapping_coverage"]["unmapped_wallet_rows"] == 0
            assert snapshot["source_claim_coverage"] == {
                "source_operations": 2, "missing_claim_operations": 0,
                "duplicate_source_values": 0}
            assert len(snapshot["source_claims"]) == 2
            report = Reconciler().audit(snapshot)
            assert report["exception_counts"] == {"evidence_loss": 1}, report
            original = exporter.read_origins_in_transaction

            def concurrent_change(cursor, lineage, epoch):
                origins = original(cursor, lineage, epoch)
                with setup.cursor() as writer:
                    writer.execute("UPDATE player_data SET copper=3 WHERE pid=7")
                return origins

            with mock.patch.object(exporter, "read_origins_in_transaction",
                                   side_effect=concurrent_change):
                fenced = capture(audit, LINEAGE, EPOCH)
            wallet = next(row for row in fenced["native"]["holdings"]
                          if row["account_key"] == key(1, 7).hex())
            assert wallet["balance"] == [2, 0, 0, 0]
            with setup.cursor() as cursor:
                cursor.execute("UPDATE player_data SET copper=2 WHERE pid=7")
            with tempfile.TemporaryDirectory(prefix="duris-sql-audit-") as directory:
                output = Path(directory) / "partial.json"
                command = [sys.executable, str(ROOT / "scripts/economic_sql_audit_snapshot.py"),
                           "--host", "127.0.0.1", "--port", str(settings["port"]),
                           "--user", reader, "--database", schema,
                           "--lineage", LINEAGE.hex(), "--epoch", EPOCH.hex(),
                           "--output", str(output)]
                environment = dict(os.environ, DB_PASSWORD="disposable-audit-only")
                subprocess.run(command, env=environment, check=True, timeout=30)
                assert json.loads(output.read_text(encoding="utf-8")) == snapshot
                result = subprocess.run(
                    [sys.executable, str(ROOT / "scripts/reconcile_economy_accounting.py"),
                     str(output)], capture_output=True, text=True, timeout=30)
                assert result.returncode == 1
                assert json.loads(result.stdout)["exception_counts"] == {"evidence_loss": 1}
            with setup.cursor() as cursor:
                cursor.execute("DELETE FROM economic_accounting_coin_posting "
                               "WHERE operation_id=%s AND line_index=1", (root,))
            missing = Reconciler().audit(capture(audit, LINEAGE, EPOCH))["exception_counts"]
            assert "unbalanced_root" in missing and "evidence_count_mismatch" in missing
            with setup.cursor() as cursor:
                cursor.execute("INSERT INTO economic_accounting_coin_posting VALUES "
                               "(%s,1,1,0,3,0,0,0,3)", (root,))
                cursor.execute("UPDATE player_data SET copper=3 WHERE pid=7")
            stale = Reconciler().audit(capture(audit, LINEAGE, EPOCH))["exception_counts"]
            assert "stale_native_balance" in stale
            with setup.cursor() as cursor:
                cursor.execute("UPDATE player_data SET copper=2 WHERE pid=7")
                cursor.execute("INSERT INTO player_data VALUES (8,0,0,0,0,1),"
                               "(9,0,0,0,0,1)")
                cursor.execute("INSERT INTO economic_account_mapping VALUES "
                               "(70,1,1,7,%s,1),(10,2,0,10,%s,1),"
                               "(71,1,0,9,%s,1)",
                               (LINEAGE, LINEAGE, bytes.fromhex("dd" * 16)))
            unmapped = Reconciler().audit(capture(audit, LINEAGE, EPOCH))["exception_counts"]
            assert unmapped["unmapped_native_wallet"] == 1
            assert unmapped["multiply_mapped_native_wallet"] == 1
            assert unmapped["dangling_bank_mapping"] == 1
            with setup.cursor() as cursor:
                cursor.execute("INSERT INTO economic_accounting_operation VALUES "
                               "(%s,%s,%s,NULL,3,1,0,%s,0,0,0,0)",
                               (bytes.fromhex("ab" * 16), LINEAGE, prior_epoch, prior_source))
                cursor.execute("INSERT INTO economic_accounting_source_claim VALUES (%s,%s,%s)",
                               (LINEAGE, bytes.fromhex("ac" * 48), bytes.fromhex("ad" * 16)))
            cross_epoch = Reconciler().audit(capture(audit, LINEAGE, EPOCH))["exception_counts"]
            assert cross_epoch["lineage_missing_source_claim"] == 1
            assert cross_epoch["lineage_duplicate_source_event"] == 1
            assert cross_epoch["orphan_source_claim"] == 1
        finally:
            audit.close()
    finally:
        setup.close()
    print("SQL partial audit snapshot: one consistent cut and fail-closed reconciliation passed")
finally:
    with admin.cursor() as cursor:
        cursor.execute(f"DROP DATABASE IF EXISTS `{schema}`")
        cursor.execute(f"DROP USER IF EXISTS '{reader}'@'%'")
    admin.close()
