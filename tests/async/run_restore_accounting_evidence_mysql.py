#!/usr/bin/env python3
"""Corrupt retained evidence on a newly created, socket-only restore database.

The parent must provide a fresh disposable daemon and a task-owned socket
mounted below /plan5-restore-. No existing database or project .env is used.
These are native SQL fixtures, not gameplay or complete restore qualification.
"""
import contextlib
import hashlib
import io
import json
import os
from pathlib import Path
import struct
import subprocess
import sys

import pymysql

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
import migration_runner as migrations
import qualify_database_restore as qualifier

if (os.environ.get("TEST_DB_DISPOSABLE") != "1" or
        os.environ.get("ENVIRONMENT") != "test" or
        os.environ.get("DB_HOST") != "127.0.0.1" or
        os.environ.get("DB_NAME") != "duris_restore" or
        not os.environ.get("DB_SOCKET", "").startswith("/plan5-restore-")):
    raise SystemExit("explicit fresh disposable restore daemon/socket required")
if not os.environ.get("DURIS_PLAN5_COIN_FIXTURE"):
    raise SystemExit("native EAI1/EAP1 fixture required; use test_restore_economic_coin_effects.py")

settings = {"unix_socket": os.environ["DB_SOCKET"], "user": os.environ["DB_USER"],
            "password": os.environ["DB_PASSWD"], "autocommit": True,
            "connect_timeout": 5, "read_timeout": 30}
connection = pymysql.connect(**settings)
created = False
reader = None
LINEAGE = bytes.fromhex("11" * 16)
EPOCH = bytes.fromhex("22" * 16)
OP = bytes.fromhex("33" * 16)
CHILD = bytes.fromhex("55" * 16)
REJECTED = bytes.fromhex("66" * 16)
ORPHAN = bytes.fromhex("77" * 16)
SOURCE = bytes.fromhex("44" * 48)
native_blocks = []
fixture = os.environ.get("DURIS_PLAN5_COIN_FIXTURE")
if fixture:
    fixture_path = Path(fixture).resolve()
    assert any(fixture_path.is_relative_to((ROOT / directory).resolve()) for directory in
               ("bin/tests/plan5-sql-baseline-restore-coins", "bin/tests/plan5-sql-baseline-restore-canonical",
                "bin/tests/plan5-retained-namespace-coins", "bin/tests/plan5-retained-namespace-canonical"))
    payload = fixture_path.read_bytes()
    assert len(payload) <= 1024 * 1024
    offset = 0
    while offset < len(payload):
        size, = struct.unpack_from("<I", payload, offset)
        offset += 4
        native_blocks.append(payload[offset:offset + size])
        offset += size
    assert offset == len(payload) and len(native_blocks) == 37
    assert native_blocks[0][:4] == b"EAI1" and native_blocks[1][:4] == b"EAP1"
    SOURCE = native_blocks[1][104:152]
    CHILD = native_blocks[2][592:608]
READER = "plan5_restore_reader"
TABLES = ("economic_accounting_operation", "economic_accounting_account_effect",
          "economic_accounting_coin_posting", "economic_accounting_child",
          "economic_accounting_item_reference", "economic_accounting_source_claim",
          "critical_operation_inbox", "item_ownership_ledger", "economic_epoch",
          "economic_lineage_state", "player_data", "currency_wallet_baseline",
          "epic_balance_baseline", "mud_schema_migrations", "mud_schema_migration_state",
          "quest_mobile_native", "economic_pending_claim_source", "economic_pending_claim_consumption",
          "economic_account_mapping", "economic_baseline_control", "economic_baseline_witness",
          "economic_baseline_reservation", "auction_money_pickups")


def execute(query, params=None):
    with connection.cursor() as cursor:
        cursor.execute(query, params)


def scalar(query):
    with connection.cursor() as cursor:
        cursor.execute(query)
        return cursor.fetchone()[0]


def captured():
    with connection.cursor() as cursor:
        result = {}
        for table in TABLES:
            cursor.execute("SELECT * FROM " + table + " ORDER BY 1")
            result[table] = cursor.fetchall()
        return result


class Executor:
    def sql(self, query):
        assert query.startswith("SELECT "), query
        with reader.cursor() as cursor:
            cursor.execute(query)
            rows = cursor.fetchall()
            assert all(len(row) == 1 for row in rows), query
            return "\n".join(str(row[0]) for row in rows)


def old_money_checks():
    qualifier.require_epic_revision_history(Executor())
    qualifier.require_currency_revision_history(Executor())
    qualifier.require_currency_values(Executor())


def main_result():
    with contextlib.redirect_stdout(io.StringIO()) as output:
        qualifier.main()
    assert json.loads(output.getvalue()) == {"history": "ok", "reconciliation": "ok"}


def admitted():
    before = captured()
    main_result()
    assert captured() == before, "qualification must not change restored authority"


def refused(code):
    before = captured()
    try:
        main_result()
    except RuntimeError as error:
        assert str(error) == code, (str(error), code)
    else:
        raise AssertionError("lost/corrupt economic evidence was qualified: " + code)
    assert captured() == before, "refusal must retain every captured row"


def damaged(query, repair, code, params=None, repair_params=None, broken_fk=False):
    before = captured()
    try:
        if broken_fk:
            execute("SET SESSION FOREIGN_KEY_CHECKS=0")
        execute(query, params)
    finally:
        execute("SET SESSION FOREIGN_KEY_CHECKS=1")
    assert scalar("SELECT @@SESSION.foreign_key_checks") == 1
    with reader.cursor() as cursor:
        cursor.execute("SELECT @@SESSION.foreign_key_checks")
        assert cursor.fetchone()[0] == 1
    old_money_checks()
    refused(code)
    try:
        if broken_fk:
            execute("SET SESSION FOREIGN_KEY_CHECKS=0")
        execute(repair, repair_params if repair_params is not None else params)
    finally:
        execute("SET SESSION FOREIGN_KEY_CHECKS=1")
    admitted()
    assert captured() == before, code
    print("refused without mutation: " + code, flush=True)


canonical_cuts = []


def canonical_cut(label, changes, repairs, code, full=False, broken_fk=False):
    original = captured()
    try:
        try:
            if broken_fk:
                execute("SET SESSION FOREIGN_KEY_CHECKS=0")
            for query, params in changes:
                execute(query, params)
        finally:
            execute("SET SESSION FOREIGN_KEY_CHECKS=1")
        assert scalar("SELECT @@SESSION.foreign_key_checks") == 1
        cut = captured()
        try:
            if full:
                main_result()
            else:
                with reader.cursor() as cursor:
                    cursor.execute("SET TRANSACTION ISOLATION LEVEL REPEATABLE READ")
                    cursor.execute("START TRANSACTION WITH CONSISTENT SNAPSHOT, READ ONLY")
                qualifier.economic_restore_evidence.require_integrity(Executor())
        except RuntimeError as error:
            assert str(error) == code, (label, str(error), code)
        else:
            assert code is None, (label, "corrupt canonical evidence was admitted", code)
        finally:
            reader.rollback()
        if label.startswith(("native-mobile-", "pending-claim-")):
            import economic_sql_canonical_audit as canonical_audit
            audit_reader = pymysql.connect(**(settings | {"database": "duris_restore", "user": READER,
                "password": "plan5-disposable-reader", "cursorclass": pymysql.cursors.DictCursor}))
            try:
                try:
                    result = canonical_audit.capture(audit_reader)
                except canonical_audit.AuditError as error:
                    assert str(error) == code, (label, str(error), code)
                else:
                    assert code is None and result["read_only"] and not result["release_qualified"]
            finally:
                audit_reader.close()
        if label.startswith("pending-claim-"):
            import economic_sql_audit_snapshot as exporter
            from reconcile_economy_accounting import Reconciler
            snapshot_reader = pymysql.connect(**(settings | {"database": "duris_restore", "user": READER,
                "password": "plan5-disposable-reader", "cursorclass": pymysql.cursors.DictCursor}))
            try:
                with snapshot_reader.cursor() as cursor:
                    cursor.execute("SET TRANSACTION ISOLATION LEVEL REPEATABLE READ")
                    cursor.execute("START TRANSACTION WITH CONSISTENT SNAPSHOT, READ ONLY")
                    native, gaps, coverage = exporter.read_native(cursor, LINEAGE)
                    native["pending_claim_consumers"], native["pending_claim_consumer_coverage"] = (
                        exporter.read_pending_claim_consumers(cursor, LINEAGE))
                report = Reconciler()
                report.audit_pending_claim_consumers("sql_partial", LINEAGE.hex(), native)
                from collections import Counter
                spent, retained = Counter(), Counter()
                for row in native.get("pending_claim_consumptions", []):
                    spent[(row["source_operation_id"], row["source_slot"])] += row["amount"]
                for row in native["pending_claim_sources"]:
                    if row["account_key"] is not None and row["claim_operation_id"] is None:
                        retained[row["account_key"]] += row["amount"]-spent[(row["source_operation_id"], row["source_slot"])]
                live = {row["account_key"]: row["balance"][0] for row in native["holdings"]
                        if bytes.fromhex(row["account_key"])[18:20] == b"\x05\x00"}
                invalid = (sum(native["pending_claim_source_coverage"][field] for field in
                    ("invalid_account_mappings", "invalid_source_roots", "invalid_consumer_roots")) or
                    sum(report.counts.values()) or any(retained[key] != live.get(key, 0) for key in set(retained)|set(live)))
                assert bool(invalid) == bool(code), (label, native, dict(report.counts))
            finally:
                snapshot_reader.rollback()
                snapshot_reader.close()
        assert captured() == cut, label + ": audit changed authority"
    finally:
        try:
            if broken_fk:
                execute("SET SESSION FOREIGN_KEY_CHECKS=0")
            for query, params in repairs:
                execute(query, params)
        finally:
            execute("SET SESSION FOREIGN_KEY_CHECKS=1")
    assert captured() == original, label + ": disposable fixture was not restored"
    canonical_cuts.append({"label": label, "code": code, "full_entry": full})
    print("CANONICAL_CUT " + json.dumps(canonical_cuts[-1], sort_keys=True), flush=True)


