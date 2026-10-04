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
    assert fixture_path.is_relative_to((ROOT / "bin/tests/plan5-sql-coin-effects").resolve())
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
          "epic_balance_baseline", "mud_schema_migrations", "mud_schema_migration_state")


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
            return str(cursor.fetchone()[0])


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
            "VALUES(%s,%s,%s,1,1,1,1,2,1,38,%s,%s,%s,%s,%s,%s,1,0,2,2,0,0,0,0)",
            (OP, LINEAGE, EPOCH, SOURCE,
             hashlib.sha256(b"DURIS-ECONOMIC-INTENT-V1\0" + native_blocks[0]).digest() if native_blocks else bytes(32),
             native_blocks[0][192:224] if native_blocks else bytes(32),
             hashlib.sha256(native_blocks[1]).digest() if native_blocks else bytes(32),
             native_blocks[0] if native_blocks else bytes(256), native_blocks[1] if native_blocks else bytes(256)))
    keys = [LINEAGE + struct.pack("<HHQQ4x", 1, kind, lifetime, 0)
            for kind, lifetime in ((1, 7), (9, 1))]
    for index, amount in ((0, 7), (1, 0)):
        execute("INSERT INTO economic_accounting_account_effect(operation_id,account_index,account_key,"
                "before_copper,before_silver,before_gold,before_platinum,after_copper,after_silver,"
                "after_gold,after_platinum,before_revision,after_revision) "
                "VALUES(%s,%s,%s,0,0,0,0,%s,0,0,0,0,%s)", (OP, index, keys[index], amount, int(index == 0)))
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
            (REJECTED, LINEAGE, EPOCH, native_blocks[4][112:160] if native_blocks else None,
             hashlib.sha256(b"DURIS-ECONOMIC-INTENT-V1\0" + native_blocks[4]).digest() if native_blocks else bytes(32),
             native_blocks[4][192:224] if native_blocks else bytes(32), native_blocks[4] if native_blocks else bytes(256)))
    before = captured()
    old_money_checks()
    admitted()
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
            "VALUES(%s,0,99,99,1,42,0,2,4,0,2,1,2,1,1)", (CHILD,))
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
              "canonical_constraint_refusals": constrained, "schema_head": "0056_spell_ward_durability",
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
