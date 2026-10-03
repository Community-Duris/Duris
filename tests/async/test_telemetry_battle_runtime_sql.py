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
from test_telemetry_battle_history import qualify_native_history
from test_telemetry_repository import prepare_sql_fixture, drop_sql_fixture
from test_telemetry_incidents import runtime_fingerprint

sys.path.insert(0, str(ROOT / "scripts/telemetry"))
import battle_contract
import battle_contribution_contract
from db_access import RAW_COLUMNS


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
                history_atomic_publication=False,
                complete_association_references=True, exact_damage_total=112,
                unavailable_opponent_source_gap=True,
                inactivity_prefixes_preserved=True, inactivity_fixture_future_pulse=True,
                available_metric_mask=27, native_control_producer=False,
                battle_field_count=70, native_runtime=True, actual_worker=True,
                native_sql_writer=True, private_writer=True, running_server=False)
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
