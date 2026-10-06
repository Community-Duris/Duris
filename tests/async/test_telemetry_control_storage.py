#!/usr/bin/env python3
"""Disposable full-chain typed control schema, exact values and loss inventory."""
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
from scripts.telemetry import control_contract as controls, battle_result_contract as results, incident
from scripts.telemetry.db_access import ConnectionSettings, PyMySQLConnectionFactory, PyMySQLRollupDatabase, RAW_COLUMNS
from test_telemetry_battle_contribution_contract import ControlContractTests, ResultContractTests
from test_telemetry_incidents import runtime_fingerprint
from test_telemetry_repository import prepare_sql_fixture, drop_sql_fixture


def qualification():
    if os.environ.get("TELEMETRY_REPOSITORY_DISPOSABLE") != "1":
        raise RuntimeError("explicit disposable qualification required")
    import pymysql

    ControlContractTests.setUpClass()
    try:
        operations = tuple(dict(item["fields"]) for item in ControlContractTests.sources if item["case"] == 1)
        native = dict(ControlContractTests.value)
        prefix = tuple(dict(item["fields"]) for item in ControlContractTests.sources if item["case"] == 2)
        gap = dict(ControlContractTests.gap)
    finally:
        ControlContractTests.tearDownClass()
    ResultContractTests.setUpClass()
    try:
        result_values = tuple(dict(value) for value in ResultContractTests.rows)
    finally:
        ResultContractTests.tearDownClass()
    environment, command, database = prepare_sql_fixture()
    admin, reviewer, user = None, None, None
    migration = ROOT / "migrations/immutable/0067_telemetry_typed_control.sql"
    sealed_names = ("telemetry_incident_registry_v5", "telemetry_incident_v5",
                    "telemetry_battle_source_v6", "telemetry_rollup_battle_row_v6")
    try:
        fingerprint = runtime_fingerprint(environment)
        admin = pymysql.connect(host="127.0.0.1", port=int(environment["DB_PORT"]),
            user=environment["DB_USER"], password=environment["DB_PASSWD"], database=database,
            autocommit=True, cursorclass=pymysql.cursors.DictCursor,
            connect_timeout=3, read_timeout=10, write_timeout=10)

        def query(sql, parameters=()):
            with admin.cursor() as cursor:
                cursor.execute(sql, parameters)
                return list(cursor.fetchall()) if cursor.description else []

        def insert(value, receipt):
            row = dict(value, boot_id=value["ctl_boot_id"], process_id=value["ctl_process_id"],
                record_seq=receipt, schema_version=1, record_kind=13,
                occurrence_utc_usec=value["ctl_decision_utc_usec"], ingested_utc_usec=123456)
            controls.validate_raw_observation(row)
            names = tuple(row)
            query("INSERT INTO telemetry_interval (" + ",".join(names) + ") VALUES (" +
                  ",".join(["%s"] * len(names)) + ")", tuple(row[name] for name in names))

        assert len(operations) == 112
        for receipt, value in enumerate(operations, 1):
            insert(value, receipt)
        actual = query("SELECT " + ",".join(RAW_COLUMNS) + " FROM telemetry_interval ORDER BY ingest_id")
        assert [controls.validate_raw_observation(row) for row in actual] == list(operations)
        receipt = next(index for index, value in enumerate(operations, 1) if value == native)
        original = query("SELECT * FROM telemetry_interval")
        for name, value in (("ctl_configured_ticks", None), ("ctl_definition_version", 2),
            ("ctl_producer_version", 2), ("ctl_boot_id", native["ctl_boot_id"] + 1),
            ("ctl_decision_utc_usec", 0), ("record_kind", 11), ("ctl_target_actor_id", 0),
            ("ctl_source_actor_pid", -1), ("ctl_target_actor_kind", 4), ("ctl_target_context_version", 0),
            ("ctl_target_zone_vnum", -2), ("ctl_target_group_key", 1 << 63),
            ("ctl_target_group_revision", 1), ("ctl_source_session_boot_id", 0),
            ("ctl_target_association_revision", 0), ("ctl_last_target_association_fact_sequence", 0),
            ("ctl_state_available", 0), ("ctl_duration_coverage", 255), ("ctl_before_mask", 256),
            ("ctl_after_mask", 0), ("ctl_flags", 64), ("ctl_flags", 32), ("ctl_quality_flags", 1024),
            ("ctl_config_id", 0), ("ctl_scope_zone_vnum", 0), ("ctl_family", 0),
            ("ctl_result", 0), ("ctl_kind", 5), ("ctl_boundary", 1),
            ("ctl_previous_state_sequence", native["ctl_sequence"]), ("duration_usec", 0),
            ("bctx_equipment_occupied_slots_count", 0)):
            try:
                query("UPDATE telemetry_interval SET " + name + "=%s WHERE record_seq=%s", (value, receipt))
            except (pymysql.err.IntegrityError, pymysql.err.OperationalError) as error:
                assert error.args[0] in (3819, 4025, 1644), (name, error.args[0])
            else:
                raise AssertionError("control CHECK accepted " + name)
        assert query("SELECT * FROM telemetry_interval") == original
        # SQL must enforce the complete tagged payload. Zero in an inactive
        # field is present data, even when an earlier immutable family owns it.
        headers = {"ingest_id", "boot_id", "process_id", "record_seq", "schema_version",
                   "record_kind", "occurrence_utc_usec", "ingested_utc_usec"}
        inactive = set(RAW_COLUMNS) - headers - set(controls.FIELDS)
        shapes = query("SELECT column_name AS name,data_type AS sql_type,character_maximum_length AS width FROM information_schema.columns "
                       "WHERE table_schema=DATABASE() AND table_name='telemetry_interval'")
        checked = 0
        for shape in shapes:
            field = shape["name"]
            if field not in inactive and field not in controls.FIELDS:
                continue
            value = None if field in controls.FIELDS else (
                bytes(shape["width"]) if shape["sql_type"] == "binary" else 0)
            try:
                query("UPDATE telemetry_interval SET " + field + "=%s WHERE record_seq=%s", (value, receipt))
            except (pymysql.err.IntegrityError, pymysql.err.OperationalError) as error:
                assert error.args[0] in (3819, 4025, 1644), (field, error.args[0])
            else:
                raise AssertionError("tagged control CHECK accepted " + field)
            checked += 1
        assert checked == len(inactive) + len(controls.FIELDS)
        assert query("SELECT * FROM telemetry_interval") == original
        try:
            insert(native, 5000)
        except pymysql.err.IntegrityError as error:
            assert error.args[0] == 1062
        else:
            raise AssertionError("second receipt bypassed control logical uniqueness")

        # Each toy timeline is a separate producer lifetime. Its receipt keys
        # cannot be spliced into the resolution producer's chronological history.
        for value in prefix:
            value["ctl_boot_id"] += 1000
            insert(value, value["ctl_sequence"])
        gap.update(ctl_boot_id=gap["ctl_boot_id"] + 2000, ctl_kind=4, ctl_boundary=8,
            ctl_config_id=0, ctl_classifier_version=0, ctl_policy_version=0,
            ctl_build_version=0, ctl_content_version=0)
        insert(gap, gap["ctl_sequence"])
        for row in query("SELECT " + ",".join(RAW_COLUMNS) + " FROM telemetry_interval ORDER BY ingest_id"):
            controls.validate_raw_observation(row)

        token = hashlib.sha256(database.encode()).hexdigest()[:12]
        user, password = "tctl_review_" + token, "control-review-fixture-" + token
        query("CREATE USER %s@'%%' IDENTIFIED BY %s", (user, password))
        for table in ("telemetry_interval", "telemetry_progression_context", "telemetry_progression_configuration", "telemetry_incident_registry_v6", "telemetry_incident_v6",
                      "telemetry_incident_registry_v7", "telemetry_incident_v7"):
            permissions = "SELECT" if table in ("telemetry_interval", "telemetry_progression_context", "telemetry_progression_configuration") else "SELECT,INSERT"
            query("GRANT " + permissions + " ON `" + database + "`." + table + " TO %s@'%%'", (user,))
        reviewer = PyMySQLRollupDatabase(PyMySQLConnectionFactory(ConnectionSettings(
            host="127.0.0.1", port=int(environment["DB_PORT"]), database=database, user=user, password=password)))
        packet = incident.template(6)
        packet.update(environment_id=native["ctl_environment_id"], season_id=native["ctl_season_id"],
            reviewer_token="11" * 32, review_evidence_digest="22" * 32)
        packet["incidents"][0].update(record_kind_mask=1 << 13, evidence_digest="33" * 32,
            fix_reference_digest="44" * 32, first_verified_postfix=dict(boot_id=native["ctl_boot_id"],
                process_id=native["ctl_process_id"], record_seq=receipt, record_kind=13,
                occurrence_utc_usec=native["ctl_decision_utc_usec"]))
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
        bad["season_id"] += 1
        try:
            reviewer.register_incident_packet(bad)
        except incident.IncidentError as error:
            assert str(error) == "postfix_fact_mismatch"
        else:
            raise AssertionError("control inventory accepted a different scope")
        execute = reviewer._execute

        def reject_detail(sql, parameters=()):
            if sql.startswith("INSERT INTO telemetry_incident_v6 "):
                raise RuntimeError("synthetic control review detail failure")
            return execute(sql, parameters)

        reviewer._execute = reject_detail
        try:
            try:
                reviewer.register_incident_packet(packet)
            except RuntimeError as error:
                assert str(error) == "synthetic control review detail failure"
            else:
                raise AssertionError("control review detail refusal missing")
        finally:
            reviewer._execute = execute
        assert query("SELECT COUNT(*) AS n FROM telemetry_incident_registry_v6")[0]["n"] == 0
        assert query("SELECT COUNT(*) AS n FROM telemetry_incident_v6")[0]["n"] == 0
        assert reviewer.register_incident_packet(packet)["status"] == "registered"
        assert reviewer.register_incident_packet(packet)["status"] == "already_registered"
        bad = deepcopy(packet)
        bad["registry_schema_version"] = 5
        try:
            incident.validate_packet(bad)
        except incident.IncidentError:
            pass
        else:
            raise AssertionError("sealed incident schema 5 widened to control family 13")
        assert all(query("SELECT COUNT(*) AS n FROM " + table)[0]["n"] == 0 for table in sealed_names)

        # These are native value fixtures, separate from the real-server journey.
        # Qualify every positive value shape and exact operation bytes in SQL.
        assert len(result_values) == 17
        for index, value in enumerate(result_values, 1):
            row = dict(value, boot_id=value["bout_boot_id"], process_id=value["bout_process_id"],
                record_seq=6000 + index, schema_version=1, record_kind=14,
                occurrence_utc_usec=value["bout_at_utc_usec"], ingested_utc_usec=123456)
            results.validate_raw_observation(row)
            names = tuple(row)
            query("INSERT INTO telemetry_interval (" + ",".join(names) + ") VALUES (" +
                  ",".join(["%s"] * len(names)) + ")", tuple(row[name] for name in names))
        actual = query("SELECT " + ",".join(RAW_COLUMNS) + " FROM telemetry_interval "
                       "WHERE record_kind=14 ORDER BY ingest_id")
        assert [results.validate_raw_observation(row) for row in actual] == list(result_values)
        result_saved = query("SELECT * FROM telemetry_interval WHERE record_kind=14 ORDER BY ingest_id")
        result_inactive = set(RAW_COLUMNS) - headers - set(results.FIELDS)
        result_checked = 0
        for shape in shapes:
            field = shape["name"]
            if field not in result_inactive and field not in results.FIELDS:
                continue
            value = None if field in results.FIELDS else (
                bytes(shape["width"]) if shape["sql_type"] == "binary" else 0)
            try:
                query("UPDATE telemetry_interval SET " + field + "=%s WHERE record_kind=14 AND record_seq=6007", (value,))
            except (pymysql.err.IntegrityError, pymysql.err.OperationalError) as error:
                assert error.args[0] in (3819, 4025, 1644), (field, error.args[0])
            else:
                raise AssertionError("tagged result CHECK accepted " + field)
            result_checked += 1
        assert result_checked == len(result_inactive) + len(results.FIELDS)
        assert query("SELECT * FROM telemetry_interval WHERE record_kind=14 ORDER BY ingest_id") == result_saved
        packet7 = incident.template(7)
        packet7.update(environment_id=result_values[0]["bout_environment_id"],
            season_id=result_values[0]["bout_season_id"], reviewer_token="55" * 32,
            review_evidence_digest="66" * 32)
        packet7["incidents"][0].update(record_kind_mask=1 << 14, evidence_digest="77" * 32,
            fix_reference_digest="88" * 32, first_verified_postfix=dict(
                boot_id=result_values[0]["bout_boot_id"], process_id=result_values[0]["bout_process_id"],
                record_seq=6001, record_kind=14, occurrence_utc_usec=result_values[0]["bout_at_utc_usec"]))
        for changes, reason in (({"environment_id": packet7["environment_id"] + 1}, "postfix_fact_mismatch"),
                                ({"season_id": packet7["season_id"] + 1}, "postfix_fact_mismatch")):
            bad = dict(packet7, **changes)
            try:
                reviewer.register_incident_packet(bad)
            except incident.IncidentError as error:
                assert str(error) == reason
            else:
                raise AssertionError("result review accepted a different scope")
        assert reviewer.register_incident_packet(packet7)["status"] == "registered"
        assert reviewer.register_incident_packet(packet7)["status"] == "already_registered"
        assert all(query("SELECT COUNT(*) AS n FROM " + table)[0]["n"] == 0 for table in sealed_names)

        def apply():
            subprocess.run(command + [database], input=migration.read_bytes(), env=environment,
                           check=True, capture_output=True)

        def verify(expected=True):
            result = subprocess.run(["bash", str(migration.with_suffix(".sh"))], env=environment,
                                    capture_output=True, text=True, timeout=45)
            assert (result.returncode == 0) == expected, result.stderr[-600:]

        saved = query("SELECT * FROM telemetry_interval ORDER BY ingest_id")
        apply()
        verify()
        assert query("SELECT * FROM telemetry_interval ORDER BY ingest_id") == saved
        query("ALTER TABLE telemetry_interval MODIFY ctl_flags INT UNSIGNED NULL DEFAULT NULL")
        verify(False)
        apply()
        verify(False)
        query("ALTER TABLE telemetry_interval MODIFY ctl_flags SMALLINT UNSIGNED NULL DEFAULT NULL")
        verify()
        query("ALTER TABLE telemetry_interval DROP INDEX uq_telemetry_control, "
              "ADD UNIQUE KEY uq_telemetry_control (ctl_sequence,ctl_boot_id,ctl_process_id)")
        verify(False)
        apply()
        verify(False)
        query("ALTER TABLE telemetry_interval DROP INDEX uq_telemetry_control, "
              "ADD UNIQUE KEY uq_telemetry_control (ctl_boot_id,ctl_process_id,ctl_sequence)")
        verify()
        trigger = "telemetry_control_insert"
        original_action = query("SELECT action_statement AS body FROM information_schema.triggers "
                                "WHERE trigger_schema=DATABASE() AND trigger_name=%s", (trigger,))[0]["body"]
        query("DROP TRIGGER " + trigger)
        query("CREATE TRIGGER " + trigger + " BEFORE INSERT ON telemetry_interval FOR EACH ROW PRECEDES telemetry_battle_result_insert SET @typed_control_drift=1")
        verify(False)
        assert runtime_fingerprint(environment) != fingerprint
        apply()
        verify(False)
        query("DROP TRIGGER " + trigger)
        query("CREATE TRIGGER " + trigger + " BEFORE INSERT ON telemetry_interval FOR EACH ROW PRECEDES telemetry_battle_result_insert " + original_action)
        verify()
        # Parentheses affect SQL operator precedence. A digest that discarded
        # them would accept this different rule despite identical other tokens.
        changed_action = original_action.replace("(NEW.record_kind<>13)*76", "(NEW.record_kind<>13*76)")
        assert changed_action != original_action
        assert changed_action.replace("(", "").replace(")", "") == original_action.replace("(", "").replace(")", "")
        query("DROP TRIGGER " + trigger)
        query("CREATE TRIGGER " + trigger + " BEFORE INSERT ON telemetry_interval FOR EACH ROW PRECEDES telemetry_battle_result_insert " + changed_action)
        verify(False)
        assert runtime_fingerprint(environment) != fingerprint
        apply()
        verify(False)
        query("DROP TRIGGER " + trigger)
        query("CREATE TRIGGER " + trigger + " BEFORE INSERT ON telemetry_interval FOR EACH ROW PRECEDES telemetry_battle_result_insert " + original_action)
        verify()
        assert runtime_fingerprint(environment) == fingerprint
        assert query("SELECT * FROM telemetry_interval ORDER BY ingest_id") == saved
        proof = dict(engine=os.environ["TELEMETRY_REPOSITORY_DB_IMAGE"], status="passed",
            migration_head=environment["TELEMETRY_REPOSITORY_MIGRATION_HEAD"],
            control_migration_head=migration.stem,
            migration_count=int(environment["TELEMETRY_REPOSITORY_MIGRATION_COUNT"]),
            normalized_metadata_fingerprint=fingerprint,
            exact_fields=76, wire_bytes=368, typed_resolutions=112, independent_incident_schema=6,
            sealed_versions_preserved=True, full_schema_rerun=True, schema_drift_refused=True,
            restored_metadata_fingerprint=True, verified_postfix_scope=True, private_review_role=True,
            tagged_payload_columns_checked=checked, inactive_family_columns_checked=len(inactive),
            validation_triggers=2, trigger_drift_refused=True, runtime_trigger_metadata_pinned=True,
            trigger_operator_grouping_drift_refused=True, validation_trigger_order_preserved=True,
            migration_apply_checksum=hashlib.sha256(migration.read_bytes()).hexdigest(),
            migration_verify_checksum=hashlib.sha256(migration.with_suffix(".sh").read_bytes()).hexdigest(),
            result_evidence_source="native value fixtures; actual gameplay qualified separately",
            result_values=len(result_values), result_fields=len(results.FIELDS),
            result_tagged_payload_columns_checked=result_checked,
            result_inactive_family_columns_checked=len(result_inactive),
            result_operation_bytes=16, result_review_schema=7,
            result_review_verified_postfix_scope=True, sealed_result_review_inventory=True)
        destination = os.environ.get("TELEMETRY_CONTROL_STORAGE_RESULT")
        if destination:
            with Path(destination).open("x", encoding="utf-8") as output:
                json.dump(proof, output, indent=2)
                output.write("\n")
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
