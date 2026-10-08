#!/usr/bin/env python3
"""Exercise partial SQL audit export on a disposable, SELECT-only database."""

import hashlib
import copy
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
from reconcile_economy_accounting import Reconciler, SnapshotError, view  # noqa: E402
from test_economic_sql_audit_origins import (EPOCH, LINEAGE, OPENING, OP, baseline_projections,
                                            baseline_root, key, witness)  # noqa: E402
from test_reconcile_economy_accounting import malformed_sources, source_identity  # noqa: E402

INSTALL = bytes.fromhex("77" * 16)
ROOT_INSERT = ("INSERT INTO economic_accounting_operation(operation_id,lineage,epoch,original_operation_id,"
               "reason,outcome,result_code,source_event,account_count,posting_count,child_count,item_event_count,"
               "realized_price_copper) VALUES ")
POSTING_INSERT = ("INSERT INTO economic_accounting_coin_posting(operation_id,line_index,account_index,child_index,"
                  "delta_copper,delta_silver,delta_gold,delta_platinum,copper_value) VALUES ")
REFERENCE_INSERT = ("INSERT INTO economic_accounting_item_reference(operation_id,event_index,child_index,"
                    "item_uid,before_revision,after_revision,legacy_operation_id,legacy_event_index) VALUES ")
LEDGER_INSERT = ("INSERT INTO item_ownership_ledger(operation_id,event_index,item_uid,root_item_uid,"
                 "parent_item_uid,to_owner_type,to_owner_id,to_owner_context_id,item_revision,"
                 "from_owner_revision,reason_type) VALUES ")
CHILD_INSERT = ("INSERT INTO economic_accounting_child(operation_id,child_index,child_operation_id,parent_index) VALUES ")
ITEM_INSERT = ("INSERT INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,owner_type,"
               "owner_id,owner_context_id,item_revision,state,vnum,coin_payload) VALUES ")


def bind_synthetic_baseline(cursor, blob):
    baseline = baseline_root(bytes(blob))
    fields = ("accounting_version", "writer_id", "policy_version", "compiler_version",
              "actor_kind", "actor_id", "intent_digest", "domain_digest", "plan_digest",
              "canonical_intent", "canonical_plan", "before_witness_count", "after_witness_count",
              "account_count", "posting_count")
    cursor.execute("UPDATE economic_accounting_operation SET " +
                   ",".join(field + "=%s" for field in fields) + " WHERE operation_id=%s",
                   (*[baseline[field] for field in fields], OP))
    cursor.execute("UPDATE critical_operation_inbox SET command_hash=%s WHERE operation_id=%s",
                   (baseline["inbox_command_hash"], OP))
    for table, rows in zip(("economic_accounting_account_effect", "economic_accounting_coin_posting",
                            "economic_baseline_reservation"), baseline_projections(baseline)):
        cursor.execute("DELETE FROM " + table + " WHERE operation_id=%s", (OP,))
        for row in rows:
            cursor.execute("INSERT INTO " + table + "(" + ",".join(row) + ") VALUES(" +
                           ",".join(["%s"] * len(row)) + ")", tuple(row.values()))

private_socket = (os.environ.get("DB_SOCKET", "").startswith("/plan5-restore-baseline-") and
                  os.environ.get("TEST_DB_DISPOSABLE") == "1" and os.environ.get("ENVIRONMENT") == "test")
if (os.environ.get("ECONOMIC_ACCOUNTING_DISPOSABLE_SCHEMA") != "1" or
        os.environ.get("DB_HOST") != "127.0.0.1" or (os.environ.get("DB_SOCKET") and not private_socket)):
    raise SystemExit("explicit disposable loopback database required")

settings = {"host": "127.0.0.1", "port": int(os.environ.get("DB_PORT", "3306")),
            "user": os.environ["DB_USER"], "password": os.environ["DB_PASSWORD"],
            "autocommit": True, "cursorclass": pymysql.cursors.DictCursor,
            "connect_timeout": 5, "read_timeout": 20, "write_timeout": 5}
if private_socket:
    settings.pop("host")
    settings.pop("port")
    settings["unix_socket"] = os.environ["DB_SOCKET"]
schema = "economic_schema_test_snapshot_" + uuid.uuid4().hex
reader = "audit_" + uuid.uuid4().hex[:12]
reader_host = "localhost" if private_socket else "127.0.0.1"
transport_args = (["--socket", settings["unix_socket"]] if private_socket else
                  ["--port", str(settings["port"])])
assert re.fullmatch(r"economic_schema_test_snapshot_[0-9a-f]{32}", schema)
assert re.fullmatch(r"audit_[0-9a-f]{12}", reader)

TABLES = (
    "CREATE TABLE economic_baseline_control (lineage BINARY(16),epoch BINARY(16),"
    "opening_account VARBINARY(40),revision BIGINT UNSIGNED,last_operation_id BINARY(16)) ENGINE=InnoDB",
    "CREATE TABLE economic_baseline_witness (operation_id BINARY(16),lineage BINARY(16),"
    "epoch BINARY(16),book_revision BIGINT UNSIGNED,holding_count INT,item_count INT,"
    "witness_digest BINARY(32),canonical_witness MEDIUMBLOB,witness_version INT DEFAULT 1) ENGINE=InnoDB",
    "CREATE TABLE economic_baseline_reservation (lineage BINARY(16),epoch BINARY(16),"
    "identity_kind INT,identity_id BIGINT UNSIGNED,operation_id BINARY(16)) ENGINE=InnoDB",
    "CREATE TABLE economic_accounting_operation (operation_id BINARY(16),lineage BINARY(16),"
    "epoch BINARY(16),original_operation_id BINARY(16),reason INT,outcome INT,result_code INT,"
    "source_event BINARY(48),account_count INT,posting_count INT,child_count INT,"
    "item_event_count INT,realized_price_copper BIGINT NULL,"
    "accounting_version INT NULL,writer_id INT NULL,policy_version INT NULL,compiler_version INT NULL,"
    "actor_kind INT NULL,actor_id BIGINT UNSIGNED NULL,intent_digest BINARY(32) NULL,domain_digest BINARY(32) NULL,"
    "plan_digest BINARY(32) NULL,canonical_intent MEDIUMBLOB NULL,canonical_plan MEDIUMBLOB NULL,"
    "before_witness_count INT DEFAULT 0,after_witness_count INT DEFAULT 0) ENGINE=InnoDB",
    "CREATE TABLE critical_operation_inbox (operation_id BINARY(16),status INT,result_code INT,keys_hash BINARY(32),"
    "command_hash BINARY(32),"
    "failure_stage INT NOT NULL DEFAULT 0,committed_at TIMESTAMP NULL DEFAULT CURRENT_TIMESTAMP,"
    "durable_revision BIGINT UNSIGNED DEFAULT 1,command_type INT DEFAULT 20,schema_version INT DEFAULT 2,"
    "payload_version INT DEFAULT 1,result_payload VARBINARY(16) DEFAULT X'') ENGINE=InnoDB",
    "CREATE TABLE economic_accounting_account_effect (operation_id BINARY(16),account_index INT,"
    "account_key BINARY(40),before_copper BIGINT,before_silver BIGINT,before_gold BIGINT,"
    "before_platinum BIGINT,after_copper BIGINT,after_silver BIGINT,after_gold BIGINT,"
    "after_platinum BIGINT,before_revision BIGINT UNSIGNED,after_revision BIGINT UNSIGNED) ENGINE=InnoDB",
    "CREATE TABLE economic_accounting_coin_posting (operation_id BINARY(16),line_index INT,"
    "account_index INT,child_index INT,delta_copper BIGINT,delta_silver BIGINT,delta_gold BIGINT,"
    "delta_platinum BIGINT,copper_value BIGINT,event_index INT DEFAULT 0) ENGINE=InnoDB",
    "CREATE TABLE economic_accounting_child (operation_id BINARY(16),child_index INT,"
    "child_operation_id BINARY(16),parent_index INT,domain_id INT UNSIGNED NULL,"
    "discriminator BIGINT UNSIGNED NULL,relationship INT NULL,receipt_operation_id BINARY(16) NULL) ENGINE=InnoDB",
    "CREATE TABLE economic_accounting_item_reference (operation_id BINARY(16),event_index INT,"
    "child_index INT,item_uid BIGINT,before_revision BIGINT UNSIGNED,after_revision BIGINT UNSIGNED,"
    "legacy_operation_id BINARY(16),legacy_event_index INT,line_index INT DEFAULT 0) ENGINE=InnoDB",
    "CREATE TABLE economic_accounting_source_claim (lineage BINARY(16),source_event BINARY(48),"
    "operation_id BINARY(16)) ENGINE=InnoDB",
    "CREATE TABLE economic_account_mapping (mapping_id BIGINT,account_kind INT,locator_kind INT NOT NULL,context_id BIGINT,"
    "active_native_id BIGINT,lineage BINARY(16),backend_kind INT,"
    "retiring_operation_id BINARY(16) NULL,native_id BIGINT,"
    "creating_operation_id BINARY(16)) ENGINE=InnoDB",
    "CREATE TABLE economic_sql_lifecycle_installation (operation_id BINARY(16),"
    "lineage BINARY(16),epoch BINARY(16),baseline_operation_id BINARY(16),phase INT,"
    "selected_epoch BINARY(16),revision BIGINT,"
    "native_boundary_digest BINARY(32) NULL,request_digest BINARY(32) NULL) ENGINE=InnoDB",
    "CREATE TABLE economic_pending_claim_source (source_operation_id BINARY(16),source_slot INT,"
    "lineage BINARY(16),claim_mapping_id BIGINT,beneficiary_pid BIGINT,amount BIGINT,"
    "claim_operation_id BINARY(16)) ENGINE=InnoDB",
    "CREATE TABLE economic_pending_claim_consumption (spending_operation_id BINARY(16),"
    "source_operation_id BINARY(16),source_slot INT,amount BIGINT UNSIGNED) ENGINE=InnoDB",
    "CREATE TABLE player_data (pid BIGINT,copper BIGINT,silver BIGINT,gold BIGINT,platinum BIGINT,"
    "wallet_revision BIGINT UNSIGNED) ENGINE=InnoDB",
    "CREATE TABLE player_items (id INT UNSIGNED,pid INT UNSIGNED,container_id INT UNSIGNED NULL,"
    "obj_uid BIGINT UNSIGNED NULL,vnum INT,equip_slot TINYINT NULL,quantity SMALLINT UNSIGNED NULL,"
    "item_type TINYINT NULL,value0 INT NULL,value1 INT NULL,value2 INT NULL,value3 INT NULL) ENGINE=InnoDB",
    "CREATE TABLE player_pets (id INT UNSIGNED,owner_pid INT UNSIGNED,pet_uid BIGINT UNSIGNED NULL) ENGINE=InnoDB",
    "CREATE TABLE corpses (id INT,value3 INT NULL,save_id BIGINT,corpse_revision BIGINT UNSIGNED,room_vnum INT NULL) ENGINE=InnoDB",
    "CREATE TABLE corpse_items (id INT UNSIGNED,corpse_id INT,container_id INT UNSIGNED NULL,obj_uid BIGINT UNSIGNED NULL,"
    "vnum INT,quantity SMALLINT UNSIGNED NULL,weight INT NULL,extra_flags BIGINT UNSIGNED NULL,"
    "value0 INT NULL,value1 INT NULL,value2 INT NULL,value3 INT NULL) ENGINE=InnoDB",
    "CREATE TABLE lockers (id INT UNSIGNED,racewar TINYINT NULL,owner_pid INT NULL,owner_assoc_id INT NULL) ENGINE=InnoDB",
    "CREATE TABLE account_lockers (id INT UNSIGNED,racewar TINYINT NULL) ENGINE=InnoDB",
    "CREATE TABLE private_chests (id INT UNSIGNED,locker_id INT UNSIGNED,is_public TINYINT NULL) ENGINE=InnoDB",
    "CREATE TABLE locker_chests (id INT UNSIGNED,locker_id INT UNSIGNED,is_public TINYINT NULL) ENGINE=InnoDB",
    "CREATE TABLE locker_items (id INT UNSIGNED,locker_id INT UNSIGNED,chest_id INT UNSIGNED NULL,"
    "container_id INT UNSIGNED NULL,obj_uid BIGINT UNSIGNED NULL,vnum INT,quantity SMALLINT UNSIGNED NULL,"
    "weight INT NULL,extra_flags BIGINT UNSIGNED NULL,item_type TINYINT NULL,"
    "value0 INT NULL,value1 INT NULL,value2 INT NULL,value3 INT NULL) ENGINE=InnoDB",
    "CREATE TABLE account_locker_items (id INT UNSIGNED,chest_id INT UNSIGNED,container_id INT UNSIGNED NULL,"
    "obj_uid BIGINT UNSIGNED NULL,vnum INT,quantity SMALLINT UNSIGNED NULL,weight INT NULL,extra_flags BIGINT UNSIGNED NULL,"
    "value0 INT NULL,value1 INT NULL,value2 INT NULL,value3 INT NULL) ENGINE=InnoDB",
    "CREATE TABLE player_pet_items (id INT UNSIGNED,pet_id INT UNSIGNED,container_id INT UNSIGNED NULL,"
    "obj_uid BIGINT UNSIGNED NULL,vnum INT,equip_slot TINYINT NULL,item_type TINYINT NULL,"
    "value0 INT NULL,value1 INT NULL,value2 INT NULL,value3 INT NULL) ENGINE=InnoDB",
    "CREATE TABLE account_banks (id BIGINT,bank_copper BIGINT,bank_silver BIGINT,bank_gold BIGINT,"
    "bank_platinum BIGINT,bank_revision BIGINT UNSIGNED) ENGINE=InnoDB",
    "CREATE TABLE auctions (id BIGINT,status VARCHAR(16),cur_price BIGINT,"
    "auction_revision BIGINT,winning_bidder_pid BIGINT,seller_pid BIGINT DEFAULT 1,"
    "custody_state INT DEFAULT 0,quantity INT DEFAULT 1,obj_vnum INT DEFAULT 0,"
    "obj_blob_str LONGBLOB) ENGINE=InnoDB",
    "CREATE TABLE auction_item_custody (auction_id BIGINT,slot INT,item_uid BIGINT UNSIGNED,"
    "item_revision BIGINT UNSIGNED,vnum INT,claim_pid BIGINT NULL,claim_operation_id BINARY(16) NULL,"
    "claimed_at TIMESTAMP NULL,obj_blob LONGBLOB) ENGINE=InnoDB",
    "CREATE TABLE auction_item_pickups (id BIGINT,pid BIGINT,quantity INT,retrieved INT,"
    "obj_blob_str LONGBLOB) ENGINE=InnoDB",
    "CREATE TABLE auction_money_pickups (pid BIGINT,money BIGINT,"
    "claim_revision BIGINT UNSIGNED) ENGINE=InnoDB",
    "CREATE TABLE shopkeepers (id BIGINT,cash BIGINT,shop_revision BIGINT UNSIGNED,shop_id INT DEFAULT 0) ENGINE=InnoDB",
    "CREATE TABLE shopkeeper_items (id INT UNSIGNED,shopkeeper_id INT,container_id INT UNSIGNED NULL,"
    "obj_uid BIGINT UNSIGNED NULL,vnum INT,equip_slot TINYINT NULL,quantity SMALLINT UNSIGNED NULL) ENGINE=InnoDB",
    "CREATE TABLE ships (id INT NOT NULL PRIMARY KEY,money INT NULL) ENGINE=InnoDB",
    "CREATE TABLE guilds (id INT UNSIGNED NOT NULL PRIMARY KEY,"
    "copper INT UNSIGNED NOT NULL,silver INT UNSIGNED NOT NULL,"
    "gold INT UNSIGNED NOT NULL,platinum INT UNSIGNED NOT NULL,"
    "outcome_revision BIGINT UNSIGNED NOT NULL) ENGINE=InnoDB",
    "CREATE TABLE item_current_owner (item_uid BIGINT,root_item_uid BIGINT,parent_item_uid BIGINT,"
    "owner_type INT,owner_id BIGINT,owner_context_id BIGINT,item_revision BIGINT UNSIGNED,state INT,"
    "vnum INT,coin_payload MEDIUMBLOB,equipment_slot SMALLINT UNSIGNED NOT NULL DEFAULT 0) ENGINE=InnoDB",
    "CREATE TABLE item_ownership_ledger (operation_id BINARY(16),event_index INT,item_uid BIGINT,"
    "root_item_uid BIGINT,parent_item_uid BIGINT,to_owner_type INT,to_owner_id BIGINT,"
    "to_owner_context_id BIGINT,item_revision BIGINT UNSIGNED,from_owner_revision BIGINT,"
    "reason_type INT,from_owner_type INT NULL,from_owner_id BIGINT UNSIGNED NULL,"
    "from_owner_context_id BIGINT UNSIGNED NULL,from_equipment_slot INT DEFAULT 0,"
    "to_equipment_slot INT DEFAULT 0) ENGINE=InnoDB",
    "CREATE TABLE currency_ledger (operation_id BINARY(16)) ENGINE=InnoDB",
    "CREATE TABLE critical_outbox (operation_id BINARY(16)) ENGINE=InnoDB",
)


