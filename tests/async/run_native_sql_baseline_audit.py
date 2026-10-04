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
assert fixture.is_relative_to((ROOT / "bin/tests/plan5-baseline-projection-audit").resolve())
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
        except pymysql.MySQLError as error:
            assert error.args[0] in (error_code if isinstance(error_code, tuple) else (error_code,)), (label, error.args)
            actual_code = error.args[0]
        else:
            raise AssertionError(label + ": canonical constraint admitted corruption")
        assert captured() == original
        constraints.append({"label": label, "error_code": actual_code})
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
    posting = next(row for row in intact["economic_accounting_coin_posting"] if row["operation_id"] == selected)
    cut("selected-posting-missing",
        [("DELETE FROM economic_accounting_coin_posting WHERE operation_id=%s AND line_index=%s",
          (selected, posting["line_index"]))],
        [("INSERT INTO economic_accounting_coin_posting(" + ",".join(posting) + ") VALUES(" +
          ",".join(["%s"] * len(posting)) + ")", tuple(posting.values()))],
        refusal="EAB1 SQL projection mismatch")

    def insert_row(table, row):
        return ("INSERT INTO " + table + "(" + ",".join(row) + ") VALUES(" +
                ",".join(["%s"] * len(row)) + ")", tuple(row.values()))

    def projection_fault(operation):
        return ({"findings": {"baseline_source_claim": 1}} if operation == old else
                {"refusal": "EAB1 SQL projection mismatch"})

    for operation, scope_name in ((selected, "selected"), (old, "retained")):
        for table, index_field, family in (("economic_accounting_account_effect", "account_index", "effect"),
                                           ("economic_accounting_coin_posting", "line_index", "posting"),
                                           ("economic_baseline_reservation", "identity_id", "reservation")):
            row = next(row for row in intact[table] if row["operation_id"] == operation and
                       (row["identity_kind"] == 1 if family == "reservation" else row[index_field] == 0))
            where = "operation_id=%s AND " + index_field + "=%s"
            parameters = (operation, row[index_field])
            if family == "reservation":
                where += " AND identity_kind=%s AND lineage=%s AND epoch=%s"
                parameters += (row["identity_kind"], row["lineage"], row["epoch"])
            dependent = ([item for item in intact["economic_accounting_coin_posting"]
                          if item["operation_id"] == operation and item["account_index"] == row["account_index"]]
                         if family == "effect" else [])
            if not (operation == selected and family == "posting"):
                changes = [("DELETE FROM economic_accounting_coin_posting WHERE operation_id=%s AND line_index=%s",
                            (operation, item["line_index"])) for item in dependent]
                changes.append(("DELETE FROM " + table + " WHERE " + where, parameters))
                repairs = [insert_row(table, row), *[insert_row("economic_accounting_coin_posting", item)
                                                    for item in dependent]]
                cut(scope_name + "-" + family + "-missing", changes, repairs, **projection_fault(operation))
            extra = dict(row)
            extra[index_field] += 100
            if family == "effect":
                extra["account_key"] = row["account_key"][:20] + (99).to_bytes(8, "little") + row["account_key"][28:]
            if family == "posting":
                extra["event_index"] = extra["line_index"]
            extra_parameters = (operation, extra[index_field])
            if family == "reservation":
                extra_parameters += (row["identity_kind"], row["lineage"], row["epoch"])
            cut(scope_name + "-" + family + "-extra", [insert_row(table, extra)],
                [("DELETE FROM " + table + " WHERE " + where, extra_parameters)], **projection_fault(operation))
            fields = ({field: row[field] + 1 for field in
                       ("before_copper", "before_silver", "before_gold", "before_platinum", "after_copper",
                        "after_silver", "after_gold", "after_platinum", "before_revision", "after_revision")}
                      if family == "effect" else
                      {field: row[field] + 1 for field in
                       ("delta_copper", "delta_silver", "delta_gold", "delta_platinum", "copper_value",
                        "account_index", "child_index")} if family == "posting" else
                      {"identity_kind": 2, "identity_id": row["identity_id"] + 100})
            if family == "effect":
                fields["account_key"] = row["account_key"][:28] + (1).to_bytes(8, "little") + row["account_key"][36:]
            for field_name, changed in fields.items():
                # Reservation identity fields are also part of its primary key.
                restore_parameters = (operation, changed if field_name == index_field else row[index_field])
                if family == "reservation":
                    restore_parameters += (changed if field_name == "identity_kind" else row["identity_kind"],
                                           row["lineage"], row["epoch"])
                query = "UPDATE " + table + " SET " + field_name + "=%s WHERE " + where
                cut(scope_name + "-" + family + "-" + field_name,
                    [(query, (changed, *parameters))], [(query, (row[field_name], *restore_parameters))],
                    **projection_fault(operation))
            if family == "posting":
                query = "UPDATE " + table + " SET line_index=%s,event_index=%s WHERE operation_id=%s AND line_index=%s"
                cut(scope_name + "-posting-dense-index",
                    [(query, (100, 100, operation, row["line_index"]))],
                    [(query, (row["line_index"], row["event_index"], operation, 100))], **projection_fault(operation))
            if family == "reservation":
                other = selected if operation == old else old
                query = "UPDATE " + table + " SET operation_id=%s WHERE " + where
                constrained(scope_name + "-reservation-foreign-book", query, (other, *parameters), 1452)
                cut(scope_name + "-reservation-operation-rebound", [(query, (other, *parameters))],
                    [(query, (operation, other, *parameters[1:]))], broken_fk=True,
                    refusal="EAB1 SQL projection mismatch")
                for field_name in ("lineage", "epoch"):
                    foreign = dict(row)
                    foreign[field_name], foreign["identity_id"] = bytes([88]) * 16, 99
                    cut(scope_name + "-reservation-extra-foreign-" + field_name, [insert_row(table, foreign)],
                        [("DELETE FROM " + table + " WHERE " + where,
                          (operation, 99, row["identity_kind"], foreign["lineage"], foreign["epoch"]))],
                        broken_fk=True, **projection_fault(operation))
    constrained("posting-event-index-check",
                "UPDATE economic_accounting_coin_posting SET event_index=99 WHERE operation_id=%s AND line_index=%s",
                (selected, posting["line_index"]), (3819, 4025))
    constrained("reservation-kind-check",
                "UPDATE economic_baseline_reservation SET identity_kind=3 WHERE operation_id=%s",
                (selected,), (3819, 4025))
    def root_fault(operation):
        return ({"findings": {"baseline_source_claim": 1}} if operation == old else
                {"refusal": "EAB1 committed root mismatch"})

    for operation, scope_name in ((selected, "selected"), (old, "retained")):
        retained_witness = next(row for row in intact["economic_baseline_witness"] if row["operation_id"] == operation)
        original_blob = retained_witness["canonical_witness"]
        item_offset = 192 + retained_witness["holding_count"] * 112
        for label, offset in (("opening-value", 232), ("actor", 64), ("boundary", 120), ("coverage", 152),
                              ("holding-revision", 264), ("holding-source", 272),
                              ("item-owner", item_offset + 16), ("item-revision", item_offset + 48),
                              ("item-source", item_offset + 56)):
            blob = bytearray(original_blob)
            blob[offset] ^= 1
            query = "UPDATE economic_baseline_witness SET canonical_witness=%s,witness_digest=%s WHERE operation_id=%s"
            cut(scope_name + "-" + label + "-rehashed",
                [(query, (bytes(blob), hashlib.sha256(blob).digest(), operation))],
                [(query, (original_blob, retained_witness["witness_digest"], operation))], **root_fault(operation))
        for column, changed in (("actor_id", 8), ("writer_id", 5), ("compiler_version", 2),
                                ("account_count", 0), ("before_witness_count", 0),
                                ("intent_digest", bytes([1]) * 32), ("domain_digest", bytes([2]) * 32),
                                ("plan_digest", bytes([3]) * 32)):
            field(scope_name + "-root-" + column, "economic_accounting_operation", column,
                  operation, changed, **root_fault(operation))
        for column, changed in (("durable_revision", 2), ("result_payload", b"x")):
            field(scope_name + "-receipt-" + column, "critical_operation_inbox", column,
                  operation, changed, **root_fault(operation))
        root_row = next(row for row in intact["economic_accounting_operation"] if row["operation_id"] == operation)
        for column, offset in (("canonical_intent", 96), ("canonical_plan", 76)):
            damaged = bytearray(root_row[column])
            damaged[offset] ^= 1
            digest_column = "intent_digest" if column == "canonical_intent" else "plan_digest"
            tagged = b"DURIS-ECONOMIC-INTENT-V1\0" if column == "canonical_intent" else b""
            query = "UPDATE economic_accounting_operation SET " + column + "=%s," + digest_column + "=%s WHERE operation_id=%s"
            cut(scope_name + "-" + column + "-metadata-rehashed",
                [(query, (bytes(damaged), hashlib.sha256(tagged + damaged).digest(), operation))],
                [(query, (root_row[column], root_row[digest_column], operation))], **root_fault(operation))
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
        source_fault = ({"findings": {"baseline_source_claim": 1, "lineage_missing_source_claim": 1,
                                      "lineage_duplicate_source_event": 1}} if operation == old else
                        {"refusal": "EAB1 committed root mismatch"})
        field(label, "economic_accounting_operation", "source_event", operation, claims[other]["source_event"],
              broken_fk=True, **source_fault)
    field("retained-required-source-missing", "economic_accounting_operation", "source_event", old, None,
          {"baseline_source_claim": 1, "lineage_missing_required_source_event": 1}, broken_fk=True)
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
