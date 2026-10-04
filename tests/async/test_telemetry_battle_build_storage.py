#!/usr/bin/env python3
"""Disposable full-chain build schema, replay and independent loss inventory."""
from __future__ import annotations

import argparse
from copy import deepcopy
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
sys.path.insert(0, str(Path(__file__).resolve().parent))
from scripts.telemetry import battle_build_contract as builds, incident
from scripts.telemetry.db_access import ConnectionSettings, PyMySQLConnectionFactory, PyMySQLRollupDatabase, RAW_COLUMNS
from test_telemetry_battle_build_contract import BuildContractTests
from test_telemetry_incidents import runtime_fingerprint
from test_telemetry_repository import prepare_sql_fixture, drop_sql_fixture


def qualification():
    if os.environ.get("TELEMETRY_REPOSITORY_DISPOSABLE") != "1":
        raise RuntimeError("explicit disposable qualification required")
    import pymysql

    BuildContractTests.setUpClass()
    try:
        native = dict(BuildContractTests.value)
    finally:
        BuildContractTests.tearDownClass()
    environment, command, database = prepare_sql_fixture()
    admin, reviewer, user = None, None, None
    try:
        fingerprint = runtime_fingerprint(environment)
        manifest = json.loads((ROOT / "migrations/runtime_compatibility_manifest.json").read_text())
        engine_key = "mariadb10_11" if "mariadb" in os.environ["TELEMETRY_REPOSITORY_DB_IMAGE"] else "mysql8"
        assert fingerprint == manifest["normalized_metadata_fingerprints"][engine_key]
        admin = pymysql.connect(host="127.0.0.1", port=int(environment["DB_PORT"]),
            user=environment["DB_USER"], password=environment["DB_PASSWD"], database=database,
            autocommit=True, cursorclass=pymysql.cursors.DictCursor,
            connect_timeout=3, read_timeout=10, write_timeout=10)

        def query(sql, parameters=()):
            with admin.cursor() as cursor:
                cursor.execute(sql, parameters)
                return list(cursor.fetchall()) if cursor.description else []

        def insert(value, receipt):
            row = dict(value, boot_id=value["bctx_battle_boot_id"], process_id=value["bctx_battle_process_id"],
                record_seq=receipt, schema_version=1, record_kind=12, occurrence_utc_usec=value["bctx_at_utc_usec"],
                ingested_utc_usec=123456)
            builds.validate_raw_observation(row)
            names = tuple(row)
            query("INSERT INTO telemetry_interval (" + ",".join(names) + ") VALUES (" +
                  ",".join(["%s"] * len(names)) + ")", tuple(row[name] for name in names))

        insert(native, 22)
        actual = query("SELECT " + ",".join(RAW_COLUMNS) + " FROM telemetry_interval")[0]
        assert builds.validate_raw_observation(actual) == native
        original = query("SELECT * FROM telemetry_interval")
        for name, value in (("bctx_base_hit", None), ("bctx_definition_version", 2),
            ("bctx_battle_boot_id", 102), ("bctx_at_utc_usec", 0), ("record_kind", 11),
            ("bctx_actor_id", 0), ("bctx_actor_kind", 4), ("bctx_association_revision", 0),
            ("bctx_available", 1024), ("bctx_available", 1023 & ~1),
            ("bctx_available", 1023 & ~32), ("bctx_available", 1023 & ~64),
            ("bctx_equipment_occupied_slots_count", 4), ("bctx_equipment_digest", bytes(32)),
            ("bctx_epic_catalog_skills", 310), ("bctx_epic_learned_skills", 3),
            ("bctx_epic_digest", bytes(32)), ("bctx_affects_complete", 0),
            ("bctx_arena_team", 1), ("bctx_arena_membership", 0),
            ("bctx_context_quality", 0), ("bctx_quality_flags", 1024),
            ("bctx_status", 2), ("bctx_boundary", 6), ("duration_usec", 0)):
            try:
                query("UPDATE telemetry_interval SET " + name + "=%s", (value,))
            except (pymysql.err.IntegrityError, pymysql.err.OperationalError) as error:
                assert error.args[0] in (3819, 4025), (name, error.args[0])
            else:
                raise AssertionError("build CHECK accepted " + name)
        assert query("SELECT * FROM telemetry_interval") == original
        try:
            insert(native, 23)
        except pymysql.err.IntegrityError as error:
            assert error.args[0] == 1062
        else:
            raise AssertionError("second receipt bypassed build logical uniqueness")

        # Independent fields may be unknown without losing the known prefix.
        partial = dict(native, bctx_sequence=2, bctx_available=1023 & ~64,
            bctx_context_quality=128 | 1, bctx_epic_catalog_skills=0,
            bctx_epic_learned_skills=0, bctx_epic_digest=bytes(32))
        insert(partial, 24)
        gap = {name: native[name] if name in builds.METADATA_FIELDS else
               bytes(width) if signed is None else 0 for name, width, signed in builds.FIELD_LAYOUT}
        gap.update(bctx_sequence=3, bctx_status=2, bctx_boundary=8,
                   bctx_config_id=0, bctx_build_version=0, bctx_content_version=0)
        insert(gap, 25)
        for row in query("SELECT " + ",".join(RAW_COLUMNS) + " FROM telemetry_interval ORDER BY ingest_id"):
            builds.validate_raw_observation(row)

        token = hashlib.sha256(database.encode()).hexdigest()[:10]
        user, password = "tbb_review_" + token, "synthetic-build-review-" + token
        query("CREATE USER %s@'%%' IDENTIFIED BY %s", (user, password))
        for table in incident.schema_contract(5)[2:]:
            query(f"GRANT SELECT,INSERT ON `{database}`.`{table}` TO %s@'%%'", (user,))
        query(f"GRANT SELECT ON `{database}`.`telemetry_interval` TO %s@'%%'", (user,))
        reviewer = PyMySQLRollupDatabase(PyMySQLConnectionFactory(ConnectionSettings(
            host="127.0.0.1", port=int(environment["DB_PORT"]), database=database, user=user, password=password)))
        packet = incident.template(5)
        packet.update(environment_id=1, season_id=2, reviewer_token="11" * 32, review_evidence_digest="22" * 32)
        packet["incidents"][0].update(record_kind_mask=1 << 12, evidence_digest="33" * 32,
            fix_reference_digest="44" * 32, first_verified_postfix=dict(boot_id=101, process_id=201,
                record_seq=22, record_kind=12, occurrence_utc_usec=None))
        for field, value, reason in (("record_seq", 9999, "postfix_fact_not_committed"),
                                    ("occurrence_utc_usec", 0, "postfix_fact_mismatch")):
            bad = deepcopy(packet)
            bad["incidents"][0]["first_verified_postfix"][field] = value
            try:
                reviewer.register_incident_packet(bad)
            except incident.IncidentError as error:
                assert str(error) == reason
            else:
                raise AssertionError(reason)
        bad = deepcopy(packet)
        bad["season_id"] = 3
        try:
            reviewer.register_incident_packet(bad)
        except incident.IncidentError as error:
            assert str(error) == "postfix_fact_mismatch"
        else:
            raise AssertionError("build inventory accepted a different scope")
        execute = reviewer._execute

        def reject_detail(sql, parameters=()):
            if sql.startswith("INSERT INTO telemetry_incident_v5 "):
                raise RuntimeError("synthetic build review detail failure")
            return execute(sql, parameters)

        reviewer._execute = reject_detail
        try:
            try:
                reviewer.register_incident_packet(packet)
            except RuntimeError as error:
                assert str(error) == "synthetic build review detail failure"
            else:
                raise AssertionError("build review detail refusal missing")
        finally:
            reviewer._execute = execute
        assert query("SELECT COUNT(*) AS n FROM telemetry_incident_registry_v5")[0]["n"] == 0
        assert query("SELECT COUNT(*) AS n FROM telemetry_incident_v5")[0]["n"] == 0
        assert reviewer.register_incident_packet(packet)["status"] == "registered"
        assert reviewer.register_incident_packet(packet)["status"] == "already_registered"
        assert all(query("SELECT COUNT(*) AS n FROM " + table)[0]["n"] == 0
                   for schema in (1, 2, 3, 4) for table in incident.schema_contract(schema)[2:])
        # Re-run the new additive migration and its full metadata verifier.
        migration = ROOT / "migrations/immutable/0065_telemetry_battle_builds.sql"
        def apply():
            subprocess.run(command + [database], input=migration.read_bytes(), env=environment,
                           check=True, stdout=subprocess.DEVNULL)

        def verify(expected=True):
            result = subprocess.run(["bash", str(migration.with_suffix(".sh"))], env=environment,
                                    capture_output=True, text=True, timeout=45)
            assert (result.returncode == 0) == expected, result.stdout + result.stderr

        apply()
        verify()
        # Existing incompatible declarations remain visible; guarded reruns do
        # not silently rewrite either schema drift or the retained point facts.
        query("ALTER TABLE telemetry_interval MODIFY COLUMN bctx_base_str INT NULL")
        verify(False)
        apply()
        verify(False)
        query("ALTER TABLE telemetry_interval MODIFY COLUMN bctx_base_str SMALLINT NULL")
        verify()
        query("ALTER TABLE telemetry_interval DROP INDEX uq_telemetry_battle_build, "
              "ADD UNIQUE KEY uq_telemetry_battle_build (bctx_sequence,bctx_battle_boot_id,bctx_battle_process_id)")
        verify(False)
        apply()
        verify(False)
        query("ALTER TABLE telemetry_interval DROP INDEX uq_telemetry_battle_build, "
              "ADD UNIQUE KEY uq_telemetry_battle_build (bctx_battle_boot_id,bctx_battle_process_id,bctx_sequence)")
        verify()
        check_name = "chk_telemetry_bctx_equipment"
        original_check = query("SELECT CHECK_CLAUSE AS clause FROM information_schema.check_constraints "
                               "WHERE constraint_schema=DATABASE() AND constraint_name=%s", (check_name,))[0]["clause"]
        drop = "DROP CONSTRAINT" if engine_key == "mariadb10_11" else "DROP CHECK"
        query(f"ALTER TABLE telemetry_interval {drop} {check_name}")
        query(f"ALTER TABLE telemetry_interval ADD CONSTRAINT {check_name} CHECK (bctx_equipment_occupied_slots_count<=43)")
        verify(False)
        apply()
        verify(False)
        query(f"ALTER TABLE telemetry_interval {drop} {check_name}")
        query(f"ALTER TABLE telemetry_interval ADD CONSTRAINT {check_name} CHECK (" + original_check + ")")
        verify()
        assert runtime_fingerprint(environment) == fingerprint
        assert query("SELECT * FROM telemetry_interval WHERE ingest_id=1") == original
        assert query("SELECT COUNT(*) AS n FROM telemetry_interval")[0]["n"] == 3
        proof = dict(engine=os.environ["TELEMETRY_REPOSITORY_DB_IMAGE"], status="passed",
            migration_head=migration.stem, normalized_metadata_fingerprint=fingerprint,
            exact_fields=110, wire_bytes=447, independent_incident_schema=5,
            sealed_definition_five_preserved=True, full_schema_rerun=True,
            schema_drift_refused=True, restored_metadata_fingerprint=True)
        name = "mariadb" if "mariadb" in proof["engine"] else "mysql"
        (ROOT / f"bin/telemetry-build-storage-{name}.json").write_text(json.dumps(proof, indent=2) + "\n")
        print(json.dumps(proof), flush=True)
    finally:
        if reviewer is not None:
            reviewer.close()
        if admin is not None:
            if user is not None:
                with admin.cursor() as cursor:
                    cursor.execute("DROP USER IF EXISTS %s@'%%'", (user,))
            admin.close()
        drop_sql_fixture(environment, command, database)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sql-fixture", action="store_true")
    arguments = parser.parse_args()
    if not arguments.sql_fixture:
        parser.error("--sql-fixture and an owned disposable loopback database are required")
    qualification()
