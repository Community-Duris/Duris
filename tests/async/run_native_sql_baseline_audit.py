#!/usr/bin/env python3
"""Read native baseline claims from a freshly bootstrapped private socket."""
import hashlib
import contextlib
import io
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

import pymysql

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
import economic_sql_audit_snapshot as exporter
import migration_runner as migrations
import qualify_database_restore as restore_qualifier
from reconcile_economy_accounting import Reconciler, view
from economic_sql_audit_origins import OriginError

if (os.environ.get("TEST_DB_DISPOSABLE") != "1" or os.environ.get("ENVIRONMENT") != "test" or
        os.environ.get("DB_HOST") != "127.0.0.1" or os.environ.get("DB_NAME") != "duris_restore" or
        not os.environ.get("DB_SOCKET", "").startswith("/plan5-restore-baseline-")):
    raise SystemExit("fresh private baseline daemon/socket required")
fixture = Path(os.environ["DURIS_PLAN5_BASELINE_FIXTURE"]).resolve()
assert fixture.is_relative_to((ROOT / "bin/tests/plan5-baseline-sql-restore").resolve())
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
TABLES += ("economic_accounting_child", "economic_accounting_item_reference", "critical_outbox")


def execute(query, params=()):
    with owner.cursor() as cursor:
        cursor.execute(query, params)


