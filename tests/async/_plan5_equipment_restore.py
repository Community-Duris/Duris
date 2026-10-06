"""Actual native-book dump/import with modeled live equipment positions."""

import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import uuid

import pymysql

import economic_sql_audit_snapshot as exporter
import persistence_restore as restore
from reconcile_economy_accounting import Reconciler


def inventory(connection):
    result = {}
    with connection.cursor() as cursor:
        cursor.execute("SELECT TABLE_NAME AS name FROM information_schema.tables "
                       "WHERE table_schema=DATABASE() ORDER BY TABLE_NAME")
        tables = [row["name"] for row in cursor.fetchall()]
        for table in tables:
            assert table.replace("_", "").isalnum(), table
            cursor.execute("SELECT * FROM `" + table + "`")
            rows = sorted(repr(row) for row in cursor.fetchall())
            result[table] = {"rows": len(rows),
                             "sha256": hashlib.sha256("\n".join(rows).encode()).hexdigest()}
    return result


class Cursor:
    def __init__(self, cursor):
        self.actual = cursor
        self.queries = []
        self.closes = 0

    def execute(self, sql, params=None):
        assert sql.startswith(("SELECT", "SET TRANSACTION", "START TRANSACTION")), sql
        self.queries.append(sql)
        return self.actual.execute(sql, params)

    def close(self):
        self.closes += 1
        return self.actual.close()

    def __getattr__(self, name):
        return getattr(self.actual, name)


class Connection:
    def __init__(self, connection):
        self.actual = connection
        self.rollbacks = 0

    def cursor(self):
        self.observer = Cursor(self.actual.cursor())
        return self.observer

    def rollback(self):
        self.rollbacks += 1
        return self.actual.rollback()


