#!/usr/bin/env python3
"""Bounded incident semantics and disposable real SQL publication/permission proof."""
from __future__ import annotations

import argparse
from copy import deepcopy
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/telemetry"))
sys.path.insert(0, str(Path(__file__).resolve().parent))
import incident
from db_access import AmbiguousCommit, ConnectionSettings, PyMySQLConnectionFactory, PyMySQLRollupDatabase
from report import BoundsExceeded, ReportDatabase, ReportRequest, ReportCache
from rollup_definitions import RollupTarget
from telemetry_reports_fixtures import state_row, session_rows


def packet() -> dict:
    p = incident.template()
    p.update(environment_id=8, season_id=9, reviewer_token="11" * 32,
             review_evidence_digest="22" * 32)
    p["incidents"][0]["evidence_digest"] = "33" * 32
    return p


def project(p: dict, start=100, end=200):
    meta, rows = incident.validate_packet(p)
    rows = [dict(r, occurrence_relation=incident.occurrence_relation(r, start, end)) for r in rows]
    return incident.publication_summary((1, 1, 8, 9), meta, rows), rows


class IncidentSemantics(unittest.TestCase):
    def test_unknown_end_and_backlog_stay_unknown(self):
        meta, rows = project(packet())
        public = incident.public_coverage(meta, rows)
        self.assertEqual(public["unknown_end_count"], 1)
        self.assertEqual(public["unresolved_backlog_count"], 1)
        self.assertEqual(public["incidents"][0]["end_utc_usec"], None)
        self.assertEqual(public["incidents"][0]["backlog_disposition"], "unknown")
        self.assertEqual(public["incidents"][0]["occurrence_relation"], "possible_overlap")
        self.assertFalse(public["zero_activity_implied"])
        self.assertFalse(public["incident_inventory_complete"])

    def test_missing_inventory_and_reviewed_empty_are_distinct(self):
        missing = incident.publication_summary((1, 1, 8, 9), None, [])
        self.assertEqual(incident.public_coverage(missing, [])["status"], "not_registered")
        self.assertEqual(missing["quality_flags"], incident.QUALITY_INVENTORY_UNKNOWN)
        p = packet()
        p["incidents"] = []
        meta, rows = project(p)
        self.assertEqual(incident.public_coverage(meta, rows)["status"], "reviewed_inventory")
        self.assertEqual(meta["quality_flags"], 0)
        self.assertFalse(incident.public_coverage(meta, rows)["incident_inventory_complete"])

    def test_outside_overlap_withdrawal_and_incremental_window(self):
        p = packet()
        p["incidents"][0].update(start_utc_usec=300, end_utc_usec=400,
                                  backlog_disposition="delivered")
        meta, rows = project(p)
        self.assertEqual(meta["relevant_incident_count"], 0)
        public = incident.public_coverage(meta, rows, occurrence_window=(100, 350))
        self.assertEqual(public["relevant_incident_count"], 1)
        self.assertEqual(public["quality_flags"], incident.QUALITY_INCIDENT_GAP)
        self.assertEqual(public["incidents"][0]["occurrence_relation"], "overlaps_occurrence_window")
        p["incidents"][0]["status"] = "withdrawn"
        meta, rows = project(p, 100, 350)
        self.assertEqual(meta["relevant_incident_count"], 0)
        self.assertEqual(len(incident.public_coverage(meta, rows)["incidents"]), 1)

    def test_unknown_window_cannot_claim_no_gap(self):
        row = incident.validate_packet(packet())[1][0]
        self.assertEqual(incident.occurrence_relation(row, None, None), 2)
        row = dict(row, start_utc_usec=300, end_utc_usec=400)
        self.assertEqual(incident.occurrence_relation(row, None, 200), 3)
        self.assertEqual(incident.occurrence_relation(row, 100, None), 2)
        self.assertEqual(incident.occurrence_window(dict(coverage_start_utc_usec=100, coverage_end_utc_usec=200, quality_flags=1 << 17)), (None, None))

    def test_reconstruction_does_not_erase_gap(self):
        p = packet()
        p["incidents"][0]["observation_provenance"] = "reconstructed_separately"
        meta, rows = project(p)
        public = incident.public_coverage(meta, rows)
        self.assertTrue(public["incidents"][0]["gap_remains_after_reconstruction"])
        self.assertEqual(public["quality_flags"], incident.QUALITY_INCIDENT_GAP)

    def test_capacity_and_duplicate_ids_are_refused(self):
        p = packet()
        p["incidents"] = [dict(p["incidents"][0], incident_id=n) for n in range(1, 65)]
        self.assertEqual(len(incident.validate_packet(p)[1]), 64)
        p["incidents"].append(dict(p["incidents"][0], incident_id=65))
        with self.assertRaisesRegex(incident.IncidentError, "incident_capacity"):
            incident.validate_packet(p)
        p = packet()
        p["incidents"].append(deepcopy(p["incidents"][0]))
        with self.assertRaisesRegex(incident.IncidentError, "duplicate_incident"):
            incident.validate_packet(p)

    def test_field_type_range_and_private_payload_negatives(self):
        mutations = [
            lambda p: p.update(environment_id=True),
            lambda p: p.update(registry_version=3),
            lambda p: p.update(reviewer_token="private-name"),
            lambda p: p.update(notes="private command payload"),
            lambda p: p["incidents"][0].update(start_utc_usec=201, end_utc_usec=200),
            lambda p: p["incidents"][0].update(start_utc_usec=incident.UTC_UNKNOWN),
            lambda p: p["incidents"][0].update(producer_boot_id=1),
            lambda p: p["incidents"][0].update(first_record_seq=1),
            lambda p: p["incidents"][0].update(record_kind_mask=1 << 9),
            lambda p: p["incidents"][0].update(backlog_disposition="assume_delivered"),
            lambda p: p["incidents"][0].update(evidence_digest="00" * 31),
        ]
        for mutation in mutations:
            with self.subTest(mutation=mutation):
                p = packet()
                mutation(p)
                with self.assertRaises(incident.IncidentError):
                    incident.validate_packet(p)

    def test_postfix_fact_requires_fix_and_complete_key(self):
        p = packet()
        p["incidents"][0]["first_verified_postfix"] = dict(
            boot_id=11, process_id=22, record_seq=33, record_kind=6, occurrence_utc_usec=None)
        with self.assertRaisesRegex(incident.IncidentError, "verification_without_fix"):
            incident.validate_packet(p)
        p["incidents"][0]["fix_reference_digest"] = "44" * 32
        self.assertEqual(incident.validate_packet(p)[1][0]["verified_record_seq"], 33)
        p["incidents"][0]["record_kind_mask"] = 2
        with self.assertRaisesRegex(incident.IncidentError, "verification_family_mismatch"):
            incident.validate_packet(p)

    def test_canonical_retry_digest_and_revision(self):
        p = packet()
        p["incidents"].append(dict(p["incidents"][0], incident_id=2))
        before = incident.validate_packet(p)[0]["packet_digest"]
        p["incidents"].reverse()
        self.assertEqual(before, incident.validate_packet(p)[0]["packet_digest"])
        p.update(registry_version=2, previous_registry_version=1)
        self.assertNotEqual(before, incident.validate_packet(p)[0]["packet_digest"])

    def test_bounded_reader_duplicates_nesting_and_json_numbers(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "packet.json"
            for data, reason in ((b"x" * (incident.MAX_PACKET_BYTES + 1), "packet_capacity"),
                                 (b'{"environment_id":1,"environment_id":2}', "duplicate_field"),
                                 (b"[" * 10_000 + b"]" * 10_000, "invalid_json"),
                                 (b'{"environment_id":NaN}', "invalid_number")):
                path.write_bytes(data)
                with self.assertRaisesRegex(incident.IncidentError, reason):
                    incident.load_packet(path)

    def test_incomplete_publication_is_refused(self):
        meta, rows = project(packet())
        with self.assertRaisesRegex(incident.IncidentError, "incomplete_publication"):
            incident.public_coverage(meta, [])
        with self.assertRaisesRegex(incident.IncidentError, "incomplete_publication"):
            incident.public_coverage(None, rows)

    def test_stored_digest_and_projected_field_validation(self):
        meta, rows = incident.validate_packet(packet())
        incident.validate_stored(meta, rows)
        changed = [dict(rows[0], end_utc_usec=200)]
        with self.assertRaisesRegex(incident.IncidentError, "stored_inventory_digest_mismatch"):
            incident.validate_stored(meta, changed)
        summary, projected = project(packet())
        projected[0]["start_utc_usec"] = incident.UTC_UNKNOWN
        with self.assertRaisesRegex(incident.IncidentError, "invalid_utc"):
            incident.public_coverage(summary, projected)

    def test_byte_reservation_refuses_before_detail_fetch(self):
        class Database(ReportDatabase):
            def __init__(self):
                super().__init__(object())
                self.calls = 0
            def _execute(self, _statement, _parameters=()):
                self.calls += 1
                return [{"incident_count": 64}]
        database = Database()
        with self.assertRaises(BoundsExceeded):
            database._read_incident_coverage(RollupTarget(1, 1, 8, 9), state_row(), max_bytes=4096)
        self.assertEqual(database.calls, 1)


def sql_qualification() -> None:
    import pymysql
    from test_telemetry_repository import prepare_sql_fixture, drop_sql_fixture
    if os.environ.get("TELEMETRY_REPOSITORY_DISPOSABLE") != "1":
        raise RuntimeError("explicit disposable qualification required")
    environment, command, name = prepare_sql_fixture()
    admin = None
    databases = []
    users = []
    try:
        admin = pymysql.connect(host="127.0.0.1", port=int(environment["DB_PORT"]),
                                user=environment["DB_USER"], password=environment["DB_PASSWD"],
                                database=name, autocommit=True, cursorclass=pymysql.cursors.DictCursor)
        token = hashlib.sha256(name.encode()).hexdigest()[:10]
        password = "synthetic-incident-fixture-" + token
        grants = {
            "review": {"telemetry_incident_registry": "SELECT,INSERT", "telemetry_incident": "SELECT,INSERT", "telemetry_interval": "SELECT"},
            "rollup": {t: "SELECT,INSERT,UPDATE" for t in ("telemetry_rollup_state", "telemetry_rollup_session", "telemetry_cohort_day", "telemetry_cohort_member", "telemetry_player_day")},
            "report": {t: "SELECT" for t in ("telemetry_rollup_state", "telemetry_rollup_session", "telemetry_cohort_day", "telemetry_cohort_member", "telemetry_rollup_incident_coverage", "telemetry_rollup_incident")},
        }
        grants["rollup"].update({t: "SELECT" for t in ("telemetry_interval", "telemetry_incident_registry", "telemetry_incident")})
        grants["rollup"].update({t: "SELECT,INSERT" for t in ("telemetry_rollup_incident_coverage", "telemetry_rollup_incident")})
        with admin.cursor() as cursor:
            for role, tables in grants.items():
                user = "ti_" + role + "_" + token
                cursor.execute("CREATE USER %s@'%%' IDENTIFIED BY %s", (user, password))
                users.append(user)
                for table, permissions in tables.items():
                    cursor.execute(f"GRANT {permissions} ON `{name}`.`{table}` TO %s@'%%'", (user,))
            cursor.execute("INSERT INTO telemetry_interval (boot_id,process_id,record_seq,schema_version,record_kind,occurrence_utc_usec,ingested_utc_usec,environment_id,season_id) VALUES (11,22,33,1,6,150,150,8,9)")

        def settings(role):
            return ConnectionSettings(host="127.0.0.1", port=int(environment["DB_PORT"]), database=name,
                                      user="ti_"+role+"_"+token, password=password)
        review = PyMySQLRollupDatabase(PyMySQLConnectionFactory(settings("review")))
        rollup = PyMySQLRollupDatabase(PyMySQLConnectionFactory(settings("rollup")))
        databases.extend((review, rollup))
        p = packet()
        p["incidents"][0].update(start_utc_usec=120, fix_reference_digest="44"*32,
                                  first_verified_postfix=dict(boot_id=11, process_id=22, record_seq=33,
                                                              record_kind=6, occurrence_utc_usec=150))
        # Unknown and mismatched references must not leave even a header behind.
        for value, error in ((34, "postfix_fact_not_committed"), (33, "postfix_fact_mismatch")):
            bad = deepcopy(p)
            bad["incidents"][0]["first_verified_postfix"].update(record_seq=value, occurrence_utc_usec=151)
            try:
                review.register_incident_packet(bad)
            except incident.IncidentError as found:
                assert str(found) == error
            else:
                raise AssertionError(error)
        assert review.register_incident_packet(p)["status"] == "registered"
        assert review.register_incident_packet(p)["status"] == "already_registered"
        conflict = deepcopy(p)
        conflict["incidents"][0]["backlog_disposition"] = "delivered"
        try:
            review.register_incident_packet(conflict)
        except incident.IncidentError as error:
            assert str(error) == "registry_version_conflict"
        else:
            raise AssertionError("conflicting review was overwritten")

        def seed(generation, environment_id=8):
            with admin.cursor() as cursor:
                state = state_row(generation=generation, publication_status=0)
                state.update(coverage_start_utc_usec=100, coverage_end_utc_usec=200, environment_id=environment_id)
                def insert(table, row):
                    cursor.execute("INSERT INTO "+table+" ("+",".join(row)+") VALUES ("+",".join(["%s"]*len(row))+")", tuple(row.values()))
                insert("telemetry_rollup_state", state)
                for row in session_rows():
                    insert("telemetry_rollup_session", dict(row, definition_version=1, generation=generation, environment_id=environment_id, season_id=9, input_watermark=42, provisional=1))
        seed(7)
        target = RollupTarget(1, 7, 8, 9)
        assert rollup.publish_generation(target)["status"] == "published"

        def read(generation, environment_id=8, **kwargs):
            service = ReportDatabase(PyMySQLConnectionFactory(settings("report")))
            try:
                return service.read(ReportRequest("playtime", 1, environment_id, 9, generation=generation, **kwargs))
            finally:
                service.close()
        first = read(7)
        coverage = first["coverage"]["incident_coverage"]
        assert coverage["registry_version"] == 1 and coverage["unknown_end_count"] == 1
        assert coverage["incidents"][0]["end_utc_usec"] is None
        assert coverage["incidents"][0]["verified_record_seq"] == 33
        assert first["coverage"]["quality_flags"] & incident.QUALITY_INCIDENT_GAP
        assert first["summary"]["covered_active_usec"] == sum(r["covered_active_usec"] for r in session_rows())
        # Register a corrected observation without rewriting generation 7.
        correction = deepcopy(p)
        correction.update(registry_version=2, previous_registry_version=1)
        correction["incidents"][0].update(end_utc_usec=140, backlog_disposition="delivered")
        assert review.register_incident_packet(correction)["status"] == "registered"
        assert review.register_incident_packet(p)["status"] == "already_registered"
        assert read(7)["coverage"]["incident_coverage"] == coverage
        assert rollup.publish_generation(target)["status"] == "published"
        assert read(7)["coverage"]["incident_coverage"] == coverage
        published = rollup.read_report(target, "session_playtime")
        assert published.coverage.incident_coverage == coverage
        assert rollup.read_coverage(target).incident_coverage == coverage
        removal = deepcopy(correction)
        removal.update(registry_version=3, previous_registry_version=2, incidents=[])
        try:
            review.register_incident_packet(removal)
        except incident.IncidentError as error:
            assert str(error) == "incident_removed_without_withdrawal"
        else:
            raise AssertionError("incident silently removed")
        seed(8)
        assert rollup.publish_generation(RollupTarget(1, 8, 8, 9))["status"] == "published"
        second = read(8)["coverage"]["incident_coverage"]
        assert second["registry_version"] == 2 and second["unknown_end_count"] == 0
        assert second["incidents"][0]["end_utc_usec"] == 140
        # A commit whose reply disappears remains atomic and replayable. Use
        # actual SQL commits, then simulate only the lost acknowledgement.
        original_commit = review._commit
        def lose_review_reply():
            original_commit()
            raise AmbiguousCommit("synthetic lost review commit reply")
        review._commit = lose_review_reply
        third = deepcopy(correction)
        third.update(registry_version=3, previous_registry_version=2)
        third["incidents"][0].update(observation_provenance="reconstructed_separately", backlog_disposition="mixed")
        try:
            review.register_incident_packet(third)
        except AmbiguousCommit:
            pass
        else:
            raise AssertionError("lost commit reply was not surfaced")
        review._commit = original_commit
        assert review.register_incident_packet(third)["status"] == "already_registered"
        seed(9)
        original_commit = rollup._commit
        def lose_publication_reply():
            original_commit()
            raise AmbiguousCommit("synthetic lost publication commit reply")
        rollup._commit = lose_publication_reply
        assert rollup.publish_generation(RollupTarget(1, 9, 8, 9))["status"] == "published"
        rollup._commit = original_commit
        final = read(9)["coverage"]["incident_coverage"]
        assert final["registry_version"] == 3
        assert final["incidents"][0]["gap_remains_after_reconstruction"]
        assert final["quality_flags"] == incident.QUALITY_INCIDENT_GAP
        with tempfile.TemporaryDirectory() as directory:
            cache = ReportCache(directory)
            request = ReportRequest("playtime", 1, 8, 9, generation=9)
            cached_payload = read(9)
            cache.write(request, cached_payload)
            assert cache.read(request)["coverage"]["incident_coverage"] == final
        full = deepcopy(third)
        full.update(registry_version=4, previous_registry_version=3)
        for number in range(2, 65):
            full["incidents"].append(dict(packet()["incidents"][0], incident_id=number))
        full["incidents"][1].update(start_utc_usec=300, end_utc_usec=400)
        assert review.register_incident_packet(full)["incident_count"] == 64
        seed(10)
        assert rollup.publish_generation(RollupTarget(1, 10, 8, 9))["status"] == "published"
        capacity = read(10)["coverage"]["incident_coverage"]
        assert capacity["incident_count"] == 64 and capacity["relevant_incident_count"] == 63
        assert capacity["unknown_end_count"] == 62 and capacity["unresolved_backlog_count"] == 62
        try:
            read(10, max_bytes=4096)
        except BoundsExceeded:
            pass
        else:
            raise AssertionError("coverage exceeded a narrow report byte budget")
        with admin.cursor() as cursor:
            cursor.execute("UPDATE telemetry_rollup_state SET coverage_end_utc_usec=350 WHERE definition_version=1 AND generation=10 AND environment_id=8 AND season_id=9")
        advanced = read(10)["coverage"]["incident_coverage"]
        assert advanced["registry_version"] == 4 and advanced["relevant_incident_count"] == 64
        with admin.cursor() as cursor:
            cursor.execute("UPDATE telemetry_rollup_state SET quality_flags=%s WHERE definition_version=1 AND generation=10 AND environment_id=8 AND season_id=9", (1 << 17,))
        assert read(10)["coverage"]["incident_coverage"]["incidents"][1]["occurrence_relation"] == "possible_overlap"
        # Out-of-band source changes invalidate the packet digest and cannot be
        # blessed by an exact retry or a later publication.
        seed(11)
        with admin.cursor() as cursor:
            cursor.execute("UPDATE telemetry_incident SET end_utc_usec=150 WHERE environment_id=8 AND season_id=9 AND registry_version=4 AND incident_id=1")
        for action in (lambda: review.register_incident_packet(full), lambda: rollup.publish_generation(RollupTarget(1, 11, 8, 9))):
            try:
                action()
            except incident.IncidentError as error:
                assert str(error) == "stored_inventory_digest_mismatch"
            else:
                raise AssertionError("changed stored review was accepted")
        with admin.cursor() as cursor:
            cursor.execute("SELECT COUNT(*) AS count FROM telemetry_rollup_incident_coverage WHERE definition_version=1 AND generation=11 AND environment_id=8 AND season_id=9")
            assert cursor.fetchone()["count"] == 0
            cursor.execute("UPDATE telemetry_incident SET end_utc_usec=140 WHERE environment_id=8 AND season_id=9 AND registry_version=4 AND incident_id=1")
        seed(12, environment_id=10)
        assert rollup.publish_generation(RollupTarget(1, 12, 10, 9))["status"] == "published"
        missing = read(12, environment_id=10)["coverage"]["incident_coverage"]
        assert missing["status"] == "not_registered" and missing["quality_flags"] == incident.QUALITY_INVENTORY_UNKNOWN
        # SQL CHECKs reject partial nullable identities; SQL's UNKNOWN result
        # must not accidentally accept a half-present producer/verification.
        with admin.cursor() as cursor:
            for assignments in ("producer_boot_id=1", "verified_process_id=NULL", "record_kind_mask=1", "start_utc_usec=201,end_utc_usec=200"):
                try:
                    cursor.execute("UPDATE telemetry_incident SET " + assignments + " WHERE environment_id=8 AND season_id=9 AND registry_version=1 AND incident_id=1")
                except (pymysql.err.IntegrityError, pymysql.err.OperationalError) as error:
                    assert error.args[0] in (3819, 4025)
                else:
                    raise AssertionError("invalid SQL incident accepted")
        # Roles cannot cross their explicitly different authorities.
        for role, statements in {
            "report": ["SELECT * FROM telemetry_incident_registry LIMIT 0", "SELECT * FROM telemetry_interval LIMIT 0", "UPDATE telemetry_rollup_incident SET status=2 WHERE 0"],
            "review": ["UPDATE telemetry_incident SET status=2 WHERE 0", "DELETE FROM telemetry_incident WHERE 0", "SELECT * FROM player_data LIMIT 0"],
            "rollup": ["UPDATE telemetry_incident SET status=2 WHERE 0", "UPDATE telemetry_interval SET record_kind=1 WHERE 0"],
        }.items():
            factory = PyMySQLConnectionFactory(settings(role))
            connection = factory.connect()
            try:
                for statement in statements:
                    with connection.cursor() as cursor:
                        try:
                            cursor.execute(statement)
                        except pymysql.err.OperationalError as error:
                            assert error.args[0] in (1142, 1143)
                        else:
                            raise AssertionError("role permission unexpectedly allowed")
            finally:
                factory.close(connection)
        # Full manifest loaded; obtain the engine's actual normalized fingerprint.
        measured = subprocess.run(["bash", "migrations/verify_runtime_compatibility.sh", "--schema-only"],
                                  cwd=ROOT, env=environment, capture_output=True, text=True, timeout=45)
        output = measured.stdout + measured.stderr
        match = re.search(r"normalized metadata fingerprint mismatch: expected=[0-9a-f]{64} actual=([0-9a-f]{64})", output)
        if match is None:
            assert measured.returncode == 0, "runtime contract validation failed outside fingerprint measurement"
            value = json.loads((ROOT / "migrations/runtime_compatibility_manifest.json").read_text())
            engine = "mariadb10_11" if "mariadb" in os.environ["TELEMETRY_REPOSITORY_DB_IMAGE"] else "mysql8"
            fingerprint = value["normalized_metadata_fingerprints"][engine]
        else:
            assert measured.returncode != 0 and output.count("FAILED:") == 1, "runtime contract failed outside fingerprint measurement"
            fingerprint = match[1]
        artifact = {"engine": os.environ["TELEMETRY_REPOSITORY_DB_IMAGE"], "status": "passed",
                    "migration_head": "0054_telemetry_incident_coverage", "normalized_metadata_fingerprint": fingerprint}
        (ROOT / "bin").mkdir(exist_ok=True)
        engine = "mariadb" if "mariadb" in artifact["engine"] else "mysql"
        (ROOT / f"bin/telemetry-incident-{engine}.json").write_text(json.dumps(artifact, indent=2)+"\n")
        print(json.dumps(artifact), flush=True)
    finally:
        for database in databases:
            database.close()
        if admin is not None:
            with admin.cursor() as cursor:
                for user in users:
                    cursor.execute("DROP USER IF EXISTS %s@'%%'", (user,))
            admin.close()
        drop_sql_fixture(environment, command, name)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sql-fixture", action="store_true")
    args = parser.parse_args()
    result = unittest.TextTestRunner(verbosity=2).run(unittest.defaultTestLoader.loadTestsFromTestCase(IncidentSemantics))
    if not result.wasSuccessful():
        sys.exit(1)
    if args.sql_fixture:
        sql_qualification()
