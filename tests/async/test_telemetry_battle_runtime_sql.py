#!/usr/bin/env python3
"""Actual gameplay capture/worker/native-writer proof on disposable loopback SQL.

The gameplay objects and private connection factory are fixture seams. This does
not qualify a running server, account authentication, or production activation.
"""
from __future__ import annotations

import argparse
from dataclasses import replace
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

from test_telemetry_gameplay_adapters import ROOT, compile_gameplay
from test_telemetry_battle_history import (BattleHistoryTests, qualify_native_history,
    qualify_native_control_history, qualify_retained_native_source)
from test_telemetry_repository import prepare_sql_fixture, drop_sql_fixture
from test_telemetry_incidents import runtime_fingerprint

sys.path.insert(0, str(ROOT / "scripts/telemetry"))
import battle_contract
import battle_contribution_contract
import battle_build_contract
import battle_source as retained_source
import battle_history
import battle_publication
import incident
import identity_history
from db_access import (RAW_COLUMNS, AmbiguousCommit, ConnectionSettings, GenerationConflict,
    PyMySQLConnectionFactory, PyMySQLRollupDatabase)
from rollup_definitions import RollupTarget, PUBLICATION_BUILDING, PUBLICATION_PUBLISHED, PUBLICATION_SUPERSEDED
from rollup_engine import RollupEngine, RollupBounds, BoundsExceeded, SemanticError
from test_telemetry_observations import ownership


def qualify_native_build_storage(query, executable, run_environment, export, environment, command, name):
    """Actual cached reader/worker/writer values; retained reports remain sealed."""
    origin = query("SELECT MAX(ingest_id) AS n FROM telemetry_interval")[0]["n"]
    public = query("SELECT * FROM telemetry_rollup_battle_row ORDER BY definition_version,generation,environment_id,season_id,row_kind,row_key")
    subprocess.run([str(executable), "--native-build-sql"], cwd=ROOT,
        env=dict(run_environment, TELEMETRY_BUILD_CAPTURE_EXPORT=str(export)), check=True, timeout=30)
    emitted = [json.loads(line) for line in export.read_text(encoding="utf-8").splitlines()]
    for row in emitted:
        for field, _, signed in battle_build_contract.FIELD_LAYOUT:
            if signed is None:
                row[field] = bytes.fromhex(row[field])
        battle_build_contract.validate_raw_observation(row)
    receipt = lambda row: (row["boot_id"], row["process_id"], row["record_seq"])
    stored = query("SELECT " + ",".join(RAW_COLUMNS) +
        " FROM telemetry_interval WHERE record_kind=12 AND ingest_id>%s ORDER BY boot_id,process_id,record_seq", (origin,))
    assert len(stored) == len(emitted) == 20
    for expected, raw in zip(sorted(emitted, key=receipt), stored, strict=True):
        assert all(raw[name] == value for name, value in expected.items()), "native build/SQL field drift"
        battle_build_contract.validate_raw_observation(raw)
        basis = query("SELECT " + ",".join(RAW_COLUMNS) + " FROM telemetry_interval WHERE record_kind=10 AND "
            "battle_boot_id=%s AND battle_process_id=%s AND battle_seq=%s AND "
            "battle_revision=%s AND battle_fact_sequence=%s", tuple(raw[name] for name in (
                "bctx_battle_boot_id", "bctx_battle_process_id", "bctx_battle_seq",
                "bctx_association_revision", "bctx_association_fact_sequence")))
        assert len(basis) == 1 and basis[0]["battle_at_monotonic_usec"] <= raw["bctx_at_monotonic_usec"]
        if raw["bctx_config_id"]:
            config = query("SELECT build_version,content_version,environment_id,season_id FROM telemetry_config WHERE config_id=%s", (raw["bctx_config_id"],))
            assert len(config) == 1 and tuple(config[0][name] for name in (
                "build_version", "content_version", "environment_id", "season_id")) == tuple(raw[name] for name in (
                    "bctx_build_version", "bctx_content_version", "bctx_environment_id", "bctx_season_id"))
    assert query("SELECT * FROM telemetry_rollup_battle_row ORDER BY definition_version,generation,environment_id,season_id,row_kind,row_key") == public
    assert query("SELECT COUNT(*) AS n FROM telemetry_quarantine")[0]["n"] == 0
    qualification = qualify_native_build_publication(query, environment, command, name, origin)
    return dict(qualification, native_build_capture=True, native_build_records=20, native_build_fields=110,
        native_build_sql_exact=True, native_build_exact_association=True,
        native_build_configuration_gaps=True, native_build_partial_families=True,
        native_build_periodic_lifetime_safe=True, native_build_retained_publication=True,
        native_build_running_server=False)