def run(root, owner, reader, fixture, native, encoded, settings, engine):
    assert os.environ.get("TEST_DB_DISPOSABLE") == "1"
    assert os.environ.get("ENVIRONMENT") == "test"
    assert settings["unix_socket"].startswith("/plan5-restore-baseline-")
    output = fixture.parent / (engine + "-equipment-restore-" + uuid.uuid4().hex)
    output.mkdir(mode=0o700)
    initial = inventory(owner)
    assert initial["item_current_owner"]["rows"] == initial["player_data"]["rows"] == 0
    records = []

    def sample(authority, audit_reader, label, drift):
        target = output / label
        target.mkdir(mode=0o700)
        before = inventory(authority)
        connection = Connection(audit_reader)
        snapshot = exporter.capture(connection, bytes.fromhex(native["lineage"]),
                                    bytes.fromhex(native["epochs"][-1]))
        report = Reconciler().audit(snapshot)
        expected = {"evidence_loss": 1, "missing_native_holding": 2}
        if drift:
            expected["stale_native_item"] = 1
        assert report["exception_counts"] == expected, report
        assert snapshot["complete"] is False and snapshot["backend"] == "sql_partial"
        assert {row["uid"]: row["equipment_slot"] for row in snapshot["item_origins"]} == {81: 5, 82: 6}
        assert {row["uid"]: row["equipment_slot"] for row in snapshot["native"]["items"]} == {
            81: 6 if drift else 5, 82: 6}
        assert connection.rollbacks == connection.observer.closes == 1
        payload = json.dumps(snapshot, sort_keys=True).encode()
        path = target / "snapshot.json"
        path.write_bytes(payload)
        path.chmod(0o600)
        commands = []
        for limit in (0, 1, 100):
            command = [sys.executable, str(root / "scripts/reconcile_economy_accounting.py"),
                       str(path), "--view", "exceptions", "--limit", str(limit)]
            result = subprocess.run(command, capture_output=True, timeout=30)
            assert result.returncode == 1 and not result.stderr, result.stderr
            value = json.loads(result.stdout)
            assert value["exception_counts"] == expected
            assert value["exception_count"] == sum(expected.values())
            assert len(value["exceptions"]) <= limit
            assert path.read_bytes() == payload
            (target / ("limit-" + str(limit) + ".json")).write_bytes(result.stdout)
            commands.append({"command": command, "exit": result.returncode})
        assert inventory(authority) == before, label
        for name in ("before", "after"):
            (target / ("authority-" + name + ".json")).write_text(json.dumps(before, sort_keys=True) + "\n")
        (target / "report.json").write_text(json.dumps(report, sort_keys=True) + "\n")
        (target / "queries.json").write_text(json.dumps(connection.observer.queries) + "\n")
        record = {"phase": label, "exception_counts": expected, "read_only": True,
                  "query_count": len(connection.observer.queries), "rollback_calls": 1,
                  "cursor_close_calls": 1, "application_tables_unchanged": len(before),
                  "commands": commands, "modeled_current_owner_rows": 2,
                  "complete_capture": False, "release_complete": False}
        (target / "results.json").write_text(json.dumps(record, indent=2) + "\n")
        records.append(record)

    with owner.cursor() as cursor:
        # These current positions are explicit fixture inputs. The original
        # EAB2 books and commands were admitted by the actual native owner.
        cursor.execute("INSERT INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,"
                       "owner_type,owner_id,owner_context_id,item_revision,vnum,state,equipment_slot,updated_at) "
                       "VALUES(81,81,NULL,1,7,0,3,7,1,5,'2026-01-01'),"
                       "(82,82,NULL,1,8,0,4,7,1,6,'2026-01-01')")
    try:
        for drift in (False, True):
            phase = "drift" if drift else "clean"
            if drift:
                with owner.cursor() as cursor:
                    cursor.execute("UPDATE item_current_owner SET equipment_slot=6 WHERE item_uid=81")
            sample(owner, reader, "source-" + phase, drift)
            source = inventory(owner)
            dump = output / (phase + ".sql")
            command = ["mysqldump", "--no-defaults", "--protocol=socket",
                       "--socket=" + settings["unix_socket"], "--user=" + settings["user"],
                       "--single-transaction", "--skip-lock-tables", "--hex-blob", "--routines",
                       "--triggers", "--events", "duris_restore"]
            with dump.open("xb") as stream:
                subprocess.run(command, stdout=stream, stderr=subprocess.PIPE, check=True,
                               env=dict(os.environ), timeout=180)
            assert 0 < dump.stat().st_size < 32 * 1024 * 1024
            assert inventory(owner) == source
            with tempfile.TemporaryDirectory(prefix="plan5-restore-baseline-equipment-clone-", dir="/") as directory:
                candidate = Path(directory) / engine
                candidate.mkdir(mode=0o700)
                with restore.private_database(candidate, engine) as clone:
                    with dump.open("rb") as payload:
                        subprocess.run(["mysql", "--no-defaults", "--protocol=socket",
                            "--socket=" + clone["DB_SOCKET"], "--user=" + clone["DB_USER"], "duris_restore"],
                            env=clone, stdin=payload, capture_output=True, check=True, timeout=180)
                    authority = pymysql.connect(unix_socket=clone["DB_SOCKET"], user=clone["DB_USER"],
                        password=clone["DB_PASSWD"], database="duris_restore", autocommit=True,
                        cursorclass=pymysql.cursors.DictCursor)
                    cold_reader = None
                    try:
                        assert inventory(authority) == source
                        # Role creation uses only this helper's newly initialized
                        # private daemon administrator; application data is exact.
                        admin = pymysql.connect(unix_socket=clone["DB_SOCKET"], user="root", autocommit=True)
                        try:
                            with admin.cursor() as cursor:
                                cursor.execute("CREATE USER 'equipment_reader'@'localhost' IDENTIFIED BY 'private-equipment-reader'")
                                cursor.execute("GRANT SELECT ON duris_restore.* TO 'equipment_reader'@'localhost'")
                        finally:
                            admin.close()
                        cold_reader = pymysql.connect(unix_socket=clone["DB_SOCKET"], user="equipment_reader",
                            password="private-equipment-reader", database="duris_restore", autocommit=True,
                            cursorclass=pymysql.cursors.DictCursor)
                        sample(authority, cold_reader, "cold-" + phase, drift)
                        with cold_reader.cursor() as cursor:
                            try:
                                cursor.execute("UPDATE item_current_owner SET equipment_slot=equipment_slot")
                            except pymysql.MySQLError as error:
                                assert error.args[0] == 1142, error.args
                            else:
                                raise AssertionError("SELECT-only cold reader admitted UPDATE")
                        result = subprocess.run([sys.executable, str(root / "scripts/qualify_database_restore.py")],
                                                env=clone, capture_output=True, timeout=120)
                        assert result.returncode == 0, result.stderr
                        assert json.loads(result.stdout) == {"history": "ok", "reconciliation": "ok"}
                        replay_env = dict(clone, TEST_DB_DISPOSABLE="1", ENVIRONMENT="test",
                                          ASAN_OPTIONS=os.environ["ASAN_OPTIONS"], UBSAN_OPTIONS=os.environ["UBSAN_OPTIONS"])
                        assert subprocess.check_output([str(fixture), "--reconcile"],
                            env=replay_env, timeout=120) == encoded
                        assert inventory(authority) == source
                        records[-1].update(dump_command=command, dump_sha256=hashlib.sha256(dump.read_bytes()).hexdigest(),
                            dump_bytes=dump.stat().st_size, exact_dump_import=True, immutable_native_replay=True,
                            retained_evidence_qualifier=True, permission_denial=1142)
                    finally:
                        if cold_reader is not None:
                            cold_reader.close()
                        authority.close()
            assert inventory(owner) == source
    finally:
        with owner.cursor() as cursor:
            cursor.execute("DELETE FROM item_current_owner WHERE item_uid IN (81,82)")
    assert inventory(owner) == initial
    result = {"engine": engine, "phases": records, "modeled_live_positions": True,
              "original_native_books_preserved": True, "source_fixture_restored": True,
              "accounting_activated": False, "complete_capture": False, "release_complete": False}
    (output / "results.json").write_text(json.dumps(result, indent=2) + "\n")
    print("NATIVE_BASELINE_EQUIPMENT_RESTORE " + json.dumps(result, sort_keys=True), flush=True)
    return result
