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

INSTALL = bytes.fromhex("77" * 16)

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
    "item_event_count INT,realized_price_copper BIGINT NULL) ENGINE=InnoDB",
    "CREATE TABLE critical_operation_inbox (operation_id BINARY(16),status INT,result_code INT,"
    "failure_stage INT NOT NULL DEFAULT 0,committed_at TIMESTAMP NULL DEFAULT CURRENT_TIMESTAMP) ENGINE=InnoDB",
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
    "CREATE TABLE economic_account_mapping (mapping_id BIGINT,account_kind INT,locator_kind INT NOT NULL,context_id BIGINT,"
    "active_native_id BIGINT,lineage BINARY(16),backend_kind INT,"
    "retiring_operation_id BINARY(16) NULL,native_id BIGINT,"
    "creating_operation_id BINARY(16)) ENGINE=InnoDB",
    "CREATE TABLE economic_sql_lifecycle_installation (operation_id BINARY(16),"
    "lineage BINARY(16),epoch BINARY(16),baseline_operation_id BINARY(16),phase INT,"
    "selected_epoch BINARY(16),revision BIGINT) ENGINE=InnoDB",
    "CREATE TABLE economic_pending_claim_source (source_operation_id BINARY(16),source_slot INT,"
    "lineage BINARY(16),claim_mapping_id BIGINT,beneficiary_pid BIGINT,amount BIGINT,"
    "claim_operation_id BINARY(16)) ENGINE=InnoDB",
    "CREATE TABLE player_data (pid BIGINT,copper BIGINT,silver BIGINT,gold BIGINT,platinum BIGINT,"
    "wallet_revision BIGINT) ENGINE=InnoDB",
    "CREATE TABLE account_banks (id BIGINT,bank_copper BIGINT,bank_silver BIGINT,bank_gold BIGINT,"
    "bank_platinum BIGINT,bank_revision BIGINT) ENGINE=InnoDB",
    "CREATE TABLE auctions (id BIGINT,status VARCHAR(16),cur_price BIGINT,"
    "auction_revision BIGINT,winning_bidder_pid BIGINT) ENGINE=InnoDB",
    "CREATE TABLE auction_money_pickups (pid BIGINT,money BIGINT,"
    "claim_revision BIGINT) ENGINE=InnoDB",
    "CREATE TABLE shopkeepers (id BIGINT,cash BIGINT,shop_revision BIGINT) ENGINE=InnoDB",
    "CREATE TABLE item_current_owner (item_uid BIGINT,root_item_uid BIGINT,parent_item_uid BIGINT,"
    "owner_type INT,owner_id BIGINT,owner_context_id BIGINT,item_revision BIGINT,state INT,"
    "vnum INT,coin_payload MEDIUMBLOB) ENGINE=InnoDB",
    "CREATE TABLE item_ownership_ledger (operation_id BINARY(16),event_index INT,item_uid BIGINT,"
    "root_item_uid BIGINT,parent_item_uid BIGINT,to_owner_type INT,to_owner_id BIGINT,"
    "to_owner_context_id BIGINT,item_revision BIGINT,from_owner_revision BIGINT,"
    "reason_type INT) ENGINE=InnoDB",
)


def coin_payload(uid, amounts):
    blob = bytearray(struct.pack("<IihQqibB", 1, -1, -1, uid, 0, 3, 20, 0))
    blob.extend(struct.pack("<I", 0) * 4)
    blob.extend(struct.pack("<8i", *amounts, 0, 0, 0, 0))
    blob.extend(struct.pack("<6q", *([0] * 6)))
    blob.extend(struct.pack("<5I", *([0] * 5)))
    blob.extend(struct.pack("<ibihh", 0, 0, 0, 100, 0))
    blob.extend(struct.pack("<5Q", *([0] * 5)))
    blob.extend(struct.pack("<8h", *([0] * 8)))
    blob.extend(struct.pack("<2I", 0, 0))
    return bytes(blob)


def item_origin(uid, owner_type, state, owner_id, revision):
    return struct.pack("<QBB6x5Q32s", uid, owner_type, state, owner_id, 0,
                       uid, 0, revision, bytes.fromhex("a5" * 32))