def qualify_native_build_publication(query, environment, command, name, origin):
    """Exact native points retained/published with restricted definition-6 roles."""
    token = hashlib.sha256(name.encode()).hexdigest()[:12]
    password = "synthetic-build-publication-" + token
    users, adapters = [], []
    selected = query("SELECT " + ",".join(RAW_COLUMNS) +
        " FROM telemetry_interval WHERE ingest_id>%s AND record_kind IN (9,10,11,12) ORDER BY ingest_id", (origin,))
    first = next(row for row in selected if row["record_kind"] == 12)
    target = RollupTarget(6, 1, first["bctx_environment_id"], first["bctx_season_id"])
    through = query("SELECT MAX(ingest_id) AS n FROM telemetry_interval")[0]["n"]
    tables = ("telemetry_battle_source_v6", "telemetry_battle_input_v6", "telemetry_rollup_battle_coverage_v6", "telemetry_rollup_battle_row_v6")
    old_public = {table: query("SELECT * FROM " + table + " ORDER BY definition_version,generation,environment_id,season_id" +
        (",row_kind,row_key" if table.endswith("_row") else "")) for table in ("telemetry_rollup_battle_row", "telemetry_rollup_battle_coverage")}
    scope_where = "definition_version=%s AND generation=%s AND environment_id=%s AND season_id=%s"
    try:
        grants = {
            "rollup": {"telemetry_rollup_state": "SELECT,INSERT,UPDATE", "telemetry_generation_identity": "SELECT,INSERT",
                "telemetry_interval": "SELECT", "telemetry_config": "SELECT", "telemetry_identity_registry": "SELECT",
                "telemetry_identity_association": "SELECT", "telemetry_incident_registry_v5": "SELECT", "telemetry_incident_v5": "SELECT",
                tables[0]: "SELECT,INSERT,UPDATE", tables[1]: "SELECT,INSERT", tables[2]: "SELECT,INSERT", tables[3]: "SELECT,INSERT",
                "telemetry_rollup_incident_coverage": "SELECT,INSERT", "telemetry_rollup_incident": "SELECT,INSERT"},
            "report": {"telemetry_rollup_state": "SELECT", "telemetry_generation_identity": "SELECT", tables[2]: "SELECT", tables[3]: "SELECT",
                "telemetry_rollup_incident_coverage": "SELECT", "telemetry_rollup_incident": "SELECT"},
            "review": {"telemetry_interval": "SELECT", "telemetry_incident_registry_v5": "SELECT,INSERT", "telemetry_incident_v5": "SELECT,INSERT"},
        }
        for role, permissions in grants.items():
            user = "tbr_build_" + role + "_" + token
            query("CREATE USER %s@'%%' IDENTIFIED BY %s", (user, password))
            users.append(user)
            for table, permission in permissions.items():
                query(f"GRANT {permission} ON `{name}`.`{table}` TO %s@'%%'", (user,))

        def adapter(role):
            database = PyMySQLRollupDatabase(PyMySQLConnectionFactory(ConnectionSettings(host="127.0.0.1",
                port=int(environment["DB_PORT"]), database=name, user="tbr_build_" + role + "_" + token, password=password)))
            adapters.append(database)
            return database

        rollup, reporter, reviewer = adapter("rollup"), adapter("report"), adapter("review")
        rollup.reserve_identity_generation(target.scope_tuple, None)
        commit, lost_page_ack = rollup._commit, [False]
        def lose_page_ack():
            commit()
            if not lost_page_ack[0]:
                lost_page_ack[0] = True
                raise AmbiguousCommit("fixture_build_source_lost_ack")
        rollup._commit = lose_page_ack
        try:
            assert RollupEngine(rollup).run(target, origin_ingest_id=origin, through_ingest_id=through,
                bounds=RollupBounds(page_size=3, max_runtime_s=30)).complete
        finally:
            rollup._commit = commit
        assert lost_page_ack[0]
        retained = rollup.read_battle_source(target)
        assert retained.header["build_count"] == 20 and len(retained.facts) == len(selected)
        for raw, value, config in zip(selected, retained.facts, retained.configurations, strict=True):
            assert all(value[field] == raw[field] for field in retained_source.SOURCE_COLUMNS[raw["record_kind"]])
            if raw["record_kind"] == 12 and raw["bctx_config_id"]:
                assert config == {field: raw["bctx_" + field] for field in retained_source.CONFIG_COLUMNS}
            else:
                assert config is None

        for table in (tables[0], tables[1], "telemetry_interval", "telemetry_config", "telemetry_incident_registry_v5", "telemetry_incident_v5"):
            try:
                reporter._execute("SELECT COUNT(*) AS n FROM " + table)
            except Exception as error:
                assert getattr(error.__cause__, "args", (None,))[0] == 1142 or getattr(error, "args", (None,))[0] == 1142
            else:
                raise AssertionError("build report role can read private inputs")
        try:
            rollup.publish_generation(target, bounds=RollupBounds(max_output_fanout=1))
        except SemanticError as error:
            assert "output_capacity" in str(error)
        else:
            raise AssertionError("build publication fanout bound not enforced")
        assert rollup.read_state(target)["publication_status"] == PUBLICATION_BUILDING
        assert query("SELECT COUNT(*) AS n FROM " + tables[2])[0]["n"] == 0
        assert not rollup.read_battle_source(target).header["publication_complete"]

        # A detail refusal rolls back coverage, source completion and status.
        execute = rollup._execute
        def refuse_detail(statement, parameters=()):
            if statement.startswith("INSERT INTO " + tables[3]):
                raise SemanticError("fixture_build_detail_refusal")
            return execute(statement, parameters)
        rollup._execute = refuse_detail
        try:
            try:
                rollup.publish_generation(target)
            except SemanticError as error:
                assert str(error) == "fixture_build_detail_refusal"
            else:
                raise AssertionError("build detail refusal was not reached")
        finally:
            rollup._execute = execute
        assert query("SELECT COUNT(*) AS n FROM " + tables[2])[0]["n"] == 0
        assert query("SELECT COUNT(*) AS n FROM telemetry_rollup_incident_coverage WHERE " + scope_where, target.scope_tuple)[0]["n"] == 0

        # Review an explicit independent schema-5 inventory; older schema-4
        # reviews are never reused to certify the new family.
        occurrences = [row["occurrence_utc_usec"] for row in selected if row["occurrence_utc_usec"] != incident.UTC_UNKNOWN]
        packet = dict(incident.template(5), incidents=[], environment_id=target.environment_id, season_id=target.season_id,
            reviewer_token="a" * 64, review_evidence_digest="b" * 64,
            reviewed_from_utc_usec=min(occurrences) - 1, reviewed_through_utc_usec=max(occurrences) + 1)
        assert reviewer.register_incident_packet(packet)["status"] == "registered"
        lost_public_ack = [False]
        def lose_public_ack():
            commit()
            if not lost_public_ack[0]:
                lost_public_ack[0] = True
                raise AmbiguousCommit("fixture_build_publication_lost_ack")
        rollup._commit = lose_public_ack
        try:
            assert rollup.publish_generation(target)["status"] == "published"
        finally:
            rollup._commit = commit
        assert lost_public_ack[0]
        reports = {report: reporter.read_report(target, report) for report in battle_publication.BUILD_ROW_KINDS}
        points = reports["battle_build_points"]
        assert len(points.rows) == 20 and not points.truncated
        assert points.coverage.incident_coverage["registry_schema_version"] == 5
        assert points.coverage.battle_coverage["build_row_count"] == 20
        assert points.coverage.battle_coverage["verified_build_links"] + points.coverage.battle_coverage["partial_build_links"] == 20
        assert all(not row["continuous_build_exposure_implied"] for row in points.rows)
        raw_points = {battle_build_contract.observation_key(row): row for row in selected if row["record_kind"] == 12}
        for row in points.rows:
            raw = raw_points[battle_build_contract.observation_key(row)]
            assert all(row[field] == (raw[field].hex() if field in battle_build_contract.BYTE_FIELDS else raw[field])
                for field in battle_build_contract.FIELDS)
        assert rollup.publish_generation(target)["status"] == "published"
        assert reporter.read_report(target, "battle_build_points") == points
        tiny = reporter.read_report(target, "battle_build_points", max_rows=1)
        assert tiny.truncated and len(tiny.rows) == 1
        cli_environment = dict(environment, TELEMETRY_ROLLUP_DB_HOST="127.0.0.1", TELEMETRY_ROLLUP_DB_PORT=str(environment["DB_PORT"]),
            TELEMETRY_ROLLUP_DB_DATABASE=name, TELEMETRY_ROLLUP_DB_USER="tbr_build_report_" + token, TELEMETRY_ROLLUP_DB_PASSWORD=password)
        args = ["--definition-version", "6", "--generation", "1", "--environment-id", str(target.environment_id), "--season-id", str(target.season_id)]
        result = subprocess.run([sys.executable, "scripts/telemetry/rollup.py", "report", "--name", "battle_build_points", *args],
            cwd=ROOT, env=cli_environment, check=True, capture_output=True, text=True, timeout=45)
        assert json.loads(result.stdout)["rows"] == list(points.rows)

        # A real kind-12 incident publishes through the shared public snapshot
        # store. A corrected review belongs to a new generation; it cannot
        # reinterpret the earlier generation's frozen inventory or point rows.
        corrected = dict(packet, registry_version=2, previous_registry_version=1)
        loss = dict(incident.template(5)["incidents"][0], producer_boot_id=first["boot_id"], producer_process_id=first["process_id"],
            record_kind_mask=1 << 12, start_utc_usec=first["occurrence_utc_usec"], end_utc_usec=first["occurrence_utc_usec"],
            first_record_seq=first["record_seq"], last_record_seq=first["record_seq"], evidence_digest="c" * 64)
        corrected["incidents"] = [loss]
        assert reviewer.register_incident_packet(corrected)["status"] == "registered"
        next_target = replace(target, generation=2)
        rollup.reserve_identity_generation(next_target.scope_tuple, None)
        assert RollupEngine(rollup).run(next_target, origin_ingest_id=origin, through_ingest_id=through,
            bounds=RollupBounds(page_size=3, max_runtime_s=30)).complete
        assert rollup.publish_generation(next_target)["status"] == "published"
        changed = reporter.read_report(next_target, "battle_build_points")
        assert changed.coverage.incident_coverage["registry_version"] == 2
        assert changed.coverage.incident_coverage["incidents"][0]["record_kind_mask"] == 1 << 12
        lost = next(row for row in changed.rows if battle_build_contract.observation_key(row) == battle_build_contract.observation_key(first))
        assert not lost["point_context_verified"] and lost["publication_quality_flags"] & incident.QUALITY_INCIDENT_GAP
        frozen = reporter.read_report(target, "battle_build_points")
        assert frozen.rows == points.rows and frozen.coverage.battle_coverage == points.coverage.battle_coverage
        assert frozen.coverage.incident_coverage == points.coverage.incident_coverage
        assert frozen.coverage.publication_status == PUBLICATION_SUPERSEDED
        points = frozen

        # Raw retention and later catalogue changes cannot alter this generation.
        config_id = first["bctx_config_id"]
        query("UPDATE telemetry_config SET content_version=content_version+1 WHERE config_id=%s", (config_id,))
        query("DELETE FROM telemetry_interval WHERE ingest_id>%s", (origin,))
        assert rollup.read_battle_source(target) == replace(retained, header=dict(retained.header, publication_complete=1))
        assert rollup.publish_generation(next_target)["status"] == "published"
        assert reporter.read_report(target, "battle_build_points") == points
        for table, expected in old_public.items():
            assert query("SELECT * FROM " + table + " ORDER BY definition_version,generation,environment_id,season_id" +
                (",row_kind,row_key" if table.endswith("_row") else "")) == expected
        schema = ROOT / "migrations/immutable/0066_telemetry_build_publication.sql"
        fingerprint = runtime_fingerprint(environment)
        def apply():
            subprocess.run(command + [name], input=schema.read_bytes(), env=environment, check=True, stdout=subprocess.DEVNULL)
        def verify(expected=True):
            result = subprocess.run(["bash", str(schema.with_suffix(".sh"))], env=environment, capture_output=True, timeout=45)
            assert (result.returncode == 0) is expected, result.stderr.decode(errors="replace")
        apply()
        verify()
        query("ALTER TABLE telemetry_battle_input_v6 MODIFY payload VARBINARY(8191) NOT NULL")
        verify(False)
        apply()
        verify(False)
        query("ALTER TABLE telemetry_battle_input_v6 MODIFY payload VARBINARY(8192) NOT NULL")
        verify()
        query("ALTER TABLE telemetry_battle_input_v6 DROP INDEX uq_battle6_input_replay, "
            "ADD UNIQUE KEY uq_battle6_input_replay (definition_version,generation,environment_id,season_id,record_seq,boot_id,process_id)")
        verify(False)
        apply()
        verify(False)
        query("ALTER TABLE telemetry_battle_input_v6 DROP INDEX uq_battle6_input_replay, "
            "ADD UNIQUE KEY uq_battle6_input_replay (definition_version,generation,environment_id,season_id,boot_id,process_id,record_seq)")
        verify()
        drop = "DROP CONSTRAINT" if "mariadb" in os.environ["TELEMETRY_REPOSITORY_DB_IMAGE"] else "DROP CHECK"
        query(f"ALTER TABLE telemetry_rollup_battle_row_v6 {drop} chk_battle6_row_kind")
        query("ALTER TABLE telemetry_rollup_battle_row_v6 ADD CONSTRAINT chk_battle6_row_kind CHECK (row_kind BETWEEN 1 AND 7)")
        verify(False)
        apply()
        verify(False)
        query(f"ALTER TABLE telemetry_rollup_battle_row_v6 {drop} chk_battle6_row_kind")
        query("ALTER TABLE telemetry_rollup_battle_row_v6 ADD CONSTRAINT chk_battle6_row_kind CHECK (row_kind BETWEEN 1 AND 6)")
        verify()
        assert runtime_fingerprint(environment) == fingerprint
        assert reporter.read_report(target, "battle_build_points") == points
        return dict(build_publication_definition=6, build_publication_incident_schema=5,
            build_publication_exact_points=20, build_publication_retained_configuration=True,
            build_publication_private_roles=True, build_publication_rollback=True, build_publication_bounded_reports=True,
            build_publication_page_lost_acknowledgement=True, build_publication_lost_acknowledgement=True,
            build_publication_kind_twelve_loss=True, build_publication_review_correction=True, build_publication_old_generation_immutable=True,
            build_publication_cli=True, build_publication_raw_retention=True, build_publication_catalogue_independent=True,
            build_publication_sealed_definition_five_preserved=True, build_publication_schema_rerun=True,
            build_publication_schema_drift_refused=True, build_publication_restored_metadata=True,
            build_publication_qualified_points=points.coverage.battle_coverage["qualified_build_points"],
            build_publication_partial_links=points.coverage.battle_coverage["partial_build_links"])
    finally:
        for database in adapters:
            database.close()
        for user in users:
            query("DROP USER IF EXISTS %s@'%%'", (user,))