def coin_payload(uid, amounts, vnum=3, *, dynamic_count=0, spell_counts=()):
    blob = bytearray(struct.pack("<IihQqibB", 1, -1, -1, uid, 0, vnum, 20, 0))
    blob.extend(struct.pack("<I", 0) * 4)
    blob.extend(struct.pack("<8i", *amounts, 0, 0, 0, 0))
    blob.extend(struct.pack("<6q", *([0] * 6)))
    blob.extend(struct.pack("<5I", *([0] * 5)))
    blob.extend(struct.pack("<ibihh", 0, 0, 0, 100, 0))
    blob.extend(struct.pack("<5Q", *([0] * 5)))
    blob.extend(struct.pack("<8h", *([0] * 8)))
    blob.extend(struct.pack("<I", dynamic_count))
    blob.extend(bytes(12 * dynamic_count))
    blob.extend(struct.pack("<I", len(spell_counts)))
    for count in spell_counts:
        blob.extend(bytes(9))  # Two empty strings and a false spellbook flag.
        blob.extend(struct.pack("<I", count))
        blob.extend(bytes(4 * count))
    return bytes(blob)


def verify_coin_payload_row_budget(owner, reader, snapshot):
    """The SELECT-only exporter uses the full native codec's shared row bound."""
    from _plan5_equipment_restore import Connection, inventory

    output = ROOT / "bin/tests/plan5-coin-row-budget" / uuid.uuid4().hex
    output.mkdir(parents=True)
    initial = inventory(owner)
    cases = (
        ("empty", 0, (), True),
        ("affects_exact", 8191, (), True),
        ("affects_above", 8192, (), False),
        ("descriptions_exact", 0, (0,) * 8191, True),
        ("descriptions_above", 0, (0,) * 8192, False),
        ("spells_exact", 0, (8190,), True),
        ("spells_above", 0, (8191,), False),
        ("combined_exact", 4095, (4095,), True),
        ("combined_above", 4095, (4096,), False),
        ("two_descriptions_exact", 0, (4094, 4095), True),
        ("two_descriptions_above", 0, (4095, 4095), False),
        ("affects_descriptions_above", 8190, (0, 0), False),
    )
    records = []
    try:
        for vnum in (3, 402013):
            for label, dynamic, spells, accepted in cases:
                payload = coin_payload(82, [1, 2, 3, 4], vnum,
                                       dynamic_count=dynamic, spell_counts=spells)
                with owner.cursor() as cursor:
                    cursor.execute("UPDATE item_current_owner SET vnum=%s,coin_payload=%s "
                                   "WHERE item_uid=82", (vnum, payload))
                before = inventory(owner)
                connection = Connection(reader)
                captured, error = None, None
                try:
                    captured = capture(connection, LINEAGE, EPOCH)
                except exporter.ExportError as caught:
                    error = str(caught)
                assert connection.rollbacks == connection.observer.closes == 1
                assert inventory(owner) == before
                row = dict(case=label, vnum=vnum, rows=1 + dynamic + len(spells) + sum(spells),
                           accepted=captured is not None, error=error, rollback_calls=1,
                           cursor_close_calls=1, native_sources_unchanged=True)
                records.append(row)
                print("COIN_ROW_SOURCE " + json.dumps(row, sort_keys=True), flush=True)
                target = output / (str(vnum) + "-" + label)
                target.mkdir()
                (target / "payload.bin").write_bytes(payload)
                for name in ("before", "after"):
                    (target / ("authority-" + name + ".json")).write_text(
                        json.dumps(before, sort_keys=True) + "\n")
                (target / "queries.json").write_text(json.dumps(connection.observer.queries) + "\n")
                if accepted:
                    expected = copy.deepcopy(snapshot)
                    next(item for item in expected["native"]["items"] if item["uid"] == 82)["vnum"] = vnum
                    assert error is None and captured == expected, row
                    (target / "snapshot.json").write_text(json.dumps(captured, sort_keys=True) + "\n")
                else:
                    assert captured is None and error == "coin-pile nested row count exceeds limit", row
    finally:
        with owner.cursor() as cursor:
            cursor.execute("UPDATE item_current_owner SET vnum=3,coin_payload=%s WHERE item_uid=82",
                           (coin_payload(82, [1, 2, 3, 4]),))
    assert inventory(owner) == initial and capture(reader, LINEAGE, EPOCH) == snapshot
    (output / "observations.json").write_text(json.dumps(records, indent=2) + "\n")


def verify_area_coin_views(owner, reader, snapshot):
    """Area-specific ITEM_MONEY literals retain the same independent balances."""
    from _plan5_equipment_restore import Connection, inventory

    output = ROOT / "bin/tests/plan5-area-coin-views" / uuid.uuid4().hex
    output.mkdir(parents=True)
    initial = inventory(owner)
    records = []

    def update(vnum, state=1, payload=None):
        with owner.cursor() as cursor:
            cursor.execute("UPDATE item_current_owner SET vnum=%s,state=%s,coin_payload=%s "
                           "WHERE item_uid=82", (vnum, state, payload if payload is not None else
                                                coin_payload(82, [1, 2, 3, 4], vnum)))

    def sample(label, expected=None, error=None):
        before = inventory(owner)
        connection = Connection(reader)
        captured, actual_error = None, None
        try:
            captured = capture(connection, LINEAGE, EPOCH)
        except exporter.ExportError as caught:
            actual_error = str(caught)
        assert connection.rollbacks == connection.observer.closes == 1
        assert inventory(owner) == before
        target = output / label
        target.mkdir()
        for name in ("before", "after"):
            (target / ("authority-" + name + ".json")).write_text(
                json.dumps(before, sort_keys=True) + "\n")
        (target / "queries.json").write_text(json.dumps(connection.observer.queries) + "\n")
        if captured is not None:
            (target / "snapshot.json").write_text(json.dumps(captured, sort_keys=True) + "\n")
        row = dict(case=label, error=actual_error, rollback_calls=1, cursor_close_calls=1,
                   native_sources_unchanged=True)
        records.append(row)
        (output / "observations.json").write_text(json.dumps(records, indent=2) + "\n")
        print("AREA_COIN_SOURCE " + json.dumps(row, sort_keys=True), flush=True)
        assert actual_error == error, row
        if expected is not None:
            assert captured == expected, label
        return captured

    try:
        for state in (1, 2, 3):
            update(3, state)
            expected = capture(reader, LINEAGE, EPOCH)
            for vnum in (402013, 402014, 2**31 - 1):
                update(vnum, state)
                # Prototype identity is now retained in current UID metadata;
                # every other projection, including the money, remains equal.
                next(row for row in expected["native"]["items"] if row["uid"] == 82)["vnum"] = vnum
                sample(f"prototype-{vnum}-state-{state}", expected)
        update(402013)
        with owner.cursor() as cursor:
            cursor.execute("UPDATE item_current_owner SET coin_payload=%s WHERE item_uid=82",
                           (coin_payload(82, [2, 2, 3, 4], 402013),))
        drift = sample("area-denomination-drift")
        assert Reconciler().audit(drift)["exception_counts"].get("stale_native_balance", 0) == (
            Reconciler().audit(snapshot)["exception_counts"].get("stale_native_balance", 0) + 1)
        update(402013, payload=coin_payload(82, [1, 2, 3, 4], 402014))
        sample("area-prototype-mismatch", error="coin-pile payload identity or values are invalid")
        update(402013, payload=coin_payload(83, [1, 2, 3, 4], 402013))
        sample("area-uid-mismatch", error="coin-pile payload identity or values are invalid")
        update(402013, payload=coin_payload(82, [-1, 2, 3, 4], 402013))
        sample("area-negative-amount", error="coin-pile payload identity or values are invalid")
        update(402013)
        with reader.cursor() as cursor:
            try:
                cursor.execute("UPDATE item_current_owner SET state=state WHERE item_uid=82")
            except pymysql.MySQLError as caught:
                assert caught.args[0] == 1142, caught.args
            else:
                raise AssertionError("area coin reader admitted UPDATE")
    finally:
        update(3)
    assert inventory(owner) == initial and capture(reader, LINEAGE, EPOCH) == snapshot
    (output / "evidence.json").write_text(json.dumps(dict(probes=records, output=str(output),
        modeled_partial_sql=True, source_fixture_restored=True, permission_denial=1142,
        accounting_activated=False, release_complete=False), indent=2) + "\n")


def bounded_coin_payload(uid, size, vnum=3):
    # Valid snapshot-codec strings make byte-ceiling checks independent of an
    # earlier malformed-payload refusal. Every string stays within4096 bytes.
    prefix = coin_payload(uid, [1, 2, 3, 4], vnum)[:-4] + struct.pack("<I", 512)
    record = struct.pack("<I", 4096) + b"x" * 4096 + struct.pack("<I", 4096) + b"y" * 4096 + bytes(5)
    prefix += record * 511
    tail = size - len(prefix) - 13
    assert 0 <= tail <= 4096
    result = prefix + struct.pack("<I", tail) + b"z" * tail + struct.pack("<I", 0) + bytes(5)
    assert len(result) == size
    assert exporter.decode_coin_payload(result, uid, vnum) == [1, 2, 3, 4]
    return result