root = bytes.fromhex("aa" * 16)
source = bytes.fromhex("bb" * 48)
prior_root = bytes.fromhex("dd" * 16)
prior_epoch = bytes.fromhex("ee" * 16)
prior_source = bytes.fromhex("ff" * 48)
consumed_source_root = bytes.fromhex("e1" * 16)
consumer_root = bytes.fromhex("e2" * 16)
consumed_source_event = bytes.fromhex("ab" * 48)
consumer_source_event = bytes.fromhex("ac" * 48)
creation_root = bytes.fromhex("e3" * 16)
creation_source_event = bytes.fromhex("ad" * 48)
new_wallet_root = bytes.fromhex("e4" * 16)
other_mapping_lineage = bytes.fromhex("44" * 16)
prior_item_root = bytes.fromhex("e5" * 16)
prior_item_source = bytes.fromhex("b0" * 48)
prior_unlinked_operation = bytes.fromhex("b1" * 16)
admin = pymysql.connect(**settings)
try:
    with admin.cursor() as cursor:
        cursor.execute(f"CREATE DATABASE `{schema}`")
        cursor.execute(f"CREATE USER '{reader}'@'127.0.0.1' IDENTIFIED BY 'disposable-audit-only'")
        cursor.execute(f"GRANT SELECT ON `{schema}`.* TO '{reader}'@'127.0.0.1'")
    setup = pymysql.connect(**(settings | {"database": schema}))
    try:
        with setup.cursor() as cursor:
            for statement in TABLES:
                cursor.execute(statement)
            row = witness()
            blob = bytearray(row["canonical_witness"])
            bank = key(2, 9) + struct.pack("<4qQ", 3, 0, 0, 0, 1) + bytes.fromhex("99" * 32)
            blob[304:304] = bank
            other_holdings = b"".join(
                key(kind, identity) + struct.pack("<4qQ", *amounts, 1) +
                bytes.fromhex("99" * 32)
                for kind, identity, amounts in (
                    (3, 11, (1, 2, 3, 4)), (4, 12, (123, 0, 0, 0)),
                    (5, 13, (250, 0, 0, 0)), (6, 14, (500, 0, 0, 0))))
            blob[416:416] = other_holdings
            blob.extend(item_origin(82, 1, 1, 7, 1))
            blob.extend(item_origin(83, 8, 2, 0, 2))
            struct.pack_into("<I", blob, 8, len(blob))
            struct.pack_into("<II", blob, 184, 6, 3)
            blob = bytes(blob)
            cursor.execute("INSERT INTO economic_baseline_control VALUES (%s,%s,%s,1,%s)",
                           (LINEAGE, EPOCH, OPENING, OP))
            cursor.execute("INSERT INTO economic_baseline_witness VALUES (%s,%s,%s,1,6,3,%s,%s)",
                           (OP, LINEAGE, EPOCH, hashlib.sha256(blob).digest(), blob))
            cursor.execute("INSERT INTO economic_accounting_operation VALUES "
                           "(%s,%s,%s,NULL,38,1,0,%s,2,0,0,0,NULL)",
                           (OP, LINEAGE, EPOCH, bytes.fromhex("cc" * 48)))
            cursor.execute("INSERT INTO economic_sql_lifecycle_installation VALUES "
                           "(%s,%s,%s,%s,2,%s,1)",
                           (INSTALL, LINEAGE, EPOCH, OP, EPOCH))
            cursor.execute("INSERT INTO economic_accounting_operation VALUES "
                           "(%s,%s,%s,NULL,3,1,0,%s,2,2,0,0,NULL)",
                           (root, LINEAGE, EPOCH, source))
            cursor.execute("INSERT INTO economic_accounting_operation VALUES "
                           "(%s,%s,%s,NULL,1,1,0,NULL,2,2,0,0,NULL)",
                           (new_wallet_root, LINEAGE, EPOCH))
            cursor.execute("INSERT INTO critical_operation_inbox "
                           "(operation_id,status,result_code) VALUES "
                           "(%s,1,0),(%s,1,0),(%s,1,0)",
                           (OP, root, INSTALL))
            cursor.execute("INSERT INTO critical_operation_inbox "
                           "(operation_id,status,result_code) VALUES (%s,1,0)",
                           (creation_root,))
            cursor.execute("INSERT INTO critical_operation_inbox "
                           "(operation_id,status,result_code) VALUES (%s,1,0)",
                           (new_wallet_root,))
            cursor.execute("INSERT INTO critical_operation_inbox "
                           "(operation_id,status,result_code) VALUES "
                           "(%s,1,0),(%s,1,0),(%s,1,0)",
                           (prior_root, consumed_source_root, consumer_root))
            for index, account, before, after, before_revision, after_revision in (
                    (0, key(1, 7), 5, 2, 4, 5), (1, key(2, 9), 3, 6, 1, 2)):
                cursor.execute("INSERT INTO economic_accounting_account_effect VALUES "
                               "(%s,%s,%s,%s,0,0,0,%s,0,0,0,%s,%s)",
                               (root, index, account, before, after, before_revision, after_revision))
            for index, delta in ((0, -3), (1, 3)):
                cursor.execute("INSERT INTO economic_accounting_coin_posting VALUES "
                               "(%s,%s,%s,0,%s,0,0,0,%s)",
                               (root, index, index, delta, delta))
            for index, account, before, after, before_revision, after_revision in (
                    (0, key(2, 9), 6, 5, 2, 3),
                    (1, key(1, 15), 0, 1, 0, 1)):
                cursor.execute("INSERT INTO economic_accounting_account_effect VALUES "
                               "(%s,%s,%s,%s,0,0,0,%s,0,0,0,%s,%s)",
                               (new_wallet_root, index, account, before, after,
                                before_revision, after_revision))
            for index, delta in ((0, -1), (1, 1)):
                cursor.execute("INSERT INTO economic_accounting_coin_posting VALUES "
                               "(%s,%s,%s,0,%s,0,0,0,%s)",
                               (new_wallet_root, index, index, delta, delta))
            cursor.execute("INSERT INTO economic_accounting_source_claim VALUES (%s,%s,%s)",
                           (LINEAGE, source, root))
            cursor.execute("INSERT INTO economic_accounting_operation VALUES "
                           "(%s,%s,%s,NULL,3,1,0,%s,2,2,0,0,NULL)",
                           (prior_root, LINEAGE, prior_epoch, prior_source))
            cursor.execute("INSERT INTO economic_accounting_operation VALUES "
                           "(%s,%s,%s,NULL,3,1,0,%s,2,2,0,0,NULL),"
                           "(%s,%s,%s,NULL,31,1,0,%s,2,2,0,0,NULL)",
                           (consumed_source_root, LINEAGE, prior_epoch, consumed_source_event,
                            consumer_root, LINEAGE, prior_epoch, consumer_source_event))
            cursor.execute("INSERT INTO economic_accounting_operation VALUES "
                           "(%s,%s,%s,NULL,33,1,0,%s,0,0,0,1,NULL)",
                           (creation_root, LINEAGE, EPOCH, creation_source_event))
            for index, account, before, after in (
                    (0, key(5, 13), 0, 250), (1, key(8, 9), 0, -250)):
                cursor.execute("INSERT INTO economic_accounting_account_effect VALUES "
                               "(%s,%s,%s,%s,0,0,0,%s,0,0,0,0,1)",
                               (prior_root, index, account, before, after))
            for index, delta in ((0, 250), (1, -250)):
                cursor.execute("INSERT INTO economic_accounting_coin_posting VALUES "
                               "(%s,%s,%s,0,%s,0,0,0,%s)",
                               (prior_root, index, index, delta, delta))
            for operation_id, claim_before, claim_after, before_revision, after_revision, sink_after in (
                    (consumed_source_root, 250, 350, 1, 2, -100),
                    (consumer_root, 350, 250, 2, 3, 100)):
                for index, account, before, after, before_rev, after_rev in (
                        (0, key(5, 13), claim_before, claim_after,
                         before_revision, after_revision),
                        (1, key(8, 9), 0, sink_after, 0, 1)):
                    cursor.execute("INSERT INTO economic_accounting_account_effect VALUES "
                                   "(%s,%s,%s,%s,0,0,0,%s,0,0,0,%s,%s)",
                                   (operation_id, index, account, before, after,
                                    before_rev, after_rev))
                for index, delta in ((0, claim_after - claim_before), (1, sink_after)):
                    cursor.execute("INSERT INTO economic_accounting_coin_posting VALUES "
                                   "(%s,%s,%s,0,%s,0,0,0,%s)",
                                   (operation_id, index, index, delta, delta))
            cursor.execute("INSERT INTO economic_accounting_source_claim VALUES (%s,%s,%s)",
                           (LINEAGE, prior_source, prior_root))
            cursor.execute("INSERT INTO economic_accounting_source_claim VALUES "
                           "(%s,%s,%s),(%s,%s,%s)",
                           (LINEAGE, consumed_source_event, consumed_source_root,
                            LINEAGE, consumer_source_event, consumer_root))
            cursor.execute("INSERT INTO economic_accounting_source_claim VALUES (%s,%s,%s)",
                           (LINEAGE, creation_source_event, creation_root))
            cursor.execute("INSERT INTO economic_accounting_item_reference VALUES "
                           "(%s,0,0,84,0,1,%s,0)", (creation_root, creation_root))
            cursor.execute("INSERT INTO item_ownership_ledger VALUES "
                           "(%s,0,84,84,NULL,1,7,0,1,0,2)", (creation_root,))
            cursor.execute("INSERT INTO economic_account_mapping VALUES "
                           "(7,1,1,0,7,%s,1,NULL,7,%s),(9,2,2,0,9,%s,1,NULL,9,%s),"
                           "(11,3,3,0,82,%s,1,NULL,82,%s),(12,4,4,0,5,%s,1,NULL,5,%s),"
                           "(13,5,5,0,7,%s,1,NULL,7,%s),(14,6,6,0,3,%s,1,NULL,3,%s),"
                           "(15,1,1,0,15,%s,1,NULL,15,%s),"
                           "(16,1,1,0,16,%s,1,NULL,16,NULL)",
                           (LINEAGE, INSTALL, LINEAGE, INSTALL, LINEAGE, INSTALL,
                            LINEAGE, INSTALL, LINEAGE, INSTALL, LINEAGE, INSTALL,
                            LINEAGE, new_wallet_root,
                            other_mapping_lineage))
            cursor.execute("INSERT INTO economic_pending_claim_source VALUES (%s,1,%s,13,7,250,NULL)",
                           (prior_root, LINEAGE))
            cursor.execute("INSERT INTO economic_pending_claim_source VALUES "
                           "(%s,1,%s,13,7,100,%s)",
                           (consumed_source_root, LINEAGE, consumer_root))
            cursor.execute("INSERT INTO player_data VALUES "
                           "(7,2,0,0,0,5),(15,1,0,0,0,1),(16,1,0,0,0,1)")
            cursor.execute("INSERT INTO account_banks VALUES (9,5,0,0,0,3)")
            cursor.execute("INSERT INTO auctions VALUES (5,'REMOVED',123,1,7)")
            cursor.execute("INSERT INTO auction_money_pickups VALUES (7,250,1)")
            cursor.execute("INSERT INTO shopkeepers VALUES (3,500,1)")
            cursor.execute("INSERT INTO item_current_owner VALUES "
                           "(81,81,NULL,1,7,0,2,1,1,NULL),"
                           "(82,82,NULL,1,7,0,1,1,3,%s),"
                           "(83,83,NULL,8,0,0,2,2,3,NULL),"
                           "(84,84,NULL,1,7,0,1,1,1,NULL)",
                           (coin_payload(82, [1, 2, 3, 4]),))
        audit = pymysql.connect(**(settings | {"database": schema, "user": reader,
                                             "password": "disposable-audit-only"}))
        try:
            snapshot = capture(audit, LINEAGE, EPOCH)
            assert snapshot["complete"] is False and snapshot["quiescent"] is True
            assert len(snapshot["operations"]) == 3
            assert len(snapshot["receipts"]) == len(snapshot["operations"])
            assert all(row["status"] == 1 and row["result_code"] == 0 and
                       row["failure_stage"] == 0 and row["committed_at_present"] is True
                       for row in snapshot["receipts"])
            assert len(snapshot["effects"]) == len(snapshot["postings"]) == 4
            assert len(snapshot["native"]["holdings"]) == 7
            native_holdings = {struct.unpack_from("<HQ", bytes.fromhex(item["account_key"]), 18): item
                               for item in snapshot["native"]["holdings"]}
            assert native_holdings[(3, 11)]["balance"] == [1, 2, 3, 4]
            assert native_holdings[(4, 12)]["balance"] == [123, 0, 0, 0]
            assert native_holdings[(5, 13)]["balance"] == [250, 0, 0, 0]
            assert native_holdings[(6, 14)]["balance"] == [500, 0, 0, 0]
            assert native_holdings[(1, 15)]["balance"] == [1, 0, 0, 0]
            assert next(row for row in snapshot["account_origins"]
                        if row["account_key"] == key(1, 15).hex()) == {
                            "account_key": key(1, 15).hex(), "origin": "creation",
                            "balance": [0, 0, 0, 0], "revision": 0}
            assert snapshot["native_mapping_coverage"]["pile_rows"] == 1
            creation_coverage = snapshot["native"]["mapping_creation_coverage"]
            assert creation_coverage["rows"] == len(snapshot["native"]["mapping_creations"])
            assert creation_coverage["creator_rows"] + sum(
                row["creating_operation_id"] is None
                for row in snapshot["native"]["mapping_creations"]) == creation_coverage["rows"]
            assert creation_coverage["root_rows"] + creation_coverage["missing_roots"] == len({
                row["creating_operation_id"] for row in snapshot["native"]["mapping_creations"]
                if row["creating_operation_id"] is not None})
            assert snapshot["native"]["baseline_operation_ids"] == [OP.hex()]
            installation_root = next(
                row for row in snapshot["native"]["mapping_creation_roots"]
                if row["creator_kind"] == "baseline_installation")
            assert installation_root["baseline_operation_id"] == OP.hex()
            assert installation_root["installation_selected_epoch"] == EPOCH.hex()
            assert installation_root["installation_revision"] == 1
            assert installation_root["installation_failure_stage"] == 0
            assert installation_root["installation_committed_at_present"] is True
            creator_root = next(
                row for row in snapshot["native"]["mapping_creation_roots"]
                if row.get("creator_kind") != "baseline_installation")
            assert creator_root["creator_inbox_status"] == 1
            assert creator_root["creator_inbox_result_code"] == creator_root["result_code"]
            assert creator_root["creator_inbox_failure_stage"] == 0
            assert creator_root["creator_inbox_committed_at_present"] is True
            assert snapshot["native_mapping_coverage"]["auction_escrow_rows"] == 1
            assert snapshot["native_mapping_coverage"]["pending_claim_rows"] == 1
            assert snapshot["native_mapping_coverage"]["treasury_rows"] == 1
            with setup.cursor() as writer:
                writer.execute("INSERT INTO auctions VALUES (6,'REMOVED',0,1,0)")
                writer.execute("INSERT INTO economic_account_mapping VALUES "
                               "(17,4,4,0,6,%s,1,NULL,6,NULL)", (LINEAGE,))
            empty_removed = capture(audit, LINEAGE, EPOCH)
            empty_removed_report = Reconciler().audit(empty_removed)
            assert "dangling_auction_escrow_mapping" in \
                empty_removed_report["exception_counts"]
            with setup.cursor() as writer:
                writer.execute("DELETE FROM economic_account_mapping WHERE mapping_id=17")
                writer.execute("DELETE FROM auctions WHERE id=6")
                writer.execute("INSERT INTO auctions VALUES (7,'OPEN',25,1,0)")
                writer.execute("INSERT INTO economic_account_mapping VALUES "
                               "(18,4,4,0,7,%s,1,NULL,7,NULL)", (LINEAGE,))
            unfunded = capture(audit, LINEAGE, EPOCH)
            unfunded_report = Reconciler().audit(unfunded)
            assert "invalid_native_auction_escrow" in \
                unfunded_report["exception_counts"]
            with setup.cursor() as writer:
                writer.execute("DELETE FROM economic_account_mapping WHERE mapping_id=18")
                writer.execute("DELETE FROM auctions WHERE id=7")
            assert snapshot["native"]["pending_claim_sources"] == [
                {"source_operation_id": prior_root.hex(), "source_slot": 1,
                 "account_key": key(5, 13).hex(), "beneficiary_pid": 7,
                 "amount": 250, "mapping_native_id": 7,
                 "mapping_active_native_id": 7,
                 "mapping_valid": True, "source_root_valid": True,
                 "source_inbox_receipt": {"status": 1, "result_code": 0,
                                           "failure_stage": 0,
                                           "committed_at_present": True},
                 "consumer_inbox_receipt": None,
                 "claim_operation_id": None, "consumer_root_valid": None},
                {"source_operation_id": consumed_source_root.hex(), "source_slot": 1,
                 "account_key": key(5, 13).hex(), "beneficiary_pid": 7,
                 "amount": 100, "mapping_native_id": 7,
                 "mapping_active_native_id": 7,
                 "mapping_valid": True, "source_root_valid": True,
                 "source_inbox_receipt": {"status": 1, "result_code": 0,
                                           "failure_stage": 0,
                                           "committed_at_present": True},
                 "claim_operation_id": consumer_root.hex(), "consumer_root_valid": True,
                 "consumer_inbox_receipt": {"status": 1, "result_code": 0,
                                             "failure_stage": 0,
                                             "committed_at_present": True}},
            ]
            assert snapshot["native"]["pending_claim_source_coverage"] == {
                "rows": 2, "open_rows": 1, "consumed_rows": 1,
                "invalid_account_mappings": 0, "invalid_source_roots": 0,
                "invalid_consumer_roots": 0}
            assert snapshot["native"]["pending_claim_consumers"] == [{
                "operation_id": consumer_root.hex(), "epoch": prior_epoch.hex(),
                "outcome": "committed", "result_code": 0, "source_rows": 1,
                "inbox_receipt": {"status": 1, "result_code": 0,
                                  "failure_stage": 0,
                                  "committed_at_present": True},
                "source_amount": 100,
                "pending_claim_debits": [{"account_key": key(5, 13).hex(), "amount": 100,
                                           "before": [350, 0, 0, 0],
                                           "after": [250, 0, 0, 0]}]}]
            assert snapshot["native"]["pending_claim_consumer_coverage"] == {
                "rows": 1, "missing_source_rows": 0, "mismatched_source_amounts": 0}
            assert snapshot["native"]["coin_pile_coverage"] == {
                "rows": 2, "payload_rows": 1, "missing_payload_rows": 1,
                "mapped_live_rows": 1, "unmapped_live_rows": 0,
                "multiply_mapped_live_rows": 0, "dangling_mappings": 0,
                "invalid_mappings": 0}
            assert snapshot["native"]["coin_pile_mappings"] == [{
                "account_key": key(3, 11).hex(), "uid": 82, "item_exists": True,
                "holding_valid": True, "balance": [1, 2, 3, 4], "revision": 1}]
            pile_creator_mapping = next(
                row for row in snapshot["native"]["mapping_creations"]
                if row["account_key"] == key(3, 11).hex())
            assert pile_creator_mapping["native_id"] == 82
            assert snapshot["native"]["coin_piles"] == [
                {"uid": 82, "owner": [1, 7, 0], "revision": 1, "state": "live",
                 "amounts": [1, 2, 3, 4]},
                {"uid": 83, "owner": [8, 0, 0], "revision": 2, "state": "tombstone",
                 "amounts": None}]
            assert snapshot["native_mapping_coverage"]["wallet_rows"] == 3
            assert snapshot["native_mapping_coverage"]["unmapped_wallet_rows"] == 1
            assert snapshot["source_claim_coverage"] == {
                "source_operations": 5, "missing_claim_operations": 0,
                "duplicate_source_values": 0}
            assert snapshot["source_event_policy_coverage"] == {
                "required_committed_operations": 2,
                "missing_required_source_events": 0}
            assert len(snapshot["source_claims"]) == 5
            assert all(
                row["operation_inbox_receipt"] == {
                    "status": 1, "result_code": row["operation_result_code"],
                    "failure_stage": 0, "committed_at_present": True}
                for row in snapshot["source_claims"]
                if row["operation_outcome"] == "committed")
            assert next(row for row in snapshot["item_origins"] if row["uid"] == 84) == {
                "uid": 84, "origin": "creation", "revision": 0, "root": 84,
                "parent": None, "owner": [0, 0, 0], "state": "absent"}
            assert snapshot["native"]["uid_event_coverage"] == {
                "tracked_uids": 4, "ledger_events": 1,
                "referenced_events": 1, "unreferenced_events": 0}
            assert snapshot["native"]["uid_scope_coverage"] == {
                "ownership_uid_count": 1, "anchored_ownership_uid_count": 1,
                "unanchored_ownership_uid_count": 0,
                "other_lineage_ownership_uid_count": 0,
                "unattributed_ownership_uid_count": 0,
                "ambiguous_lineage_ownership_uid_count": 0}
            assert snapshot["native"]["ambiguous_lineage_ownership_uids"] == []
            assert snapshot["native"]["retirement_coverage"] == {
                "rows": 0, "current_epoch_rows": 0,
                "matched_opening_origins": 0, "unmatched_current_epoch_rows": 0,
                "root_rows": 0}
            report = Reconciler().audit(snapshot)
            # This cut deliberately includes an unrelated-lineage wallet and
            # a deposit root that cannot authorize creating a wallet mapping.
            expected_exceptions = {"evidence_loss": 1,
                                   "unmapped_native_wallet": 1,
                                   "unauthorized_mapping_creation": 1}
            assert report["exception_counts"] == expected_exceptions, report
            # A consistent current/history pair can still refer to a missing
            # native parent. Lineage history must not bypass topology checks.
            with setup.cursor() as writer:
                writer.execute("UPDATE item_current_owner SET root_item_uid=999,"
                               "parent_item_uid=999 WHERE item_uid=84")
                writer.execute("UPDATE item_ownership_ledger SET root_item_uid=999,"
                               "parent_item_uid=999 WHERE item_uid=84")
            orphan_snapshot = capture(audit, LINEAGE, EPOCH)
            orphan_report = Reconciler().audit(orphan_snapshot)
            assert orphan_report["exception_counts"].get("orphan_item_parent") == 1, orphan_report
            assert {"code": "orphan_item_parent", "uid": 84, "parent_uid": 999} in \
                orphan_report["exceptions"]
            assert "stale_native_item" not in orphan_report["exception_counts"], orphan_report
            with setup.cursor() as writer:
                writer.execute("UPDATE item_current_owner SET root_item_uid=84,"
                               "parent_item_uid=NULL WHERE item_uid=84")
                writer.execute("UPDATE item_ownership_ledger SET root_item_uid=84,"
                               "parent_item_uid=NULL WHERE item_uid=84")
            assert Reconciler().audit(capture(audit, LINEAGE, EPOCH))["exception_counts"] == \
                expected_exceptions
            # Re-creating a UID is invalid even if all revisions, references,
            # roots and the final native row agree across the lineage cut.
            duplicate_uid_root = bytes.fromhex("e6" * 16)
            duplicate_uid_source = bytes.fromhex("b2" * 48)
            with setup.cursor() as writer:
                writer.execute("INSERT INTO economic_accounting_operation VALUES "
                               "(%s,%s,%s,NULL,33,1,0,%s,0,0,0,1,NULL)",
                               (duplicate_uid_root, LINEAGE, EPOCH, duplicate_uid_source))
                writer.execute("INSERT INTO critical_operation_inbox "
                               "(operation_id,status,result_code) VALUES (%s,1,0)",
                               (duplicate_uid_root,))
                writer.execute("INSERT INTO economic_accounting_source_claim VALUES (%s,%s,%s)",
                               (LINEAGE, duplicate_uid_source, duplicate_uid_root))
                writer.execute("INSERT INTO economic_accounting_item_reference VALUES "
                               "(%s,0,0,84,1,2,%s,0)", (duplicate_uid_root, duplicate_uid_root))
                writer.execute("INSERT INTO item_ownership_ledger VALUES "
                               "(%s,0,84,84,NULL,1,7,0,2,1,2)", (duplicate_uid_root,))
                writer.execute("UPDATE item_current_owner SET item_revision=2 WHERE item_uid=84")
            duplicate_uid_report = Reconciler().audit(capture(audit, LINEAGE, EPOCH))
            assert duplicate_uid_report["exception_counts"] == {
                **expected_exceptions, "duplicate_uid": 1}, duplicate_uid_report
            # A destruction reason with live custody is corrupt even when the
            # reference, revisions and final native state agree. The independent
            # reader must diagnose it and recover after a valid retirement.
            with setup.cursor() as writer:
                writer.execute("UPDATE item_ownership_ledger SET reason_type=3 "
                               "WHERE operation_id=%s", (duplicate_uid_root,))
            supply_state_report = Reconciler().audit(capture(audit, LINEAGE, EPOCH))
            assert supply_state_report["exception_counts"] == {
                **expected_exceptions, "invalid_item_supply_state": 1}, supply_state_report
            with setup.cursor() as writer:
                writer.execute("UPDATE economic_accounting_operation SET reason=34 "
                               "WHERE operation_id=%s", (duplicate_uid_root,))
                writer.execute("UPDATE item_ownership_ledger SET reason_type=3,"
                               "to_owner_type=8,to_owner_id=0 WHERE operation_id=%s",
                               (duplicate_uid_root,))
                writer.execute("UPDATE item_current_owner SET state=2,owner_type=8,"
                               "owner_id=0 WHERE item_uid=84")
            assert Reconciler().audit(capture(audit, LINEAGE, EPOCH))["exception_counts"] == \
                expected_exceptions
            revived_uid_root = bytes.fromhex("e7" * 16)
            revived_uid_source = bytes.fromhex("b3" * 48)
            with setup.cursor() as writer:
                writer.execute("INSERT INTO economic_accounting_operation VALUES "
                               "(%s,%s,%s,NULL,32,1,0,%s,0,0,0,1,NULL)",
                               (revived_uid_root, LINEAGE, EPOCH, revived_uid_source))
                writer.execute("INSERT INTO critical_operation_inbox "
                               "(operation_id,status,result_code) VALUES (%s,1,0)", (revived_uid_root,))
                writer.execute("INSERT INTO economic_accounting_source_claim VALUES (%s,%s,%s)",
                               (LINEAGE, revived_uid_source, revived_uid_root))
                writer.execute("INSERT INTO economic_accounting_item_reference VALUES "
                               "(%s,0,0,84,2,3,%s,0)", (revived_uid_root, revived_uid_root))
                writer.execute("INSERT INTO item_ownership_ledger VALUES "
                               "(%s,0,84,84,NULL,1,7,0,3,2,1)", (revived_uid_root,))
                writer.execute("UPDATE item_current_owner SET item_revision=3,state=1,"
                               "owner_type=1,owner_id=7 WHERE item_uid=84")
            revived_uid_report = Reconciler().audit(capture(audit, LINEAGE, EPOCH))
            assert revived_uid_report["exception_counts"] == {
                **expected_exceptions, "resurrected_item_uid": 1}, revived_uid_report
            # A second, internally consistent destruction must not count the
            # already retired UID as another supply expense.
            with setup.cursor() as writer:
                writer.execute("UPDATE economic_accounting_operation SET reason=34 "
                               "WHERE operation_id=%s", (revived_uid_root,))
                writer.execute("UPDATE item_ownership_ledger SET reason_type=3,"
                               "to_owner_type=8,to_owner_id=0 WHERE operation_id=%s",
                               (revived_uid_root,))
                writer.execute("UPDATE item_current_owner SET state=2,owner_type=8,"
                               "owner_id=0 WHERE item_uid=84")
            duplicate_retirement_report = Reconciler().audit(capture(audit, LINEAGE, EPOCH))
            assert duplicate_retirement_report["exception_counts"] == {
                **expected_exceptions, "duplicate_item_retirement": 1}, duplicate_retirement_report
            with setup.cursor() as writer:
                for table in ("economic_accounting_operation", "critical_operation_inbox",
                              "economic_accounting_source_claim", "economic_accounting_item_reference",
                              "item_ownership_ledger"):
                    writer.execute(f"DELETE FROM {table} WHERE operation_id IN (%s,%s)",
                                   (duplicate_uid_root, revived_uid_root))
                writer.execute("UPDATE item_current_owner SET item_revision=1,state=1,"
                               "owner_type=1,owner_id=7 WHERE item_uid=84")
            assert Reconciler().audit(capture(audit, LINEAGE, EPOCH))["exception_counts"] == \
                expected_exceptions
            # A deep, unanchored native forest is still auditable. Its missing
            # origins must remain explicit, and a corrupt cycle must terminate.
            deep_first, deep_count = 1000, 1200
            deep_last = deep_first + deep_count - 1
            with setup.cursor() as writer:
                writer.executemany("INSERT INTO item_current_owner VALUES "
                                   "(%s,%s,%s,1,7,0,1,1,1,NULL)",
                                   [(uid, deep_last, uid + 1 if uid < deep_last else None)
                                    for uid in range(deep_first, deep_last + 1)])
            deep_report = Reconciler().audit(capture(audit, LINEAGE, EPOCH))
            assert deep_report["exception_counts"] == {
                **expected_exceptions, "unknown_legacy_origin": deep_count}, deep_report
            with setup.cursor() as writer:
                writer.execute("UPDATE item_current_owner SET parent_item_uid=%s WHERE item_uid=%s",
                               (deep_first, deep_last))
            cycle_report = Reconciler().audit(capture(audit, LINEAGE, EPOCH))
            assert cycle_report["exception_counts"] == {
                **expected_exceptions, "unknown_legacy_origin": deep_count,
                "cyclic_native_topology": deep_count}, cycle_report
            with setup.cursor() as writer:
                writer.execute("DELETE FROM item_current_owner WHERE item_uid BETWEEN %s AND %s",
                               (deep_first, deep_last))
            assert Reconciler().audit(capture(audit, LINEAGE, EPOCH))["exception_counts"] == \
                expected_exceptions
            with setup.cursor() as writer:
                writer.execute("INSERT INTO economic_accounting_operation VALUES "
                               "(%s,%s,%s,NULL,33,1,0,%s,0,0,0,1,NULL)",
                               (prior_item_root, LINEAGE, prior_epoch, prior_item_source))
                writer.execute("INSERT INTO critical_operation_inbox "
                               "(operation_id,status,result_code) VALUES (%s,1,0)",
                               (prior_item_root,))
                writer.execute("INSERT INTO economic_accounting_source_claim VALUES (%s,%s,%s)",
                               (LINEAGE, prior_item_source, prior_item_root))
                writer.execute("INSERT INTO economic_accounting_item_reference VALUES "
                               "(%s,0,0,86,0,1,%s,0)",
                               (prior_item_root, prior_item_root))
                writer.execute("INSERT INTO item_ownership_ledger VALUES "
                               "(%s,0,86,86,NULL,1,7,0,1,0,2)", (prior_item_root,))
                writer.execute("INSERT INTO item_ownership_ledger VALUES "
                               "(%s,0,86,86,NULL,8,0,0,2,1,3)",
                               (prior_unlinked_operation,))
                writer.execute("INSERT INTO item_current_owner VALUES "
                               "(86,86,NULL,8,0,0,2,2,1,NULL)")
            historical_uid_snapshot = capture(audit, LINEAGE, EPOCH)
            assert historical_uid_snapshot["native"]["lineage_uid_reference_coverage"] == {
                "rows": 2, "root_rows": 2}
            prior_root_evidence = next(
                row for row in historical_uid_snapshot["native"]["lineage_uid_reference_roots"]
                if row["operation_id"] == prior_item_root.hex())
            assert prior_root_evidence["reason"] == 33
            assert prior_root_evidence["source_event"] == prior_item_source.hex()
            assert prior_root_evidence["result_code"] == 0
            assert prior_root_evidence["inbox_receipt"] == {
                "status": 1, "result_code": 0, "failure_stage": 0,
                "committed_at_present": True}
            assert historical_uid_snapshot["native"]["uid_event_coverage"] == {
                "tracked_uids": 5, "ledger_events": 2,
                "referenced_events": 2, "unreferenced_events": 0}, historical_uid_snapshot["native"]["uid_event_coverage"]
            assert historical_uid_snapshot["native"]["unattributed_uid_event_coverage"] == {
                "uids": 1, "events": 1}
            uid_86_history = [row for row in historical_uid_snapshot["native"]["uid_history_events"]
                              if row["uid"] == 86]
            assert [(row["before_revision"], row["revision"], row["referenced"])
                    for row in uid_86_history] == [(0, 1, True)]
            unattributed_86 = [row for row in historical_uid_snapshot["native"]["unattributed_uid_events"]
                               if row["uid"] == 86]
            assert [(row["before_revision"], row["revision"])
                    for row in unattributed_86] == [(1, 2)]
            assert next(row for row in historical_uid_snapshot["item_origins"]
                        if row["uid"] == 86)["origin"] == "creation"
            assert any(row["operation_id"] == prior_item_root.hex()
                       for row in historical_uid_snapshot["native"]["lineage_uid_references"])
            historical_uid_report = Reconciler().audit(historical_uid_snapshot)
            assert historical_uid_report["exception_counts"] == {
                **expected_exceptions,
                "evidence_loss": 1,
                "ambiguous_lineage_ownership_uid": 1,
                "stale_native_item": 1,
                "unattributed_ownership_event": 1}, historical_uid_report
            assert historical_uid_report["exception_counts"].get("unreferenced_uid_event", 0) == 0
            assert historical_uid_report["exception_counts"].get(
                "lineage_orphan_uid_reference", 0) == 0
            with setup.cursor() as writer:
                writer.execute("DELETE FROM economic_accounting_item_reference "
                               "WHERE operation_id=%s", (prior_item_root,))
                writer.execute("DELETE FROM economic_accounting_source_claim "
                               "WHERE source_event=%s", (prior_item_source,))
                writer.execute("DELETE FROM economic_accounting_operation "
                               "WHERE operation_id=%s", (prior_item_root,))
                writer.execute("DELETE FROM critical_operation_inbox WHERE operation_id=%s",
                               (prior_item_root,))
                writer.execute("DELETE FROM item_ownership_ledger WHERE item_uid=86")
                writer.execute("DELETE FROM item_current_owner WHERE item_uid=86")
            unanchored_operation = bytes.fromhex("97" * 16)
            unscoped_operation = bytes.fromhex("96" * 16)
            unattributed_operation = bytes.fromhex("95" * 16)
            other_lineage_operation = bytes.fromhex("94" * 16)
            other_lineage = bytes.fromhex("44" * 16)
            with setup.cursor() as writer:
                writer.execute("INSERT INTO economic_accounting_operation VALUES "
                               "(%s,%s,%s,NULL,33,1,0,NULL,0,0,0,0,NULL)",
                               (unanchored_operation, LINEAGE, EPOCH))
                writer.execute("INSERT INTO economic_accounting_operation VALUES "
                               "(%s,%s,%s,NULL,33,1,0,NULL,0,0,0,0,NULL)",
                               (unscoped_operation, LINEAGE, EPOCH))
                writer.execute("INSERT INTO economic_accounting_operation VALUES "
                               "(%s,%s,%s,NULL,33,1,0,NULL,0,0,0,0,NULL)",
                               (other_lineage_operation, other_lineage, EPOCH))
                writer.execute("INSERT INTO item_current_owner VALUES "
                               "(85,85,NULL,1,7,0,1,1,1,NULL)")
                writer.execute("INSERT INTO item_ownership_ledger VALUES "
                               "(%s,0,85,85,NULL,1,7,0,1,0,2)",
                               (unanchored_operation,))
                writer.execute("INSERT INTO item_ownership_ledger VALUES "
                               "(%s,0,87,87,NULL,1,7,0,1,0,2)",
                               (unscoped_operation,))
                writer.execute("INSERT INTO item_ownership_ledger VALUES "
                               "(%s,0,88,88,NULL,1,7,0,1,0,2)",
                               (unattributed_operation,))
                writer.execute("INSERT INTO item_ownership_ledger VALUES "
                               "(%s,0,89,89,NULL,1,7,0,1,0,2)",
                               (other_lineage_operation,))
            unanchored_snapshot = capture(audit, LINEAGE, EPOCH)
            assert unanchored_snapshot["native"]["uid_event_coverage"] == {
                "tracked_uids": 6, "ledger_events": 3,
                "referenced_events": 1, "unreferenced_events": 2}
            assert {row["uid"] for row in
                    unanchored_snapshot["native"]["unreferenced_uid_events"]} == {85, 87}
            assert next(row for row in unanchored_snapshot["item_origins"]
                        if row["uid"] == 87)["origin"] == "creation"
            assert unanchored_snapshot["native"]["unanchored_ownership_uids"] == []
            assert unanchored_snapshot["native"]["uid_scope_coverage"] == {
                "ownership_uid_count": 3, "anchored_ownership_uid_count": 3,
                "unanchored_ownership_uid_count": 0,
                "other_lineage_ownership_uid_count": 1,
                "unattributed_ownership_uid_count": 1,
                "ambiguous_lineage_ownership_uid_count": 0}
            assert unanchored_snapshot["native"]["ambiguous_lineage_ownership_uids"] == []
            unanchored_report = Reconciler().audit(unanchored_snapshot)
            assert unanchored_report["exception_counts"]["unreferenced_uid_event"] == 2
            assert unanchored_report["exception_counts"].get("unknown_legacy_origin", 0) == 0
            assert unanchored_report["exception_counts"].get(
                "unanchored_ownership_uid", 0) == 0
            with setup.cursor() as writer:
                writer.execute("DELETE FROM item_current_owner WHERE item_uid=85")
                writer.execute("DELETE FROM item_ownership_ledger WHERE operation_id IN (%s,%s)",
                               (unanchored_operation, unattributed_operation))
                writer.execute("DELETE FROM economic_accounting_operation "
                               "WHERE operation_id IN (%s,%s)",
                               (unanchored_operation, unscoped_operation))
                writer.execute("DELETE FROM item_ownership_ledger WHERE operation_id=%s",
                               (unscoped_operation,))
                writer.execute("DELETE FROM item_ownership_ledger WHERE operation_id=%s",
                               (other_lineage_operation,))
                writer.execute("DELETE FROM economic_accounting_operation "
                               "WHERE operation_id=%s", (other_lineage_operation,))
            unlinked_operation = bytes.fromhex("98" * 16)
            with setup.cursor() as writer:
                writer.execute("INSERT INTO item_ownership_ledger VALUES "
                               "(%s,0,84,84,NULL,8,0,0,2,1,3)", (unlinked_operation,))
            unlinked_snapshot = capture(audit, LINEAGE, EPOCH)
            assert unlinked_snapshot["native"]["uid_event_coverage"] == {
                "tracked_uids": 4, "ledger_events": 1,
                "referenced_events": 1, "unreferenced_events": 0}
            assert unlinked_snapshot["native"]["unreferenced_uid_events"] == []
            unlinked_report = Reconciler().audit(unlinked_snapshot)
            assert unlinked_report["exception_counts"] == {
                **expected_exceptions,
                "evidence_loss": 1,
                "ambiguous_lineage_ownership_uid": 1,
                "unattributed_ownership_event": 1}, unlinked_report
            assert any(exception.get("uid") == 84 for exception in
                       unlinked_report["exceptions"]
                       if exception["code"] == "ambiguous_lineage_ownership_uid")
            with setup.cursor() as writer:
                writer.execute("DELETE FROM item_ownership_ledger WHERE operation_id=%s",
                               (unlinked_operation,))
                writer.execute("UPDATE economic_account_mapping SET active_native_id=NULL,"
                               "retiring_operation_id=%s WHERE mapping_id=9", (root,))
            invalid_retirement = Reconciler().audit(capture(audit, LINEAGE, EPOCH))
            assert "retired_nonzero_holding" in invalid_retirement["exception_counts"]
            assert "invalid_mapping_retirement_effect" in invalid_retirement["exception_counts"]
            with setup.cursor() as writer:
                writer.execute("UPDATE economic_account_mapping SET active_native_id=9,"
                               "retiring_operation_id=NULL WHERE mapping_id=9")
                writer.execute("INSERT INTO economic_account_mapping VALUES "
                               "(16,2,2,0,NULL,%s,1,%s,16,%s)",
                               (LINEAGE, prior_root, prior_root))
            earlier_snapshot = capture(audit, LINEAGE, EPOCH)
            earlier_retirement = Reconciler().audit(earlier_snapshot)
            assert "invalid_mapping_retirement_effect" in earlier_retirement["exception_counts"]
            historical_root = next(
                root for root in earlier_snapshot["native"]["retirement_roots"]
                if root["operation_id"] == prior_root.hex())
            assert historical_root["reason"] == 3
            assert historical_root["retirement_inbox_status"] == 1
            assert historical_root["retirement_inbox_result_code"] == 0
            assert historical_root["retirement_inbox_failure_stage"] == 0
            assert historical_root["retirement_inbox_committed_at_present"] is True
            assert len(historical_root["effects"]) == len(historical_root["postings"]) == 2
            with setup.cursor() as writer:
                writer.execute("DELETE FROM economic_account_mapping WHERE mapping_id=16")
            with setup.cursor() as writer:
                writer.execute("UPDATE auction_money_pickups SET money=300 WHERE pid=7")
            claim_mismatch = Reconciler().audit(capture(audit, LINEAGE, EPOCH))["exception_counts"]
            assert claim_mismatch["pending_claim_source_balance_mismatch"] == 1
            with setup.cursor() as writer:
                writer.execute("UPDATE auction_money_pickups SET money=250 WHERE pid=7")
                writer.execute("UPDATE economic_accounting_account_effect SET after_copper=249 "
                               "WHERE operation_id=%s AND account_index=0", (prior_root,))
            broken_source_root = Reconciler().audit(capture(audit, LINEAGE, EPOCH))["exception_counts"]
            assert broken_source_root["invalid_pending_claim_source_root"] == 1
            with setup.cursor() as writer:
                writer.execute("UPDATE economic_accounting_account_effect SET after_copper=250 "
                               "WHERE operation_id=%s AND account_index=0", (prior_root,))
                writer.execute("UPDATE economic_accounting_account_effect SET after_copper=251 "
                               "WHERE operation_id=%s AND account_index=0", (consumer_root,))
            broken_consumer_root = Reconciler().audit(capture(audit, LINEAGE, EPOCH))["exception_counts"]
            assert broken_consumer_root["invalid_pending_claim_consumer_root"] == 1
            with setup.cursor() as writer:
                writer.execute("UPDATE economic_accounting_account_effect SET after_copper=250 "
                               "WHERE operation_id=%s AND account_index=0", (consumer_root,))
            with setup.cursor() as writer:
                writer.execute("UPDATE item_current_owner SET coin_payload=%s WHERE item_uid=82",
                               (coin_payload(82, [2, 2, 3, 4]),))
            stale_pile = Reconciler().audit(capture(audit, LINEAGE, EPOCH))["exception_counts"]
            assert stale_pile["stale_native_balance"] == 1, stale_pile
            with setup.cursor() as writer:
                writer.execute("UPDATE item_current_owner SET coin_payload=%s WHERE item_uid=82",
                               (coin_payload(82, [1, 2, 3, 4]),))
            with setup.cursor() as writer:
                writer.execute("INSERT INTO item_current_owner VALUES "
                               "(85,85,NULL,1,7,0,1,1,3,X'00')")
            try:
                capture(audit, LINEAGE, EPOCH)
                raise AssertionError("malformed coin-pile payload passed the audit export")
            except exporter.ExportError:
                pass
            with setup.cursor() as writer:
                writer.execute("DELETE FROM item_current_owner WHERE item_uid=85")
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
                assert json.loads(result.stdout)["exception_counts"] == expected_exceptions
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
                               "(70,1,1,1,7,%s,1,NULL,7,%s),(10,2,2,0,10,%s,1,NULL,10,%s),"
                               "(71,1,1,0,9,%s,1,NULL,9,%s)",
                               (LINEAGE, OP, LINEAGE, OP,
                                bytes.fromhex("dd" * 16), OP))
            unmapped = Reconciler().audit(capture(audit, LINEAGE, EPOCH))["exception_counts"]
            assert unmapped["unmapped_native_wallet"] == 3, unmapped
            assert unmapped["multiply_mapped_native_wallet"] == 1
            assert unmapped["dangling_bank_mapping"] == 1
            with setup.cursor() as cursor:
                cursor.execute("INSERT INTO economic_accounting_operation VALUES "
                               "(%s,%s,%s,NULL,5,1,0,NULL,0,0,0,0,NULL)",
                               (bytes.fromhex("9a" * 16), LINEAGE, prior_epoch))
                cursor.execute("INSERT INTO economic_accounting_operation VALUES "
                               "(%s,%s,%s,NULL,3,1,0,%s,0,0,0,0,NULL)",
                               (bytes.fromhex("ab" * 16), LINEAGE, prior_epoch, prior_source))
                cursor.execute("INSERT INTO economic_accounting_source_claim VALUES (%s,%s,%s)",
                               (LINEAGE, bytes.fromhex("ae" * 48), bytes.fromhex("ad" * 16)))
            cross_epoch = Reconciler().audit(capture(audit, LINEAGE, EPOCH))["exception_counts"]
            assert cross_epoch["lineage_missing_source_claim"] == 1
            assert cross_epoch["lineage_duplicate_source_event"] == 1
            assert cross_epoch["orphan_source_claim"] == 1
            assert cross_epoch["lineage_missing_required_source_event"] == 1
            for reason in (24, 27):
                with setup.cursor() as cursor:
                    cursor.execute("UPDATE economic_accounting_operation "
                                   "SET reason=%s,realized_price_copper=125 "
                                   "WHERE operation_id=%s", (reason, root))
                priced_snapshot = capture(audit, LINEAGE, EPOCH)
                assert priced_snapshot["native"]["realized_price_coverage"] == {
                    "column_available": True, "candidate_rows": 1,
                    "missing_price_rows": 0}
                assert priced_snapshot["native"]["lineage_realized_prices"] == [{
                    "operation_id": root.hex(), "lineage": LINEAGE.hex(),
                    "epoch": EPOCH.hex(), "reason": reason,
                    "outcome": "committed", "result_code": 0,
                    "inbox_receipt": {"status": 1, "result_code": 0,
                                      "failure_stage": 0,
                                      "committed_at_present": True},
                    "realized_price_copper": 125}]
                priced_report = Reconciler().audit(priced_snapshot)
                assert "missing_realized_trade_price" not in priced_report[
                    "exception_counts"]
                assert "realized_price_scope_mismatch" not in priced_report[
                    "exception_counts"]
            with setup.cursor() as cursor:
                cursor.execute("ALTER TABLE economic_accounting_operation "
                               "DROP COLUMN realized_price_copper")
            legacy_price_snapshot = capture(audit, LINEAGE, EPOCH)
            assert legacy_price_snapshot["native"]["realized_price_coverage"] == {
                "column_available": False, "candidate_rows": 1, "missing_price_rows": 1}
            legacy_price_report = Reconciler().audit(legacy_price_snapshot)
            assert "realized_price_column_missing" in legacy_price_report["exception_counts"]
            assert "missing_realized_trade_price" in legacy_price_report["exception_counts"]
        finally:
            audit.close()
    finally:
        setup.close()
    print("SQL partial audit snapshot: one consistent cut and fail-closed reconciliation passed")
finally:
    with admin.cursor() as cursor:
        cursor.execute(f"DROP DATABASE IF EXISTS `{schema}`")
        cursor.execute(f"DROP USER IF EXISTS '{reader}'@'127.0.0.1'")
    admin.close()