def qualify_native_control_publication(query, rollup, reporter, executable, run_environment, export):
    origin = query("SELECT MAX(ingest_id) AS n FROM telemetry_interval")[0]["n"]
    subprocess.run([str(executable), "--native-control-sql"], cwd=ROOT,
        env=dict(run_environment, TELEMETRY_BATTLE_CAPTURE_EXPORT=str(export)), check=True, timeout=30)
    emitted = [json.loads(line) for line in export.read_text(encoding="utf-8").splitlines()]
    stored = query("SELECT " + ",".join(RAW_COLUMNS) +
        " FROM telemetry_interval WHERE ingest_id>%s AND record_kind IN (10,11) ORDER BY boot_id,process_id,record_seq", (origin,))
    emitted.sort(key=lambda row: (row["boot_id"], row["process_id"], row["record_seq"]))
    assert len(emitted) == len(stored) > 0
    for observed, row in zip(emitted, stored, strict=True):
        assert all(row[column] == value for column, value in observed.items()), "accepted control source/SQL drift"
    result = qualify_native_control_history(stored)
    first = next(row for row in stored if row["record_kind"] == 10)
    generation = query("SELECT MAX(generation) AS n FROM telemetry_rollup_state WHERE definition_version=5")[0]["n"] + 1
    target = RollupTarget(5, generation, first["battle_environment_id"], first["battle_season_id"])
    through = query("SELECT MAX(ingest_id) AS n FROM telemetry_interval")[0]["n"]
    rollup.reserve_identity_generation(target.scope_tuple, None)
    assert RollupEngine(rollup).run(target, origin_ingest_id=origin,
        through_ingest_id=through, bounds=RollupBounds(page_size=3, max_runtime_s=30)).complete
    retained = rollup.read_battle_source(target)
    assert retained.header["contribution_count"] == result.summary["contribution_count"]
    assert sum(row["bc_control_applications"] for row in retained.facts if row["record_kind"] == 11) == 8
    assert sum(row["bc_control_received"] for row in retained.facts if row["record_kind"] == 11) == 8
    assert rollup.publish_generation(target)["status"] == "published"
    report = reporter.read_report(target, "battle_contributions")
    assert not report.truncated
    assert sum(row["bc_control_applications"] for row in report.rows) == 8
    assert sum(row["bc_control_received"] for row in report.rows) == 8
    assert all(row["account_token"] is None and row["controller_token"] is None and
        not row["complete_metric_coverage_implied"] for row in report.rows)
    assert report.coverage.battle_coverage["verified_contribution_links"] == len(report.rows)
    assert report.coverage.battle_coverage["partial_contribution_links"] == 0
    summary = reporter.read_report(target, "battle_observations")
    assert len(summary.rows) == 1 and summary.rows[0]["control_applications"] == summary.rows[0]["control_received"] == 8
    assert summary.rows[0]["outcome"] is None and not summary.rows[0]["complete_metric_coverage_implied"]
    assert rollup.publish_generation(target)["status"] == "published"
    assert reporter.read_report(target, "battle_contributions") == report
    return dict(native_control_producer=True, native_control_source_helpers=["blind", "Stun"],
        native_control_sql_exact=True, native_control_atomic_publication=True,
        native_control_rejection_gates=True, native_control_self_isolation=True,
        native_control_records=len(stored), native_control_contributions=len(report.rows),
        native_control_applications=8, native_control_received=8,
        native_control_complete_coverage_implied=False, native_control_running_server=False)


