#!/usr/bin/env python3
"""Disposable full-chain contribution schema, private review and compatibility.

Uses exact rows emitted by the native accumulator. Repository delivery retries,
quarantine and logical replay are exercised by test_telemetry_repository.py.
"""
from __future__ import annotations

import argparse
from copy import deepcopy
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time
from types import SimpleNamespace

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/telemetry"))
sys.path.insert(0, str(Path(__file__).resolve().parent))

import battle_contribution_contract as contributions
import incident
from db_access import (AmbiguousCommit, ConnectionSettings, PyMySQLConnectionFactory,
                       PyMySQLRollupDatabase, RAW_COLUMNS)
from report import ReportDatabase
from rollup_definitions import RollupTarget
from rollup_engine import RollupBounds, RollupEngine, build_page_contributions
from telemetry_rollup_fixtures import FIXTURE_DIR, golden_rows
from test_telemetry_battle_contribution_contract import ContributionContractTests
from test_telemetry_incidents import runtime_fingerprint
from test_telemetry_repository import prepare_sql_fixture, drop_sql_fixture


def qualification():
    if os.environ.get("TELEMETRY_REPOSITORY_DISPOSABLE") != "1":
        raise RuntimeError("explicit disposable qualification required")
    import pymysql

    ContributionContractTests.setUpClass()
    try:
        case = next(item["case"] for item, row in zip(ContributionContractTests.sources,
                    ContributionContractTests.rows) if row == ContributionContractTests.value)
        values = [row for item, row in zip(ContributionContractTests.sources,
                    ContributionContractTests.rows) if item["case"] == case]
        source = [ContributionContractTests.raw_segment(row, 20000 + index)
                  for index, row in enumerate(values)]
        edge_cases = []
        for predicate in (
            lambda row: row["bc_casting_unresolved"] == 1,
            lambda row: row["bc_quality_flags"] & (1 << 7),
            lambda row: row["bc_quality_flags"] & (1 << 9),
            lambda row: row["bc_end_reason"] == 4,
            lambda row: row["bc_actor_kind"] == 2,
            lambda row: row["bc_actor_kind"] == 3,
            lambda row: row["bc_damage_dealt"] == (1 << 64) - 1,
        ):
            row = next(row for row in ContributionContractTests.rows if predicate(row))
            edge_cases.append(ContributionContractTests.raw_segment(row, 10000))
    finally:
        ContributionContractTests.tearDownClass()
    environment, command, name = prepare_sql_fixture()
    admin, adapters, users = None, [], []
    try:
        fingerprint = runtime_fingerprint(environment)
        admin = pymysql.connect(host="127.0.0.1", port=int(environment["DB_PORT"]),
            user=environment["DB_USER"], password=environment["DB_PASSWD"], database=name,
            autocommit=True, cursorclass=pymysql.cursors.DictCursor,
            connect_timeout=3, read_timeout=10, write_timeout=10)

        def query(sql, parameters=()):
            with admin.cursor() as cursor:
                cursor.execute(sql, parameters)
                return list(cursor.fetchall()) if cursor.description else []

        def count(table):
            return query("SELECT COUNT(*) AS n FROM " + table)[0]["n"]

        def insert(row):
            columns = tuple(column for column in RAW_COLUMNS if column in row and column != "ingest_id")
            query("INSERT INTO telemetry_interval (" + ",".join(columns) + ") VALUES (" +
                  ",".join(["%s"] * len(columns)) + ")", tuple(row[column] for column in columns))

        # Separate disposable inserts retain exact native values, including
        # counters/flags whose meaning a default zero fixture would not expose.
        for row in edge_cases:
            insert(row)
            actual = query("SELECT " + ",".join(RAW_COLUMNS) + " FROM telemetry_interval")[0]
            assert contributions.validate_raw_segment(actual) == {field: row[field] for field in contributions.FIELDS}
            query("DELETE FROM telemetry_interval")
        unavailable = dict(source[0], bc_available_metrics=0)
        for _bit, fields in contributions.COUNTER_FAMILIES:
            for field in fields: unavailable["bc_" + field] = 0
        contributions.validate_raw_segment(unavailable)
        insert(unavailable)
        try:
            query("UPDATE telemetry_interval SET bc_damage_dealt=1")
        except (pymysql.err.IntegrityError, pymysql.err.OperationalError) as error:
            assert error.args[0] in (3819, 4025)
        else:
            raise AssertionError("unavailable family accepted a counter")
        query("DELETE FROM telemetry_interval")
        query("ALTER TABLE telemetry_interval AUTO_INCREMENT=1")
        for row in source:
            insert(row)
        for raw in query("SELECT " + ",".join(RAW_COLUMNS) + " FROM telemetry_interval ORDER BY ingest_id"):
            contributions.validate_raw_segment(raw)
        assert count("telemetry_interval") == len(source)
        original = query("SELECT * FROM telemetry_interval ORDER BY ingest_id")
        for column, value in (("bc_damage_dealt", None), ("bc_definition_version", 2),
            ("bc_battle_boot_id", source[0]["boot_id"] + 1), ("bc_decision_utc_usec", source[0]["occurrence_utc_usec"] + 1),
            ("bc_scope_zone_vnum", 700), ("record_kind", 8), ("bc_available_metrics", 0),
            ("bc_overhealing", source[0]["bc_healing_attempted"] + 1), ("bc_casting_unresolved", 2),
            ("bc_start_monotonic_usec", source[0]["bc_observed_through_monotonic_usec"] + 1),
            ("bc_actor_context_version", 2), ("bc_actor_pid", -1)):
            try:
                query("UPDATE telemetry_interval SET " + column + "=%s WHERE ingest_id=1", (value,))
            except (pymysql.err.IntegrityError, pymysql.err.OperationalError) as error:
                assert error.args[0] in (3819, 4025)
            else:
                raise AssertionError("contribution CHECK accepted " + column)
        assert query("SELECT * FROM telemetry_interval ORDER BY ingest_id") == original
        try:
            insert(dict(source[0], record_seq=30000))
        except pymysql.err.IntegrityError as error:
            assert error.args[0] == 1062
        else:
            raise AssertionError("second receipt bypassed contribution logical uniqueness")

        token = hashlib.sha256(name.encode()).hexdigest()[:10]
        password = "synthetic-contribution-storage-" + token
        base = ("telemetry_rollup_state", "telemetry_rollup_session", "telemetry_cohort_day",
                "telemetry_cohort_member", "telemetry_player_day")
        observation = ("telemetry_rollup_progression_day", "telemetry_rollup_level_event",
                       "telemetry_rollup_encounter", "telemetry_rollup_encounter_participant", "telemetry_rollup_combat_actor")
        public = ("telemetry_rollup_incident_coverage", "telemetry_rollup_incident",
                  "telemetry_rollup_identity_coverage", "telemetry_rollup_identity_effort", "telemetry_rollup_portfolio_xp")
        private = tuple(table for version in (1, 2, 3, 4) for table in incident.schema_contract(version)[2:]) + (
                  "telemetry_identity_registry", "telemetry_identity_association")
        grants = {
            "review": {table: "SELECT,INSERT" for table in incident.schema_contract(4)[2:]},
            "rollup": {table: "SELECT,INSERT,UPDATE" for table in (*base, *observation)},
            "report": {table: "SELECT" for table in (*base, *observation, *public, "telemetry_generation_identity")},
            "writer": {"telemetry_interval": "SELECT,INSERT"},
        }
        grants["review"]["telemetry_interval"] = "SELECT"
        grants["rollup"].update({table: "SELECT" for table in (*private, "telemetry_interval")})
        grants["rollup"].update({table: "SELECT,INSERT" for table in (*public, "telemetry_generation_identity", "telemetry_identity_input")})
        grants["rollup"]["telemetry_rollup_identity_coverage"] = "SELECT,INSERT,UPDATE"
        for role, tables in grants.items():
            user = "tbcs_" + role + "_" + token
            query("CREATE USER %s@'%%' IDENTIFIED BY %s", (user, password))
            users.append(user)
            for table, permissions in tables.items():
                query(f"GRANT {permissions} ON `{name}`.`{table}` TO %s@'%%'", (user,))

        def settings(role):
            return ConnectionSettings(host="127.0.0.1", port=int(environment["DB_PORT"]), database=name,
                user="tbcs_" + role + "_" + token, password=password)

        def adapter(role):
            value = PyMySQLRollupDatabase(PyMySQLConnectionFactory(settings(role)))
            adapters.append(value)
            return value

        reviewer, rollup, reporter = adapter("review"), adapter("rollup"), adapter("report")
        scope = (source[0]["bc_environment_id"], source[0]["bc_season_id"])
        at = source[0]["occurrence_utc_usec"]
        state = dict(publication_status=0, coverage_start_utc_usec=at - 100,
                     coverage_end_utc_usec=at + 100, quality_flags=0)
        def target(generation):
            return SimpleNamespace(definition_version=5, generation=generation,
                environment_id=scope[0], season_id=scope[1], scope_tuple=(5, generation, *scope))
        def snapshot(generation, published=False):
            with rollup._identity_transaction(scope):
                rollup._publish_incident_coverage(target(generation), dict(state, publication_status=int(published)))
        def read(generation):
            service = ReportDatabase(PyMySQLConnectionFactory(settings("report")))
            service.deadline = time.monotonic() + 5
            try:
                service._begin_snapshot()
                return service._read_incident_coverage(target(generation), state, max_bytes=262144)
            finally:
                service.close()

        snapshot(1)
        assert read(1)["status"] == "not_registered" and read(1)["registry_schema_version"] == 4
        packet = incident.template(4)
        packet.update(environment_id=scope[0], season_id=scope[1], reviewer_token="11" * 32,
                      review_evidence_digest="22" * 32)
        packet["incidents"][0].update(record_kind_mask=1 << 11, evidence_digest="33" * 32,
            fix_reference_digest="44" * 32, first_verified_postfix=dict(boot_id=source[0]["boot_id"],
            process_id=source[0]["process_id"], record_seq=source[0]["record_seq"], record_kind=11, occurrence_utc_usec=at))
        for field, value, reason in (("record_seq", 999999, "postfix_fact_not_committed"),
                                     ("occurrence_utc_usec", at + 1, "postfix_fact_mismatch")):
            malformed = deepcopy(packet)
            malformed["incidents"][0]["first_verified_postfix"][field] = value
            try: reviewer.register_incident_packet(malformed)
            except incident.IncidentError as error: assert str(error) == reason
            else: raise AssertionError(reason)
            assert count("telemetry_incident_registry_v4") == 0
        malformed = deepcopy(packet)
        malformed["season_id"] += 1
        try: reviewer.register_incident_packet(malformed)
        except incident.IncidentError as error: assert str(error) == "postfix_fact_mismatch"
        else: raise AssertionError("contribution incident accepted a different scope")
        execute = reviewer._execute
        def fail_detail(sql, parameters=()):
            if sql.startswith("INSERT INTO telemetry_incident_v4 "):
                raise RuntimeError("synthetic contribution review detail failure")
            return execute(sql, parameters)
        reviewer._execute = fail_detail
        try:
            try: reviewer.register_incident_packet(packet)
            except RuntimeError as error: assert str(error) == "synthetic contribution review detail failure"
            else: raise AssertionError("detail fault did not refuse")
        finally:
            reviewer._execute = execute
        assert count("telemetry_incident_registry_v4") == count("telemetry_incident_v4") == 0
        assert reviewer.register_incident_packet(packet)["status"] == "registered"
        assert reviewer.register_incident_packet(packet)["status"] == "already_registered"
        snapshot(2)
        before = read(2)
        assert before["incidents"][0]["verified_record_kind"] == 11
        assert before["unknown_end_count"] == 1 and not before["incident_inventory_complete"]
        correction = deepcopy(packet)
        correction.update(registry_version=2, previous_registry_version=1)
        correction["incidents"][0].update(end_utc_usec=at, backlog_disposition="delivered")
        with tempfile.TemporaryDirectory(dir=ROOT / "bin") as directory:
            path = Path(directory) / "review.json"
            path.write_text(json.dumps(correction), encoding="utf-8")
            cli_env = dict(os.environ)
            for field in ("host", "port", "database", "user", "password"):
                cli_env["TELEMETRY_INCIDENT_DB_" + field.upper()] = str(getattr(settings("review"), field))
            completed = subprocess.run([sys.executable, "scripts/telemetry/incident.py", str(path), "--register"],
                cwd=ROOT, env=cli_env, capture_output=True, text=True, timeout=15)
            assert completed.returncode == 0, completed.stderr
            assert json.loads(completed.stdout)["status"] == "registered"
        snapshot(2, published=True)
        assert read(2) == before and read(1)["status"] == "not_registered"
        snapshot(3)
        assert read(3)["unknown_end_count"] == 0 and read(3)["registry_version"] == 2
        commit = reviewer._commit
        def lose_reply():
            commit()
            raise AmbiguousCommit("synthetic lost contribution review reply")
        reviewer._commit = lose_reply
        third = deepcopy(correction)
        third.update(registry_version=3, previous_registry_version=2)
        third["incidents"][0]["observation_provenance"] = "reconstructed_separately"
        try:
            try: reviewer.register_incident_packet(third)
            except AmbiguousCommit: assert not reviewer.connected
            else: raise AssertionError("review ambiguity not surfaced")
        finally:
            reviewer._commit = commit
        assert reviewer.register_incident_packet(third)["status"] == "already_registered"
        assert all(count(incident.schema_contract(version)[2]) == 0 for version in (1, 2, 3))
        for role, statements in {
            "report": ("SELECT * FROM telemetry_incident_v4 LIMIT 0", "SELECT * FROM telemetry_interval LIMIT 0"),
            "review": ("UPDATE telemetry_incident_v4 SET status=2 WHERE 0", "DELETE FROM telemetry_incident_v4 WHERE 0"),
            "rollup": ("INSERT INTO telemetry_incident_v4 SELECT * FROM telemetry_incident_v4 WHERE 0",),
            "writer": ("SELECT * FROM telemetry_incident_v4 LIMIT 0", "UPDATE telemetry_interval SET record_kind=11 WHERE 0"),
        }.items():
            factory = PyMySQLConnectionFactory(settings(role))
            connection = factory.connect()
            try:
                for statement in statements:
                    with connection.cursor() as cursor:
                        try: cursor.execute(statement)
                        except pymysql.err.OperationalError as error: assert error.args[0] in (1142, 1143)
                        else: raise AssertionError("unexpected contribution storage role permission")
            finally:
                factory.close(connection)

        _, legacy = golden_rows(FIXTURE_DIR / "normal_interval.json")
        for row in legacy: insert(row)
        interval = next(row for row in legacy if row["record_kind"] == 1)
        through = query("SELECT MAX(ingest_id) AS n FROM telemetry_interval")[0]["n"]
        bounds = RollupBounds(page_size=2, max_rows=100, max_total_bytes=16 * 1024 * 1024,
                            max_output_fanout=200, max_runtime_s=10)
        for version in (1, 2, 3):
            old = RollupTarget(version, 1, interval["environment_id"], interval["season_id"])
            expected = build_page_contributions(legacy, old, max_page_bytes=1_000_000)
            if version == 3: rollup.reserve_identity_generation(old.scope_tuple, None)
            assert RollupEngine(rollup).run(old, bounds=bounds, through_ingest_id=through).complete
            rollup.publish_generation(old)
            result = reporter.read_report(old, "session_playtime")
            assert result.rows and result.coverage.input_watermark == through
            assert sum(row["covered_active_usec"] for row in result.rows) == sum(
                value.covered["covered_active_usec"] for value in expected.sessions.values())
        assert count("telemetry_interval") == len(source) + len(legacy)

        for database in adapters: database.close()
        sql = ROOT / "migrations/immutable/0062_telemetry_battle_contributions.sql"
        verifier = ["bash", str(sql.with_suffix(".sh"))]
        preserved = query("SELECT * FROM telemetry_interval ORDER BY ingest_id")
        def apply():
            subprocess.run(command + [name], input=sql.read_bytes(), env=environment,
                           check=True, timeout=20, stdout=subprocess.DEVNULL)
        def verify(success=True):
            result = subprocess.run(verifier, cwd=ROOT, env=environment, capture_output=True, timeout=45)
            assert (result.returncode == 0) is success, result.stderr.decode(errors="replace")
        apply()
        verify()
        assert query("SELECT * FROM telemetry_interval ORDER BY ingest_id") == preserved
        query("ALTER TABLE telemetry_interval MODIFY bc_actor_power_band SMALLINT UNSIGNED NULL DEFAULT 1")
        verify(False)
        apply()
        verify(False)
        query("ALTER TABLE telemetry_interval MODIFY bc_actor_power_band SMALLINT UNSIGNED NULL DEFAULT NULL")
        verify()
        drop = "DROP CONSTRAINT" if "mariadb" in environment["TELEMETRY_REPOSITORY_DB_IMAGE"] else "DROP CHECK"
        query("ALTER TABLE telemetry_incident_v4 " + drop + " chk_incident_family_v4")
        query("ALTER TABLE telemetry_incident_v4 ADD CONSTRAINT chk_incident_family_v4 CHECK (record_kind_mask BETWEEN 2 AND 8190 AND (record_kind_mask & 1)=0)")
        verify(False)
        apply()
        verify(False)
        query("ALTER TABLE telemetry_incident_v4 " + drop + " chk_incident_family_v4")
        query("ALTER TABLE telemetry_incident_v4 ADD CONSTRAINT chk_incident_family_v4 CHECK (record_kind_mask BETWEEN 2 AND 4094 AND (record_kind_mask & 1)=0)")
        verify()
        assert runtime_fingerprint(environment) == fingerprint
        suffix = "mariadb" if "mariadb" in environment["TELEMETRY_REPOSITORY_DB_IMAGE"] else "mysql"
        artifact = dict(engine=environment["TELEMETRY_REPOSITORY_DB_IMAGE"], status="passed",
            migration_head=sql.stem, normalized_metadata_fingerprint=fingerprint,
            apply_checksum=hashlib.sha256(sql.read_bytes()).hexdigest(), verify_checksum=hashlib.sha256(sql.with_suffix(".sh").read_bytes()).hexdigest(),
            contribution_field_count=len(contributions.FIELDS), independent_review_history=True,
            maintained_cli=True, private_roles=True, retained_history=True,
            earlier_definition_compatibility=True, schema_drift_refused=True, restored_metadata=True)
        (ROOT / f"bin/telemetry-contribution-storage-{suffix}.json").write_text(json.dumps(artifact, indent=2) + "\n", encoding="utf-8")
        print(json.dumps(artifact), flush=True)
    finally:
        for database in adapters: database.close()
        if admin is not None:
            try:
                for user in users:
                    with admin.cursor() as cursor: cursor.execute("DROP USER IF EXISTS %s@'%%'", (user,))
            finally:
                admin.close()
        drop_sql_fixture(environment, command, name)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sql-fixture", action="store_true")
    args = parser.parse_args()
    if args.sql_fixture:
        qualification()
    else:
        parser.error("use the disposable --sql-fixture or run_telemetry_repository_sql.sh --contribution-storage")