def verify_coin_payload_source_bounds(setup, audit, snapshot):
    original_bytes = len(coin_payload(82, [1, 2, 3, 4]))

    def authority():
        with setup.cursor() as cursor:
            cursor.execute("SELECT item_uid,vnum,OCTET_LENGTH(coin_payload) AS bytes,"
                           "SHA2(coin_payload,256) AS digest FROM item_current_owner ORDER BY item_uid")
            items = cursor.fetchall()
            cursor.execute("SELECT * FROM economic_account_mapping ORDER BY mapping_id")
            return items, cursor.fetchall()

    def observed(case, expected_error=None):
        before = authority()
        connection = mock.Mock(wraps=audit)
        cursor = mock.Mock(wraps=audit.cursor())
        connection.cursor.return_value = cursor
        actual_fetch = cursor._mock_wraps.fetchall
        selected = []

        def fetched():
            rows = actual_fetch()
            for row in rows:
                for field in ("coin_payload", "pile_payload"):
                    blob = row.get(field)
                    if blob is not None:
                        selected.append(len(blob))
            return rows

        cursor.fetchall.side_effect = fetched
        result, error = None, None
        try:
            result = capture(connection, LINEAGE, EPOCH)
        except exporter.ExportError as caught:
            error = str(caught)
        finally:
            connection.rollback.assert_called_once_with()
            cursor.close.assert_called_once_with()
        assert all(call.args[0].upper().startswith(("SELECT", "SET TRANSACTION", "START TRANSACTION"))
                   for call in cursor.execute.call_args_list)
        assert authority() == before
        observation = dict(case=case, selected_payload_bytes=sum(selected), payload_values=len(selected),
                           error=error, rollback=1, cursor_closed=True, native_sources_unchanged=True)
        print("COIN_PAYLOAD_SOURCE " + json.dumps(observation, sort_keys=True), flush=True)
        assert error == expected_error, observation
        if expected_error is not None:
            assert not selected, observation
        return result, sum(selected)

    def seed(uid, size, mapping_ids, vnum=3):
        with setup.cursor() as cursor:
            cursor.execute(ITEM_INSERT + "(%s,%s,NULL,1,7,0,1,1,%s,%s)",
                           (uid, uid, vnum, bounded_coin_payload(uid, size, vnum)))
            for mapping_id in mapping_ids:
                cursor.execute("INSERT INTO economic_account_mapping VALUES "
                               "(%s,3,3,0,%s,%s,1,NULL,%s,%s)",
                               (mapping_id, uid, LINEAGE, uid, INSTALL))

    def clean():
        with setup.cursor() as cursor:
            cursor.execute("DELETE FROM economic_account_mapping WHERE mapping_id>=100")
            cursor.execute("DELETE FROM item_current_owner WHERE item_uid>=100")
            cursor.execute("UPDATE item_current_owner SET coin_payload=NULL WHERE item_uid=81")
        assert capture(audit, LINEAGE, EPOCH) == snapshot

    try:
        seed(100, exporter.MAX_ITEM_PAYLOAD_BYTES, [100])
        with setup.cursor() as cursor:
            cursor.execute("UPDATE item_current_owner SET coin_payload=CONCAT(coin_payload,X'00') WHERE item_uid=100")
        observed("individual_above_4MiB", "coin-pile source exceeds audit bounds")
        clean()

        for uid in range(100, 108):
            seed(uid, exporter.MAX_ITEM_PAYLOAD_BYTES - original_bytes if uid == 100 else
                 exporter.MAX_ITEM_PAYLOAD_BYTES, [uid])
        _, selected = observed("aggregate_exact_32MiB")
        assert selected == 2 * exporter.MAX_INPUT_BYTES
        with setup.cursor() as cursor:
            # Enlarge a valid string in the first payload by one byte.
            payload = bounded_coin_payload(100, exporter.MAX_ITEM_PAYLOAD_BYTES - original_bytes + 1)
            cursor.execute("UPDATE item_current_owner SET coin_payload=%s WHERE item_uid=100", (payload,))
        observed("aggregate_above_32MiB", "coin-pile source exceeds audit bounds")
        clean()

        seed(100, exporter.MAX_ITEM_PAYLOAD_BYTES, list(range(100, 109)))
        observed("repeated_mapping_join_above_32MiB", "mapped coin-pile source exceeds audit bounds")
        clean()

        seed(100, exporter.MAX_ITEM_PAYLOAD_BYTES, [100], 402013)
        with setup.cursor() as cursor:
            cursor.execute("UPDATE item_current_owner SET coin_payload=CONCAT(coin_payload,X'00') WHERE item_uid=100")
        observed("area_individual_above_4MiB", "coin-pile source exceeds audit bounds")
        clean()
        for uid in range(100, 108):
            seed(uid, exporter.MAX_ITEM_PAYLOAD_BYTES - original_bytes if uid == 100 else
                 exporter.MAX_ITEM_PAYLOAD_BYTES, [uid], 402013)
        _, selected = observed("area_aggregate_exact_32MiB")
        assert selected == 2 * exporter.MAX_INPUT_BYTES
        with setup.cursor() as cursor:
            payload = bounded_coin_payload(100, exporter.MAX_ITEM_PAYLOAD_BYTES - original_bytes + 1, 402013)
            cursor.execute("UPDATE item_current_owner SET coin_payload=%s WHERE item_uid=100", (payload,))
        observed("area_aggregate_above_32MiB", "coin-pile source exceeds audit bounds")
        clean()
        seed(100, exporter.MAX_ITEM_PAYLOAD_BYTES, list(range(100, 109)), 402013)
        observed("area_repeated_mapping_join_above_32MiB", "mapped coin-pile source exceeds audit bounds")
        clean()

        with setup.cursor() as cursor:
            cursor.execute("UPDATE item_current_owner SET coin_payload=REPEAT(X'78',%s) WHERE item_uid=81",
                           (exporter.MAX_ITEM_PAYLOAD_BYTES,))
        unchanged, selected = observed("noncoin_payload_excluded")
        assert unchanged == snapshot and selected == 2 * original_bytes
        with setup.cursor() as cursor:
            cursor.execute("INSERT INTO economic_account_mapping VALUES (100,3,3,0,81,%s,1,NULL,81,%s)",
                           (LINEAGE, INSTALL))
        dangling, selected = observed("noncoin_pile_mapping_payload_excluded")
        assert selected == 2 * original_bytes
        assert dangling['native_mapping_coverage']['dangling_pile_mappings'] == 1
    finally:
        clean()


def item_origin(uid, owner_type, state, owner_id, revision):
    return struct.pack("<QBB6x5Q32s", uid, owner_type, state, owner_id, 0,
                       uid, 0, revision, bytes.fromhex("a5" * 32))


def verify_compound_item_actions(owner, reader, snapshot):
    """Classify existing supply endpoints without promoting modeled evidence."""
    from _plan5_equipment_restore import Connection, inventory

    output = ROOT / "bin/tests/plan5-compound-item-actions" / uuid.uuid4().hex
    output.mkdir(parents=True)
    initial = inventory(owner)
    original_counts = Reconciler().audit(snapshot)["exception_counts"]
    retirement_root = bytes.fromhex("ad" * 16)
    retirement_source = bytes.fromhex(source_identity(kind=18, identity="c7"))
    with owner.cursor() as cursor:
        cursor.execute("SELECT * FROM item_ownership_ledger WHERE operation_id=%s AND event_index=0", (creation_root,))
        original = cursor.fetchone()
        for table in ("economic_accounting_operation", "critical_operation_inbox", "item_ownership_ledger"):
            cursor.execute("SELECT COUNT(*) AS n FROM " + table + " WHERE operation_id=%s", (retirement_root,))
            assert cursor.fetchone()["n"] == 0
        for table in ("economic_accounting_operation", "economic_accounting_source_claim"):
            cursor.execute("SELECT COUNT(*) AS n FROM " + table + " WHERE lineage=%s AND source_event=%s",
                           (LINEAGE, retirement_source))
            assert cursor.fetchone()["n"] == 0
    records = []
    try:
        for phase, reason in (("craft-creation", 34), ("craft-retirement", 34),
                              ("quest-retirement", 33), ("collector-retirement", 21)):
            with owner.cursor() as cursor:
                if phase == "craft-creation":
                    cursor.execute("UPDATE item_ownership_ledger SET reason_type=%s,from_owner_type=7,"
                                   "from_owner_id=0,from_owner_context_id=0 WHERE operation_id=%s AND event_index=0",
                                   (reason, creation_root))
                else:
                    cursor.execute("UPDATE item_ownership_ledger SET reason_type=%s,from_owner_type=%s,"
                                   "from_owner_id=%s,from_owner_context_id=%s WHERE operation_id=%s AND event_index=0",
                                   (original["reason_type"], original["from_owner_type"], original["from_owner_id"],
                                    original["from_owner_context_id"], creation_root))
                    if phase == "craft-retirement":
                        cursor.execute(ROOT_INSERT + "(%s,%s,%s,NULL,34,1,0,%s,0,0,0,1,NULL)",
                                       (retirement_root, LINEAGE, EPOCH, retirement_source))
                        cursor.execute("INSERT INTO critical_operation_inbox (operation_id,status,result_code) "
                                       "VALUES (%s,1,0)", (retirement_root,))
                        cursor.execute("INSERT INTO economic_accounting_source_claim VALUES (%s,%s,%s)",
                                       (LINEAGE, retirement_source, retirement_root))
                        cursor.execute(REFERENCE_INSERT + "(%s,0,0,84,1,2,%s,0)", (retirement_root, retirement_root))
                        cursor.execute(LEDGER_INSERT + "(%s,0,84,84,NULL,8,0,0,2,1,%s)", (retirement_root, reason))
                    cursor.execute("UPDATE item_ownership_ledger SET from_owner_type=1,from_owner_id=7,"
                                   "from_owner_context_id=0,reason_type=%s WHERE operation_id=%s AND event_index=0",
                                   (reason, retirement_root))
                    cursor.execute("UPDATE item_current_owner SET owner_type=8,owner_id=0,item_revision=2,state=2 WHERE item_uid=84")
            before = inventory(owner)
            connection = Connection(reader)
            captured = capture(connection, LINEAGE, EPOCH)
            assert connection.rollbacks == connection.observer.closes == 1 and inventory(owner) == before
            report = Reconciler().audit(captured)
            expected_counts = dict(original_counts)
            if phase != "craft-creation":
                expected_counts["missing_original_plan"] += 1
                expected_counts["player_uid_missing_physical"] = 1
            assert report["exception_counts"] == expected_counts, report
            assert captured["complete"] is False and captured["backend"] == "sql_partial"
            origin = next(row for row in captured["item_origins"] if row["uid"] == 84)
            assert origin["origin"] == "creation" and origin["revision"] == 0 and origin["state"] == "absent"
            expected = ["create"] if phase == "craft-creation" else ["create", "destroy"]
            from reconcile_economy_accounting import view
            assert [row["action"] for row in view(captured, report, "provenance", 100, uid=84)["rows"]] == expected
            target = output / phase
            target.mkdir()
            encoded = json.dumps(captured, sort_keys=True).encode()
            path = target / "snapshot.json"
            path.write_bytes(encoded)
            commands = []
            for limit in (0, 1, 100):
                command = [sys.executable, str(ROOT / "scripts/reconcile_economy_accounting.py"), str(path),
                           "--view", "provenance", "--uid", "84", "--limit", str(limit)]
                result = subprocess.run(command, capture_output=True, timeout=30)
                assert result.returncode == 1 and not result.stderr, result.stderr
                value = json.loads(result.stdout)
                assert value == view(captured, Reconciler(limit).audit(captured), "provenance", limit, uid=84)
                assert [row["action"] for row in value["rows"]] == expected[:limit] and value["count"] == len(expected)
                assert value["coverage"]["exception_count"] == sum(expected_counts.values())
                assert path.read_bytes() == encoded
                (target / ("limit-" + str(limit) + ".json")).write_bytes(result.stdout)
                commands.append(dict(command=command, exit=result.returncode))
            for name in ("before", "after"):
                (target / ("authority-" + name + ".json")).write_text(json.dumps(before, sort_keys=True) + "\n")
            (target / "queries.json").write_text(json.dumps(connection.observer.queries) + "\n")
            records.append(dict(phase=phase, reason=reason, actions=expected, exception_counts=expected_counts,
                commands=commands, query_count=len(connection.observer.queries), application_tables_unchanged=len(before),
                rollback_calls=1, cursor_close_calls=1, original_partial_findings_preserved=True,
                additional_modeled_findings={} if phase == "craft-creation" else {"missing_original_plan": 1}))
    finally:
        with owner.cursor() as cursor:
            for table in ("economic_accounting_item_reference", "item_ownership_ledger",
                          "economic_accounting_source_claim", "economic_accounting_operation", "critical_operation_inbox"):
                cursor.execute("DELETE FROM " + table + " WHERE operation_id=%s", (retirement_root,))
            cursor.execute("UPDATE item_current_owner SET owner_type=1,owner_id=7,item_revision=1,state=1 WHERE item_uid=84")
            cursor.execute("UPDATE item_ownership_ledger SET reason_type=%s,from_owner_type=%s,from_owner_id=%s,"
                           "from_owner_context_id=%s WHERE operation_id=%s AND event_index=0",
                           (original["reason_type"], original["from_owner_type"], original["from_owner_id"],
                            original["from_owner_context_id"], creation_root))
    assert inventory(owner) == initial and capture(reader, LINEAGE, EPOCH) == snapshot
    result = dict(probes=records, modeled_partial_sql=True, source_fixture_restored=True, output=str(output),
                  accounting_activated=False, release_complete=False, native_compound_gameplay=False)
    (output / "results.json").write_text(json.dumps(result, indent=2) + "\n")
    print("COMPOUND_ITEM_ACTION_SQL_QUALIFIED " + json.dumps(result, sort_keys=True), flush=True)