def qualify_persisted_source(query, environment, command, name, history, executable, run_environment, control_export):
    """Actual cursor/source transactions through a restricted rollup principal."""
    import pymysql
    token = hashlib.sha256(name.encode()).hexdigest()[:12]
    password = "synthetic-battle-source-" + token
    users, adapters = [], []
    scope = (history.battles[0]["battle"][0], history.battles[0]["battle"][1])
    native = query("SELECT " + ",".join(RAW_COLUMNS) + " FROM telemetry_interval WHERE record_kind=10 ORDER BY ingest_id")
    environment_id, season_id = native[0]["battle_environment_id"], native[0]["battle_season_id"]
    try:
        grants = {
            "rollup": {"telemetry_rollup_state": "SELECT,INSERT,UPDATE",
                "telemetry_generation_identity": "SELECT,INSERT", "telemetry_battle_source": "SELECT,INSERT,UPDATE",
                "telemetry_battle_input": "SELECT,INSERT", "telemetry_interval": "SELECT",
                "telemetry_identity_registry": "SELECT", "telemetry_identity_association": "SELECT",
                "telemetry_incident_registry_v4": "SELECT", "telemetry_incident_v4": "SELECT"},
            "report": {"telemetry_rollup_state": "SELECT", "telemetry_generation_identity": "SELECT"},
            "review": {"telemetry_identity_reviewer": "SELECT", "telemetry_identity_registry": "SELECT,INSERT",
                "telemetry_identity_association": "SELECT,INSERT", "telemetry_interval": "SELECT",
                "telemetry_incident_registry_v4": "SELECT,INSERT", "telemetry_incident_v4": "SELECT,INSERT"},
        }
        for table in ("telemetry_rollup_battle_coverage", "telemetry_rollup_battle_row",
                "telemetry_rollup_incident_coverage", "telemetry_rollup_incident"):
            grants["rollup"][table] = "SELECT,INSERT"
            grants["report"][table] = "SELECT"
        for role, tables in grants.items():
            user = "tbr_source_" + role + "_" + token
            query("CREATE USER %s@'%%' IDENTIFIED BY %s", (user, password))
            users.append(user)
            for table, permissions in tables.items():
                query(f"GRANT {permissions} ON `{name}`.`{table}` TO %s@'%%'", (user,))
        query(f"GRANT SELECT (environment_id,season_id,account_token) ON `{name}`.telemetry_account_token TO %s@'%%'",
            ("tbr_source_review_" + token,))

        def settings(role):
            return ConnectionSettings(host="127.0.0.1", port=int(environment["DB_PORT"]), database=name,
                user="tbr_source_" + role + "_" + token, password=password)

        def adapter(role):
            database = PyMySQLRollupDatabase(PyMySQLConnectionFactory(settings(role)))
            adapters.append(database)
            return database

        rollup = adapter("rollup")
        target = RollupTarget(5, 1, environment_id, season_id)
        through = query("SELECT MAX(ingest_id) AS n FROM telemetry_interval")[0]["n"]
        bounds = RollupBounds(page_size=1, max_runtime_s=30)
        try:
            RollupEngine(rollup).run(target, bounds=bounds, through_ingest_id=through)
        except identity_history.IdentityError as error:
            assert str(error) == "generation_identity_not_reserved"
        else:
            raise AssertionError("battle source did not require identity reservation")
        assert query("SELECT COUNT(*) AS n FROM telemetry_rollup_state WHERE definition_version=5")[0]["n"] == 0
        assert query("SELECT COUNT(*) AS n FROM telemetry_battle_source")[0]["n"] == 0
        rollup.reserve_identity_generation(target.scope_tuple, None)
        assert RollupEngine(rollup).run(target, bounds=bounds, through_ingest_id=through).complete
        original = rollup.read_battle_source(target)
        selected = query("SELECT " + ",".join(RAW_COLUMNS) +
            " FROM telemetry_interval WHERE record_kind IN (9,10,11) ORDER BY ingest_id")
        expected = [dict(row) for row in selected if tuple(row[name] for name in retained_source.SOURCE_SCOPE[row["record_kind"]]) == target.scope_tuple[2:]]

        def check_values(value, rows=expected):
            assert len(value.facts) == len(rows)
            for raw, restored in zip(rows, value.facts, strict=True):
                assert restored == {name: raw[name] for name in retained_source.SOURCE_COLUMNS[raw["record_kind"]]}
            facts = [row for row in value.facts if row["record_kind"] in (10, 11)]
            rebuilt = battle_history.build_history(facts, target.scope_tuple[2:])
            assert (rebuilt.summary, rebuilt.battles, rebuilt.actors, rebuilt.contributions, rebuilt.exposures) == (
                history.summary, history.battles, history.actors, history.contributions, history.exposures)

        check_values(original)
        assert original.header["association_count"] == 123 and original.header["contribution_count"] == 28
        state = rollup.read_state(target)
        assert state["input_watermark"] == original.header["input_watermark"] == through
        assert state["publication_status"] == PUBLICATION_BUILDING and not original.header["publication_complete"]
        assert RollupEngine(rollup).run(target, bounds=bounds, through_ingest_id=through).complete
        assert rollup.read_battle_source(target) == original
        try:
            rollup.publish_generation(target, bounds=RollupBounds(page_size=1, max_rows=1))
        except BoundsExceeded:
            pass
        else:
            raise AssertionError("battle publication exceeded its source row reservation")
        assert rollup.read_state(target)["publication_status"] == PUBLICATION_BUILDING

        # Fail after source/header writes but before cursor update; none survive.
        failed = RollupTarget(5, 2, environment_id, season_id)
        rollup.reserve_identity_generation(failed.scope_tuple, None)
        update = rollup._update_state
        def fail_update(*args):
            raise RuntimeError("injected battle cursor failure")
        rollup._update_state = fail_update
        try:
            try:
                RollupEngine(rollup).run(failed, bounds=bounds, through_ingest_id=through)
            except RuntimeError as error:
                assert str(error) == "injected battle cursor failure"
            else:
                raise AssertionError("cursor failure was not injected")
        finally:
            rollup._update_state = update
        for table in ("telemetry_rollup_state", "telemetry_battle_source", "telemetry_battle_input"):
            assert query(f"SELECT COUNT(*) AS n FROM {table} WHERE definition_version=5 AND generation=2")[0]["n"] == 0
        assert RollupEngine(rollup).run(failed, bounds=bounds, through_ingest_id=through).complete
        check_values(rollup.read_battle_source(failed))

        # Lost acknowledgements on both sides of COMMIT preserve exact receipts.
        for generation, committed in ((3, True), (4, False)):
            retry = RollupTarget(5, generation, environment_id, season_id)
            rollup.reserve_identity_generation(retry.scope_tuple, None)
            commit = rollup._commit
            injected = False
            def lost_ack():
                nonlocal injected
                if not injected:
                    injected = True
                    if committed:
                        commit()
                    raise AmbiguousCommit("injected battle source acknowledgement loss")
                commit()
            rollup._commit = lost_ack
            try:
                assert RollupEngine(rollup).run(retry, bounds=bounds, through_ingest_id=through).complete
            finally:
                rollup._commit = commit
            assert injected
            check_values(rollup.read_battle_source(retry))

        # Corrupt or missing retained evidence cannot be returned as verified.
        scope_where = "definition_version=%s AND generation=%s AND environment_id=%s AND season_id=%s"
        retained = query("SELECT * FROM telemetry_battle_input WHERE " + scope_where + " ORDER BY ingest_id LIMIT 1", target.scope_tuple)[0]
        def refused(action, reason):
            try:
                action()
            except SemanticError as error:
                assert reason in str(error), str(error)
            else:
                raise AssertionError("battle source read accepted " + reason)
        query("UPDATE telemetry_battle_input SET payload_digest=%s WHERE " + scope_where + " AND ingest_id=%s",
            (b"x" * 32, *target.scope_tuple, retained["ingest_id"]))
        refused(lambda: rollup.read_battle_source(target), "payload_digest")
        query("UPDATE telemetry_battle_input SET payload_digest=%s WHERE " + scope_where + " AND ingest_id=%s",
            (retained["payload_digest"], *target.scope_tuple, retained["ingest_id"]))
        query("DELETE FROM telemetry_battle_input WHERE " + scope_where + " AND ingest_id=%s", (*target.scope_tuple, retained["ingest_id"]))
        refused(lambda: rollup.read_battle_source(target), "missing_or_changed")
        fields = retained_source.INPUT_COLUMNS
        query("INSERT INTO telemetry_battle_input (" + ",".join(fields) + ") VALUES (" +
            ",".join(["%s"] * len(fields)) + ")", tuple(retained[name] for name in fields))
        query("UPDATE telemetry_battle_source SET input_watermark=input_watermark+1 WHERE " + scope_where, target.scope_tuple)
        refused(lambda: rollup.read_battle_source(target), "state_checkpoint")
        refused(lambda: RollupEngine(rollup).run(target, bounds=bounds, through_ingest_id=through), "cursor_conflict")
        query("UPDATE telemetry_battle_source SET input_watermark=%s WHERE " + scope_where, (through, *target.scope_tuple))
        assert rollup.read_battle_source(target) == original
        query("UPDATE telemetry_rollup_state SET rebuild_from_ingest_id=1 WHERE " + scope_where, target.scope_tuple)
        refused(lambda: rollup.read_battle_source(target), "state_checkpoint")
        query("UPDATE telemetry_rollup_state SET rebuild_from_ingest_id=0 WHERE " + scope_where, target.scope_tuple)
        assert rollup.read_battle_source(target) == original
        empty = RollupTarget(5, 6, environment_id, season_id)
        rollup.reserve_identity_generation(empty.scope_tuple, None)
        assert RollupEngine(rollup).run(empty, bounds=bounds, through_ingest_id=through, origin_ingest_id=through).complete
        empty_source = rollup.read_battle_source(empty)
        assert empty_source.header["input_origin"] == empty_source.header["input_watermark"] == through
        assert empty_source.header["source_fact_count"] == 0 and empty_source.facts == ()
        assert not empty_source.header["publication_complete"]
        try:
            rollup.read_battle_source(target, max_bytes=original.reserved_bytes)
        except BoundsExceeded:
            pass
        else:
            raise AssertionError("source read failed to reserve buffering and decoded values")

        # Original arrival labels and selected scope survive cursor advancement.
        config = query("SELECT * FROM telemetry_config WHERE environment_id=%s LIMIT 1", (environment_id,))[0]
        for foreign in (False, True):
            row = ownership(environment_id=environment_id + int(foreign), season_id=season_id,
                boot_id=scope[0] + 500, process_id=scope[1] + 500, record_seq=1 + int(foreign),
                connection_boot_id=scope[0] + 500, connection_process_id=scope[1] + 500,
                config_id=config["config_id"], classifier_version=config["classifier_version"], policy_version=config["policy_version"])
            row.pop("ingest_id", None)
            row["ingested_utc_usec"] = 987654321 + int(foreign)
            fields = tuple(row)
            query("INSERT INTO telemetry_interval (" + ",".join(fields) + ") VALUES (" +
                ",".join(["%s"] * len(fields)) + ")", tuple(row[name] for name in fields))
        through = query("SELECT MAX(ingest_id) AS n FROM telemetry_interval")[0]["n"]
        assert RollupEngine(rollup).run(target, bounds=bounds, through_ingest_id=through).complete
        expanded = rollup.read_battle_source(target)
        assert expanded.header["source_fact_count"] == original.header["source_fact_count"] + 1
        assert expanded.header["ownership_count"] == original.header["ownership_count"] + 1
        assert expanded.header["input_watermark"] == through
        assert expanded.facts[-1]["ingested_utc_usec"] == 987654321
        assert expanded.facts[-1]["occurrence_utc_usec"] != expanded.facts[-1]["ingested_utc_usec"]

        cli_target = RollupTarget(5, 5, environment_id, season_id)
        rollup.reserve_identity_generation(cli_target.scope_tuple, None)
        cli_environment = dict(environment, TELEMETRY_ROLLUP_DB_HOST="127.0.0.1", TELEMETRY_ROLLUP_DB_PORT=str(environment["DB_PORT"]),
            TELEMETRY_ROLLUP_DB_DATABASE=name, TELEMETRY_ROLLUP_DB_USER=settings("rollup").user,
            TELEMETRY_ROLLUP_DB_PASSWORD=password)
        target_args = ["--definition-version", "5", "--generation", "5", "--environment-id", str(environment_id), "--season-id", str(season_id)]
        subprocess.run([sys.executable, "scripts/telemetry/rollup.py", "run", *target_args, "--through-ingest-id", str(through),
            "--page-size", "3", "--max-runtime-s", "30"], cwd=ROOT, env=cli_environment, capture_output=True, check=True, timeout=45)
        assert rollup.read_battle_source(cli_target).facts == expanded.facts
        catalog = subprocess.run([sys.executable, "scripts/telemetry/rollup.py", "definitions", "--definition-version", "5"],
            cwd=ROOT, env=cli_environment, capture_output=True, check=True, timeout=15)
        assert set(row["name"] for row in json.loads(catalog.stdout)["definitions"]) == set(battle_publication.ROW_KINDS)

        for role in ("rollup", "report"):
            principal = PyMySQLConnectionFactory(settings(role)).connect()
            try:
                denied = ["UPDATE telemetry_battle_input SET record_kind=9 WHERE 0", "DELETE FROM telemetry_battle_input WHERE 0",
                    "UPDATE telemetry_rollup_battle_row SET row_kind=1 WHERE 0", "DELETE FROM telemetry_rollup_battle_row WHERE 0",
                    "SELECT * FROM accounts LIMIT 0"] if role == "rollup" else ["SELECT * FROM telemetry_battle_source LIMIT 0",
                    "SELECT * FROM telemetry_battle_input LIMIT 0", "UPDATE telemetry_rollup_battle_row SET row_kind=1 WHERE 0"]
                for statement in denied:
                    try:
                        with principal.cursor() as cursor:
                            cursor.execute(statement)
                    except pymysql.err.OperationalError as error:
                        assert error.args[0] == 1142
                    else:
                        raise AssertionError("private source principal exceeded permissions: " + statement)
            finally:
                principal.close()

        def rejected_sql(statement, parameters, codes):
            try:
                query(statement, parameters)
            except (pymysql.err.IntegrityError, pymysql.err.OperationalError) as error:
                assert error.args[0] in codes, error.args
            else:
                raise AssertionError("battle source SQL guard accepted " + statement)
        rejected_sql("UPDATE telemetry_battle_source SET association_count=association_count+1 WHERE " + scope_where,
            target.scope_tuple, (3819, 4025))
        rejected_sql("UPDATE telemetry_battle_input SET record_kind=8 WHERE " + scope_where,
            target.scope_tuple, (3819, 4025))
        duplicate = dict(retained, ingest_id=through + 100)
        fields = retained_source.INPUT_COLUMNS
        insert_sql = "INSERT INTO telemetry_battle_input (" + ",".join(fields) + ") VALUES (" + ",".join(["%s"] * len(fields)) + ")"
        rejected_sql(insert_sql, tuple(duplicate[name] for name in fields), (1062,))
        orphan = dict(retained, generation=999)
        rejected_sql(insert_sql, tuple(orphan[name] for name in fields), (1452,))

        # Durable retained evidence remains available after raw retention.
        raw_before = query("SELECT " + ",".join(RAW_COLUMNS) + " FROM telemetry_interval WHERE record_kind IN (9,10,11) ORDER BY ingest_id")
        query("DELETE FROM telemetry_interval WHERE record_kind IN (9,10,11)")
        assert rollup.read_battle_source(target) == expanded
        raw_insert = "INSERT INTO telemetry_interval (" + ",".join(RAW_COLUMNS) + ") VALUES (" + ",".join(["%s"] * len(RAW_COLUMNS)) + ")"
        for row in raw_before:
            query(raw_insert, tuple(row[name] for name in RAW_COLUMNS))
        assert query("SELECT " + ",".join(RAW_COLUMNS) + " FROM telemetry_interval WHERE record_kind IN (9,10,11) ORDER BY ingest_id") == raw_before

        # Schema application is re-runnable and refuses to bless existing drift.
        schema = ROOT / "migrations/immutable/0063_telemetry_battle_source.sql"
        def apply():
            subprocess.run(command + [name], input=schema.read_bytes(), env=environment,
                check=True, timeout=20, stdout=subprocess.DEVNULL)
        def verify(success=True):
            result = subprocess.run(["bash", str(schema.with_suffix(".sh"))], cwd=ROOT, env=environment,
                capture_output=True, timeout=45)
            assert (result.returncode == 0) is success, result.stderr.decode(errors="replace")
        fingerprint = runtime_fingerprint(environment)
        snapshot = query("SELECT * FROM telemetry_battle_input ORDER BY definition_version,generation,environment_id,season_id,ingest_id")
        apply()
        verify()
        assert query("SELECT * FROM telemetry_battle_input ORDER BY definition_version,generation,environment_id,season_id,ingest_id") == snapshot
        query("SET lock_wait_timeout=2")
        drop_check = "DROP CONSTRAINT" if "mariadb" in environment["TELEMETRY_REPOSITORY_DB_IMAGE"] else "DROP CHECK"
        query("ALTER TABLE telemetry_battle_input MODIFY payload VARBINARY(32768) NOT NULL, " + drop_check + " chk_battle_input_payload")
        query("ALTER TABLE telemetry_battle_input ADD CONSTRAINT chk_battle_input_payload CHECK (OCTET_LENGTH(payload) BETWEEN 1 AND 32768)")
        verify(False)
        apply()
        verify(False)
        query("UPDATE telemetry_battle_input SET payload=%s WHERE " + scope_where + " AND ingest_id=%s",
            (b"x" * 32000, *target.scope_tuple, retained["ingest_id"]))
        refused(lambda: rollup.read_battle_source(target), "payload_capacity")
        query("UPDATE telemetry_battle_input SET payload=%s WHERE " + scope_where + " AND ingest_id=%s",
            (retained["payload"], *target.scope_tuple, retained["ingest_id"]))
        query("ALTER TABLE telemetry_battle_input " + drop_check + " chk_battle_input_payload")
        query("ALTER TABLE telemetry_battle_input MODIFY payload VARBINARY(8192) NOT NULL, ADD CONSTRAINT chk_battle_input_payload CHECK (OCTET_LENGTH(payload) BETWEEN 1 AND 8192)")
        verify()
        query("ALTER TABLE telemetry_battle_input " + drop_check + " chk_battle_input_kind")
        query("ALTER TABLE telemetry_battle_input ADD CONSTRAINT chk_battle_input_kind CHECK (record_kind IN (8,9,10,11))")
        verify(False)
        apply()
        verify(False)
        query("ALTER TABLE telemetry_battle_input " + drop_check + " chk_battle_input_kind")
        query("ALTER TABLE telemetry_battle_input ADD CONSTRAINT chk_battle_input_kind CHECK (record_kind IN (9,10,11))")
        verify()
        assert runtime_fingerprint(environment) == fingerprint
        assert rollup.read_battle_source(target) == expanded

        reporter, reviewer = adapter("report"), adapter("review")
        public_tables = ("telemetry_rollup_battle_coverage", "telemetry_rollup_battle_row",
            "telemetry_rollup_incident_coverage", "telemetry_rollup_incident")
        try:
            rollup.publish_generation(target, bounds=RollupBounds(max_output_fanout=1))
        except (BoundsExceeded, SemanticError):
            pass
        else:
            raise AssertionError("battle publication exceeded its output row reservation")
        # A failure after a detail insert must undo the public header, loss
        # snapshot, detail, source completion flag and generation transition.
        insert = rollup._insert_review_rows
        def fail_public_detail(table, fields, rows, **kwargs):
            if table == "telemetry_rollup_battle_row":
                insert(table, fields, rows[:1], **kwargs)
                raise RuntimeError("injected battle detail failure")
            return insert(table, fields, rows, **kwargs)
        rollup._insert_review_rows = fail_public_detail
        try:
            try:
                rollup.publish_generation(target)
            except RuntimeError as error:
                assert str(error) == "injected battle detail failure"
            else:
                raise AssertionError("battle publication did not roll back its detail failure")
        finally:
            rollup._insert_review_rows = insert
        for table in public_tables:
            assert query(f"SELECT COUNT(*) AS n FROM {table} WHERE " + scope_where, target.scope_tuple)[0]["n"] == 0
        assert rollup.read_battle_source(target) == expanded
        assert rollup.read_state(target)["publication_status"] == PUBLICATION_BUILDING
        query("UPDATE telemetry_rollup_state SET rebuild_through_ingest_id=input_watermark+1 WHERE " + scope_where, target.scope_tuple)
        try:
            rollup.publish_generation(target)
        except GenerationConflict:
            pass
        else:
            raise AssertionError("battle publication ignored an unfinished fixed bound")
        query("UPDATE telemetry_rollup_state SET rebuild_through_ingest_id=input_watermark WHERE " + scope_where, target.scope_tuple)
        assert rollup.publish_generation(target)["status"] == "published"
        assert rollup.read_battle_source(target).header["publication_complete"] == 1

        def reports(generation):
            return {kind: reporter.read_report(generation, kind) for kind in battle_publication.ROW_KINDS}
        original_reports = reports(target)
        covered = original_reports["battle_exposure"].coverage.battle_coverage
        assert covered["source_fact_count"] == expanded.header["source_fact_count"]
        assert covered["identity"]["status"] == "published_unknown_identity"
        assert covered["verified_contribution_links"] == 28 and covered["partial_contribution_links"] == 0
        assert covered["observed_present_usec"] == history.summary["verified_exposure_present_usec"]
        assert covered["observed_pc_present_usec"] == covered["unknown_account_pc_present_usec"]
        assert covered["association_row_count"] == 123 and covered["contribution_row_count"] == 28
        assert original_reports["battle_exposure"].coverage.incident_coverage["registry_schema_version"] == 4
        assert all(not report.truncated for report in original_reports.values())
        associations = {battle_contract.fact_key(row): row for row in original_reports["battle_associations"].rows}
        for row in expanded.facts:
            if row["record_kind"] == 10:
                assert {name: associations[battle_contract.fact_key(row)][name] for name in retained_source.SOURCE_COLUMNS[10]} == row
        summaries = original_reports["battle_observations"].rows
        assert sum(row["damage_dealt"] or 0 for row in summaries if row["canonical"]) == 112
        assert all(row["outcome"] is None and row["control_applications"] ==
            (0 if row["available_metric_mask"] & 4 else None) for row in summaries)
        assert reporter.read_report(target, "battle_associations", max_rows=1).truncated
        for action in (lambda: reporter.read_report(target, "battle_exposure", max_bytes=100_000),
                lambda: reporter.read_coverage(target, max_bytes=100_000)):
            try:
                action()
            except BoundsExceeded:
                pass
            else:
                raise AssertionError("battle report exceeded its metadata byte budget")
        rollup.publish_generation(target)
        assert reports(target) == original_reports

        public_row = query("SELECT * FROM telemetry_rollup_battle_row WHERE " + scope_where +
            " AND row_kind=3 ORDER BY row_key LIMIT 1", target.scope_tuple)[0]
        detail_where = scope_where + " AND row_kind=%s AND row_key=%s"
        detail_key = (*target.scope_tuple, public_row["row_kind"], public_row["row_key"])
        query("UPDATE telemetry_rollup_battle_row SET payload_digest=%s WHERE " + detail_where, (b"x" * 32, *detail_key))
        refused(lambda: reporter.read_report(target, "battle_exposure"), "snapshot_changed")
        refused(lambda: rollup.publish_generation(target), "snapshot_changed")
        query("UPDATE telemetry_rollup_battle_row SET payload_digest=%s WHERE " + detail_where, (public_row["payload_digest"], *detail_key))
        query("UPDATE telemetry_rollup_battle_row SET payload=%s WHERE " + detail_where, (b"x", *detail_key))
        refused(lambda: reporter.read_report(target, "battle_contributions"), "payload_digest")
        query("UPDATE telemetry_rollup_battle_row SET payload=%s WHERE " + detail_where, (public_row["payload"], *detail_key))
        query("DELETE FROM telemetry_rollup_battle_row WHERE " + detail_where, detail_key)
        refused(lambda: reporter.read_report(target, "battle_exposure"), "detail_missing")
        fields = battle_publication.ROW_COLUMNS
        query("INSERT INTO telemetry_rollup_battle_row (" + ",".join(fields) + ") VALUES (" + ",".join(["%s"] * len(fields)) + ")",
            tuple(public_row[name] for name in fields))
        query("UPDATE telemetry_battle_source SET publication_complete=0 WHERE " + scope_where, target.scope_tuple)
        refused(lambda: rollup.publish_generation(target), "publication_state_conflict")
        query("UPDATE telemetry_battle_source SET publication_complete=1 WHERE " + scope_where, target.scope_tuple)
        assert reports(target) == original_reports

        for generation, committed in ((3, True), (4, False)):
            retry = RollupTarget(5, generation, environment_id, season_id)
            commit, injected = rollup._commit, False
            def lost_publication_ack():
                nonlocal injected
                if not injected:
                    injected = True
                    if committed:
                        commit()
                    raise AmbiguousCommit("injected battle publication acknowledgement loss")
                commit()
            rollup._commit = lost_publication_ack
            try:
                assert rollup.publish_generation(retry)["status"] == "published"
            finally:
                rollup._commit = commit
            assert injected
            saved = reports(retry)
            rollup.publish_generation(retry)
            assert reports(retry) == saved
        assert rollup.read_state(target)["publication_status"] == PUBLICATION_SUPERSEDED
        assert rollup.read_battle_source(target).facts == expanded.facts
        old_after_supersede = reports(target)
        assert {kind: report.rows for kind, report in old_after_supersede.items()} == {
            kind: report.rows for kind, report in original_reports.items()}
        assert old_after_supersede["battle_exposure"].coverage.battle_coverage == covered

        subprocess.run([sys.executable, "scripts/telemetry/rollup.py", "publish", *target_args], cwd=ROOT,
            env=cli_environment, capture_output=True, check=True, timeout=30)
        report_environment = dict(cli_environment, TELEMETRY_ROLLUP_DB_USER=settings("report").user)
        cli_report = subprocess.run([sys.executable, "scripts/telemetry/rollup.py", "report", *target_args,
            "--name", "battle_contributions"], cwd=ROOT, env=report_environment, capture_output=True, check=True, timeout=30)
        assert len(json.loads(cli_report.stdout)["rows"]) == 28
        assert rollup.read_state(cli_target)["publication_status"] == PUBLICATION_PUBLISHED

        # Use controlled coherent UTC labels as a separate dated boundary
        # fixture. The original native writer/SQL values were checked above.
        fixture = BattleHistoryTests("test_publication_dated_transfer_conserves_presence_without_dividing_amounts")
        fixture.rows = [row for row in expanded.facts if row["record_kind"] in (10, 11)]
        fixture.scope = environment_id, season_id
        dated, registry_fixture, segment_key, first, middle, last, epoch = fixture._dated_publication_fixture()
        registry = identity_history.Registry.from_packet(registry_fixture.input_dict())
        principal = reviewer.connection_factory.connect()
        try:
            with principal.cursor() as cursor:
                cursor.execute("SELECT CURRENT_USER() AS principal")
                principal_name = cursor.fetchone()["principal"]
        finally:
            reviewer.connection_factory.close(principal)
        query("INSERT INTO telemetry_identity_reviewer VALUES (%s,%s,%s,%s,1)",
            (environment_id, season_id, principal_name, bytes.fromhex(registry.reviewer_token)))
        query("INSERT INTO telemetry_account_lifetime VALUES (900001,NULL),(900002,NULL)")
        query("INSERT INTO telemetry_account_token VALUES (%s,%s,101,900001),(%s,%s,102,900002)",
            (environment_id, season_id, environment_id, season_id))
        assert reviewer.register_identity_packet(registry.input_dict())["status"] == "registered"
        loss_packet = incident.template(4)
        loss_packet.update(environment_id=environment_id, season_id=season_id, reviewer_token="a" * 64,
            review_evidence_digest="b" * 64, reviewed_from_utc_usec=epoch + first - 1,
            reviewed_through_utc_usec=epoch + last + 1, incidents=[])
        assert reviewer.register_incident_packet(loss_packet)["status"] == "registered"
        raw_original = query("SELECT " + ",".join(RAW_COLUMNS) + " FROM telemetry_interval WHERE record_kind IN (9,10,11) ORDER BY ingest_id")
        query("DELETE FROM telemetry_interval WHERE record_kind IN (9,10,11)")
        for row in dated.facts:
            names = [name for name in row if name != "ingest_id"]
            query("INSERT INTO telemetry_interval (" + ",".join(names) + ") VALUES (" + ",".join(["%s"] * len(names)) + ")",
                tuple(row[name] for name in names))
        dated_through = query("SELECT MAX(ingest_id) AS n FROM telemetry_interval")[0]["n"]
        reviewed_target = RollupTarget(5, 7, environment_id, season_id)
        rollup.reserve_identity_generation(reviewed_target.scope_tuple, 1)
        assert RollupEngine(rollup).run(reviewed_target, bounds=RollupBounds(page_size=3), through_ingest_id=dated_through).complete
        rollup.publish_generation(reviewed_target)
        reviewed_reports = reports(reviewed_target)
        reviewed_exposure = reviewed_reports["battle_exposure"]
        assert reviewed_exposure.coverage.battle_coverage["owned_pc_present_usec"] > 0
        assert reviewed_exposure.coverage.battle_coverage["confirmed_controller_pc_present_usec"] > 0
        assert any(row["controller_token"] == 701 for row in reviewed_exposure.rows)
        selected = next(row for row in reviewed_reports["battle_contributions"].rows if battle_contribution_contract.segment_key(row) == segment_key)
        assert selected["attribution_status"] == "identity_changes_inside_segment" and selected["account_token"] is None
        corrected = replace(registry, registry_version=2, previous_registry_version=1, previous_packet_digest=registry.packet_digest,
            associations=tuple(replace(row, controller_token=799) if row.account_token == 101 else row for row in registry.associations))
        assert reviewer.register_identity_packet(corrected.input_dict())["status"] == "registered"
        source_segment = next(row for row in dated.facts if row["record_kind"] == 11 and battle_contribution_contract.segment_key(row) == segment_key)
        loss = dict(incident.template(4)["incidents"][0], producer_boot_id=source_segment["boot_id"],
            producer_process_id=source_segment["process_id"], start_utc_usec=epoch + first + (middle - first) // 2,
            end_utc_usec=epoch + middle, record_kind_mask=1 << 9, evidence_digest="d" * 64)
        loss_packet.update(registry_version=2, previous_registry_version=1, incidents=[loss])
        assert reviewer.register_incident_packet(loss_packet)["status"] == "registered"
        corrected_target = RollupTarget(5, 8, environment_id, season_id)
        rollup.reserve_identity_generation(corrected_target.scope_tuple, 2)
        assert RollupEngine(rollup).run(corrected_target, through_ingest_id=dated_through).complete
        rollup.publish_generation(corrected_target)
        corrected_exposure = reporter.read_report(corrected_target, "battle_exposure")
        assert corrected_exposure.coverage.battle_coverage["identity"]["registry_version"] == 2
        assert corrected_exposure.coverage.incident_coverage["registry_version"] == 2
        assert any(row["controller_token"] == 799 for row in corrected_exposure.rows)
        pieces = [row for row in corrected_exposure.rows if row["battle_actor_id"] == source_segment["bc_actor_id"] and
            row["source_battle"] == [source_segment[name] for name in battle_contribution_contract.BATTLE] and
            first <= row["start_monotonic_usec"] < last]
        assert any(row["account_token"] is None and row["quality_flags"] & incident.QUALITY_INCIDENT_GAP for row in pieces)
        assert any(row["account_token"] == 102 and row["start_monotonic_usec"] >= middle for row in pieces)
        assert corrected_exposure.coverage.battle_coverage["observed_present_usec"] == reviewed_exposure.coverage.battle_coverage["observed_present_usec"]
        old = reports(reviewed_target)
        assert old["battle_exposure"].rows == reviewed_exposure.rows
        assert old["battle_exposure"].coverage.battle_coverage == reviewed_exposure.coverage.battle_coverage
        assert old["battle_exposure"].coverage.incident_coverage == reviewed_exposure.coverage.incident_coverage
        query("DELETE FROM telemetry_interval WHERE record_kind IN (9,10,11)")
        for row in raw_original:
            query(raw_insert, tuple(row[name] for name in RAW_COLUMNS))

        publication_schema = ROOT / "migrations/immutable/0064_telemetry_battle_publication.sql"
        def apply_publication():
            subprocess.run(command + [name], input=publication_schema.read_bytes(), env=environment,
                check=True, timeout=20, stdout=subprocess.DEVNULL)
        def verify_publication(success=True):
            result = subprocess.run(["bash", str(publication_schema.with_suffix(".sh"))], cwd=ROOT, env=environment,
                capture_output=True, timeout=45)
            assert (result.returncode == 0) is success, result.stderr.decode(errors="replace")
        saved_public = query("SELECT * FROM telemetry_rollup_battle_row ORDER BY definition_version,generation,environment_id,season_id,row_kind,row_key")
        apply_publication()
        verify_publication()
        assert query("SELECT * FROM telemetry_rollup_battle_row ORDER BY definition_version,generation,environment_id,season_id,row_kind,row_key") == saved_public
        rejected_sql("UPDATE telemetry_rollup_battle_coverage SET owned_pc_present_usec=owned_pc_present_usec+1 WHERE " + scope_where,
            target.scope_tuple, (3819, 4025))
        rejected_sql("UPDATE telemetry_rollup_battle_row SET row_kind=6 WHERE " + detail_where, detail_key, (3819, 4025))
        query("ALTER TABLE telemetry_rollup_battle_row MODIFY payload VARBINARY(32768) NOT NULL, " + drop_check + " chk_battle_row_payload")
        query("ALTER TABLE telemetry_rollup_battle_row ADD CONSTRAINT chk_battle_row_payload CHECK (OCTET_LENGTH(payload) BETWEEN 1 AND 32768)")
        verify_publication(False)
        apply_publication()
        verify_publication(False)
        query("UPDATE telemetry_rollup_battle_row SET payload=%s WHERE " + detail_where, (b"x" * 32000, *detail_key))
        refused(lambda: reporter.read_report(target, "battle_contributions"), "payload_capacity")
        query("UPDATE telemetry_rollup_battle_row SET payload=%s WHERE " + detail_where, (public_row["payload"], *detail_key))
        query("ALTER TABLE telemetry_rollup_battle_row " + drop_check + " chk_battle_row_payload")
        query("ALTER TABLE telemetry_rollup_battle_row MODIFY payload VARBINARY(8192) NOT NULL, ADD CONSTRAINT chk_battle_row_payload CHECK (OCTET_LENGTH(payload) BETWEEN 1 AND 8192)")
        verify_publication()
        assert runtime_fingerprint(environment) == fingerprint
        assert reporter.read_report(target, "battle_contributions").rows == original_reports["battle_contributions"].rows

        controls = qualify_native_control_publication(query, rollup, reporter, executable, run_environment, control_export)
        assert runtime_fingerprint(environment) == fingerprint
        return dict(controls, history_persisted_source_checkpoint=True, battle_source_identity_reservation=True,
            battle_source_cursor_atomicity=True, battle_source_lost_acknowledgements=True,
            battle_source_exact_values=True, battle_source_ownership_arrival=True, battle_source_private_roles=True,
            battle_source_bounded_reads=True, battle_source_cli_preparation=True,
            battle_publication_capacity_refused=True, battle_source_selected_inputs=expanded.header["source_fact_count"],
            battle_source_retention=True, battle_source_schema_drift_refused=True, battle_source_restored_metadata=True,
            battle_source_read_transaction_released=True,
            battle_source_migration_apply_checksum=hashlib.sha256(schema.read_bytes()).hexdigest(),
            battle_source_migration_verify_checksum=hashlib.sha256(schema.with_suffix(".sh").read_bytes()).hexdigest(),
            history_atomic_publication=True, battle_publication_rollback=True, battle_publication_lost_acknowledgements=True,
            battle_publication_exact_native_values=True, battle_publication_bounded_reports=True,
            battle_publication_snapshot_verified=True, battle_publication_read_transaction_released=True,
            battle_publication_private_report_role=True, battle_publication_cli=True,
            battle_publication_dated_fixture=True, battle_publication_review_correction=True,
            battle_publication_ownership_loss_recovery=True, battle_publication_old_generation_immutable=True,
            battle_publication_schema_drift_refused=True, battle_publication_restored_metadata=True,
            battle_publication_migration_apply_checksum=hashlib.sha256(publication_schema.read_bytes()).hexdigest(),
            battle_publication_migration_verify_checksum=hashlib.sha256(publication_schema.with_suffix(".sh").read_bytes()).hexdigest())
    finally:
        for database in adapters:
            database.close()
        for user in users:
            query("DROP USER IF EXISTS %s@'%%'", (user,))


