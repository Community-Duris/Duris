#!/usr/bin/env python3
"""Read native baseline claims from a freshly bootstrapped private socket."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

import pymysql

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
import economic_sql_audit_snapshot as exporter
import migration_runner as migrations
from reconcile_economy_accounting import Reconciler
from economic_sql_audit_origins import OriginError

if (os.environ.get("TEST_DB_DISPOSABLE") != "1" or os.environ.get("ENVIRONMENT") != "test" or
        os.environ.get("DB_HOST") != "127.0.0.1" or os.environ.get("DB_NAME") != "duris_restore" or
        not os.environ.get("DB_SOCKET", "").startswith("/plan5-restore-baseline-")):
    raise SystemExit("fresh private baseline daemon/socket required")
fixture = Path(os.environ["DURIS_PLAN5_BASELINE_FIXTURE"]).resolve()
assert fixture.is_relative_to((ROOT / "bin/tests/plan5-native-baseline-audit").resolve())
settings = dict(unix_socket=os.environ["DB_SOCKET"], user=os.environ["DB_USER"],
                password=os.environ["DB_PASSWD"], database="duris_restore", autocommit=True,
                cursorclass=pymysql.cursors.DictCursor, read_timeout=30, write_timeout=30)
owner = pymysql.connect(**settings)
reader = None
TABLES = ("economic_accounting_operation", "economic_accounting_account_effect",
          "economic_accounting_coin_posting", "economic_accounting_source_claim",
          "economic_baseline_control", "economic_baseline_witness", "economic_baseline_reservation",
          "critical_operation_inbox", "economic_epoch", "economic_lineage_state",
          "player_data", "account_banks", "item_current_owner", "item_ownership_ledger", "currency_ledger")


def execute(query, params=()):
    with owner.cursor() as cursor:
        cursor.execute(query, params)


def captured():
    with owner.cursor() as cursor:
        result = {}
        for table in TABLES:
            cursor.execute("SELECT * FROM " + table + " ORDER BY 1")
            result[table] = cursor.fetchall()
        return result


try:
    with owner.cursor() as cursor:
        cursor.execute("SELECT COUNT(*) AS n FROM information_schema.tables WHERE table_schema='duris_restore'")
        assert cursor.fetchone()["n"] == 0
    subprocess.run(["mysql", "--no-defaults", "--protocol=socket", "--socket=" + settings["unix_socket"],
                    "--user=" + settings["user"], "duris_restore"],
                   input=(ROOT / "migrations/bootstrap_multithread_safe.sql").read_bytes(), check=True, timeout=180)
    manifest = migrations.load_manifest()
    executor = migrations.MysqlExecutor(manifest)
    try:
        executor.adopt("fresh_bootstrap")
        migrations.run_pending(manifest, executor)
        assert manifest.migrations[-1].migration_id == "0056_spell_ward_durability"
    finally:
        executor.release_lock()
    with owner.cursor() as cursor:
        cursor.execute("SELECT VERSION() AS version")
        print("native baseline engine=" + cursor.fetchone()["version"] + " migration_head=0056_spell_ward_durability", flush=True)
    initial = captured()
    encoded = subprocess.check_output([str(fixture)], env=dict(os.environ), timeout=120)
    native = json.loads(encoded)
    print("NATIVE_BASELINE_PUBLISHED " + encoded.decode().strip(), flush=True)
    for table in ("player_data", "account_banks", "item_current_owner", "item_ownership_ledger", "currency_ledger"):
        assert captured()[table] == initial[table], table
    with owner.cursor() as cursor:
        cursor.execute("SELECT active_epoch FROM economic_lineage_state")
        assert cursor.fetchone()["active_epoch"] is None
    execute("CREATE USER 'plan5_baseline_reader'@'localhost' IDENTIFIED BY 'plan5-private-baseline-reader'")
    execute("GRANT SELECT ON duris_restore.* TO 'plan5_baseline_reader'@'localhost'")
    reader = pymysql.connect(**(settings | dict(user="plan5_baseline_reader", password="plan5-private-baseline-reader")))
    try:
        with reader.cursor() as cursor:
            cursor.execute("UPDATE economic_lineage_state SET active_epoch=NULL")
    except pymysql.MySQLError as error:
        assert error.args[0] == 1142
    else:
        raise AssertionError("SELECT-only reader admitted UPDATE")

    def audit():
        for connection in (owner, reader):
            with connection.cursor() as cursor:
                cursor.execute("SELECT @@SESSION.foreign_key_checks AS enabled")
                assert cursor.fetchone()["enabled"] == 1
        before = captured()
        snapshot = exporter.capture(reader, bytes.fromhex(native["lineage"]), bytes.fromhex(native["epochs"][-1]))
        report = Reconciler().audit(snapshot)
        assert captured() == before
        return snapshot, report["exception_counts"]

    snapshot, counts = audit()
    print("NATIVE_BASELINE_INITIAL_COUNTS " + json.dumps(counts, sort_keys=True), flush=True)
    assert len(snapshot["source_claims"]) == 2
    assert counts.get("baseline_source_claim", 0) == 0, "native baseline claims were falsely rejected"
    base_counts = {"evidence_loss": 1, "missing_native_holding": 2, "missing_native_item": 2}
    assert counts == base_counts, counts
    assert snapshot["source_claim_coverage"] == {
        "source_operations": 2, "missing_claim_operations": 0, "duplicate_source_values": 0}
    assert snapshot["source_event_policy_coverage"] == {
        "required_committed_operations": 2, "missing_required_source_events": 0}
    assert all(row["baseline_witness"]["source_event"] == row["source_event"]
               for row in snapshot["source_claims"])
    intact = captured()
    cuts = []
    constraints = []

    def constrained(label, query, params, error_code):
        original = captured()
        try:
            execute(query, params)
        except pymysql.IntegrityError as error:
            assert error.args[0] == error_code, (label, error.args)
        else:
            raise AssertionError(label + ": canonical constraint admitted corruption")
        assert captured() == original
        constraints.append({"label": label, "error_code": error_code})
        print("BASELINE_SQL_CONSTRAINT " + json.dumps(constraints[-1], sort_keys=True), flush=True)

    def fixture_changes(changes, broken_fk):
        # The private owner models a damaged import. Normal constraints remain
        # enabled for every reader invocation and every native owner replay.
        if broken_fk:
            execute("SET SESSION FOREIGN_KEY_CHECKS=0")
        try:
            for query, params in changes:
                execute(query, params)
        finally:
            if broken_fk:
                execute("SET SESSION FOREIGN_KEY_CHECKS=1")

    def cut(label, changes, repairs, findings=None, refusal=None, broken_fk=False):
        original = captured()
        try:
            fixture_changes(changes, broken_fk)
            damaged = captured()
            try:
                _, observed = audit()
            except OriginError as error:
                assert refusal is not None and refusal in str(error), (label, str(error), refusal)
            else:
                assert refusal is None and observed == base_counts | findings, (label, observed, findings)
            assert captured() == damaged, label + ": reader changed authority"
        finally:
            fixture_changes(repairs, broken_fk)
        assert captured() == original, label + ": disposable fixture was not restored"
        cuts.append({"label": label, "findings": findings, "refusal": refusal,
                     "damaged_import": broken_fk})
        print("BASELINE_CUT " + json.dumps(cuts[-1], sort_keys=True), flush=True)

    def field(label, table, column, operation, changed, findings=None, refusal=None, broken_fk=False):
        with owner.cursor() as cursor:
            cursor.execute("SELECT * FROM " + table + " WHERE operation_id=%s", (operation,))
            rows = cursor.fetchall()
            assert len(rows) == 1
            previous = rows[0][column]
        where, identity = "operation_id=%s", (operation,)
        if column == "operation_id":
            where, identity = "lineage=%s AND source_event=%s", (rows[0]["lineage"], rows[0]["source_event"])
        query = "UPDATE " + table + " SET " + column + "=%s WHERE " + where
        cut(label, [(query, (changed, *identity))], [(query, (previous, *identity))],
            findings, refusal, broken_fk)

    old, selected = [bytes.fromhex(value) for value in native["operations"]]
    old_epoch = bytes.fromhex(native["epochs"][0])
    lineage = bytes.fromhex(native["lineage"])
    claims = {row["operation_id"]: row for row in intact["economic_accounting_source_claim"]}
    for operation, label in ((old, "retained-claim-missing"), (selected, "selected-claim-missing")):
        claim = claims[operation]
        cut(label, [("DELETE FROM economic_accounting_source_claim WHERE operation_id=%s", (operation,))],
            [("INSERT INTO economic_accounting_source_claim(lineage,source_event,operation_id,outcome) VALUES(%s,%s,%s,%s)",
              (claim["lineage"], claim["source_event"], claim["operation_id"], claim["outcome"]))],
            {"lineage_missing_source_claim": 1})
    changed_source = claims[old]["source_event"][:4] + b"\x99" + claims[old]["source_event"][5:]
    constrained("source-claim-composite-foreign-key",
                "UPDATE economic_accounting_source_claim SET source_event=%s WHERE operation_id=%s",
                (changed_source, old), 1452)
    constrained("source-claim-operation-uniqueness",
                "UPDATE economic_accounting_source_claim SET operation_id=%s WHERE operation_id=%s",
                (selected, old), 1062)
    field("retained-claim-source-disagrees-with-witness", "economic_accounting_source_claim", "source_event",
          old, changed_source, {"baseline_source_claim": 1, "lineage_missing_source_claim": 1}, broken_fk=True)
    for operation, other, label in ((old, selected, "retained-root-reuses-selected-source"),
                                    (selected, old, "selected-root-reuses-retained-source")):
        field(label, "economic_accounting_operation", "source_event", operation, claims[other]["source_event"],
              {"orphan_source_claim": 1, "lineage_missing_source_claim": 1, "lineage_duplicate_source_event": 1},
              broken_fk=True)
    field("retained-required-source-missing", "economic_accounting_operation", "source_event", old, None,
          {"orphan_source_claim": 1, "lineage_missing_required_source_event": 1}, broken_fk=True)
    field("retained-receipt-not-committed", "critical_operation_inbox", "status", old, 0,
          {"baseline_source_claim": 1})
    field("selected-receipt-not-committed", "critical_operation_inbox", "status", selected, 0,
          refusal="uncommitted or noncanonical baseline witness")
    field("retained-receipt-lost-commit-time", "critical_operation_inbox", "committed_at", old, None,
          {"baseline_source_claim": 1})
    for column, changed in (("revision", 2), ("last_operation_id", selected)):
        control = next(row for row in intact["economic_baseline_control"] if row["epoch"] == old_epoch)
        query = "UPDATE economic_baseline_control SET " + column + "=%s WHERE lineage=%s AND epoch=%s"
        cut("retained-control-" + column, [(query, (changed, lineage, old_epoch))],
            [(query, (control[column], lineage, old_epoch))], {"baseline_source_claim": 1})
    field("retained-witness-digest-lost", "economic_baseline_witness", "witness_digest", old, bytes(32),
          {"baseline_source_claim": 1})
    field("selected-witness-digest-lost", "economic_baseline_witness", "witness_digest", selected, bytes(32),
          refusal="invalid witness digest")
    field("retained-witness-revision-gap", "economic_baseline_witness", "book_revision", old, 2,
          {"baseline_source_claim": 1})
    witness = next(row for row in intact["economic_baseline_witness"] if row["operation_id"] == old)
    encoded_witness = witness["canonical_witness"]
    for label, offset in (("retained-witness-preparation-rebound", 48),
                          ("retained-witness-batch-rebound", 72), ("retained-witness-foreign-epoch", 32)):
        damaged = bytearray(encoded_witness)
        damaged[offset] ^= 1
        query = "UPDATE economic_baseline_witness SET canonical_witness=%s,witness_digest=%s WHERE operation_id=%s"
        cut(label, [(query, (bytes(damaged), hashlib.sha256(damaged).digest(), old))],
            [(query, (encoded_witness, witness["witness_digest"], old))], {"baseline_source_claim": 1})
    malformed = claims[old]["source_event"][:2] + bytes(2) + claims[old]["source_event"][4:]
    field("retained-malformed-claim-identity", "economic_accounting_source_claim", "source_event", old, malformed,
          {"invalid_source_claim": 1, "baseline_source_claim": 1, "lineage_missing_source_claim": 1}, broken_fk=True)
    reservations = [row for row in intact["economic_baseline_reservation"] if row["operation_id"] == old]
    changes = [("DELETE FROM economic_baseline_reservation WHERE operation_id=%s", (old,)),
               ("DELETE FROM economic_baseline_witness WHERE operation_id=%s", (old,))]
    repairs = []
    for table, rows in (("economic_baseline_witness", [witness]), ("economic_baseline_reservation", reservations)):
        for row in rows:
            repairs.append(("INSERT INTO " + table + "(" + ",".join(row) + ") VALUES(" +
                            ",".join(["%s"] * len(row)) + ")", tuple(row.values())))
    cut("retained-witness-book-lost", changes, repairs, {"baseline_source_claim": 1})
    assert captured() == intact
    replayed = subprocess.check_output([str(fixture), "--reconcile"], env=dict(os.environ), timeout=120)
    assert replayed == encoded and captured() == intact
    print("NATIVE_BASELINE_REPLAY " + replayed.decode().strip(), flush=True)
    sibling_environment = dict(os.environ, ECONOMIC_ACCOUNTING_DISPOSABLE_SCHEMA="1", DB_PASSWORD=settings["password"])
    subprocess.run([sys.executable, "-u", str(ROOT / "tests/async/run_economic_sql_audit_snapshot_mysql.py")],
                   env=sibling_environment, check=True, timeout=900)
    assert captured() == intact
    print("NATIVE_BASELINE_AUDIT_QUALIFIED " + json.dumps({"baseline_claims": 2, "epochs": 2,
          "cuts": len(cuts), "constraint_refusals": len(constraints),
          "authority_unchanged": True, "activation": False,
          "production_access": False, "complete_native_capture": False}, sort_keys=True), flush=True)
finally:
    if reader is not None:
        reader.close()
    owner.close()