def captured(connection=owner):
    with connection.cursor() as cursor:
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
        engine_version = cursor.fetchone()["version"]
        engine_name = "mariadb" if "MariaDB" in engine_version else "mysql"
        print("native baseline engine=" + engine_version + " migration_head=0056_spell_ward_durability", flush=True)
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

    def restore_qualification():
        before = captured()
        try:
            with contextlib.redirect_stdout(io.StringIO()) as output:
                restore_qualifier.main()
            assert json.loads(output.getvalue()) == {"history": "ok", "reconciliation": "ok"}
        finally:
            assert captured() == before, "restore qualification changed authority"

    restore_qualification()
    witness = intact["economic_baseline_witness"][0]
    execute("UPDATE economic_baseline_witness SET witness_digest=%s WHERE operation_id=%s",
            (bytes([99]) * 32, witness["operation_id"]))
    try:
        try:
            restore_qualification()
        except RuntimeError as error:
            assert str(error) == "restore_economic_baseline_witness_mismatch", str(error)
        else:
            raise AssertionError("corrupt native EAB1 witness was qualified by the full SQL restore gate")
    finally:
        execute("UPDATE economic_baseline_witness SET witness_digest=%s WHERE operation_id=%s",
                (witness["witness_digest"], witness["operation_id"]))
    assert captured() == intact
    print("RESTORE_BASELINE_FULL_DIGEST_REFUSAL passed", flush=True)

    def restore_evidence():
        before = captured()
        with reader.cursor() as cursor:
            cursor.execute("SET TRANSACTION ISOLATION LEVEL REPEATABLE READ")
            cursor.execute("START TRANSACTION WITH CONSISTENT SNAPSHOT, READ ONLY")
            class Executor:
                def sql(self, query):
                    assert query.startswith("SELECT "), query
                    cursor.execute(query)
                    rows = cursor.fetchall()
                    assert all(len(row) == 1 for row in rows)
                    return "\n".join(str(next(iter(row.values()))) for row in rows)
            try:
                restore_qualifier.require_economic_evidence_integrity(Executor())
            finally:
                reader.rollback()
                assert captured() == before, "independent restore reader changed authority"

    restore_evidence()
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

    def cut(label, changes, repairs, findings=None, refusal=None, broken_fk=False, probe=None):
        original = captured()
        try:
            fixture_changes(changes, broken_fk)
            damaged = captured()
            try:
                damaged_snapshot, observed = audit()
            except OriginError as error:
                assert refusal is not None and refusal in str(error), (label, str(error), refusal)
            else:
                assert refusal is None and observed == base_counts | findings, (label, observed, findings)
                if probe is not None:
                    original_snapshot = json.dumps(damaged_snapshot, sort_keys=True)
                    probe(damaged_snapshot)
                    assert json.dumps(damaged_snapshot, sort_keys=True) == original_snapshot
            try:
                restore_evidence()
            except RuntimeError as error:
                restore_code = str(error)
                assert restore_code.startswith("restore_economic_"), (label, restore_code)
            else:
                raise AssertionError(label + ": corrupt baseline passed independent restore qualification")
            assert captured() == damaged, label + ": reader changed authority"
        finally:
            fixture_changes(repairs, broken_fk)
        assert captured() == original, label + ": disposable fixture was not restored"
        cuts.append({"label": label, "findings": findings, "refusal": refusal,
                     "damaged_import": broken_fk, "restore_refusal": restore_code})
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
    reservation_orphan = bytes([89]) * 16
    orphan_insert = ("INSERT INTO economic_baseline_reservation(lineage,epoch,identity_kind,identity_id,operation_id) "
                     "VALUES(%s,%s,%s,%s,%s)")
    orphan_values = (bytes([99]) * 16, bytes([77]) * 16, 1, 2**64 - 1, reservation_orphan)
    constrained("orphan-reservation-composite-foreign-key", orphan_insert, orphan_values, 1452)

    def orphan_operator_probe(snapshot):
        report = Reconciler().audit(snapshot)
        expected = [{"record": "orphan_evidence", "table": "baseline_reservations",
                     "operation_id": reservation_orphan.hex(), "row_index": row[2], "identity_kind": row[2],
                     "identity_id": row[3], "claimed_lineage": row[0].hex(), "claimed_epoch": row[1].hex()}
                    for row in orphan_rows]
        expected.sort(key=lambda row: (row["row_index"], json.dumps(row, sort_keys=True)))
        path = fixture.parent / (engine_name + "-reservation-orphan-snapshot.json")
        payload = json.dumps(snapshot, sort_keys=True).encode()
        path.write_bytes(payload)
        for limit in (0, 1, 100):
            output = view(snapshot, report, "operation", limit, operation_id=reservation_orphan.hex())
            assert output["count"] == len(expected)
            assert output["record_counts"]["orphan_evidence"] == len(expected)
            assert all(value == 0 for key, value in output["record_counts"].items() if key != "orphan_evidence")
            assert output["rows"] == expected[:limit]
            assert output["truncated"] == (len(expected) > limit)
            assert output["coverage"]["exception_count"] == 5 + len(expected)
            assert output["coverage"]["complete"] is False
            result = subprocess.run([sys.executable, str(ROOT / "scripts/reconcile_economy_accounting.py"),
                str(path), "--view", "operation", "--operation-id", reservation_orphan.hex(),
                "--limit", str(limit)], capture_output=True, timeout=30)
            assert result.returncode == 1 and result.stderr == b"", (limit, result.stderr)
            assert json.loads(result.stdout) == output
            assert path.read_bytes() == payload
            (fixture.parent / (engine_name + "-reservation-operation-limit" + str(limit) + ".json")).write_bytes(result.stdout)
        print("RESERVATION_ORPHAN_OPERATOR_QUALIFIED " + json.dumps({"engine": engine_name,
              "rows": len(expected), "limits": [0, 1, 100], "cli_exit": 1,
              "snapshot_unchanged": True, "authority_unchanged": True}, sort_keys=True), flush=True)

    orphan_cases = [("unknown-book-and-operation", orphan_values),
                    ("known-lineage-unknown-epoch-and-operation", (lineage, bytes([77]) * 16, 2, 2**63, reservation_orphan)),
                    ("unknown-lineage-known-epoch-and-operation", (bytes([99]) * 16, old_epoch, 1, 1, reservation_orphan)),
                    ("known-book-unknown-operation", (lineage, old_epoch, 2, 2**64 - 1, reservation_orphan)),
                    ("receipt-without-witness", (bytes([99]) * 16, bytes([77]) * 16, 2, 1,
                                                intact["economic_baseline_control"][0]["creating_operation_id"]))]
    orphan_delete = ("DELETE FROM economic_baseline_reservation WHERE lineage=%s AND epoch=%s "
                     "AND identity_kind=%s AND identity_id=%s")
    for label, values in orphan_cases:
        cut("reservation-" + label, [(orphan_insert, values)], [(orphan_delete, values[:4])],
            {"orphan_baseline_reservation": 1} | ({"baseline_source_claim": 1}
                if label == "known-book-unknown-operation" else {}), broken_fk=True)
    # One missing operation can have several distinct reservation identities.
    # Preserve both kinds, unsigned IDs, and distinct claimed books in the view.
    orphan_rows = [orphan_values,
                   (bytes([99]) * 16, bytes([77]) * 16, 2, 2**64 - 1, reservation_orphan),
                   (bytes([99]) * 16, bytes([78]) * 16, 1, 2**64 - 1, reservation_orphan),
                   (bytes([100]) * 16, bytes([77]) * 16, 1, 2**63, reservation_orphan)]
    cut("reservation-multiple-natural-keys", [(orphan_insert, values) for values in orphan_rows],
        [(orphan_delete, values[:4]) for values in orphan_rows], {"orphan_baseline_reservation": 4},
        broken_fk=True, probe=orphan_operator_probe)
    cut("selected-baseline-extra-child",
        [("INSERT INTO economic_accounting_child(operation_id,child_index,child_operation_id,domain_id,"
          "discriminator,parent_index,relationship) VALUES(%s,1,%s,1,1,0,1)", (selected, bytes([98]) * 16))],
        [("DELETE FROM economic_accounting_child WHERE operation_id=%s AND child_index=1", (selected,))],
        refusal="EAB1 SQL zero-effect mismatch")
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

    def zero_effect_fault(operation):
        return ({"findings": {"baseline_source_claim": 1}} if operation == old else
                {"refusal": "EAB1 SQL zero-effect mismatch"})

    def ownership_event(operation, legacy=None):
        witness = next(row for row in intact["economic_baseline_witness"] if row["operation_id"] == operation)
        blob = witness["canonical_witness"]
        offset = 192 + witness["holding_count"] * 112
        uid = int.from_bytes(blob[offset:offset + 8], "little")
        revision = int.from_bytes(blob[offset + 48:offset + 56], "little")
        return dict(operation_id=operation if legacy is None else legacy, event_index=0,
                    item_uid=uid, root_item_uid=uid, parent_item_uid=None,
                    from_owner_type=1, from_owner_id=7, from_owner_context_id=0,
                    to_owner_type=1, to_owner_id=7, to_owner_context_id=0,
                    item_revision=revision + 1, from_owner_revision=revision,
                    to_owner_revision=revision + 1, reason_type=1, source_site=1)

    creator = intact["economic_baseline_control"][0]["creating_operation_id"]
    for operation, other, scope_name in ((selected, old, "selected"), (old, selected, "retained")):
        if operation == old:
            cut("retained-baseline-extra-child",
                [("INSERT INTO economic_accounting_child(operation_id,child_index,child_operation_id,domain_id,"
                  "discriminator,parent_index,relationship) VALUES(%s,1,%s,1,1,0,1)", (operation, bytes([98]) * 16))],
                [("DELETE FROM economic_accounting_child WHERE operation_id=%s AND child_index=1", (operation,))],
                **zero_effect_fault(operation))
        # Both roots are genuine native baselines; using either as the other's
        # child violates the zero-effect invariant in the selected book too.
        cut(scope_name + "-baseline-used-as-child",
            [("INSERT INTO economic_accounting_child(operation_id,child_index,child_operation_id,domain_id,"
              "discriminator,parent_index,relationship) VALUES(%s,1,%s,1,1,0,1)", (other, operation))],
            [("DELETE FROM economic_accounting_child WHERE operation_id=%s AND child_index=1", (other,))],
            refusal="EAB1 SQL zero-effect mismatch")
        currency = dict(operation_id=operation, pid=7, bank_id=99, wallet_revision=1, bank_revision=1,
                        reason_type=1, source_site=1)
        currency.update({side + "_" + coin: 0 for side in
                         ("wallet_delta", "bank_delta", "wallet_after", "bank_after")
                         for coin in ("copper", "silver", "gold", "platinum")})
        cut(scope_name + "-baseline-extra-currency-ledger", [insert_row("currency_ledger", currency)],
            [("DELETE FROM currency_ledger WHERE operation_id=%s", (operation,))], **zero_effect_fault(operation))
        event = ownership_event(operation)
        cut(scope_name + "-baseline-extra-ownership-ledger", [insert_row("item_ownership_ledger", event)],
            [("DELETE FROM item_ownership_ledger WHERE operation_id=%s", (operation,))], **zero_effect_fault(operation))
        legacy = ownership_event(operation, creator)
        reference = dict(operation_id=operation, line_index=0, event_index=0, child_index=0,
                         item_uid=legacy["item_uid"], before_revision=legacy["item_revision"] - 1,
                         after_revision=legacy["item_revision"], legacy_operation_id=creator, legacy_event_index=0)
        cut(scope_name + "-baseline-extra-item-reference",
            [insert_row("item_ownership_ledger", legacy), insert_row("economic_accounting_item_reference", reference)],
            [("DELETE FROM economic_accounting_item_reference WHERE operation_id=%s", (operation,)),
             ("DELETE FROM item_ownership_ledger WHERE operation_id=%s AND event_index=0", (creator,))],
            **({"findings": {"baseline_source_claim": 1, "unattributed_ownership_event": 1,
                             "unattributed_ownership_uid": 1}} if operation == old else zero_effect_fault(operation)))
        for status in range(4):
            outbox = dict(operation_id=operation, event_index=0, destination=1, event_type=1,
                          payload_version=1, payload=b"private-zero-effect-fixture", status=status)
            cut(scope_name + "-baseline-extra-outbox-status-" + str(status), [insert_row("critical_outbox", outbox)],
                [("DELETE FROM critical_outbox WHERE operation_id=%s AND event_index=0", (operation,))],
                **zero_effect_fault(operation))

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
                        broken_fk=True, **({"findings": {"baseline_source_claim": 1, "orphan_baseline_reservation": 1}}
                                           if operation == old else projection_fault(operation)))
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
    restore_qualification()
    # An initialized empty book is valid inactive history, with no witness yet.
    empty_epoch = bytes([96]) * 16
    control = intact["economic_baseline_control"][0]
    execute("INSERT INTO economic_epoch(lineage,epoch,ordinal,transition_kind,transition_digest,creating_operation_id) "
            "VALUES(%s,%s,3,1,%s,%s)", (lineage, empty_epoch, bytes([1]) * 32, creator))
    execute("INSERT INTO economic_baseline_control(lineage,epoch,opening_account,creating_operation_id) VALUES(%s,%s,%s,%s)",
            (lineage, empty_epoch, control["opening_account"], creator))
    try:
        restore_qualification()
        execute("UPDATE economic_baseline_control SET opening_account=%s WHERE lineage=%s AND epoch=%s",
                (bytes([97]) * 16 + control["opening_account"][16:], lineage, empty_epoch))
        try:
            restore_qualification()
        except RuntimeError as error:
            assert str(error) == "restore_economic_baseline_book_mismatch", str(error)
        else:
            raise AssertionError("foreign empty-book opening was qualified")
        execute("UPDATE economic_baseline_control SET opening_account=%s WHERE lineage=%s AND epoch=%s",
                (control["opening_account"], lineage, empty_epoch))
        restore_qualification()
    finally:
        execute("DELETE FROM economic_baseline_control WHERE lineage=%s AND epoch=%s", (lineage, empty_epoch))
        execute("DELETE FROM economic_epoch WHERE lineage=%s AND epoch=%s", (lineage, empty_epoch))
    assert captured() == intact
    print("RESTORE_BASELINE_EMPTY_BOOK admitted_inactive_foreign_opening_refused_unchanged", flush=True)
    sibling_environment = dict(os.environ, ECONOMIC_ACCOUNTING_DISPOSABLE_SCHEMA="1", DB_PASSWORD=settings["password"])
    subprocess.run([sys.executable, "-u", str(ROOT / "tests/async/run_economic_sql_audit_snapshot_mysql.py")],
                   env=sibling_environment, check=True, timeout=900)
    assert captured() == intact
    dump_path = fixture.parent / (engine_name + "-baseline.sql")
    with dump_path.open("wb") as output:
        result = subprocess.run(["mysqldump", "--no-defaults", "--protocol=socket",
            "--socket=" + settings["unix_socket"], "--user=" + settings["user"],
            "--single-transaction", "--skip-lock-tables", "--hex-blob", "--routines", "--triggers",
            "--events", "duris_restore"], env=dict(os.environ), stdout=output, stderr=subprocess.PIPE,
            timeout=180)
    assert result.returncode == 0, result.stderr
    assert 0 < dump_path.stat().st_size < 32 * 1024 * 1024
    assert captured() == intact
    import persistence_restore as restore
    with tempfile.TemporaryDirectory(prefix="plan5-restore-baseline-clone-", dir="/") as directory:
        candidate = Path(directory) / engine_name
        candidate.mkdir(mode=0o700)
        with restore.private_database(candidate, engine_name) as clone:
            with dump_path.open("rb") as payload:
                result = subprocess.run(["mysql", "--no-defaults", "--protocol=socket",
                    "--socket=" + clone["DB_SOCKET"], "--user=" + clone["DB_USER"], "duris_restore"],
                    env=clone, stdin=payload, capture_output=True, timeout=180)
            assert result.returncode == 0, result.stderr
            connection = pymysql.connect(unix_socket=clone["DB_SOCKET"], user=clone["DB_USER"],
                password=clone["DB_PASSWD"], database="duris_restore", autocommit=True,
                cursorclass=pymysql.cursors.DictCursor)
            try:
                assert captured(connection) == intact
                result = subprocess.run([sys.executable, str(ROOT / "scripts/qualify_database_restore.py")],
                    env=clone, capture_output=True, timeout=120)
                assert result.returncode == 0, result.stderr
                assert json.loads(result.stdout) == {"history": "ok", "reconciliation": "ok"}
                replay_env = dict(clone, TEST_DB_DISPOSABLE="1", ENVIRONMENT="test",
                                  ASAN_OPTIONS=os.environ["ASAN_OPTIONS"], UBSAN_OPTIONS=os.environ["UBSAN_OPTIONS"])
                cold_replay = subprocess.check_output([str(fixture), "--reconcile"], env=replay_env, timeout=120)
                assert cold_replay == encoded and captured(connection) == intact
            finally:
                connection.close()
    assert captured() == intact
    print("RESTORE_BASELINE_COLD_CLONE " + json.dumps({"engine": engine_name,
          "dump_sha256": hashlib.sha256(dump_path.read_bytes()).hexdigest(), "dump_bytes": dump_path.stat().st_size,
          "books": 2, "tables_unchanged": len(TABLES), "full_qualifier": True, "exact_native_replay": True,
          "active_epoch_null": True, "complete_world_capture": False}, sort_keys=True), flush=True)
    print("NATIVE_BASELINE_AUDIT_QUALIFIED " + json.dumps({"baseline_claims": 2, "epochs": 2,
          "cuts": len(cuts), "constraint_refusals": len(constraints),
          "restore_refusals": len(cuts), "native_baseline_dump_import": True,
          "authority_unchanged": True, "activation": False,
          "production_access": False, "complete_native_capture": False}, sort_keys=True), flush=True)
finally:
    if reader is not None:
        reader.close()
    owner.close()
