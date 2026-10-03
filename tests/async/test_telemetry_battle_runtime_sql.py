#!/usr/bin/env python3
"""Actual gameplay capture/worker/native-writer proof on disposable loopback SQL.

The gameplay objects and private connection factory are fixture seams. This does
not qualify a running server, account authentication, or production activation.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

from test_telemetry_gameplay_adapters import ROOT, compile_gameplay
from test_telemetry_battle_history import qualify_native_history, qualify_retained_native_source
from test_telemetry_repository import prepare_sql_fixture, drop_sql_fixture
from test_telemetry_incidents import runtime_fingerprint

sys.path.insert(0, str(ROOT / "scripts/telemetry"))
import battle_contract
import battle_contribution_contract
import battle_source as retained_source
import battle_history
import identity_history
from db_access import (RAW_COLUMNS, AmbiguousCommit, ConnectionSettings, GenerationConflict,
    PyMySQLConnectionFactory, PyMySQLRollupDatabase)
from rollup_definitions import RollupTarget, PUBLICATION_BUILDING
from rollup_engine import RollupEngine, RollupBounds, BoundsExceeded, SemanticError
from test_telemetry_observations import ownership


def qualify_persisted_source(query, environment, command, name, history):
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
                "telemetry_battle_input": "SELECT,INSERT", "telemetry_interval": "SELECT"},
            "report": {"telemetry_rollup_state": "SELECT"},
        }
        for role, tables in grants.items():
            user = "tbr_source_" + role + "_" + token
            query("CREATE USER %s@'%%' IDENTIFIED BY %s", (user, password))
            users.append(user)
            for table, permissions in tables.items():
                query(f"GRANT {permissions} ON `{name}`.`{table}` TO %s@'%%'", (user,))

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
            rollup.publish_generation(target)
        except GenerationConflict:
            pass
        else:
            raise AssertionError("source preparation was published as a battle report")
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
        result = subprocess.run([sys.executable, "scripts/telemetry/rollup.py", "publish", *target_args], cwd=ROOT,
            env=cli_environment, capture_output=True, timeout=30)
        assert result.returncode != 0 and b"preparation cannot publish" in result.stderr

        for role in ("rollup", "report"):
            principal = PyMySQLConnectionFactory(settings(role)).connect()
            try:
                denied = ["UPDATE telemetry_battle_input SET record_kind=9 WHERE 0", "DELETE FROM telemetry_battle_input WHERE 0",
                    "SELECT * FROM accounts LIMIT 0"] if role == "rollup" else ["SELECT * FROM telemetry_battle_source LIMIT 0", "SELECT * FROM telemetry_battle_input LIMIT 0"]
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

        return dict(history_persisted_source_checkpoint=True, battle_source_identity_reservation=True,
            battle_source_cursor_atomicity=True, battle_source_lost_acknowledgements=True,
            battle_source_exact_values=True, battle_source_ownership_arrival=True, battle_source_private_roles=True,
            battle_source_bounded_reads=True, battle_source_cli_preparation=True,
            battle_source_publication_refused=True, battle_source_selected_inputs=expanded.header["source_fact_count"],
            battle_source_retention=True, battle_source_schema_drift_refused=True, battle_source_restored_metadata=True,
            battle_source_read_transaction_released=True,
            battle_source_migration_apply_checksum=hashlib.sha256(schema.read_bytes()).hexdigest(),
            battle_source_migration_verify_checksum=hashlib.sha256(schema.with_suffix(".sh").read_bytes()).hexdigest())
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
                assert raw["bc_available_metrics"] == 27 and raw["bc_control_applications"] == raw["bc_control_received"] == 0
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
            persistence = qualify_persisted_source(query, environment, command, name, history)
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
                available_metric_mask=27, native_control_producer=False,
                battle_field_count=70, native_runtime=True, actual_worker=True,
                native_sql_writer=True, private_writer=True, running_server=False)
            result.update(persistence)
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