def qualify() -> None:
    if os.environ.get("TELEMETRY_REPOSITORY_DISPOSABLE") != "1":
        raise RuntimeError("explicit disposable qualification required")
    import pymysql

    artifacts = ROOT / "bin/tests"
    artifacts.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="telemetry-battle-runtime-sql-", dir=artifacts) as directory:
        executable = Path(directory) / "capture"
        export = Path(directory) / "capture.jsonl"
        compile_gameplay(executable, native_sql=True)
        environment, command, name = prepare_sql_fixture()
        admin = None
        writer = None
        user_created = False
        user = "tbr_writer_" + hashlib.sha256(name.encode()).hexdigest()[:16]
        password = "synthetic-battle-runtime-" + user
        try:
            fingerprint = runtime_fingerprint(environment)
            admin = pymysql.connect(host="127.0.0.1", port=int(environment["DB_PORT"]),
                user=environment["DB_USER"], password=environment["DB_PASSWD"], database=name,
                autocommit=True, cursorclass=pymysql.cursors.DictCursor,
                connect_timeout=3, read_timeout=10, write_timeout=10)

            def query(statement, parameters=()):
                with admin.cursor() as cursor:
                    cursor.execute(statement, parameters)
                    return list(cursor.fetchall()) if cursor.description else []

            query("CREATE USER %s@'%%' IDENTIFIED BY %s", (user, password))
            user_created = True
            for table in ("telemetry_interval", "telemetry_config", "telemetry_quarantine"):
                query(f"GRANT SELECT,INSERT ON `{name}`.`{table}` TO %s@'%%'", (user,))
            query(f"GRANT SELECT,INSERT,UPDATE ON `{name}`.telemetry_session TO %s@'%%'", (user,))
            run_environment = dict(environment, TELEMETRY_BATTLE_WRITER_USER=user,
                TELEMETRY_BATTLE_WRITER_PASSWORD=password, TELEMETRY_BATTLE_CAPTURE_EXPORT=str(export))
            subprocess.run([str(executable), "--native-battle-sql"], check=True,
                           cwd=ROOT, env=run_environment, timeout=30)
            source = [json.loads(line) for line in export.read_text(encoding="utf-8").splitlines()]
            def receipt(row):
                return row["boot_id"], row["process_id"], row["record_seq"]
            battle_source = sorted((row for row in source if row["record_kind"] == 10), key=receipt)
            contribution_source = sorted((row for row in source if row["record_kind"] == 11), key=receipt)
            stored = query("SELECT " + ",".join(RAW_COLUMNS) +
                " FROM telemetry_interval WHERE record_kind=10 ORDER BY boot_id,process_id,record_seq")
            assert len(battle_source) == len(stored) > 0
            for emitted, raw in zip(battle_source, stored, strict=True):
                assert all(raw[column] == value for column, value in emitted.items()), "native source/SQL field drift"
                values = battle_contract.validate_raw_fact(raw)
                assert values == {column: value for column, value in emitted.items() if column.startswith("battle_")}
            cursor = 0
            packets = 0
            while cursor < len(stored):
                count = stored[cursor]["battle_fact_count"]
                battle_contract.validate_packet([battle_contract.validate_raw_fact(raw)
                    for raw in stored[cursor:cursor + count]])
                cursor += count
                packets += 1
            contributions = query("SELECT " + ",".join(RAW_COLUMNS) +
                " FROM telemetry_interval WHERE record_kind=11 ORDER BY boot_id,process_id,record_seq")
            assert len(contribution_source) == len(contributions) > 0
            # References must resolve to complete, real association packets.
            bases = {(row["battle_boot_id"], row["battle_process_id"], row["battle_seq"],
                      row["battle_revision"], row["battle_fact_sequence"]): row
                     for row in stored if row["battle_fact_index"] + 1 == row["battle_fact_count"]}
            for emitted, raw in zip(contribution_source, contributions, strict=True):
                assert all(raw[column] == value for column, value in emitted.items()), "native contribution/SQL field drift"
                values = battle_contribution_contract.validate_raw_segment(raw)
                assert values == {column: value for column, value in emitted.items() if column.startswith("bc_")}
                identity = tuple(raw[column] for column in ("bc_battle_boot_id", "bc_battle_process_id", "bc_battle_seq"))
                first = bases[identity + (raw["bc_first_association_revision"], raw["bc_first_association_fact_sequence"])]
                last = bases[identity + (raw["bc_last_association_revision"], raw["bc_last_association_fact_sequence"])]
                assert first["battle_at_monotonic_usec"] <= raw["bc_start_monotonic_usec"]
                assert last["battle_at_monotonic_usec"] <= raw["bc_observed_through_monotonic_usec"]
                for basis in (first, last):
                    assert basis["battle_config_id"] == raw["bc_config_id"]
                    assert basis["battle_policy_version"] == raw["bc_policy_version"]
                    assert basis["battle_classifier_version"] == raw["bc_classifier_version"]
                assert first["battle_mode"] == raw["bc_mode"]
                assert first["battle_side_status"] == raw["bc_side_status"]
                assert raw["bc_available_metrics"] == 31 and raw["bc_control_applications"] == raw["bc_control_received"] == 0
            assert sum(row["bc_damage_dealt"] for row in contributions) == 112
            assert sum(row["bc_damage_taken"] for row in contributions) == 112
            assert sum(row["bc_healing_attempted"] for row in contributions) == 45
            assert sum(row["bc_effective_healing"] for row in contributions) == 15
            assert sum(row["bc_overhealing"] for row in contributions) == 30
            assert sum(row["bc_healing_received"] for row in contributions) == 15
            assert sum(row["bc_casting_attempts"] for row in contributions) == 6
            assert sum(row["bc_casting_completions"] for row in contributions) == 1
            assert sum(row["bc_casting_aborts"] for row in contributions) == 1
            assert sum(row["bc_casting_unresolved"] for row in contributions) == 4
            history = qualify_native_history(stored + contributions)
            source_window, _retained_inputs = qualify_retained_native_source(stored + contributions,
                (stored[0]["battle_environment_id"], stored[0]["battle_season_id"]), history)
            persistence = qualify_persisted_source(query, environment, command, name, history,
                executable, run_environment, export.with_name("controls.jsonl"))
            unavailable_opponent = [row for row in contributions if row["bc_actor_id"] == 8951]
            assert len(unavailable_opponent) == 1
            assert unavailable_opponent[0]["bc_end_reason"] == 4
            assert unavailable_opponent[0]["bc_quality_flags"] & 17 == 17
            assert not {8804, 8805}.intersection(row["bc_actor_id"] for row in contributions)
            assert query("SELECT COUNT(*) AS n FROM telemetry_quarantine")[0]["n"] == 0
            assert query("SELECT COUNT(*) AS n FROM telemetry_session")[0]["n"] == 2
            assert query("SELECT COUNT(*) AS n FROM telemetry_config")[0]["n"] == 2
            assert {row["battle_close_reason"] for row in stored if row["battle_fact_kind"] == 7} == {1, 2, 3}
            inactivity_closes = [row for row in stored if row["battle_fact_kind"] == 7 and row["battle_close_reason"] == 1]
            assert len(inactivity_closes) == 2
            for close in inactivity_closes:
                matching = [row for row in contributions if
                    (row["bc_battle_boot_id"], row["bc_battle_process_id"], row["bc_battle_seq"]) ==
                    (close["battle_boot_id"], close["battle_process_id"], close["battle_seq"])]
                assert len(matching) == 2
                for row in matching:
                    assert row["bc_end_reason"] == 3
                    assert row["bc_observed_through_monotonic_usec"] == close["battle_observed_through_monotonic_usec"]
                    assert row["bc_decision_monotonic_usec"] == close["battle_at_monotonic_usec"]
                    assert row["bc_engaged_target_usec"] == row["bc_observed_through_monotonic_usec"] - row["bc_start_monotonic_usec"]
            writer = pymysql.connect(host="127.0.0.1", port=int(environment["DB_PORT"]),
                user=user, password=password, database=name, autocommit=True,
                connect_timeout=3, read_timeout=10, write_timeout=10)
            for statement in ("SELECT * FROM accounts LIMIT 0", "DELETE FROM accounts WHERE 0",
                              "UPDATE telemetry_interval SET record_kind=10 WHERE 0",
                              "UPDATE telemetry_quarantine SET recovery_state=0 WHERE 0"):
                try:
                    with writer.cursor() as cursor:
                        cursor.execute(statement)
                except pymysql.err.OperationalError as error:
                    assert error.args[0] == 1142, (statement, error.args[0])
                else:
                    raise AssertionError("writer privileges exceeded: " + statement)
            result = dict(status="passed", engine=os.environ.get("TELEMETRY_REPOSITORY_DB_IMAGE"),
                normalized_metadata_fingerprint=fingerprint,
                migration_head=json.loads((ROOT / "migrations/migration_manifest.json").read_text())["migrations"][-1]["id"],
                battle_schema_migration="0061_telemetry_shared_battle_facts", records=len(stored), packets=packets,
                contribution_schema_migration="0062_telemetry_battle_contributions",
                contribution_records=len(contributions), contribution_field_count=65,
                history_complete_packets=history.summary["complete_packet_count"],
                history_verified_links=history.summary["verified_contribution_links"],
                history_partial_links=history.summary["partial_contribution_links"],
                history_alias_count=history.summary["alias_count"],
                history_reserved_bytes=history.summary["reserved_bytes"],
                history_exposure_count=history.summary["exposure_count"],
                history_verified_exposure_present_usec=history.summary["verified_exposure_present_usec"],
                history_source_value_contract=True,
                history_persisted_source_checkpoint=persistence["history_persisted_source_checkpoint"],
                history_source_reserved_bytes=source_window.reserved_bytes,
                history_atomic_publication=False,
                complete_association_references=True, exact_damage_total=112,
                unavailable_opponent_source_gap=True,
                inactivity_prefixes_preserved=True, inactivity_fixture_future_pulse=True,
                available_metric_mask=31, native_control_producer=True,
                battle_field_count=70, native_runtime=True, actual_worker=True,
                native_sql_writer=True, private_writer=True, running_server=False)
            result.update(persistence)
            result.update(qualify_native_build_storage(query, executable, run_environment,
                export.with_name("builds.jsonl"), environment, command, name))
            if artifact := os.environ.get("TELEMETRY_BATTLE_RUNTIME_RESULT"):
                Path(artifact).write_text(json.dumps(result, indent=2) + "\n")
            print(json.dumps(result, sort_keys=True), flush=True)
        finally:
            if writer:
                writer.close()
            if admin:
                if user_created:
                    with admin.cursor() as cursor:
                        cursor.execute("DROP USER IF EXISTS %s@'%%'", (user,))
                admin.close()
            drop_sql_fixture(environment, command, name)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sql-fixture", action="store_true")
    if not parser.parse_args().sql_fixture:
        parser.error("use the disposable --sql-fixture or run_telemetry_repository_sql.sh --battle-runtime")
    qualify()