def canonical_field(table, field, changed, code, condition="", full=False, operation=OP):
    with connection.cursor() as cursor:
        cursor.execute("SELECT " + field + " FROM " + table + " WHERE operation_id=%s" + condition,
                       (operation,))
        rows = cursor.fetchall()
        assert len(rows) == 1
        original = rows[0][0]
    query = "UPDATE " + table + " SET " + field + "=%s WHERE operation_id=%s" + condition
    canonical_cut(table + "." + field, [(query, (changed, operation))],
                  [(query, (original, operation))], code, full)


def modeled_claim_two_account_batch(source_operation):
    """Real SQL query coverage with modeled metadata, never capsule/producer proof."""
    original = captured()
    batch_ids = [value.to_bytes(16, "big") for value in range(128, 193)]
    overlap, follower = batch_ids[-2:]
    with connection.cursor() as cursor:
        cursor.execute("SELECT COUNT(*) FROM economic_accounting_operation WHERE operation_id IN ("+
                       ",".join(["%s"]*len(batch_ids))+")", tuple(batch_ids))
        assert cursor.fetchone()[0] == 0
        cursor.execute("SELECT COUNT(*) FROM economic_account_mapping WHERE mapping_id=10")
        assert cursor.fetchone()[0] == 0
        cursor.execute("SELECT COUNT(*) FROM auction_money_pickups WHERE pid=43")
        assert cursor.fetchone()[0] == 0
        cursor.execute("SELECT money,claim_revision FROM auction_money_pickups WHERE pid=42")
        original_cash = cursor.fetchone()
        assert original_cash is not None and original_cash[0] == 8
        cursor.execute("SELECT * FROM economic_accounting_operation WHERE operation_id=%s", (source_operation,))
        root_values = list(cursor.fetchone())
        root_columns = [column[0] for column in cursor.description]
        templates = {}
        for table in ("economic_accounting_account_effect", "economic_accounting_coin_posting"):
            cursor.execute("SELECT * FROM "+table+" WHERE operation_id=%s ORDER BY 2", (source_operation,))
            templates[table] = ([column[0] for column in cursor.description], cursor.fetchall())
    effect_columns, effect_rows = templates["economic_accounting_account_effect"]
    posting_columns, posting_rows = templates["economic_accounting_coin_posting"]
    assert len(effect_rows) == len(posting_rows) == 2
    claim_rows = [row for row in effect_rows if row[effect_columns.index("account_key")][18:20] == b"\x05\x00"]
    wallet_rows = [row for row in effect_rows if row[effect_columns.index("account_key")][18:20] == b"\x01\x00"]
    assert len(claim_rows) == len(wallet_rows) == 1
    claim, wallet = claim_rows[0], wallet_rows[0]
    claim_index = claim[effect_columns.index("account_index")]
    wallet_index = wallet[effect_columns.index("account_index")]
    assert claim[effect_columns.index("after_copper")]-claim[effect_columns.index("before_copper")] == 5
    assert wallet[effect_columns.index("before_copper")]-wallet[effect_columns.index("after_copper")] == 5
    second_index = max(row[effect_columns.index("account_index")] for row in effect_rows)+1
    second_key = LINEAGE+struct.pack("<HHQQ4x", 1, 5, 10, 0)
    assert claim[effect_columns.index("account_key")] < second_key
    def insert(table, columns, values):
        execute("INSERT INTO "+table+"("+",".join(columns)+") VALUES("+
                ",".join(["%s"]*len(values))+")", values)
    try:
        for operation in batch_ids:
            root = root_values.copy()
            root[root_columns.index("operation_id")] = operation
            if operation == overlap:
                assert root[root_columns.index("account_count")] == 2
                assert root[root_columns.index("posting_count")] == 2
                root[root_columns.index("account_count")] = 3
                root[root_columns.index("posting_count")] = 3
            execute("INSERT INTO critical_operation_inbox(operation_id,command_hash,keys_hash,"
                    "command_type,schema_version,payload_version,status,result_payload,committed_at) "
                    "VALUES(%s,%s,%s,1,2,1,1,'',CURRENT_TIMESTAMP(6))", (operation, bytes(32), bytes(32)))
            insert("economic_accounting_operation", root_columns, root)
            for row in effect_rows:
                effect = list(row)
                effect[effect_columns.index("operation_id")] = operation
                if operation == overlap and effect[effect_columns.index("account_index")] == wallet_index:
                    effect[effect_columns.index("after_copper")] -= 5
                insert("economic_accounting_account_effect", effect_columns, effect)
            if operation == overlap:
                effect = list(claim)
                effect[effect_columns.index("operation_id")] = operation
                effect[effect_columns.index("account_index")] = second_index
                effect[effect_columns.index("account_key")] = second_key
                insert("economic_accounting_account_effect", effect_columns, effect)
            for row in posting_rows:
                posting = list(row)
                posting[posting_columns.index("operation_id")] = operation
                if operation == overlap and posting[posting_columns.index("account_index")] == wallet_index:
                    posting[posting_columns.index("delta_copper")] -= 5
                    posting[posting_columns.index("copper_value")] -= 5
                insert("economic_accounting_coin_posting", posting_columns, posting)
            if operation == overlap:
                matching = [row for row in posting_rows if row[posting_columns.index("account_index")] == claim_index]
                assert len(matching) == 1
                posting = list(matching[0])
                posting[posting_columns.index("operation_id")] = operation
                next_line = max(row[posting_columns.index("line_index")] for row in posting_rows)+1
                posting[posting_columns.index("line_index")] = next_line
                posting[posting_columns.index("event_index")] = next_line
                posting[posting_columns.index("account_index")] = second_index
                insert("economic_accounting_coin_posting", posting_columns, posting)
            execute("INSERT INTO economic_pending_claim_source VALUES(%s,1,%s,9,42,5,NULL)", (operation,LINEAGE))
        execute("INSERT INTO economic_account_mapping(mapping_id,lineage,account_kind,context_id,backend_kind,"
                "locator_kind,native_id,active_native_id,creating_operation_id) VALUES(10,%s,5,0,1,5,43,43,%s)",
                (LINEAGE, overlap))
        execute("INSERT INTO economic_pending_claim_source VALUES(%s,2,%s,10,43,5,NULL)", (overlap,LINEAGE))
        execute("UPDATE auction_money_pickups SET money=%s WHERE pid=42", (8+5*len(batch_ids),))
        execute("INSERT INTO auction_money_pickups(pid,money,claim_revision) VALUES(43,5,1)")
        cut = captured()
        probe = pymysql.connect(**(settings | {"database": "duris_restore", "user": READER,
            "password": "plan5-disposable-reader", "cursorclass": pymysql.cursors.DictCursor}))
        try:
            with probe.cursor() as cursor:
                cursor.execute("START TRANSACTION WITH CONSISTENT SNAPSHOT, READ ONLY")
                from economic_sql_audit_snapshot import read_native
                native, _, _ = read_native(cursor, LINEAGE)
            assert native["pending_claim_source_coverage"]["rows"] == 68
            assert native["pending_claim_source_coverage"]["invalid_source_roots"] == 0
            assert native["pending_claim_source_coverage"]["invalid_account_mappings"] == 0
            from collections import Counter
            retained = Counter()
            for row in native["pending_claim_sources"]:
                assert row["claim_operation_id"] is None
                retained[row["account_key"]] += row["amount"]
            live = {row["account_key"]: row["balance"][0] for row in native["holdings"]
                    if bytes.fromhex(row["account_key"])[18:20] == b"\x05\x00"}
            assert retained == Counter(live)
            assert retained[second_key.hex()] == 5
            overlap_sources = [row for row in native["pending_claim_sources"]
                               if row["source_operation_id"] == overlap.hex()]
            assert len(overlap_sources) == 2 and all(row["source_root_valid"] for row in overlap_sources)
        finally:
            probe.rollback()
            probe.close()
        assert captured() == cut, "two-account SQL metadata probe changed authority"
    finally:
        for operation in batch_ids:
            execute("DELETE FROM economic_pending_claim_source WHERE source_operation_id=%s", (operation,))
        execute("DELETE FROM auction_money_pickups WHERE pid=43")
        execute("UPDATE auction_money_pickups SET money=%s WHERE pid=42", (original_cash[0],))
        execute("DELETE FROM economic_account_mapping WHERE mapping_id=10")
        for operation in batch_ids:
            for table in ("economic_accounting_coin_posting", "economic_accounting_account_effect",
                          "economic_accounting_operation", "critical_operation_inbox"):
                execute("DELETE FROM "+table+" WHERE operation_id=%s", (operation,))
    assert captured() == original, "two-account SQL metadata fixture was not restored"
    print("MODELED_CLAIM_METADATA_TWO_ACCOUNT_BATCH "+json.dumps({"source_rows": 68,
        "extra_distinct_pairs": 66, "pair_batch": 64, "accounts_in_overlap_root": 2,
        "projection_metadata_only": True, "original_capsule_identity_qualified": False,
        "claim_source_balance_projection_matches": True, "reader_authority_unchanged": True,
        "fixture_restored": True},sort_keys=True),flush=True)