def verify_collector_quarantine_views(owner, reader, snapshot):
    """Retain collector quarantine without guessing other system custody."""
    from _plan5_equipment_restore import Connection, inventory

    output = ROOT / "bin/tests/plan5-collector-quarantine" / uuid.uuid4().hex
    output.mkdir(parents=True)
    initial = inventory(owner)
    expected_counts = dict(Reconciler().audit(snapshot)["exception_counts"])
    expected_counts["missing_original_plan"] += 1
    expected_counts["player_uid_missing_physical"] = 1
    quarantine_root = bytes.fromhex("c8" * 16)
    with owner.cursor() as cursor:
        cursor.execute("SELECT * FROM item_ownership_ledger WHERE operation_id=%s AND event_index=0", (creation_root,))
        original_ledger = cursor.fetchone()
        cursor.execute("SELECT * FROM item_current_owner WHERE item_uid=84")
        original_native = cursor.fetchone()
        for table in ("economic_accounting_operation", "critical_operation_inbox", "item_ownership_ledger"):
            cursor.execute("SELECT COUNT(*) AS n FROM " + table + " WHERE operation_id=%s", (quarantine_root,))
            assert cursor.fetchone()["n"] == 0
    records = []
    try:
        with owner.cursor() as cursor:
            cursor.execute("UPDATE item_ownership_ledger SET to_owner_type=10,to_owner_id=7,to_owner_context_id=0 "
                           "WHERE operation_id=%s AND event_index=0", (creation_root,))
            cursor.execute(ROOT_INSERT + "(%s,%s,%s,NULL,32,1,0,NULL,0,0,0,1,NULL)",
                           (quarantine_root, LINEAGE, EPOCH))
            cursor.execute("INSERT INTO critical_operation_inbox (operation_id,status,result_code) VALUES (%s,1,0)",
                           (quarantine_root,))
            cursor.execute(REFERENCE_INSERT + "(%s,0,0,84,1,2,%s,0)", (quarantine_root, quarantine_root))
            cursor.execute(LEDGER_INSERT + "(%s,0,84,84,NULL,7,0,0,2,1,21)", (quarantine_root,))
            cursor.execute("UPDATE item_ownership_ledger SET from_owner_type=10,from_owner_id=7,from_owner_context_id=0 "
                           "WHERE operation_id=%s AND event_index=0", (quarantine_root,))
        for phase, reason, state, expected_state in (("collector-quarantine", 21, 3, "quarantined"),
                                                     ("other-system-custody", 8, 1, "live")):
            with owner.cursor() as cursor:
                cursor.execute("UPDATE item_ownership_ledger SET reason_type=%s WHERE operation_id=%s", (reason, quarantine_root))
                cursor.execute("UPDATE item_current_owner SET owner_type=7,owner_id=0,owner_context_id=0,"
                               "item_revision=2,state=%s WHERE item_uid=84", (state,))
            before = inventory(owner)
            connection = Connection(reader)
            captured = capture(connection, LINEAGE, EPOCH)
            after = inventory(owner)
            assert after == before and connection.rollbacks == connection.observer.closes == 1
            report = Reconciler().audit(captured)
            assert report["exception_counts"] == expected_counts, report
            assert captured["complete"] is False and captured["backend"] == "sql_partial"
            selected = [row for row in captured["ownership_events"] if row["operation_id"] == quarantine_root.hex()]
            references = [row for row in captured["native"]["lineage_uid_references"]
                          if row["operation_id"] == quarantine_root.hex()]
            assert len(selected) == len(references) == 1
            assert selected[0]["state"] == references[0]["ledger_state"] == expected_state
            assert selected[0]["action"] == references[0]["ledger_action"] == "move"
            target = output / phase
            target.mkdir()
            encoded = json.dumps(captured, sort_keys=True).encode()
            path = target / "snapshot.json"
            path.write_bytes(encoded)
            commands = []
            for limit in (0, 1, 100):
                command = [sys.executable, str(ROOT / "scripts/reconcile_economy_accounting.py"), str(path),
                           "--view", "provenance", "--uid", "84", "--limit", str(limit)]
                result = subprocess.run(command, capture_output=True, timeout=30)
                assert result.returncode == 1 and not result.stderr, result.stderr
                value = json.loads(result.stdout)
                assert value == view(captured, Reconciler(limit).audit(captured), "provenance", limit, uid=84)
                assert value["count"] == 2 and [row["state"] for row in value["rows"]] == ["live", expected_state][:limit]
                assert value["coverage"]["exception_count"] == sum(expected_counts.values()) and path.read_bytes() == encoded
                (target / ("limit-" + str(limit) + ".json")).write_bytes(result.stdout)
                commands.append(dict(command=command, exit=result.returncode))
            for name, values in (("before", before), ("after", after)):
                (target / ("authority-" + name + ".json")).write_text(json.dumps(values, sort_keys=True) + "\n")
            (target / "queries.json").write_text(json.dumps(connection.observer.queries) + "\n")
            records.append(dict(phase=phase, reason=reason, state=expected_state, actions=["create", "move"],
                exception_counts=expected_counts, commands=commands, query_count=len(connection.observer.queries),
                application_tables_unchanged=len(before), rollback_calls=1, cursor_close_calls=1))
    finally:
        with owner.cursor() as cursor:
            for table in ("economic_accounting_item_reference", "item_ownership_ledger",
                          "economic_accounting_operation", "critical_operation_inbox"):
                cursor.execute("DELETE FROM " + table + " WHERE operation_id=%s", (quarantine_root,))
            cursor.execute("UPDATE item_current_owner SET owner_type=%s,owner_id=%s,owner_context_id=%s,item_revision=%s,"
                           "state=%s WHERE item_uid=84", tuple(original_native[field] for field in
                           ("owner_type", "owner_id", "owner_context_id", "item_revision", "state")))
            cursor.execute("UPDATE item_ownership_ledger SET to_owner_type=%s,to_owner_id=%s,to_owner_context_id=%s "
                           "WHERE operation_id=%s AND event_index=0", (*[original_ledger[field] for field in
                           ("to_owner_type", "to_owner_id", "to_owner_context_id")], creation_root))
    assert inventory(owner) == initial and capture(reader, LINEAGE, EPOCH) == snapshot
    result = dict(probes=records, output=str(output), modeled_partial_sql=True, source_fixture_restored=True,
                  accounting_activated=False, release_complete=False, native_collector_gameplay=False)
    (output / "results.json").write_text(json.dumps(result, indent=2) + "\n")
    print("COLLECTOR_QUARANTINE_SQL_QUALIFIED " + json.dumps(result, sort_keys=True), flush=True)


def verify_quarantined_coin_views(owner, reader, snapshot, expected_exceptions):
    """Keep quarantined money visible without admitting an active holding."""
    from _plan5_equipment_restore import Connection, inventory

    output = ROOT / "bin/tests/plan5-quarantined-coin-views" / uuid.uuid4().hex
    output.mkdir(parents=True)
    initial = inventory(owner)
    records = []
    try:
        for phase, payload in (("payload", coin_payload(82, [1, 2, 3, 4])), ("missing-payload", None)):
            with owner.cursor() as cursor:
                cursor.execute("UPDATE item_current_owner SET state=3,coin_payload=%s WHERE item_uid=82", (payload,))
            before = inventory(owner)
            connection = Connection(reader)
            captured = capture(connection, LINEAGE, EPOCH)
            counts = {**expected_exceptions, "quarantined_coin_pile": 1,
                      "dangling_pile_mapping": 1, "dangling_coin_pile_mapping": 1,
                      "missing_native_holding": 1, "stale_native_item": 1}
            report = Reconciler().audit(captured)
            assert report["exception_counts"] == counts, report
            assert captured["complete"] is False and captured["backend"] == "sql_partial"
            pile = next(row for row in captured["native"]["coin_piles"] if row["uid"] == 82)
            assert pile["state"] == "quarantined" and pile["amounts"] == ([1, 2, 3, 4] if payload else None)
            assert captured["native_mapping_coverage"]["pile_rows"] == 0
            assert captured["native"]["coin_pile_coverage"]["mapped_live_rows"] == 0
            assert captured["native"]["coin_pile_coverage"]["unmapped_live_rows"] == 0
            assert all(row["account_key"] != key(3, 11).hex() for row in captured["native"]["holdings"])
            assert connection.rollbacks == connection.observer.closes == 1
            assert inventory(owner) == before
            target = output / phase
            target.mkdir()
            encoded = json.dumps(captured, sort_keys=True).encode()
            path = target / "snapshot.json"
            path.write_bytes(encoded)
            commands = []
            for limit in (0, 1, 100):
                command = [sys.executable, str(ROOT / "scripts/reconcile_economy_accounting.py"),
                           str(path), "--limit", str(limit)]
                result = subprocess.run(command, capture_output=True, timeout=30)
                assert result.returncode == 1 and not result.stderr, result.stderr
                value = json.loads(result.stdout)
                assert value["exception_counts"] == counts and value["exception_count"] == sum(counts.values())
                assert len(value["exceptions"]) <= limit and path.read_bytes() == encoded
                (target / ("limit-" + str(limit) + ".json")).write_bytes(result.stdout)
                commands.append(dict(command=command, exit=result.returncode))
            for name in ("before", "after"):
                (target / ("authority-" + name + ".json")).write_text(json.dumps(before, sort_keys=True) + "\n")
            (target / "queries.json").write_text(json.dumps(connection.observer.queries) + "\n")
            records.append(dict(phase=phase, exception_counts=counts, commands=commands,
                application_tables_unchanged=len(before), query_count=len(connection.observer.queries),
                rollback_calls=1, cursor_close_calls=1, active_coin_holdings=0, active_coin_piles=0))
        with reader.cursor() as cursor:
            try:
                cursor.execute("UPDATE item_current_owner SET state=state WHERE item_uid=82")
            except pymysql.MySQLError as error:
                assert error.args[0] == 1142, error.args
            else:
                raise AssertionError("quarantined coin reader admitted UPDATE")
        assert inventory(owner) == before
    finally:
        with owner.cursor() as cursor:
            cursor.execute("UPDATE item_current_owner SET state=1,coin_payload=%s WHERE item_uid=82",
                           (coin_payload(82, [1, 2, 3, 4]),))
    assert inventory(owner) == initial and capture(reader, LINEAGE, EPOCH) == snapshot
    result = dict(probes=records, modeled_partial_sql=True, source_fixture_restored=True,
                  permission_denial=1142, output=str(output), accounting_activated=False, release_complete=False)
    (output / "results.json").write_text(json.dumps(result, indent=2) + "\n")
    print("QUARANTINED_COIN_SQL_QUALIFIED " + json.dumps(result, sort_keys=True), flush=True)


def verify_supply_outcome_views(owner, reader):
    """Retain SQL corruption findings while excluding noncommitted supply."""
    from _plan5_equipment_restore import Connection, inventory

    output = ROOT / "bin/tests/plan5-supply-outcome-views" / uuid.uuid4().hex
    output.mkdir(parents=True)
    initial = inventory(owner)
    with owner.cursor() as cursor:
        cursor.execute("SELECT account_key FROM economic_accounting_account_effect "
                       "WHERE operation_id=%s AND account_index=1", (root,))
        original_key = cursor.fetchone()["account_key"]
        cursor.execute("SELECT outcome,result_code FROM economic_accounting_operation "
                       "WHERE operation_id=%s", (root,))
        original_root = cursor.fetchone()
        cursor.execute("SELECT result_code FROM critical_operation_inbox WHERE operation_id=%s", (root,))
        original_receipt = cursor.fetchone()["result_code"]
    records = []
    try:
        # Explicitly damage only the existing disposable modeled fixture. This
        # is not an admitted native system root or a correction to live data.
        with owner.cursor() as cursor:
            cursor.execute("UPDATE economic_accounting_account_effect SET account_key=%s "
                           "WHERE operation_id=%s AND account_index=1", (key(8, 9), root))
        for phase, outcome, result_code in (("committed", 1, 0), ("rejected", 2, 9), ("unknown", 3, 9)):
            with owner.cursor() as cursor:
                cursor.execute("UPDATE economic_accounting_operation SET outcome=%s,result_code=%s "
                               "WHERE operation_id=%s", (outcome, result_code, root))
                cursor.execute("UPDATE critical_operation_inbox SET result_code=%s WHERE operation_id=%s",
                               (result_code, root))
            before = inventory(owner)
            connection = Connection(reader)
            snapshot = capture(connection, LINEAGE, EPOCH)
            assert connection.rollbacks == connection.observer.closes == 1
            assert inventory(owner) == before
            probes = [(phase, snapshot)]
            if phase == "committed":
                for reversed_rows in (False, True):
                    altered = copy.deepcopy(snapshot)
                    selected = next(row for row in altered["operations"] if row["operation_id"] == root.hex())
                    altered["operations"].append(dict(selected, outcome="rejected", result_code=9))
                    if reversed_rows:
                        altered["operations"].reverse()
                    probes.append(("conflicting-reversed" if reversed_rows else "conflicting", altered))
                # SQL primary keys cannot contain duplicate identities. Damage
                # only the saved read-only projection, retaining its SQL source.
                for damage in ("same-effect", "conflicting-effect", "same-posting", "conflicting-posting"):
                    for reversed_rows in (False, True):
                        altered = copy.deepcopy(snapshot)
                        table = "effects" if "effect" in damage else "postings"
                        selected = next(row for row in altered[table]
                                        if row["operation_id"] == root.hex() and row["account_index"] == 1)
                        duplicate = copy.deepcopy(selected)
                        if damage == "conflicting-effect":
                            duplicate["account_key"] = key(7, 9).hex()
                        elif damage == "conflicting-posting":
                            duplicate.update(delta=[6, 0, 0, 0], copper_value=6)
                        altered[table].append(duplicate)
                        if reversed_rows:
                            altered[table].reverse()
                        probes.append((damage + ("-reversed" if reversed_rows else ""), altered))
            for label, probe in probes:
                target = output / label
                target.mkdir()
                original = json.dumps(probe, sort_keys=True).encode()
                path = target / "snapshot.json"
                path.write_bytes(original)
                path.chmod(0o600)
                report = Reconciler().audit(probe)
                assert report["exception_count"] and probe["complete"] is False
                if label == "rejected":
                    assert "rejected_operation_has_effects" in report["exception_counts"]
                if label == "unknown":
                    assert "unknown_outcome" in report["exception_counts"]
                if label in ("conflicting", "conflicting-reversed"):
                    assert "duplicate_operation" in report["exception_counts"]
                if label.startswith(("same-effect", "conflicting-effect")):
                    assert "duplicate_effect" in report["exception_counts"]
                if label.startswith(("same-posting", "conflicting-posting")):
                    assert "duplicate_posting" in report["exception_counts"]
                expected = [{"account_kind": 8, "reason": 3, "net_copper": 3}] if label == "committed" else []
                commands = []
                for limit in (0, 1, 100):
                    bounded_report = Reconciler(limit).audit(probe)
                    assert bounded_report["exception_counts"] == report["exception_counts"]
                    expected_output = view(probe, bounded_report, "supply", limit)
                    assert expected_output["rows"] == expected[:limit]
                    assert expected_output["count"] == len(expected)
                    assert expected_output["truncated"] == (len(expected) > limit)
                    assert expected_output["coverage"]["exception_count"] == report["exception_count"]
                    command = [sys.executable, str(ROOT / "scripts/reconcile_economy_accounting.py"),
                               str(path), "--view", "supply", "--limit", str(limit)]
                    result = subprocess.run(command, capture_output=True, timeout=30)
                    assert result.returncode == 1 and not result.stderr, result.stderr
                    assert json.loads(result.stdout) == expected_output
                    assert path.read_bytes() == original
                    assert json.dumps(probe, sort_keys=True).encode() == original
                    (target / ("limit-" + str(limit) + ".json")).write_bytes(result.stdout)
                    commands.append(dict(command=command, exit=1))
                (target / "report.json").write_text(json.dumps(report, sort_keys=True) + "\n")
                records.append(dict(phase=label, exception_counts=report["exception_counts"],
                                    supply_rows=expected, commands=commands, complete_capture=False))
            assert inventory(owner) == before
            (output / (phase + "-authority.json")).write_text(json.dumps(before, sort_keys=True) + "\n")
            (output / (phase + "-queries.json")).write_text(json.dumps(connection.observer.queries) + "\n")
    finally:
        with owner.cursor() as cursor:
            cursor.execute("UPDATE economic_accounting_account_effect SET account_key=%s "
                           "WHERE operation_id=%s AND account_index=1", (original_key, root))
            cursor.execute("UPDATE economic_accounting_operation SET outcome=%s,result_code=%s "
                           "WHERE operation_id=%s", (original_root["outcome"], original_root["result_code"], root))
            cursor.execute("UPDATE critical_operation_inbox SET result_code=%s WHERE operation_id=%s",
                           (original_receipt, root))
    assert inventory(owner) == initial
    (output / "authority-initial-final.json").write_text(json.dumps(initial, sort_keys=True) + "\n")
    result = dict(output=str(output), probes=records, modeled_partial_sql=True,
                  native_system_root_admission=False, application_tables_unchanged=len(initial),
                  capture_transactions=3, rollback_calls=3, cursor_close_calls=3,
                  source_fixture_restored=True, accounting_activated=False, release_complete=False)
    (output / "results.json").write_text(json.dumps(result, indent=2) + "\n")
    print("SUPPLY_OUTCOME_SQL_QUALIFIED " + json.dumps(result, sort_keys=True), flush=True)