def pending_claim_cuts():
    """Native codec roots with modeled retained allocations, never producer proof."""
    from test_economic_sql_canonical_audit import ClaimProjectionFixture
    from economic_restore_evidence import decode_plan
    path = Path(os.environ["DURIS_PLAN5_CLAIM_FIXTURE"]).resolve()
    assert path.parent == fixture_path.parent and path.name == "claims-sql.bin"
    raw = path.read_bytes()
    assert len(raw) <= 1024*1024
    blocks, offset = [], 0
    while offset < len(raw):
        size, = struct.unpack_from("<I", raw, offset)
        offset += 4
        blocks.append(raw[offset:offset+size])
        offset += size
    assert offset == len(raw) and len(blocks) == 10
    pairs = list(zip(blocks[::2], blocks[1::2]))
    initial, first_cut = captured(), len(canonical_cuts)
    for mode in ("unspent", "partial", "consumed", "whole"):
        model = ClaimProjectionFixture(mode, pairs)
        source_rows = [(bytes.fromhex(row[0]), row[1], bytes.fromhex(row[2]), *row[3:6],
                        bytes.fromhex(row[6]) if row[6] else None) for row in model.pending_sources]
        consumption_rows = [(bytes.fromhex(row[0]), bytes.fromhex(row[1]), *row[2:])
                            for row in model.pending_consumptions]
        operations = []
        try:
            for root in model.roots.values():
                plan = decode_plan(root.encoded)
                meta = plan["metadata"]
                operation = meta[2]
                execute("INSERT INTO critical_operation_inbox(operation_id,command_hash,keys_hash,"
                        "command_type,schema_version,payload_version,status,result_payload,committed_at) "
                        "VALUES(%s,%s,%s,1,2,1,1,'',CURRENT_TIMESTAMP(6))", (operation, bytes(32), bytes(32)))
                operations.append(operation)
                values = (operation, *meta[:2], meta[3] if any(meta[3]) else None, *meta[4:11], meta[11],
                          plan["intent_digest"], plan["domain_digest"], plan["plan_digest"],
                          root.frozen, root.encoded, 1, 0, *plan["counts"])
                execute("INSERT INTO economic_accounting_operation(operation_id,lineage,epoch,original_operation_id,"
                        "accounting_version,writer_id,policy_version,compiler_version,actor_kind,actor_id,reason,"
                        "source_event,intent_digest,domain_digest,plan_digest,canonical_intent,canonical_plan,"
                        "outcome,result_code,account_count,posting_count,child_count,before_witness_count,"
                        "after_witness_count,item_event_count) VALUES("+",".join(["%s"]*25)+")", values)
                execute("INSERT INTO economic_accounting_source_claim VALUES(%s,%s,%s,1)", (LINEAGE, meta[11], operation))
                for index, (key, before, after, old, new) in enumerate(plan["effects"]):
                    execute("INSERT INTO economic_accounting_account_effect VALUES("+",".join(["%s"]*13)+")",
                            (operation, index, key, *before, *after, old, new))
                for index, (event, account, child, delta, amount) in enumerate(plan["postings"]):
                    execute("INSERT INTO economic_accounting_coin_posting VALUES("+",".join(["%s"]*10)+")",
                            (operation, index, event, account, child, *delta, amount))
            execute("INSERT INTO economic_account_mapping(mapping_id,lineage,account_kind,context_id,backend_kind,"
                    "locator_kind,native_id,active_native_id,creating_operation_id) VALUES(9,%s,5,0,1,5,42,42,%s)",
                    (LINEAGE, bytes.fromhex("81"*16)))
            remaining = {"unspent": 8, "partial": 6, "consumed": 0, "whole": 0}[mode]
            execute("INSERT INTO auction_money_pickups(pid,money,claim_revision) VALUES(42,%s,1)", (remaining,))
            for row in source_rows:
                execute("INSERT INTO economic_pending_claim_source VALUES("+",".join(["%s"]*7)+")", row)
            for row in consumption_rows:
                execute("INSERT INTO economic_pending_claim_consumption VALUES(%s,%s,%s,%s)", row)
            canonical_cut("pending-claim-"+mode, [], [], None, full=True)
            if mode == "unspent":
                # Explicitly modeled SQL metadata tests the 64-pair collector
                # boundary. Copied capsules do NOT authenticate these new IDs;
                # this is not a qualifying canonical root/history cut.
                original_batch = captured()
                batch_ids = [value.to_bytes(16, "big") for value in range(128, 193)]
                with connection.cursor() as cursor:
                    cursor.execute("SELECT * FROM economic_accounting_operation WHERE operation_id=%s", (source_rows[0][0],))
                    values = list(cursor.fetchone())
                    columns = [column[0] for column in cursor.description]
                try:
                    for operation in batch_ids:
                        values[columns.index("operation_id")] = operation
                        execute("INSERT INTO critical_operation_inbox(operation_id,command_hash,keys_hash,"
                                "command_type,schema_version,payload_version,status,result_payload,committed_at) "
                                "VALUES(%s,%s,%s,1,2,1,1,'',CURRENT_TIMESTAMP(6))", (operation, bytes(32), bytes(32)))
                        execute("INSERT INTO economic_accounting_operation("+",".join(columns)+") VALUES("+
                                ",".join(["%s"]*len(columns))+")", values)
                        for table in ("economic_accounting_account_effect", "economic_accounting_coin_posting"):
                            with connection.cursor() as cursor:
                                cursor.execute("SELECT * FROM "+table+" WHERE operation_id=%s", (source_rows[0][0],))
                                copied = cursor.fetchall()
                            for row in copied:
                                execute("INSERT INTO "+table+" VALUES("+",".join(["%s"]*len(row))+")", (operation,*row[1:]))
                        execute("INSERT INTO economic_pending_claim_source VALUES(%s,1,%s,9,42,5,NULL)", (operation,LINEAGE))
                    probe = pymysql.connect(**(settings | {"database": "duris_restore", "user": READER,
                        "password": "plan5-disposable-reader", "cursorclass": pymysql.cursors.DictCursor}))
                    try:
                        with probe.cursor() as cursor:
                            cursor.execute("START TRANSACTION WITH CONSISTENT SNAPSHOT, READ ONLY")
                            from economic_sql_audit_snapshot import read_native
                            native, _, _ = read_native(cursor, LINEAGE)
                        assert native["pending_claim_source_coverage"]["rows"] == 67
                        assert native["pending_claim_source_coverage"]["invalid_source_roots"] == 0
                    finally:
                        probe.rollback()
                        probe.close()
                finally:
                    for operation in batch_ids:
                        execute("DELETE FROM economic_pending_claim_source WHERE source_operation_id=%s", (operation,))
                        for table in ("economic_accounting_coin_posting", "economic_accounting_account_effect",
                                      "economic_accounting_operation", "critical_operation_inbox"):
                            execute("DELETE FROM "+table+" WHERE operation_id=%s", (operation,))
                assert captured() == original_batch
                print("MODELED_CLAIM_METADATA_BATCH "+json.dumps({"source_rows": 67, "pair_batch": 64,
                    "projection_metadata_only": True, "original_capsule_identity_qualified": False,
                    "fixture_restored": True},sort_keys=True),flush=True)
                modeled_claim_two_account_batch(source_rows[0][0])
            if mode in ("consumed", "whole"):
                retiring = bytes.fromhex(("84" if mode == "consumed" else "85")*16)
                canonical_cut("pending-claim-"+mode+"-retired-mapping",
                    [("UPDATE economic_account_mapping SET active_native_id=NULL,retiring_operation_id=%s WHERE mapping_id=9", (retiring,))],
                    [("UPDATE economic_account_mapping SET active_native_id=42,retiring_operation_id=NULL WHERE mapping_id=9", None)], None, full=True)

            def cut(label, changes, repairs, code, broken_fk=False):
                canonical_cut("pending-claim-"+mode+"-"+label, changes, repairs,
                              "restore_economic_pending_claim_"+code+"_mismatch", broken_fk=broken_fk)

            original = source_rows[0]
            source = bytes.fromhex("81"*16)
            if mode == "unspent":
                cut("missing-source", [("DELETE FROM economic_pending_claim_source WHERE source_operation_id=%s", (source,))],
                    [("INSERT INTO economic_pending_claim_source VALUES("+",".join(["%s"]*7)+")", original)], "source")
                for field, changed, old in (("amount", 6, 5), ("lineage", EPOCH, LINEAGE), ("beneficiary_pid", 43, 42)):
                    query = "UPDATE economic_pending_claim_source SET "+field+"=%s WHERE source_operation_id=%s"
                    cut(field, [(query, (changed, source))], [(query, (old, source))], "source")
                cut("mapping-backend", [("UPDATE economic_account_mapping SET backend_kind=2 WHERE mapping_id=9", None)],
                    [("UPDATE economic_account_mapping SET backend_kind=1 WHERE mapping_id=9", None)], "source")
                cut("missing-mapping", [("DELETE FROM economic_account_mapping WHERE mapping_id=9", None)],
                    [("INSERT INTO economic_account_mapping(mapping_id,lineage,account_kind,context_id,backend_kind,"
                      "locator_kind,native_id,active_native_id,creating_operation_id) VALUES(9,%s,5,0,1,5,42,42,%s)",
                      (LINEAGE, source))], "source", broken_fk=True)
            if mode == "partial":
                spending = bytes.fromhex("83"*16)
                canonical_cut("pending-claim-partial-split-original-credit",
                    [("UPDATE economic_pending_claim_source SET amount=2 WHERE source_operation_id=%s", (source,)),
                     ("INSERT INTO economic_pending_claim_source VALUES(%s,2,%s,9,42,3,NULL)", (source, LINEAGE))],
                    [("DELETE FROM economic_pending_claim_source WHERE source_operation_id=%s AND source_slot=2", (source,)),
                     ("UPDATE economic_pending_claim_source SET amount=5 WHERE source_operation_id=%s", (source,))], None, full=True)
                for amount in (1, 3, 6):
                    query = "UPDATE economic_pending_claim_consumption SET amount=%s WHERE spending_operation_id=%s"
                    cut("amount-"+str(amount), [(query, (amount, spending))], [(query, (2, spending))], "consumption")
                cut("missing-consumption", [("DELETE FROM economic_pending_claim_consumption WHERE spending_operation_id=%s", (spending,))],
                    [("INSERT INTO economic_pending_claim_consumption VALUES(%s,%s,1,2)", (spending, source))], "consumption")
                cut("whole-and-partial", [("UPDATE economic_pending_claim_source SET claim_operation_id=%s WHERE source_operation_id=%s", (spending, source))],
                    [("UPDATE economic_pending_claim_source SET claim_operation_id=NULL WHERE source_operation_id=%s", (source,))], "consumption")
                cut("extra-consumption", [("INSERT INTO economic_pending_claim_consumption VALUES(%s,%s,1,1)", (OP, source))],
                    [("DELETE FROM economic_pending_claim_consumption WHERE spending_operation_id=%s", (OP,))], "consumption")
                cut("orphan-consumption", [("UPDATE economic_pending_claim_consumption SET source_operation_id=%s WHERE spending_operation_id=%s", (ORPHAN, spending))],
                    [("UPDATE economic_pending_claim_consumption SET source_operation_id=%s WHERE spending_operation_id=%s", (source, spending))], "consumption", broken_fk=True)
                cut("unattributed-consumption", [("UPDATE economic_pending_claim_consumption SET source_operation_id=%s,spending_operation_id=%s", (ORPHAN, ORPHAN))],
                    [("UPDATE economic_pending_claim_consumption SET source_operation_id=%s,spending_operation_id=%s", (source, spending))], "consumption", broken_fk=True)
                cut("rejected-consumer", [("UPDATE economic_pending_claim_consumption SET spending_operation_id=%s", (REJECTED,))],
                    [("UPDATE economic_pending_claim_consumption SET spending_operation_id=%s", (spending,))], "consumption")
                # Both retained-allocation cursors must traverse a second PK page
                # before rejecting these individually valid, extra source lots.
                extras = [(source, slot, LINEAGE, 9, 42, 5, None) for slot in range(2, 258)]
                changes = [("INSERT INTO economic_pending_claim_source VALUES("+",".join(["%s"]*7)+")", row)
                           for row in extras]
                changes += [("INSERT INTO economic_pending_claim_consumption VALUES(%s,%s,%s,1)",
                             (spending, source, row[1])) for row in extras]
                cut("paged-extra-allocations", changes,
                    [("DELETE FROM economic_pending_claim_consumption WHERE spending_operation_id=%s AND source_slot>1", (spending,)),
                     ("DELETE FROM economic_pending_claim_source WHERE source_operation_id=%s AND source_slot>1", (source,))], "source")
            if mode == "consumed":
                cut("lost-fully-consumed-source", [("DELETE FROM economic_pending_claim_source WHERE source_operation_id=%s", (source,))],
                    [("INSERT INTO economic_pending_claim_source VALUES("+",".join(["%s"]*7)+")", original)], "consumption", broken_fk=True)
            if mode == "whole":
                cut("wrong-whole-consumer", [("UPDATE economic_pending_claim_source SET claim_operation_id=%s WHERE source_operation_id=%s", (OP, source))],
                    [("UPDATE economic_pending_claim_source SET claim_operation_id=%s WHERE source_operation_id=%s", (bytes.fromhex("85"*16), source))], "consumption")
        finally:
            execute("DELETE FROM auction_money_pickups WHERE pid=42")
            execute("DELETE FROM economic_pending_claim_consumption")
            execute("DELETE FROM economic_pending_claim_source")
            execute("DELETE FROM economic_account_mapping WHERE mapping_id=9")
            for operation in operations:
                for table in ("economic_accounting_coin_posting", "economic_accounting_account_effect", "economic_accounting_source_claim",
                              "economic_accounting_operation", "critical_operation_inbox"):
                    execute("DELETE FROM "+table+" WHERE operation_id=%s", (operation,))
        assert captured() == initial, mode
    print("PENDING_CLAIM_RESTORE_CUTS "+json.dumps({"controls": 7, "cuts": len(canonical_cuts)-first_cut,
          "original_readers": 2, "native_roots": 5, "native_fixture_sha256": hashlib.sha256(raw).hexdigest(),
          "allocation_pagination_rows": {"sources": 258, "consumptions": 257},
          "schema_head": "0064_auction_custody_history", "authority_unchanged": True,
          "snapshot_allocation_reader": True,
          "producer_journey_qualified": False}, sort_keys=True), flush=True)


try:
    assert scalar("SELECT COUNT(*) FROM information_schema.schemata "
                  "WHERE schema_name='duris_restore'") == 0, "existing restore database refused"
    assert scalar(f"SELECT COUNT(*) FROM mysql.user WHERE User='{READER}' AND Host='localhost'") == 0
    execute("CREATE DATABASE duris_restore CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci")
    created = True
    connection.select_db("duris_restore")
    command = ["mysql", "--no-defaults", "--protocol=socket", "--socket=" + settings["unix_socket"],
               "--user=" + settings["user"], "duris_restore"]
    subprocess.run(command, input=(ROOT / "migrations/bootstrap_multithread_safe.sql").read_bytes(),
                   env=dict(os.environ, MYSQL_PWD=settings["password"]), check=True, timeout=180)
    manifest = migrations.load_manifest()
    assert manifest.migrations[-1].migration_id == "0064_auction_custody_history"
    executor = migrations.MysqlExecutor(manifest)
    try:
        executor.adopt("fresh_bootstrap")
        migrations.run_pending(manifest, executor)
    finally:
        executor.release_lock()
    print("restore fixture engine=" + scalar("SELECT VERSION()") +
          " migration_head=" + manifest.migrations[-1].migration_id, flush=True)
    # The audit uses SELECT-only credentials, including the actual main entry.
    execute(f"CREATE USER '{READER}'@'localhost' IDENTIFIED BY 'plan5-disposable-reader'")
    execute(f"GRANT SELECT ON duris_restore.* TO '{READER}'@'localhost'")
    reader = pymysql.connect(**(settings | {"database": "duris_restore", "user": READER,
                                          "password": "plan5-disposable-reader"}))
    try:
        with reader.cursor() as cursor:
            cursor.execute("UPDATE economic_lineage_state SET revision=revision")
    except pymysql.MySQLError as error:
        assert error.args[0] == 1142, error
    else:
        raise AssertionError("SELECT-only restore reader accepted UPDATE")
    os.environ["DB_USER"] = READER
    os.environ["DB_PASSWD"] = "plan5-disposable-reader"
    admitted()  # Inactive legacy/empty evidence remains eligible for restore.
    from test_economic_sql_canonical_audit import (NATIVE_MOBILE_STOCK_DAMAGE,
        native_mobile_forests, native_mobile_image, native_mobile_stock)
    insert_mobile = "INSERT INTO quest_mobile_native VALUES(%s,%s,%s,%s,%s)"
    remove_mobile = "DELETE FROM quest_mobile_native WHERE mobile_instance_id=%s"
    mobile_code = "restore_economic_native_mobile_mismatch"
    for version in (1, 2):
        for lifetime in (1, 2):
            stock = native_mobile_stock() if lifetime == 1 else bytes(4)
            image = native_mobile_image(version, lifetime, stock)
            canonical_cut(f"native-mobile-v{version}-state{lifetime}",
                [(insert_mobile, (42, 2, 3, lifetime, image))], [(remove_mobile, (42,))], None, full=True)
    image = native_mobile_image()
    for label, fields in (
            ("ID binding", (43, 2, 3, 1, image)), ("zero ID", (0, 2, 3, 1, image)),
            ("reserved ID", (2**64-1, 2, 3, 1, image)),
            ("mobile revision", (42, 3, 3, 1, image)), ("stock revision", (42, 2, 4, 1, image)),
            ("lifetime state", (42, 2, 3, 2, image)),
            ("zero mobile revision", (42, 0, 3, 1, image)), ("zero stock revision", (42, 2, 0, 1, image)),
            ("unknown lifetime", (42, 2, 3, 3, image)),
            ("corrupt body", (42, 2, 3, 1, b"corrupt-native-mobile-image")),
            ("checksum", (42, 2, 3, 1, image[:-1] + bytes([image[-1] ^ 1]))),
            ("truncated", (42, 2, 3, 1, image[:-1])),
            ("trailing", (42, 2, 3, 1, image + b"\0")),
            ("size bound", (42, 2, 3, 1, bytes(4*1024*1024+1)))):
        canonical_cut("native-mobile-" + label, [(insert_mobile, fields)],
                      [(remove_mobile, (fields[0],))], mobile_code, full=True)
    for label, offset, changed in NATIVE_MOBILE_STOCK_DAMAGE:
        stock = bytearray(native_mobile_stock())
        stock[offset:offset+len(changed)] = changed
        canonical_cut("native-mobile-stock-" + label,
            [(insert_mobile, (42, 2, 3, 1, native_mobile_image(items=bytes(stock))))],
            [(remove_mobile, (42,))], mobile_code, full=True)
    for label, valid, stock in native_mobile_forests():
        canonical_cut("native-mobile-forest-" + label,
            [(insert_mobile, (42, 2, 3, 1, native_mobile_image(items=stock)))],
            [(remove_mobile, (42,))], None if valid else mobile_code, full=True)
    highest = 2**64-2
    canonical_cut("native-mobile-highest-lifetime",
        [(insert_mobile, (highest, 2, 3, 1, native_mobile_image(identity=highest)))],
        [(remove_mobile, (highest,))], None, full=True)
    page = [(identity, 2, 3, 1, native_mobile_image(identity=identity)) for identity in range(1000, 1259)]
    canonical_cut("native-mobile-259-row-two-page-cut", [(insert_mobile, row) for row in page],
                  [(remove_mobile, (row[0],)) for row in page], None, full=True)
    page[-1] = (*page[-1][:4], page[-1][4][:-1] + bytes([page[-1][4][-1] ^ 1]))
    canonical_cut("native-mobile-second-page-corruption", [(insert_mobile, row) for row in page],
                  [(remove_mobile, (row[0],)) for row in page], mobile_code, full=True)
    print("NATIVE_MOBILE_RESTORE_CUTS " + json.dumps({
        "cuts": len(canonical_cuts), "original_readers": 2, "versions": 2,
        "page_rows": 259, "image_bound": 4*1024*1024,
        "authority_unchanged": True, "modeled_images_are_not_birth_authority": True}, sort_keys=True), flush=True)
    execute("INSERT INTO player_data(pid,name,copper,silver,gold,platinum,wallet_revision,epics,epic_revision) "
            "VALUES(42,'SyntheticRestore',7,0,0,0,1,0,0)")
    execute("INSERT INTO currency_wallet_baseline(pid,opening_copper,opening_silver,opening_gold,"
            "opening_platinum,opening_revision) VALUES(42,7,0,0,0,1)")
    execute("INSERT INTO epic_balance_baseline(pid,opening_balance,opening_revision) VALUES(42,0,0)")
    execute("INSERT INTO critical_operation_inbox(operation_id,command_hash,keys_hash,"
            "command_type,schema_version,payload_version,status,result_payload,committed_at) "
            "VALUES(%s,%s,%s,1,2,1,1,'',CURRENT_TIMESTAMP(6))", (OP, bytes(32), bytes(32)))
    execute("INSERT INTO economic_epoch(lineage,epoch,ordinal,transition_kind,transition_digest,"
            "creating_operation_id) VALUES(%s,%s,1,1,%s,%s)", (LINEAGE, EPOCH, bytes(32), OP))
    execute("INSERT INTO economic_lineage_state(lineage) VALUES(%s)", (LINEAGE,))
    execute("INSERT INTO economic_accounting_operation(operation_id,lineage,epoch,accounting_version,"
            "writer_id,policy_version,compiler_version,actor_kind,actor_id,reason,source_event,"
            "intent_digest,domain_digest,plan_digest,canonical_intent,canonical_plan,outcome,result_code,"
            "account_count,posting_count,child_count,item_event_count,before_witness_count,after_witness_count) "
            "VALUES(%s,%s,%s,1,1,1,1,%s,1,%s,%s,%s,%s,%s,%s,%s,1,0,2,2,0,0,0,0)",
             (OP, LINEAGE, EPOCH, native_blocks[1][72], struct.unpack_from("<H", native_blocks[1], 96)[0], SOURCE,
              hashlib.sha256(b"DURIS-ECONOMIC-INTENT-V1\0" + native_blocks[0]).digest(),
              native_blocks[0][192:224], hashlib.sha256(native_blocks[1]).digest(),
              native_blocks[0], native_blocks[1]))
    for index in range(2):
        effect = native_blocks[1][256 + index * 120:256 + (index + 1) * 120]
        execute("INSERT INTO economic_accounting_account_effect(operation_id,account_index,account_key,"
                "before_copper,before_silver,before_gold,before_platinum,after_copper,after_silver,"
                "after_gold,after_platinum,before_revision,after_revision) "
                "VALUES(" + ",".join(["%s"] * 13) + ")",
                (OP, index, effect[:40], *struct.unpack_from("<8qQQ", effect, 40)))
        execute("INSERT INTO economic_accounting_coin_posting VALUES(%s,%s,%s,%s,0,%s,0,0,0,%s)",
                (OP, index, index, index, 7 if index == 0 else -7, 7 if index == 0 else -7))
    execute("INSERT INTO economic_accounting_source_claim VALUES(%s,%s,%s,1)", (LINEAGE, SOURCE, OP))
    execute("INSERT INTO critical_operation_inbox(operation_id,command_hash,keys_hash,"
            "command_type,schema_version,payload_version,status,result_code,result_payload,committed_at) "
            "VALUES(%s,%s,%s,1,2,1,1,211,'',CURRENT_TIMESTAMP(6))", (REJECTED, bytes(32), bytes(32)))
    execute("INSERT INTO economic_accounting_operation(operation_id,lineage,epoch,accounting_version,"
            "writer_id,policy_version,compiler_version,actor_kind,actor_id,reason,source_event,"
            "intent_digest,domain_digest,outcome,result_code,canonical_intent,"
            "account_count,posting_count,child_count,item_event_count,before_witness_count,after_witness_count) "
            "VALUES(%s,%s,%s,1,1,1,1,1,1,18,%s,%s,%s,2,211,%s,0,0,0,0,0,0)",
             (REJECTED, LINEAGE, EPOCH, native_blocks[4][112:160],
              hashlib.sha256(b"DURIS-ECONOMIC-INTENT-V1\0" + native_blocks[4]).digest(),
              native_blocks[4][192:224], native_blocks[4]))
    before = captured()
    old_money_checks()
    admitted()
    # Valid ordinary capsules cannot replace their retained lifecycle namespace.
    # The fixture owner models a damaged import; every reader keeps FK checks on.
    damaged("DELETE FROM economic_lineage_state WHERE lineage=%s",
            "INSERT INTO economic_lineage_state(lineage) VALUES(%s)",
            "restore_economic_lineage_mismatch", (LINEAGE,))
    epoch_created = scalar("SELECT created_at FROM economic_epoch WHERE lineage=X'" + LINEAGE.hex() +
                           "' AND epoch=X'" + EPOCH.hex() + "'")
    damaged("DELETE FROM economic_epoch WHERE lineage=%s AND epoch=%s",
            "INSERT INTO economic_epoch(lineage,epoch,ordinal,transition_kind,transition_digest,"
            "creating_operation_id,created_at) VALUES(%s,%s,1,1,%s,%s,%s)",
            "restore_economic_epoch_mismatch", (LINEAGE, EPOCH),
            (LINEAGE, EPOCH, bytes(32), OP, epoch_created), broken_fk=True)
    assert captured() == before
    if os.environ.get("DURIS_PLAN5_CANONICAL_EVIDENCE") == "1":
        assert native_blocks, "canonical checks require actual native capsules"
        damaged("UPDATE economic_accounting_operation SET canonical_intent=CONCAT(UNHEX('00'),"
                "SUBSTRING(canonical_intent,2)) WHERE operation_id=%s",
                "UPDATE economic_accounting_operation SET canonical_intent=%s WHERE operation_id=%s",
                "restore_economic_intent_mismatch", (OP,), (native_blocks[0], OP))
        if os.environ.get("DURIS_PLAN5_CANONICAL_RED") == "1":
            raise SystemExit(0)
    execute("DELETE FROM economic_accounting_coin_posting WHERE operation_id=%s AND line_index=1", (OP,))
    old_money_checks()  # Native money still agrees; the retained root is damaged.
    refused("restore_economic_posting_count_mismatch")
    execute("INSERT INTO economic_accounting_coin_posting VALUES(%s,1,1,1,0,-7,0,0,0,-7)", (OP,))
    admitted()
    assert captured() == before
    print("refused without mutation: restore_economic_posting_count_mismatch", flush=True)
    for field, family in (("account_count", "account"), ("child_count", "child"),
                          ("item_event_count", "item")):
        original = 2 if field == "account_count" else 0
        damaged(f"UPDATE economic_accounting_operation SET {field}={original + 1} WHERE operation_id=%s",
                f"UPDATE economic_accounting_operation SET {field}={original} WHERE operation_id=%s",
                f"restore_economic_{family}_count_mismatch", (OP,))
    damaged("UPDATE economic_accounting_account_effect SET account_index=2 WHERE account_index=1",
            "UPDATE economic_accounting_account_effect SET account_index=1 WHERE account_index=2",
            "restore_economic_account_index_mismatch", broken_fk=True)
    damaged("UPDATE economic_accounting_coin_posting SET line_index=2,event_index=2 WHERE line_index=1",
            "UPDATE economic_accounting_coin_posting SET line_index=1,event_index=1 WHERE line_index=2",
            "restore_economic_posting_index_mismatch")
    damaged("UPDATE critical_operation_inbox SET committed_at=NULL WHERE operation_id=%s",
            "UPDATE critical_operation_inbox SET committed_at=%s WHERE operation_id=%s",
            "restore_economic_receipt_mismatch", (OP,),
            (scalar("SELECT committed_at FROM critical_operation_inbox WHERE operation_id=X'" + OP.hex() + "'"), OP))
    damaged("UPDATE critical_operation_inbox SET result_code=0 WHERE operation_id=%s",
            "UPDATE critical_operation_inbox SET result_code=211 WHERE operation_id=%s",
            "restore_economic_receipt_mismatch", (REJECTED,))
    damaged("DELETE FROM economic_accounting_source_claim WHERE operation_id=%s",
            "INSERT INTO economic_accounting_source_claim VALUES(%s,%s,%s,1)",
            "restore_economic_source_claim_missing", (OP,), (LINEAGE, SOURCE, OP))
    # A source claim belonging to no root is distinct from a missing claim.
    damaged("INSERT INTO economic_accounting_source_claim VALUES(%s,%s,%s,1)",
            "DELETE FROM economic_accounting_source_claim WHERE operation_id=%s",
            "restore_economic_source_claim_mismatch", (LINEAGE, bytes.fromhex("88" * 48), ORPHAN),
            (ORPHAN,), broken_fk=True)
    damaged("UPDATE economic_accounting_coin_posting SET account_index=2 WHERE line_index=1",
            "UPDATE economic_accounting_coin_posting SET account_index=1 WHERE line_index=1",
            "restore_economic_posting_account_missing", broken_fk=True)
    damaged("UPDATE economic_accounting_coin_posting SET copper_value=-6 WHERE line_index=1",
            "UPDATE economic_accounting_coin_posting SET copper_value=-7 WHERE line_index=1",
            "restore_economic_posting_value_mismatch")
    damaged("UPDATE economic_accounting_coin_posting SET delta_platinum=9223372036854775807 WHERE line_index=1",
            "UPDATE economic_accounting_coin_posting SET delta_platinum=0 WHERE line_index=1",
            "restore_economic_posting_value_mismatch")
    damaged("UPDATE economic_accounting_coin_posting SET delta_copper=-6,copper_value=-6 WHERE line_index=1",
            "UPDATE economic_accounting_coin_posting SET delta_copper=-7,copper_value=-7 WHERE line_index=1",
            "restore_economic_root_unbalanced")
    damaged("UPDATE economic_accounting_account_effect SET after_copper=6 WHERE account_index=0",
            "UPDATE economic_accounting_account_effect SET after_copper=7 WHERE account_index=0",
            "restore_economic_account_delta_mismatch")
    execute("INSERT INTO critical_operation_inbox(operation_id,command_hash,keys_hash,"
            "command_type,schema_version,payload_version,status,result_payload,committed_at) "
            "VALUES(%s,%s,%s,1,2,1,1,'',CURRENT_TIMESTAMP(6))", (CHILD, bytes(32), bytes(32)))
    execute("INSERT INTO economic_accounting_child VALUES(%s,1,%s,1,1,0,1,%s)", (OP, CHILD, CHILD))
    execute("INSERT INTO item_ownership_ledger(operation_id,event_index,item_uid,root_item_uid,"
            "from_owner_type,from_owner_id,from_owner_context_id,to_owner_type,to_owner_id,"
            "to_owner_context_id,item_revision,from_owner_revision,to_owner_revision,reason_type,source_site) "
            "VALUES(%s,0,99,99,1,42,0,3,4,0,2,1,2,1,1)", (CHILD,))
    execute("INSERT INTO economic_accounting_item_reference VALUES(%s,0,0,1,99,1,2,%s,0)", (OP, CHILD))
    execute("UPDATE economic_accounting_operation SET child_count=1,item_event_count=1 WHERE operation_id=%s", (OP,))
    execute("UPDATE economic_accounting_coin_posting SET child_index=1")
    if native_blocks:
        execute("UPDATE economic_accounting_operation SET canonical_plan=%s,plan_digest=%s,"
                "before_witness_count=1,after_witness_count=1 WHERE operation_id=%s",
                (native_blocks[2], hashlib.sha256(native_blocks[2]).digest(), OP))
    admitted()
    for table, family in (("economic_accounting_account_effect", "account"),
                          ("economic_accounting_coin_posting", "posting"),
                          ("economic_accounting_child", "child"),
                          ("economic_accounting_item_reference", "item")):
        damaged(f"UPDATE {table} SET operation_id=%s WHERE operation_id=%s",
                f"UPDATE {table} SET operation_id=%s WHERE operation_id=%s",
                f"restore_economic_orphan_{family}", (ORPHAN, OP), (OP, ORPHAN), broken_fk=True)
    damaged("DELETE FROM economic_accounting_child WHERE operation_id=%s",
            "INSERT INTO economic_accounting_child VALUES(%s,1,%s,1,1,0,1,%s)",
            "restore_economic_child_count_mismatch", (OP,), (OP, CHILD, CHILD))
    damaged("DELETE FROM economic_accounting_item_reference WHERE operation_id=%s",
            "INSERT INTO economic_accounting_item_reference VALUES(%s,0,0,1,99,1,2,%s,0)",
            "restore_economic_item_count_mismatch", (OP,), (OP, CHILD))
    damaged("UPDATE critical_operation_inbox SET status=0 WHERE operation_id=%s",
            "UPDATE critical_operation_inbox SET status=1 WHERE operation_id=%s",
            "restore_economic_child_link_mismatch", (CHILD,))
    damaged("UPDATE economic_accounting_coin_posting SET child_index=2 WHERE line_index=1",
            "UPDATE economic_accounting_coin_posting SET child_index=1 WHERE line_index=1",
            "restore_economic_child_evidence_missing")
    damaged("UPDATE economic_accounting_item_reference SET before_revision=0",
            "UPDATE economic_accounting_item_reference SET before_revision=1",
            "restore_economic_item_link_mismatch")
    damaged("UPDATE economic_accounting_item_reference SET legacy_operation_id=%s",
            "UPDATE economic_accounting_item_reference SET legacy_operation_id=%s",
            "restore_economic_item_link_mismatch", (OP,), (CHILD,), broken_fk=True)
    damaged("UPDATE item_ownership_ledger SET item_revision=3",
            "UPDATE item_ownership_ledger SET item_revision=2",
            "restore_economic_item_link_mismatch", broken_fk=True)
    try:
        execute("SET SESSION FOREIGN_KEY_CHECKS=0")
        execute("UPDATE item_ownership_ledger SET item_revision=18446744073709551615")
        execute("UPDATE economic_accounting_item_reference SET before_revision=18446744073709551614,"
                "after_revision=18446744073709551615")
    finally:
        execute("SET SESSION FOREIGN_KEY_CHECKS=1")
    if native_blocks:
        execute("UPDATE economic_accounting_operation SET canonical_plan=%s,plan_digest=%s WHERE operation_id=%s",
                (native_blocks[3], hashlib.sha256(native_blocks[3]).digest(), OP))
    admitted()  # Full uint64 counters are compared without subtraction wrap.
    if os.environ.get("DURIS_PLAN5_CANONICAL_EVIDENCE") == "1":
        prefix = "restore_economic_"
        for field in ("intent_digest", "domain_digest", "plan_digest"):
            canonical_field("economic_accounting_operation", field, bytes([99]) * 32,
                            prefix + ("plan" if field == "plan_digest" else "intent") + "_mismatch")
        canonical_field("economic_accounting_operation", "canonical_intent", bytes(256),
                        prefix + "intent_mismatch", operation=REJECTED)
        for field, value in (("writer_id", 2), ("policy_version", 2), ("compiler_version", 2),
                             ("actor_kind", 2), ("actor_id", 2), ("reason", 39),
                             ("original_operation_id", OP)):
            canonical_field("economic_accounting_operation", field, value, prefix + "metadata_mismatch")
        for field in ("account_count", "posting_count", "child_count", "before_witness_count",
                      "after_witness_count", "item_event_count"):
            value = scalar("SELECT " + field + " FROM economic_accounting_operation WHERE operation_id=UNHEX('" + OP.hex() + "')")
            canonical_field("economic_accounting_operation", field, value + 1,
                            prefix + "canonical_count_mismatch", full=field == "before_witness_count")
        canonical_field("economic_accounting_account_effect", "account_key",
                        LINEAGE + struct.pack("<HHQQ4x", 1, 1, 8, 0),
                        prefix + "canonical_account_mismatch", " AND account_index=0")
        canonical_field("economic_accounting_account_effect", "after_revision", 2,
                        prefix + "canonical_account_mismatch", " AND account_index=0")
        canonical_field("economic_accounting_coin_posting", "delta_copper", 8,
                        prefix + "canonical_posting_mismatch", " AND line_index=0")
        canonical_field("economic_accounting_coin_posting", "child_index", 0,
                        prefix + "canonical_posting_mismatch", " AND line_index=0")
        for field in ("domain_id", "discriminator"):
            canonical_field("economic_accounting_child", field, 2,
                            prefix + "canonical_child_mismatch", full=field == "domain_id")
        canonical_field("economic_accounting_item_reference", "before_revision", 1,
                        prefix + "canonical_item_mismatch")
        for field, value in (("root_item_uid", 98), ("parent_item_uid", 99),
                             ("from_owner_id", 43), ("to_owner_type", 2),
                             ("to_owner_id", 5), ("from_equipment_slot", 1), ("to_equipment_slot", 1)):
            canonical_field("item_ownership_ledger", field, value, prefix + "canonical_custody_mismatch",
                            full=field == "root_item_uid", operation=CHILD)
        # These are owner aggregate counters, not EAP1 item revisions.
        canonical_field("item_ownership_ledger", "from_owner_revision", 123, None, operation=CHILD)
        plan = native_blocks[3]
        for label, changed in (("truncated-plan", plan[:-1]), ("trailing-plan", plan + bytes(1)),
                               ("plan-4MiB-bound", plan + bytes(4 * 1024 * 1024 - len(plan)))):
            query = "UPDATE economic_accounting_operation SET canonical_plan=%s,plan_digest=%s WHERE operation_id=%s"
            canonical_cut(label, [(query, (changed, hashlib.sha256(changed).digest(), OP))],
                          [(query, (plan, hashlib.sha256(plan).digest(), OP))], prefix + "plan_mismatch")
        changes = [("UPDATE economic_accounting_account_effect SET after_copper=%s WHERE operation_id=%s AND account_index=0", (8, OP)),
                   ("UPDATE economic_accounting_account_effect SET before_copper=%s WHERE operation_id=%s AND account_index=1", (8, OP)),
                   ("UPDATE economic_accounting_coin_posting SET delta_copper=%s,copper_value=%s WHERE operation_id=%s AND line_index=0", (8, 8, OP)),
                   ("UPDATE economic_accounting_coin_posting SET delta_copper=%s,copper_value=%s WHERE operation_id=%s AND line_index=1", (-8, -8, OP))]
        repairs = [(query, tuple(7 if value == 8 else -7 if value == -8 else value for value in params))
                   for query, params in changes]
        canonical_cut("balanced-forged-projections", changes, repairs,
                      prefix + "canonical_account_mismatch", full=True)
        oracle_rows = [json.loads(line) for line in Path(fixture).with_name("native-decode-sql.jsonl").read_bytes().splitlines()]
        maximum = fixture_path.with_name("history-max-intent.bin").read_bytes()
        assert len(maximum) == 8192 and maximum[:4] == b"EAI1"
        maximum_digest = hashlib.sha256(b"DURIS-ECONOMIC-INTENT-V1\0" + maximum).digest()
        paired = plan[:152] + maximum_digest + plan[184:]
        query = "UPDATE economic_accounting_operation SET canonical_intent=%s,intent_digest=%s,canonical_plan=%s,plan_digest=%s WHERE operation_id=%s"
        canonical_cut("native-intent-8192-bound", [(query, (maximum, maximum_digest, paired, hashlib.sha256(paired).digest(), OP))],
                      [(query, (native_blocks[0], hashlib.sha256(b"DURIS-ECONOMIC-INTENT-V1\0" + native_blocks[0]).digest(),
                                plan, hashlib.sha256(plan).digest(), OP))], None)
        # Traverse more than one 256-ID page using native zero-effect capsules.
        # This is a quiescent structural cut, not a real writer workload or watermark.
        empty_intent = bytes.fromhex(next(row["bytes"] for row in oracle_rows if row["name"] == "no-source-intent"))
        empty_plan = bytes.fromhex(next(row["bytes"] for row in oracle_rows if row["name"] == "no-source-plan"))
        page_original = captured()
        added = []
        try:
            for value in range(1, 258):
                operation = value.to_bytes(16, "big")
                frozen = empty_intent[:64] + operation + empty_intent[80:]
                intent_digest = hashlib.sha256(b"DURIS-ECONOMIC-INTENT-V1\0" + frozen).digest()
                encoded = empty_plan[:40] + operation + empty_plan[56:152] + intent_digest + empty_plan[184:]
                execute("INSERT INTO critical_operation_inbox(operation_id,command_hash,keys_hash,command_type,schema_version,"
                        "payload_version,status,result_payload,committed_at) VALUES(%s,%s,%s,1,2,1,1,'',CURRENT_TIMESTAMP(6))",
                        (operation, bytes(32), bytes(32)))
                added.append(operation)
                execute("INSERT INTO economic_accounting_operation(operation_id,lineage,epoch,accounting_version,writer_id,"
                        "policy_version,compiler_version,actor_kind,actor_id,reason,intent_digest,domain_digest,plan_digest,"
                        "canonical_intent,canonical_plan,outcome,result_code,account_count,posting_count,child_count,"
                        "item_event_count,before_witness_count,after_witness_count) "
                        "VALUES(%s,%s,%s,1,1,1,1,1,1,32,%s,%s,%s,%s,%s,1,0,0,0,0,0,0,0)",
                        (operation, LINEAGE, EPOCH, intent_digest, frozen[192:224], hashlib.sha256(encoded).digest(), frozen, encoded))
            canonical_cut("259-root-two-page-cut", [], [], None, full=True)
            canonical_field("economic_accounting_operation", "canonical_intent", bytes(256),
                            prefix + "intent_mismatch", full=True, operation=added[-1])
        finally:
            for operation in added:
                execute("DELETE FROM economic_accounting_operation WHERE operation_id=%s", (operation,))
                execute("DELETE FROM critical_operation_inbox WHERE operation_id=%s", (operation,))
        assert captured() == page_original
        admitted()
        if os.environ.get("DURIS_PLAN5_CLAIM_FIXTURE"):
            pending_claim_cuts()
        print("CANONICAL_RESTORE_QUALIFIED " + json.dumps({"cuts": len(canonical_cuts),
              "refusals": sum(row["code"] is not None for row in canonical_cuts),
              "full_entry_cuts": sum(row["full_entry"] for row in canonical_cuts),
              "page_roots": 259, "intent_bound": 8192, "plan_bound": 4 * 1024 * 1024,
              "authority_unchanged": True, "schema_head": "0064_auction_custody_history",
              "production_access": False}, sort_keys=True), flush=True)
    if native_blocks:
        cases = [json.loads(block) for block in native_blocks[5:]]
        qualified, constrained = 0, 0
        for case in cases:
            original = captured()
            connection.begin()
            try:
                # The disposable owner publishes a test cut before the
                # SELECT-only reader starts its consistent transaction.
                execute("DELETE FROM economic_accounting_coin_posting WHERE operation_id=%s", (OP,))
                execute("DELETE FROM economic_accounting_account_effect WHERE operation_id=%s", (OP,))
                for index, effect in enumerate(case["effects"]):
                    execute("INSERT INTO economic_accounting_account_effect VALUES(" + ",".join(["%s"] * 13) + ")",
                            (OP, index, bytes.fromhex(effect["account_key"]), *effect["before"], *effect["after"],
                             effect["before_revision"], effect["after_revision"]))
                try:
                    for index, post in enumerate(case["postings"]):
                        execute("INSERT INTO economic_accounting_coin_posting VALUES(" + ",".join(["%s"] * 10) + ")",
                                (OP, index, post["event_index"], post["account_index"], post["child_index"],
                                 *post["delta"], post["copper"]))
                except pymysql.MySQLError as error:
                    assert case["name"] in ("sparse-events", "duplicate-events")
                    assert error.args[0] in (1062, 3819, 4025), error
                    assert not case["accepted"], case["name"]
                    constrained += 1
                    print("COIN_SQL_CONSTRAINT " + case["name"] + " native_accepted=" + str(case["accepted"]), flush=True)
                    continue
                connection.commit()
                cut = captured()
                try:
                    with reader.cursor() as cursor:
                        cursor.execute("SET TRANSACTION ISOLATION LEVEL REPEATABLE READ")
                        cursor.execute("START TRANSACTION WITH CONSISTENT SNAPSHOT, READ ONLY")
                    qualifier.require_economic_coin_effect_integrity(Executor())
                except RuntimeError as error:
                    assert not case["accepted"], (case["name"], str(error))
                else:
                    assert case["accepted"], case["name"]
                finally:
                    reader.rollback()
                assert captured() == cut
                qualified += 1
            finally:
                connection.rollback()
                connection.begin()
                execute("DELETE FROM economic_accounting_coin_posting WHERE operation_id=%s", (OP,))
                execute("DELETE FROM economic_accounting_account_effect WHERE operation_id=%s", (OP,))
                for row in original["economic_accounting_account_effect"]:
                    execute("INSERT INTO economic_accounting_account_effect VALUES(" + ",".join(["%s"] * 13) + ")", row)
                for row in original["economic_accounting_coin_posting"]:
                    execute("INSERT INTO economic_accounting_coin_posting VALUES(" + ",".join(["%s"] * 10) + ")", row)
                connection.commit()
                assert captured() == original
        assert qualified == 30 and constrained == 2
        print("COIN_RESTORE_QUALIFIED " + json.dumps({"native_cases": 32, "audited_cases": qualified,
              "canonical_constraint_refusals": constrained, "schema_head": "0064_auction_custody_history",
              "authority_unchanged": True, "production_access": False}, sort_keys=True), flush=True)
    print("economic restore: intact/inactive/rejected histories pass; damaged retained rows, "
          "receipts, sources, values and item links refuse with SELECT-only unchanged authority", flush=True)
finally:
    if reader is not None:
        reader.close()
    if created:
        execute("DROP DATABASE duris_restore")
        execute(f"DROP USER IF EXISTS '{READER}'@'localhost'")
    connection.close()