root = bytes.fromhex("aa" * 16)
source = bytes.fromhex(source_identity(identity="bb"))
prior_root = bytes.fromhex("dd" * 16)
prior_epoch = bytes.fromhex("ee" * 16)
prior_source = bytes.fromhex(source_identity(identity="ff"))
consumed_source_root = bytes.fromhex("e1" * 16)
consumer_root = bytes.fromhex("e2" * 16)
consumed_source_event = bytes.fromhex(source_identity(identity="ab"))
consumer_source_event = bytes.fromhex(source_identity(kind=13, identity="ac"))
creation_root = bytes.fromhex("e3" * 16)
creation_source_event = bytes.fromhex(source_identity(kind=18, identity="ad"))
new_wallet_root = bytes.fromhex("e4" * 16)
other_mapping_lineage = bytes.fromhex("44" * 16)
prior_item_root = bytes.fromhex("e5" * 16)
prior_item_source = bytes.fromhex(source_identity(kind=18, identity="b0"))
prior_unlinked_operation = bytes.fromhex("b1" * 16)
admin = pymysql.connect(**settings)
try:
    with admin.cursor() as cursor:
        cursor.execute(f"CREATE DATABASE `{schema}`")
        cursor.execute(f"CREATE USER '{reader}'@'{reader_host}' IDENTIFIED BY 'disposable-audit-only'")
        cursor.execute(f"GRANT SELECT ON `{schema}`.* TO '{reader}'@'{reader_host}'")
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
            cursor.execute("INSERT INTO economic_baseline_witness(operation_id,lineage,epoch,book_revision,"
                           "holding_count,item_count,witness_digest,canonical_witness) VALUES (%s,%s,%s,1,6,3,%s,%s)",
                           (OP, LINEAGE, EPOCH, hashlib.sha256(blob).digest(), blob))
            baseline = baseline_root(blob)
            cursor.execute(ROOT_INSERT + "(%s,%s,%s,NULL,38,1,0,%s,%s,%s,0,0,NULL)",
                           (OP, LINEAGE, EPOCH, baseline["source_event"], baseline["account_count"], baseline["posting_count"]))
            bind_synthetic_baseline(cursor, blob)
            cursor.execute("INSERT INTO economic_accounting_source_claim VALUES (%s,%s,%s)",
                           (LINEAGE, struct.pack("<HH", 10, 1) + blob[48:64] + EPOCH + blob[72:80] + bytes(4), OP))
            cursor.execute("INSERT INTO economic_sql_lifecycle_installation("
                           "operation_id,lineage,epoch,baseline_operation_id,phase,selected_epoch,revision) VALUES "
                           "(%s,%s,%s,%s,2,%s,1)",
                           (INSTALL, LINEAGE, EPOCH, OP, EPOCH))
            cursor.execute(ROOT_INSERT +
                           "(%s,%s,%s,NULL,3,1,0,%s,2,2,0,0,NULL)",
                           (root, LINEAGE, EPOCH, source))
            cursor.execute(ROOT_INSERT +
                           "(%s,%s,%s,NULL,1,1,0,NULL,2,2,0,0,NULL)",
                           (new_wallet_root, LINEAGE, EPOCH))
            cursor.execute("INSERT INTO critical_operation_inbox "
                           "(operation_id,status,result_code) VALUES "
                           "(%s,1,0),(%s,1,0),(%s,1,0)",
                           (OP, root, INSTALL))
            cursor.execute("UPDATE critical_operation_inbox SET keys_hash=%s,command_hash=%s WHERE operation_id=%s",
                           (baseline["inbox_keys_hash"], baseline["inbox_command_hash"], OP))
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
                cursor.execute(POSTING_INSERT +
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
                cursor.execute(POSTING_INSERT +
                               "(%s,%s,%s,0,%s,0,0,0,%s)",
                               (new_wallet_root, index, index, delta, delta))
            cursor.execute("INSERT INTO economic_accounting_source_claim VALUES (%s,%s,%s)",
                           (LINEAGE, source, root))
            cursor.execute(ROOT_INSERT +
                           "(%s,%s,%s,NULL,3,1,0,%s,2,2,0,0,NULL)",
                           (prior_root, LINEAGE, prior_epoch, prior_source))
            cursor.execute(ROOT_INSERT +
                           "(%s,%s,%s,NULL,3,1,0,%s,2,2,0,0,NULL),"
                           "(%s,%s,%s,NULL,31,1,0,%s,2,2,0,0,NULL)",
                           (consumed_source_root, LINEAGE, prior_epoch, consumed_source_event,
                            consumer_root, LINEAGE, prior_epoch, consumer_source_event))
            cursor.execute(ROOT_INSERT +
                           "(%s,%s,%s,NULL,33,1,0,%s,0,0,0,1,NULL)",
                           (creation_root, LINEAGE, EPOCH, creation_source_event))
            for index, account, before, after in (
                    (0, key(5, 13), 0, 250), (1, key(8, 9), 0, -250)):
                cursor.execute("INSERT INTO economic_accounting_account_effect VALUES "
                               "(%s,%s,%s,%s,0,0,0,%s,0,0,0,0,1)",
                               (prior_root, index, account, before, after))
            for index, delta in ((0, 250), (1, -250)):
                cursor.execute(POSTING_INSERT +
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
                    cursor.execute(POSTING_INSERT +
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
            cursor.execute(REFERENCE_INSERT +
                           "(%s,0,0,84,0,1,%s,0)", (creation_root, creation_root))
            cursor.execute(LEDGER_INSERT +
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
            cursor.execute("INSERT INTO auctions(id,status,cur_price,auction_revision,winning_bidder_pid,obj_blob_str) "
                           "VALUES (5,'REMOVED',123,1,7,X'78')")
            cursor.execute("INSERT INTO auction_money_pickups VALUES (7,250,1)")
            cursor.execute("INSERT INTO shopkeepers(id,cash,shop_revision) VALUES (3,500,1)")
            cursor.execute(ITEM_INSERT +
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
            verify_area_coin_views(setup, audit, snapshot)
            verify_compound_item_actions(setup, audit, snapshot)
            verify_collector_quarantine_views(setup, audit, snapshot)
            verify_coin_payload_source_bounds(setup, audit, snapshot)
            verify_coin_payload_row_budget(setup, audit, snapshot)
            verify_supply_outcome_views(setup, audit)
            assert capture(audit, LINEAGE, EPOCH) == snapshot
            with setup.cursor() as cursor:
                cursor.execute("INSERT INTO guilds VALUES "
                               "(31,1,2,3,4,9),(32,0,0,0,0,10),"
                               "(33,4294967295,4294967295,4294967295,4294967295,11)")
            guild_snapshot = capture(audit, LINEAGE, EPOCH)
            assert guild_snapshot["native"]["guild_treasuries"] == [
                {"guild_id":31,"balance":[1,2,3,4]},
                {"guild_id":32,"balance":[0,0,0,0]},
                {"guild_id":33,"balance":[4294967295]*4}]
            guild_counts = Reconciler().audit(guild_snapshot)["exception_counts"]
            assert guild_counts["unsupported_native_guild_treasury"] == 3
            assert guild_counts["missing_guild_money_revision"] == 3
            guild_details = Reconciler().audit(guild_snapshot)["exceptions"]
            assert {row["guild_id"] for row in guild_details if row["code"] in
                    ("unsupported_native_guild_treasury", "missing_guild_money_revision")} == {31,32,33}
            with setup.cursor() as cursor:
                cursor.execute("UPDATE guilds SET copper=7 WHERE id=31")
                cursor.execute("SELECT outcome_revision FROM guilds WHERE id=31")
                assert cursor.fetchone()["outcome_revision"] == 9
            changed_guild = capture(audit, LINEAGE, EPOCH)
            assert changed_guild["native"]["guild_treasuries"][0]["balance"] == [7,2,3,4]
            assert changed_guild["native"]["holdings"] == snapshot["native"]["holdings"]
            assert changed_guild["complete"] is False
            with setup.cursor() as cursor:
                cursor.execute("DELETE FROM guilds")
            assert capture(audit, LINEAGE, EPOCH) == snapshot
            print("guild money changes independently of outcome_revision: raw native values and missing monetary revision reported",flush=True)
            for alteration, restoration in (
                    ("RENAME TABLE guilds TO guilds_hidden", "RENAME TABLE guilds_hidden TO guilds"),
                    ("ALTER TABLE guilds ENGINE=MyISAM", "ALTER TABLE guilds ENGINE=InnoDB")):
                with setup.cursor() as cursor:
                    cursor.execute(alteration)
                try:
                    capture(audit, LINEAGE, EPOCH)
                    raise AssertionError("missing/nontransactional guild source passed capture")
                except exporter.ExportError:
                    pass
                finally:
                    with setup.cursor() as cursor:
                        cursor.execute(restoration)
                assert capture(audit, LINEAGE, EPOCH) == snapshot
            # Persisted coffers are real native value even without a supported
            # accounting lifetime/revision. Capture IDs/value only, with no
            # fabricated mapped holding or owner alias.
            with setup.cursor() as cursor:
                cursor.execute("INSERT INTO ships VALUES (25,7),(26,0),(27,NULL),(28,-1)")
            ship_snapshot = capture(audit, LINEAGE, EPOCH)
            assert ship_snapshot["native"]["ship_coffers"] == [
                {"ship_id":25,"copper":7}, {"ship_id":26,"copper":0},
                {"ship_id":27,"copper":None}, {"ship_id":28,"copper":-1}]
            ship_counts = Reconciler().audit(ship_snapshot)["exception_counts"]
            assert ship_counts["unsupported_native_ship_coffer"] == 4
            assert ship_counts["missing_ship_coffer_revision"] == 4
            assert ship_counts["unknown_native_ship_coffer"] == 1
            assert ship_counts["invalid_native_ship_coffer"] == 1
            with setup.cursor() as cursor:
                cursor.execute("UPDATE ships SET money=11 WHERE id=25")
            changed_ship = capture(audit, LINEAGE, EPOCH)
            assert changed_ship["native"]["ship_coffers"][0]["copper"] == 11
            assert changed_ship["native"]["holdings"] == snapshot["native"]["holdings"]
            assert changed_ship["complete"] is False
            with setup.cursor() as cursor:
                cursor.execute("DELETE FROM ships")
            assert capture(audit, LINEAGE, EPOCH) == snapshot
            for alteration, restoration in (
                    ("RENAME TABLE ships TO ships_hidden", "RENAME TABLE ships_hidden TO ships"),
                    ("ALTER TABLE ships ENGINE=MyISAM", "ALTER TABLE ships ENGINE=InnoDB")):
                with setup.cursor() as cursor:
                    cursor.execute(alteration)
                try:
                    capture(audit, LINEAGE, EPOCH)
                    raise AssertionError("missing/nontransactional ship source passed capture")
                except exporter.ExportError:
                    pass
                finally:
                    with setup.cursor() as cursor:
                        cursor.execute(restoration)
                assert capture(audit, LINEAGE, EPOCH) == snapshot
            print("ship coffer value change: unsupported authority/revision reported; exact restore and mapped holdings retained",flush=True)

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
                writer.execute("INSERT INTO auctions(id,status,cur_price,auction_revision,winning_bidder_pid,obj_blob_str) "
                               "VALUES (6,'REMOVED',0,1,0,X'78')")
                writer.execute("INSERT INTO economic_account_mapping VALUES "
                               "(17,4,4,0,6,%s,1,NULL,6,NULL)", (LINEAGE,))
            empty_removed = capture(audit, LINEAGE, EPOCH)
            empty_removed_report = Reconciler().audit(empty_removed)
            assert "dangling_auction_escrow_mapping" in \
                empty_removed_report["exception_counts"]
            with setup.cursor() as writer:
                writer.execute("DELETE FROM economic_account_mapping WHERE mapping_id=17")
                writer.execute("DELETE FROM auctions WHERE id=6")
                writer.execute("INSERT INTO auctions(id,status,cur_price,auction_revision,winning_bidder_pid,obj_blob_str) "
                               "VALUES (7,'OPEN',25,1,0,X'78')")
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
                "source_operations": 6, "missing_claim_operations": 0,
                "duplicate_source_values": 0}
            assert snapshot["source_event_policy_coverage"] == {
                "required_committed_operations": 3,
                "missing_required_source_events": 0}
            assert len(snapshot["source_claims"]) == 6
            assert all(
                row["operation_inbox_receipt"] == {
                    "status": 1, "result_code": row["operation_result_code"],
                    "failure_stage": 0, "committed_at_present": True}
                for row in snapshot["source_claims"]
                if row["operation_outcome"] == "committed")
            assert next(row for row in snapshot["item_origins"] if row["uid"] == 84) == {
                "uid": 84, "origin": "creation", "revision": 0, "root": 84,
                "parent": None, "owner": [0, 0, 0], "state": "absent", "equipment_slot": 0}
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
            # All three EAB1 item origins omit equipment positions; the live
            # native columns cannot supply their authenticated opening slots.
            expected_exceptions = {"evidence_loss": 1,
                                   "auction_legacy_listing_identity_unknown": 1,
                                   "player_uid_missing_physical": 2,
                                   "missing_item_equipment_evidence": 3,
                                   "unmapped_native_wallet": 1,
                                   "unauthorized_mapping_creation": 1,
                                   "missing_original_plan": 3}
            # These legacy model roots retain no original capsules or source
            # custody facts. Do not reseal their projections into proof. Native
            # original-plan authentication is covered by the canonical SQL class.
            assert report["checked"]["original_plans_verified"] == 0
            assert report["exception_counts"] == expected_exceptions, report
            for table in ("player_items", "player_pets", "player_pet_items", "corpses", "corpse_items",
                          "lockers", "private_chests", "locker_items", "account_lockers", "locker_chests", "account_locker_items"):
                for alteration, restoration in (
                        (f"RENAME TABLE {table} TO {table}_hidden", f"RENAME TABLE {table}_hidden TO {table}"),
                        (f"ALTER TABLE {table} ENGINE=MyISAM", f"ALTER TABLE {table} ENGINE=InnoDB")):
                    with setup.cursor() as cursor:
                        cursor.execute(alteration)
                    try:
                        capture(audit, LINEAGE, EPOCH)
                        raise AssertionError("missing/nontransactional physical custody source passed capture")
                    except exporter.ExportError:
                        pass
                    finally:
                        with setup.cursor() as cursor:
                            cursor.execute(restoration)
                    assert capture(audit, LINEAGE, EPOCH) == snapshot
            verify_quarantined_coin_views(setup, audit, snapshot, expected_exceptions)
            # Matching root/claim values can still be invalid native S48
            # identities, including retained roots outside the selected epoch.
            def source_rows():
                with setup.cursor() as cursor:
                    rows = []
                    for statement in TABLES:
                        table = statement.split()[2]
                        cursor.execute("SELECT * FROM " + table)
                        rows.append(sorted((repr(row) for row in cursor.fetchall())))
                    return rows
            def source_fault_capture(additions):
                before = source_rows()
                connection = mock.Mock(wraps=audit)
                cursor = mock.Mock(wraps=audit.cursor())
                connection.cursor.return_value = cursor
                corrupt = capture(connection, LINEAGE, EPOCH)
                connection.rollback.assert_called_once_with()
                cursor.close.assert_called_once_with()
                assert all(call.args[0].upper().startswith(("SELECT", "SET TRANSACTION", "START TRANSACTION"))
                           for call in cursor.execute.call_args_list)
                assert source_rows() == before
                original = json.dumps(corrupt, sort_keys=True)
                fault = Reconciler().audit(corrupt)
                assert fault["exception_counts"] == {**expected_exceptions, **additions}, fault
                assert json.dumps(corrupt, sort_keys=True) == original
            captures = 0
            for source_root, valid_source, additions in (
                    (root, source, {"invalid_source_event": 1, "invalid_source_claim": 1}),
                    (creation_root, creation_source_event, {"invalid_source_event": 1,
                        "invalid_source_claim": 1, "invalid_lineage_uid_reference_root": 1}),
                    (prior_root, prior_source, {"invalid_source_claim": 1})):
                for invalid_source in malformed_sources():
                    with setup.cursor() as writer:
                        writer.execute("UPDATE economic_accounting_operation SET source_event=%s WHERE operation_id=%s",
                                       (invalid_source, source_root))
                        writer.execute("UPDATE economic_accounting_source_claim SET source_event=%s WHERE operation_id=%s",
                                       (invalid_source, source_root))
                    source_fault_capture(additions)
                    captures += 1
                with setup.cursor() as writer:
                    writer.execute("UPDATE economic_accounting_operation SET source_event=%s WHERE operation_id=%s",
                                   (valid_source, source_root))
                    writer.execute("UPDATE economic_accounting_source_claim SET source_event=%s WHERE operation_id=%s",
                                   (valid_source, source_root))
                assert capture(audit, LINEAGE, EPOCH) == snapshot
            assert captures == 27
            print("SQL source grammar: 27 corrupt cuts, selected money/UID and prior epoch; all tables unchanged, rollback/close passed", flush=True)
            policy_captures = 0
            for source_root, original_reason, tested_reason, allowed_kind, valid_source, additions in (
                    (root, 3, 3, 16, source, {"unauthorized_source_kind": 1, "unauthorized_source_claim": 1}),
                    (creation_root, 33, 43, 18, creation_source_event, {"unauthorized_source_kind": 1,
                        "unauthorized_source_claim": 1, "unauthorized_lineage_uid_source": 1}),
                    (prior_root, 3, 3, 16, prior_source, {"unauthorized_source_claim": 1})):
                with setup.cursor() as writer:
                    writer.execute("UPDATE economic_accounting_operation SET reason=%s WHERE operation_id=%s",
                                   (tested_reason, source_root))
                assert Reconciler().audit(capture(audit, LINEAGE, EPOCH))["exception_counts"] == expected_exceptions
                for kind in range(1, 24):
                    if kind == allowed_kind:
                        continue
                    invalid_source = bytes.fromhex(source_identity(kind=kind, identity="da"))
                    with setup.cursor() as writer:
                        writer.execute("UPDATE economic_accounting_operation SET source_event=%s WHERE operation_id=%s",
                                       (invalid_source, source_root))
                        writer.execute("UPDATE economic_accounting_source_claim SET source_event=%s WHERE operation_id=%s",
                                       (invalid_source, source_root))
                    source_fault_capture(additions)
                    policy_captures += 1
                with setup.cursor() as writer:
                    writer.execute("UPDATE economic_accounting_operation SET reason=%s,source_event=%s WHERE operation_id=%s",
                                   (original_reason, valid_source, source_root))
                    writer.execute("UPDATE economic_accounting_source_claim SET source_event=%s WHERE operation_id=%s",
                                   (valid_source, source_root))
                assert capture(audit, LINEAGE, EPOCH) == snapshot
            assert policy_captures == 66
            print("SQL source policy: 66 mismatched-kind cuts, selected money/UID and prior epoch; all tables unchanged, rollback/close passed", flush=True)
            for original_root in (root, creation_root):
                with setup.cursor() as writer:
                    writer.execute("UPDATE economic_accounting_operation SET original_operation_id=operation_id WHERE operation_id=%s",
                                   (original_root,))
                source_fault_capture({"invalid_original_operation": 1})
                with setup.cursor() as writer:
                    writer.execute("UPDATE economic_accounting_operation SET original_operation_id=NULL WHERE operation_id=%s",
                                   (original_root,))
                assert capture(audit, LINEAGE, EPOCH) == snapshot
            print("SQL original links: 2 self-linked selected money/UID cuts; all tables unchanged, rollback/close passed", flush=True)
            # Root-scoped joins used to hide these real native-SQL corruptions.
            # A SELECT-only audit must expose every family without modifying it.
            orphan = bytes.fromhex("01" * 16)
            with setup.cursor() as writer:
                writer.execute("INSERT INTO economic_accounting_account_effect VALUES "
                               "(%s,0,%s,0,0,0,0,0,0,0,0,0,1)", (orphan, key(1, 7)))
                writer.execute(POSTING_INSERT +
                               "(%s,0,0,0,1,0,0,0,1)", (orphan,))
                writer.execute(CHILD_INSERT + "(%s,0,%s,0)",
                               (orphan, bytes.fromhex("02" * 16)))
                writer.execute(REFERENCE_INSERT +
                               "(%s,0,0,999,0,1,%s,0)", (orphan, orphan))
                writer.execute("INSERT INTO economic_baseline_reservation VALUES (%s,%s,2,%s,%s)",
                               (bytes([77]) * 16, bytes([88]) * 16, 2**64 - 1, orphan))

            def orphan_readback():
                with setup.cursor() as reader_cursor:
                    result = {}
                    for table, _, _ in exporter.ORPHAN_EVIDENCE_SOURCES.values():
                        reader_cursor.execute(f"SELECT * FROM {table} WHERE operation_id=%s", (orphan,))
                        result[table] = reader_cursor.fetchall()
                    return result

            before_orphans = orphan_readback()
            orphan_cut = capture(audit, LINEAGE, EPOCH)
            orphan_counts = Reconciler(0).audit(orphan_cut)["exception_counts"]
            for _, _, code in exporter.ORPHAN_EVIDENCE_SOURCES.values():
                assert orphan_counts[code] == 1, orphan_counts
            assert orphan_cut["orphan_evidence_coverage"] == {
                "scope": "database", "table_counts": dict.fromkeys(exporter.ORPHAN_EVIDENCE_SOURCES, 1)}
            rootless_lookup = view(orphan_cut, {}, "operation", 100, operation_id=orphan.hex())
            assert rootless_lookup["record_counts"]["operations"] == 0
            assert rootless_lookup["record_counts"]["orphan_evidence"] == 5
            assert rootless_lookup["count"] == 5
            assert orphan_readback() == before_orphans
            with setup.cursor() as writer:
                for table, _, _ in exporter.ORPHAN_EVIDENCE_SOURCES.values():
                    writer.execute(f"DELETE FROM {table} WHERE operation_id=%s", (orphan,))
            assert capture(audit, LINEAGE, EPOCH) == snapshot

            # A lower operation ID committing after the read view must appear
            # in the next full cut. ID/timestamp cursors cannot certify this.
            delayed = pymysql.connect(**(settings | {"database": schema, "autocommit": False}))
            try:
                with delayed.cursor() as writer:
                    writer.execute(POSTING_INSERT +
                                   "(%s,0,0,0,1,0,0,0,1)", (orphan,))
                read_origins = exporter.read_origins_in_transaction

                def commit_after_read_view(cursor, lineage, epoch):
                    origins = read_origins(cursor, lineage, epoch)
                    delayed.commit()
                    return origins

                with mock.patch.object(exporter, "read_origins_in_transaction",
                                       side_effect=commit_after_read_view):
                    assert capture(audit, LINEAGE, EPOCH) == snapshot
                assert Reconciler().audit(capture(audit, LINEAGE, EPOCH))[
                    "exception_counts"]["orphan_coin_posting"] == 1
            finally:
                delayed.rollback()
                delayed.close()
                with setup.cursor() as writer:
                    writer.execute("DELETE FROM economic_accounting_coin_posting WHERE operation_id=%s",
                                   (orphan,))
            # An interrupted scan publishes nothing and releases its read cut.
            with mock.patch.object(exporter, "read_orphan_evidence", side_effect=RuntimeError("scan interrupted")):
                try:
                    capture(audit, LINEAGE, EPOCH)
                    raise AssertionError("interrupted scan succeeded")
                except RuntimeError as error:
                    assert str(error) == "scan interrupted"
            assert capture(audit, LINEAGE, EPOCH) == snapshot
            print("SQL orphan evidence, late lower-ID commit and interrupted cut passed", flush=True)
            # Aggregate owner revisions count changes to many UIDs. They may
            # differ from this UID's revision without changing its history.
            # Money and pile item revisions are unsigned independently of
            # signed coin denominations. Retain that distinction at the SQL cut.
            money_maximum = 2**64 - 1
            money_blob = bytearray(blob)
            wallet_revision_offset = 192 + 72
            pile_revision_offset = 192 + 2 * 112 + 72
            first_item_offset = 192 + struct.unpack_from("<I", money_blob, 184)[0] * 112
            pile_item_revision_offset = first_item_offset + 88 + 48
            assert struct.unpack_from("<Q", money_blob, wallet_revision_offset)[0] == 4
            assert struct.unpack_from("<Q", money_blob, pile_revision_offset)[0] == 1
            assert struct.unpack_from("<Q", money_blob, first_item_offset + 88)[0] == 82
            struct.pack_into("<Q", money_blob, wallet_revision_offset, money_maximum - 1)
            struct.pack_into("<Q", money_blob, pile_revision_offset, money_maximum)
            struct.pack_into("<Q", money_blob, pile_item_revision_offset, money_maximum)
            with setup.cursor() as writer:
                writer.execute("UPDATE economic_baseline_witness SET witness_digest=%s,"
                               "canonical_witness=%s WHERE operation_id=%s",
                               (hashlib.sha256(money_blob).digest(), bytes(money_blob), OP))
                bind_synthetic_baseline(writer, money_blob)
                writer.execute("UPDATE player_data SET wallet_revision=%s WHERE pid=7",
                               (money_maximum,))
                writer.execute("UPDATE item_current_owner SET item_revision=%s WHERE item_uid=82",
                               (money_maximum,))
                writer.execute("UPDATE economic_accounting_account_effect SET before_revision=%s,"
                               "after_revision=%s WHERE operation_id=%s AND account_index=0",
                               (money_maximum - 1, money_maximum, root))
            money_snapshot = capture(audit, LINEAGE, EPOCH)
            assert Reconciler().audit(money_snapshot)["exception_counts"] == expected_exceptions
            pile = money_snapshot["native"]["coin_piles"][0]
            assert pile["uid"] == 82 and pile["revision"] == money_maximum
            with setup.cursor() as writer:
                writer.execute("UPDATE player_data SET wallet_revision=%s WHERE pid=7",
                               (money_maximum - 1,))
            stale_high_money = Reconciler().audit(capture(audit, LINEAGE, EPOCH))
            assert stale_high_money["exception_counts"]["stale_native_balance"] == 1
            with setup.cursor() as writer:
                writer.execute("UPDATE player_data SET wallet_revision=5 WHERE pid=7")
                writer.execute("UPDATE item_current_owner SET item_revision=1 WHERE item_uid=82")
                writer.execute("UPDATE economic_accounting_account_effect SET before_revision=4,"
                               "after_revision=5 WHERE operation_id=%s AND account_index=0", (root,))
                writer.execute("UPDATE economic_baseline_witness SET witness_digest=%s,"
                               "canonical_witness=%s WHERE operation_id=%s",
                               (hashlib.sha256(blob).digest(), blob, OP))
                bind_synthetic_baseline(writer, blob)
            assert capture(audit, LINEAGE, EPOCH) == snapshot
            print("SQL money uint64 boundary: wallet/pile authority, stale-revision refusal "
                  "and baseline recovery passed", flush=True)
            # Representable denomination fields may still overflow checked
            # copper totals. These are corrupt disposable evidence/native rows;
            # neither the exporter nor reconciler may wrap or auto-repair them.
            for table, column, predicate, params in (
                    ("economic_accounting_account_effect", "before_platinum",
                     "operation_id=%s AND account_index=0", (root,)),
                    ("economic_accounting_account_effect", "after_platinum",
                     "operation_id=%s AND account_index=0", (root,)),
                    ("player_data", "platinum", "pid=%s", (7,))):
                for amount in (2**63 - 1, -(2**63)):
                    with setup.cursor() as writer:
                        writer.execute(f"UPDATE {table} SET {column}=%s WHERE {predicate}",
                                       (amount, *params))
                    corrupted = capture(audit, LINEAGE, EPOCH)
                    try:
                        Reconciler().audit(corrupted)
                    except SnapshotError as error:
                        assert str(error) == "copper overflow", error
                    else:
                        raise AssertionError("out-of-range weighted money vector was accepted")
                    with setup.cursor() as writer:
                        writer.execute(f"UPDATE {table} SET {column}=0 WHERE {predicate}", params)
                    assert capture(audit, LINEAGE, EPOCH) == snapshot
            print("SQL checked copper: native/effect overflow refusal and exact "
                  "unchanged snapshot repair passed", flush=True)
            # The native schema uses uint64 revisions. Qualify a complete
            # witnessed move at UINT64_MAX, separately from owner counters.
            maximum_revision = 2**64 - 1
            high_revision_root = bytes.fromhex("c7" * 16)
            high_blob = bytearray(blob)
            item_offset = 192 + struct.unpack_from("<I", high_blob, 184)[0] * 112
            assert struct.unpack_from("<Q", high_blob, item_offset)[0] == 81
            assert struct.unpack_from("<Q", high_blob, item_offset + 48)[0] == 2
            struct.pack_into("<Q", high_blob, item_offset + 48, maximum_revision - 1)
            with setup.cursor() as writer:
                writer.execute("UPDATE economic_baseline_witness SET witness_digest=%s,"
                               "canonical_witness=%s WHERE operation_id=%s",
                               (hashlib.sha256(high_blob).digest(), bytes(high_blob), OP))
                bind_synthetic_baseline(writer, high_blob)
                writer.execute("UPDATE item_current_owner SET item_revision=%s WHERE item_uid=81",
                               (maximum_revision,))
                writer.execute(ROOT_INSERT +
                               "(%s,%s,%s,NULL,32,1,0,NULL,0,0,0,1,NULL)",
                               (high_revision_root, LINEAGE, EPOCH))
                writer.execute("INSERT INTO critical_operation_inbox "
                               "(operation_id,status,result_code) VALUES (%s,1,0)",
                               (high_revision_root,))
                writer.execute(REFERENCE_INSERT +
                               "(%s,0,0,81,%s,%s,%s,0)",
                               (high_revision_root, maximum_revision - 1,
                                maximum_revision, high_revision_root))
                writer.execute(LEDGER_INSERT +
                               "(%s,0,81,81,NULL,1,7,0,%s,99,1)",
                               (high_revision_root, maximum_revision))
            high_snapshot = capture(audit, LINEAGE, EPOCH)
            assert Reconciler().audit(high_snapshot)["exception_counts"] == {
                **expected_exceptions, "missing_original_plan": 4}
            high_event = next(row for row in high_snapshot["native"]["uid_history_events"]
                              if row["uid"] == 81)
            assert (high_event["before_revision"], high_event["revision"]) == (
                maximum_revision - 1, maximum_revision)
            with setup.cursor() as writer:
                writer.execute("DELETE FROM economic_accounting_item_reference WHERE operation_id=%s",
                               (high_revision_root,))
            missing_high_reference = Reconciler().audit(capture(audit, LINEAGE, EPOCH))
            assert missing_high_reference["exception_counts"]["unreferenced_uid_event"] == 1
            with setup.cursor() as writer:
                writer.execute("DELETE FROM item_ownership_ledger WHERE operation_id=%s",
                               (high_revision_root,))
                writer.execute("DELETE FROM critical_operation_inbox WHERE operation_id=%s",
                               (high_revision_root,))
                writer.execute("DELETE FROM economic_accounting_operation WHERE operation_id=%s",
                               (high_revision_root,))
                writer.execute("UPDATE item_current_owner SET item_revision=2 WHERE item_uid=81")
                writer.execute("UPDATE economic_baseline_witness SET witness_digest=%s,"
                               "canonical_witness=%s WHERE operation_id=%s",
                               (hashlib.sha256(blob).digest(), blob, OP))
                bind_synthetic_baseline(writer, blob)
            assert capture(audit, LINEAGE, EPOCH) == snapshot
            print("SQL UID uint64 boundary: witnessed exact move, missing-reference refusal "
                  "and baseline recovery passed", flush=True)
            with setup.cursor() as writer:
                writer.execute("UPDATE item_ownership_ledger SET from_owner_revision=99 "
                               "WHERE item_uid=84")
            owner_revision_snapshot = capture(audit, LINEAGE, EPOCH)
            assert Reconciler().audit(owner_revision_snapshot)["exception_counts"] == \
                expected_exceptions
            assert owner_revision_snapshot["native"]["uid_history_events"][0]["before_revision"] == 0
            with setup.cursor() as writer:
                writer.execute("UPDATE item_ownership_ledger SET from_owner_revision=0 "
                               "WHERE item_uid=84")
            # Impossible native revision zero must refuse the audit cut, then
            # permit a fresh read after restoring only the fixture corruption.
            with setup.cursor() as writer:
                writer.execute("UPDATE item_ownership_ledger SET item_revision=0 "
                               "WHERE item_uid=84")
            try:
                capture(audit, LINEAGE, EPOCH)
            except exporter.ExportError as error:
                assert str(error) == "invalid native item ledger revision", error
            else:
                raise AssertionError("zero native item revision was accepted")
            with setup.cursor() as writer:
                writer.execute("UPDATE item_ownership_ledger SET item_revision=1 "
                               "WHERE item_uid=84")
            assert Reconciler().audit(capture(audit, LINEAGE, EPOCH))["exception_counts"] == \
                expected_exceptions
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
            duplicate_uid_source = bytes.fromhex(source_identity(kind=18, identity="b2"))
            with setup.cursor() as writer:
                writer.execute(ROOT_INSERT +
                               "(%s,%s,%s,NULL,33,1,0,%s,0,0,0,1,NULL)",
                               (duplicate_uid_root, LINEAGE, EPOCH, duplicate_uid_source))
                writer.execute("INSERT INTO critical_operation_inbox "
                               "(operation_id,status,result_code) VALUES (%s,1,0)",
                               (duplicate_uid_root,))
                writer.execute("INSERT INTO economic_accounting_source_claim VALUES (%s,%s,%s)",
                               (LINEAGE, duplicate_uid_source, duplicate_uid_root))
                writer.execute(REFERENCE_INSERT +
                               "(%s,0,0,84,1,2,%s,0)", (duplicate_uid_root, duplicate_uid_root))
                writer.execute(LEDGER_INSERT +
                               "(%s,0,84,84,NULL,1,7,0,2,1,2)", (duplicate_uid_root,))
                writer.execute("UPDATE item_current_owner SET item_revision=2 WHERE item_uid=84")
            expected_exceptions["missing_original_plan"] = 4
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
            expected_exceptions["player_uid_missing_physical"] = 1
            assert Reconciler().audit(capture(audit, LINEAGE, EPOCH))["exception_counts"] == \
                expected_exceptions
            revived_uid_root = bytes.fromhex("e7" * 16)
            revived_uid_source = bytes.fromhex(source_identity(kind=18, identity="b3"))
            with setup.cursor() as writer:
                writer.execute(ROOT_INSERT +
                               "(%s,%s,%s,NULL,32,1,0,%s,0,0,0,1,NULL)",
                               (revived_uid_root, LINEAGE, EPOCH, revived_uid_source))
                writer.execute("INSERT INTO critical_operation_inbox "
                               "(operation_id,status,result_code) VALUES (%s,1,0)", (revived_uid_root,))
                writer.execute("INSERT INTO economic_accounting_source_claim VALUES (%s,%s,%s)",
                               (LINEAGE, revived_uid_source, revived_uid_root))
                writer.execute(REFERENCE_INSERT +
                               "(%s,0,0,84,2,3,%s,0)", (revived_uid_root, revived_uid_root))
                writer.execute(LEDGER_INSERT +
                               "(%s,0,84,84,NULL,1,7,0,3,2,1)", (revived_uid_root,))
                writer.execute("UPDATE item_current_owner SET item_revision=3,state=1,"
                               "owner_type=1,owner_id=7 WHERE item_uid=84")
            expected_exceptions["missing_original_plan"] = 5
            expected_exceptions["player_uid_missing_physical"] = 2
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
                **expected_exceptions, "player_uid_missing_physical": 1,
                "duplicate_item_retirement": 1}, duplicate_retirement_report
            with setup.cursor() as writer:
                for table in ("economic_accounting_operation", "critical_operation_inbox",
                              "economic_accounting_source_claim", "economic_accounting_item_reference",
                              "item_ownership_ledger"):
                    writer.execute(f"DELETE FROM {table} WHERE operation_id IN (%s,%s)",
                                   (duplicate_uid_root, revived_uid_root))
                writer.execute("UPDATE item_current_owner SET item_revision=1,state=1,"
                               "owner_type=1,owner_id=7 WHERE item_uid=84")
            expected_exceptions["missing_original_plan"] = 3
            assert Reconciler().audit(capture(audit, LINEAGE, EPOCH))["exception_counts"] == \
                expected_exceptions
            # A deep, unanchored native forest is still auditable. Its missing
            # origins must remain explicit, and a corrupt cycle must terminate.
            deep_first, deep_count = 1000, 1200
            deep_last = deep_first + deep_count - 1
            with setup.cursor() as writer:
                writer.executemany(ITEM_INSERT +
                                   "(%s,%s,%s,1,7,0,1,1,1,NULL)",
                                   [(uid, deep_last, uid + 1 if uid < deep_last else None)
                                    for uid in range(deep_first, deep_last + 1)])
            deep_report = Reconciler().audit(capture(audit, LINEAGE, EPOCH))
            assert deep_report["exception_counts"] == {
                **expected_exceptions, "unknown_legacy_origin": deep_count,
                "player_uid_missing_physical": deep_count + 2}, deep_report
            with setup.cursor() as writer:
                writer.execute("UPDATE item_current_owner SET parent_item_uid=%s WHERE item_uid=%s",
                               (deep_first, deep_last))
            cycle_report = Reconciler().audit(capture(audit, LINEAGE, EPOCH))
            assert cycle_report["exception_counts"] == {
                **expected_exceptions, "unknown_legacy_origin": deep_count,
                "player_uid_missing_physical": deep_count + 2,
                "cyclic_native_topology": deep_count}, cycle_report
            with setup.cursor() as writer:
                writer.execute("DELETE FROM item_current_owner WHERE item_uid BETWEEN %s AND %s",
                               (deep_first, deep_last))
            assert Reconciler().audit(capture(audit, LINEAGE, EPOCH))["exception_counts"] == \
                expected_exceptions
            with setup.cursor() as writer:
                writer.execute(ROOT_INSERT +
                               "(%s,%s,%s,NULL,33,1,0,%s,0,0,0,1,NULL)",
                               (prior_item_root, LINEAGE, prior_epoch, prior_item_source))
                writer.execute("INSERT INTO critical_operation_inbox "
                               "(operation_id,status,result_code) VALUES (%s,1,0)",
                               (prior_item_root,))
                writer.execute("INSERT INTO economic_accounting_source_claim VALUES (%s,%s,%s)",
                               (LINEAGE, prior_item_source, prior_item_root))
                writer.execute(REFERENCE_INSERT +
                               "(%s,0,0,86,0,1,%s,0)",
                               (prior_item_root, prior_item_root))
                writer.execute(LEDGER_INSERT +
                               "(%s,0,86,86,NULL,1,7,0,1,0,2)", (prior_item_root,))
                writer.execute(LEDGER_INSERT +
                               "(%s,0,86,86,NULL,8,0,0,2,1,3)",
                               (prior_unlinked_operation,))
                writer.execute(ITEM_INSERT +
                               "(86,86,NULL,8,0,0,2,2,1,NULL)")
            with setup.cursor() as writer:
                writer.execute("UPDATE item_ownership_ledger SET from_owner_revision=99 "
                               "WHERE item_uid=86")
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
            before_provenance = json.dumps(historical_uid_snapshot, sort_keys=True)
            historical_uid_report = Reconciler().audit(historical_uid_snapshot)
            provenance = view(historical_uid_snapshot, historical_uid_report, "provenance", 100, uid=86)
            assert provenance["count"] == 2, provenance
            assert [(row["operation_id"], row["revision"], row["action"])
                    for row in provenance["rows"]] == [
                        (prior_item_root.hex(), 1, "create"),
                        (prior_unlinked_operation.hex(), 2, "destroy")]
            assert provenance["coverage"] == {
                "lineage": LINEAGE.hex(), "selected_epoch": EPOCH.hex(), "complete": False,
                "quiescent": True, "lineage_history_available": True,
                "unattributed_history_available": True,
                "exception_count": historical_uid_report["exception_count"]}
            with tempfile.TemporaryDirectory(prefix="audit-provenance-") as temporary:
                output = Path(temporary) / "snapshot.json"
                command = [sys.executable, str(ROOT / "scripts/economic_sql_audit_snapshot.py"),
                           "--host", "127.0.0.1", *transport_args,
                           "--user", reader, "--database", schema,
                           "--lineage", LINEAGE.hex(), "--epoch", EPOCH.hex(),
                           "--output", str(output)]
                subprocess.run(command, env=dict(os.environ, DB_PASSWORD="disposable-audit-only"),
                               check=True, timeout=30)
                assert json.loads(output.read_text(encoding="utf-8")) == historical_uid_snapshot
                before_cli = output.read_bytes()
                for limit in (0, 1, 100):
                    result = subprocess.run(
                        [sys.executable, str(ROOT / "scripts/reconcile_economy_accounting.py"),
                         str(output), "--view", "provenance", "--uid", "86", "--limit", str(limit)],
                        capture_output=True, text=True, timeout=30)
                    assert result.returncode == 1, result.stderr
                    assert json.loads(result.stdout) == view(
                        historical_uid_snapshot, historical_uid_report, "provenance", limit, uid=86)
                    assert output.read_bytes() == before_cli
            assert json.dumps(historical_uid_snapshot, sort_keys=True) == before_provenance
            assert capture(audit, LINEAGE, EPOCH) == historical_uid_snapshot
            print("UID provenance: prior-epoch creation and unattributed retirement are "
                  "visible through SELECT-only export/CLI; bounded counts and unchanged "
                  "authority passed", flush=True)
            assert next(row for row in historical_uid_snapshot["item_origins"]
                        if row["uid"] == 86)["origin"] == "creation"
            assert any(row["operation_id"] == prior_item_root.hex()
                       for row in historical_uid_snapshot["native"]["lineage_uid_references"])
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
                writer.execute(ROOT_INSERT +
                               "(%s,%s,%s,NULL,33,1,0,NULL,0,0,0,0,NULL)",
                               (unanchored_operation, LINEAGE, EPOCH))
                writer.execute(ROOT_INSERT +
                               "(%s,%s,%s,NULL,33,1,0,NULL,0,0,0,0,NULL)",
                               (unscoped_operation, LINEAGE, EPOCH))
                writer.execute(ROOT_INSERT +
                               "(%s,%s,%s,NULL,33,1,0,NULL,0,0,0,0,NULL)",
                               (other_lineage_operation, other_lineage, EPOCH))
                writer.execute(ITEM_INSERT +
                               "(85,85,NULL,1,7,0,1,1,1,NULL)")
                writer.execute(LEDGER_INSERT +
                               "(%s,0,85,85,NULL,1,7,0,1,0,2)",
                               (unanchored_operation,))
                writer.execute(LEDGER_INSERT +
                               "(%s,0,87,87,NULL,1,7,0,1,0,2)",
                               (unscoped_operation,))
                writer.execute(LEDGER_INSERT +
                               "(%s,0,88,88,NULL,1,7,0,1,0,2)",
                               (unattributed_operation,))
                writer.execute(LEDGER_INSERT +
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
                writer.execute(LEDGER_INSERT +
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
                writer.execute(ITEM_INSERT +
                               "(85,85,NULL,1,7,0,1,1,3,X'00')")
            try:
                capture(audit, LINEAGE, EPOCH)
                raise AssertionError("malformed coin-pile payload passed the audit export")
            except exporter.ExportError:
                pass
            with setup.cursor() as writer:
                writer.execute("DELETE FROM item_current_owner WHERE item_uid=85")
            with setup.cursor() as writer:
                writer.execute("INSERT INTO ships VALUES (25,7)")
                writer.execute("INSERT INTO guilds VALUES (31,1,2,3,4,9)")
            original = exporter.read_origins_in_transaction

            def concurrent_change(cursor, lineage, epoch):
                origins = original(cursor, lineage, epoch)
                with setup.cursor() as writer:
                    writer.execute("UPDATE player_data SET copper=3 WHERE pid=7")
                    writer.execute("UPDATE ships SET money=11 WHERE id=25")
                    writer.execute("UPDATE guilds SET copper=7 WHERE id=31")
                    writer.execute("UPDATE auctions SET obj_vnum=1 WHERE id=5")
                    writer.execute("INSERT INTO auction_item_custody(auction_id,slot,item_uid,item_revision,vnum,obj_blob) "
                                   "VALUES (5,0,84,1,1,X'78')")
                    writer.execute("INSERT INTO auction_item_pickups VALUES (1,7,1,0,X'78')")
                    writer.execute("UPDATE shopkeepers SET shop_id=77 WHERE id=3")
                    writer.execute("INSERT INTO shopkeeper_items VALUES (400,3,NULL,84,0,0,1)")
                    writer.execute("INSERT INTO player_pets VALUES (51,7,900)")
                    writer.execute("INSERT INTO player_items(id,pid,obj_uid,vnum,equip_slot,quantity,item_type) "
                                   "VALUES (400,7,84,1,0,1,1)")
                    writer.execute("INSERT INTO player_pet_items(id,pet_id,obj_uid,vnum,equip_slot,item_type) "
                                   "VALUES (400,51,84,1,0,1)")
                    writer.execute("INSERT INTO corpses VALUES (5,7,9,1,10)")
                    writer.execute("INSERT INTO corpse_items VALUES (400,5,NULL,84,1,1,1,0,0,0,0,0)")
                    writer.execute("INSERT INTO lockers VALUES (5,0,7,NULL)")
                    writer.execute("INSERT INTO account_lockers VALUES (5,0)")
                    writer.execute("INSERT INTO private_chests VALUES (9,5,1)")
                    writer.execute("INSERT INTO locker_chests VALUES (9,5,1)")
                    writer.execute("INSERT INTO locker_items VALUES (400,5,9,NULL,84,1,1,1,0,1,0,0,0,0)")
                    writer.execute("INSERT INTO account_locker_items VALUES (400,9,NULL,85,1,1,1,0,0,0,0,0)")
                return origins

            with mock.patch.object(exporter, "read_origins_in_transaction",
                                   side_effect=concurrent_change):
                fenced = capture(audit, LINEAGE, EPOCH)
            wallet = next(row for row in fenced["native"]["holdings"]
                          if row["account_key"] == key(1, 7).hex())
            assert wallet["balance"] == [2, 0, 0, 0]
            assert fenced["native"]["ship_coffers"] == [{"ship_id":25,"copper":7}]
            assert capture(audit, LINEAGE, EPOCH)["native"]["ship_coffers"] == [{"ship_id":25,"copper":11}]
            assert fenced["native"]["guild_treasuries"] == [{"guild_id":31,"balance":[1,2,3,4]}]
            assert capture(audit, LINEAGE, EPOCH)["native"]["guild_treasuries"] == [{"guild_id":31,"balance":[7,2,3,4]}]
            assert fenced["native"]["auction_listings"][0]["vnum"] == 0
            assert fenced["native"]["auction_roots"] == fenced["native"]["auction_legacy_pickups"] == []
            later_auctions = capture(audit, LINEAGE, EPOCH)["native"]
            assert later_auctions["auction_listings"][0]["vnum"] == 1
            assert len(later_auctions["auction_roots"]) == len(later_auctions["auction_legacy_pickups"]) == 1
            assert fenced["native"]["shop_keepers"] == [{"keeper_id":3,"shop_id":0}]
            assert fenced["native"]["shop_items"] == []
            assert later_auctions["shop_keepers"] == [{"keeper_id":3,"shop_id":77}]
            assert later_auctions["shop_items"] == [{"item_id":400,"keeper_id":3,"parent_id":None,
                "uid":84,"vnum":0,"equipment_slot":0,"quantity":1}]
            assert fenced["native"]["player_pets"] == fenced["native"]["player_items"] == fenced["native"]["pet_items"] == []
            assert later_auctions["player_pets"] == [{"pet_id":51,"pid":7,"pet_uid":900}]
            for name in ("player_items", "pet_items"):
                assert len(later_auctions[name]) == 1 and later_auctions[name][0]["item_id"] == 400
                assert later_auctions[name][0]["uid"] == 84
            assert later_auctions["player_custody_coverage"] == dict(players=3,pets=1,items=1,pet_items=1)
            assert fenced["native"]["corpses"] == fenced["native"]["corpse_items"] == []
            assert later_auctions["corpses"] == [{"corpse_id":5,"pid":7,"save_id":9,"revision":1,"room_vnum":10}]
            assert later_auctions["corpse_items"] == [{"item_id":400,"corpse_id":5,"parent_id":None,"uid":84,
                "vnum":1,"quantity":1,"weight":1,"extra_flags":0,"value0":0,"value1":0,"value2":0,"value3":0}]
            assert later_auctions["corpse_custody_coverage"] == dict(corpses=1,items=1)
            locker_names = ("lockers", "private_chests", "locker_items", "account_lockers", "locker_chests", "account_locker_items")
            assert all(fenced["native"][name] == [] for name in locker_names)
            assert later_auctions["locker_custody_coverage"] == {name:1 for name in locker_names}
            assert later_auctions["lockers"] == [dict(locker_id=5,racewar=0,owner_pid=7,owner_assoc_id=None)]
            assert later_auctions["account_lockers"] == [dict(locker_id=5,racewar=0)]
            assert later_auctions["private_chests"] == later_auctions["locker_chests"] == [dict(chest_id=9,locker_id=5,is_public=1)]
            expected_locker = dict(item_id=400,chest_id=9,parent_id=None,uid=84,vnum=1,quantity=1,weight=1,extra_flags=0,
                                   value0=0,value1=0,value2=0,value3=0)
            assert later_auctions["locker_items"] == [dict(expected_locker,locker_id=5,item_type=1)]
            assert later_auctions["account_locker_items"] == [dict(expected_locker,uid=85)]
            with setup.cursor() as cursor:
                cursor.execute("UPDATE player_data SET copper=2 WHERE pid=7")
                cursor.execute("DELETE FROM ships")
                cursor.execute("DELETE FROM guilds")
                cursor.execute("UPDATE auctions SET obj_vnum=0 WHERE id=5")
                cursor.execute("DELETE FROM auction_item_custody")
                cursor.execute("DELETE FROM auction_item_pickups")
                cursor.execute("DELETE FROM shopkeeper_items")
                cursor.execute("UPDATE shopkeepers SET shop_id=0 WHERE id=3")
                cursor.execute("DELETE FROM player_pet_items")
                cursor.execute("DELETE FROM player_items")
                cursor.execute("DELETE FROM player_pets")
                cursor.execute("DELETE FROM corpse_items")
                cursor.execute("DELETE FROM corpses")
                for table in ("locker_items", "account_locker_items", "private_chests", "locker_chests", "lockers", "account_lockers"):
                    cursor.execute("DELETE FROM " + table)
            with tempfile.TemporaryDirectory(prefix="duris-sql-audit-") as directory:
                output = Path(directory) / "partial.json"
                command = [sys.executable, str(ROOT / "scripts/economic_sql_audit_snapshot.py"),
                           "--host", "127.0.0.1", *transport_args,
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
                # Exact ID views retain the global audit refusal and the
                # selected cut. Filtering never certifies a partial export.
                before_lookup = capture(audit, LINEAGE, EPOCH)
                before_bytes = output.read_bytes()
                for limit in (0, 1, 100):
                    lookup = subprocess.run(
                        [sys.executable, str(ROOT / "scripts/reconcile_economy_accounting.py"),
                         str(output), "--view", "operation", "--operation-id", root.hex(),
                         "--limit", str(limit)], capture_output=True, text=True, timeout=30)
                    assert lookup.returncode == 1, lookup.stderr
                    operation = json.loads(lookup.stdout)
                    assert operation["count"] == 7 and operation["truncated"] == (limit < 7)
                    assert operation["record_counts"] == {
                        "operations": 1, "effects": 2, "postings": 2, "children": 0,
                        "item_references": 0, "receipts": 1, "source_claims": 1, "orphan_evidence": 0}
                    assert operation["coverage"]["root_scope"] == "selected_epoch"
                    assert operation["coverage"]["complete"] is False
                    assert operation["coverage"]["exception_count"] == sum(expected_exceptions.values())
                    if limit:
                        assert operation["rows"][0]["operation_id"] == root.hex()
                        assert operation["rows"][0]["record"] == "operations"
                    holding = subprocess.run(
                        [sys.executable, str(ROOT / "scripts/reconcile_economy_accounting.py"),
                         str(output), "--view", "holdings", "--account-key", key(1, 7).hex(),
                         "--limit", str(limit)], capture_output=True, text=True, timeout=30)
                    assert holding.returncode == 1, holding.stderr
                    account = json.loads(holding.stdout)
                    assert account["count"] == 1 and account["truncated"] == (limit == 0)
                    assert account["coverage"]["account_key"] == key(1, 7).hex()
                    assert account["coverage"]["scope"] == "captured_native_holdings"
                    if limit:
                        assert account["rows"][0]["balance"] == [2, 0, 0, 0]
                    assert "alias" not in lookup.stdout and "alias" not in holding.stdout
                    assert output.read_bytes() == before_bytes
                    assert capture(audit, LINEAGE, EPOCH) == before_lookup
                print("SQL operator views: exact operation/account IDs, global refusal, "
                      "zero/one/max limits and unchanged source cut passed", flush=True)
                with setup.cursor() as cursor:
                    cursor.execute("INSERT INTO ships VALUES (25,7)")
                before_ship_cli = capture(audit, LINEAGE, EPOCH)
                ship_output = Path(directory) / "ship-partial.json"
                subprocess.run(command[:-1]+[str(ship_output)], env=environment, check=True, timeout=30)
                assert json.loads(ship_output.read_text(encoding="utf-8")) == before_ship_cli
                ship_cli = subprocess.run(
                    [sys.executable, str(ROOT / "scripts/reconcile_economy_accounting.py"),
                     str(ship_output), "--limit", "0"], capture_output=True, text=True, timeout=30)
                assert ship_cli.returncode == 1
                ship_report = json.loads(ship_cli.stdout)
                assert ship_report["exception_counts"]["unsupported_native_ship_coffer"] == 1
                assert ship_report["exception_counts"]["missing_ship_coffer_revision"] == 1
                assert ship_report["exceptions"] == []
                assert capture(audit, LINEAGE, EPOCH) == before_ship_cli
                with setup.cursor() as cursor:
                    cursor.execute("DELETE FROM ships")
                assert capture(audit, LINEAGE, EPOCH) == snapshot

                with setup.cursor() as cursor:
                    cursor.execute("INSERT INTO guilds VALUES (31,1,2,3,4,9)")
                before_guild_cli = capture(audit, LINEAGE, EPOCH)
                guild_output = Path(directory) / "guild-partial.json"
                subprocess.run(command[:-1]+[str(guild_output)], env=environment, check=True, timeout=30)
                assert json.loads(guild_output.read_text(encoding="utf-8")) == before_guild_cli
                guild_cli = subprocess.run(
                    [sys.executable, str(ROOT / "scripts/reconcile_economy_accounting.py"),
                     str(guild_output), "--limit", "0"], capture_output=True, text=True, timeout=30)
                assert guild_cli.returncode == 1
                guild_report = json.loads(guild_cli.stdout)
                assert guild_report["exception_counts"]["unsupported_native_guild_treasury"] == 1
                assert guild_report["exception_counts"]["missing_guild_money_revision"] == 1
                assert guild_report["exceptions"] == []
                assert capture(audit, LINEAGE, EPOCH) == before_guild_cli
                with setup.cursor() as cursor:
                    cursor.execute("DELETE FROM guilds")
                assert capture(audit, LINEAGE, EPOCH) == snapshot

            with setup.cursor() as cursor:
                cursor.execute("DELETE FROM economic_accounting_coin_posting "
                               "WHERE operation_id=%s AND line_index=1", (root,))
            missing = Reconciler().audit(capture(audit, LINEAGE, EPOCH))["exception_counts"]
            assert "unbalanced_root" in missing and "evidence_count_mismatch" in missing
            with setup.cursor() as cursor:
                cursor.execute(POSTING_INSERT +
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
                cursor.execute(ROOT_INSERT +
                               "(%s,%s,%s,NULL,5,1,0,NULL,0,0,0,0,NULL)",
                               (bytes.fromhex("9a" * 16), LINEAGE, prior_epoch))
                cursor.execute(ROOT_INSERT +
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
                    priced_source = bytes.fromhex(source_identity(kind=17 if reason == 24 else 13, identity="bb"))
                    cursor.execute("UPDATE economic_accounting_operation "
                                   "SET reason=%s,source_event=%s,realized_price_copper=125 "
                                   "WHERE operation_id=%s", (reason, priced_source, root))
                    cursor.execute("UPDATE economic_accounting_source_claim SET source_event=%s WHERE operation_id=%s",
                                   (priced_source, root))
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
        cursor.execute(f"DROP USER IF EXISTS '{reader}'@'{reader_host}'")
    admin.close()
