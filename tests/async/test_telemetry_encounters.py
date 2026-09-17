#!/usr/bin/env python3
"""Focused #271 encounter lifecycle, schema and adapter contracts."""

from __future__ import annotations

import json
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"
HARNESS = ROOT / "tests" / "async" / "telemetry_encounter_harness.cc"
ENGINE = SRC / "telemetry" / "telemetry_encounter.c"
MIGRATION = ROOT / "migrations" / "immutable" / "0024_telemetry_encounters.sql"
VERIFIER = ROOT / "migrations" / "immutable" / "0024_telemetry_encounters.sh"


def source_contract() -> None:
    types = (SRC / "telemetry" / "telemetry_types.h").read_text()
    assert "telemetry_record_kind::encounter" in types
    assert "telemetry_encounter_payload_is_valid" in types
    assert "TELEMETRY_RECORD_MAX_BYTES" in types

    runtime = (SRC / "telemetry" / "telemetry_runtime.c").read_text()
    for name in (
        "telemetry_runtime_game_encounter_begin",
        "telemetry_runtime_game_encounter_group_sync",
        "telemetry_runtime_game_encounter_observe",
        "telemetry_runtime_game_encounter_leave",
        "telemetry_runtime_game_encounter_complete",
        "telemetry_runtime_encounter_close_all",
    ):
        assert name in runtime
    assert "telemetry_transport_enqueue" in runtime

    fight = (SRC / "combat" / "fight.c").read_text()
    assert "telemetry_runtime_game_encounter_begin" in fight
    assert "telemetry_encounter_outcome::death" in fight
    group = (SRC / "guild" / "group.c").read_text()
    assert "telemetry_runtime_game_encounter_group_sync" in group
    assert "telemetry_encounter_outcome::withdrawal" in group
    actoff = (SRC / "cmd" / "actoff.c").read_text()
    assert "telemetry_encounter_outcome::flee" in actoff
    assert "telemetry_encounter_outcome::withdrawal" in actoff
    epic = (SRC / "world" / "epic.c").read_text()
    assert "telemetry_runtime_game_encounter_complete" in epic
    assert "telemetry_encounter_outcome::success" in epic
    makefile = (SRC / "Makefile").read_text()
    assert "telemetry/telemetry_encounter.o" in makefile


def schema_contract() -> None:
    sql = MIGRATION.read_text()
    verifier = VERIFIER.read_text()
    required = (
        "encounter_boot_id", "encounter_process_id", "encounter_seq",
        "encounter_event", "encounter_mode", "encounter_outcome",
        "encounter_revision", "encounter_environment_id", "encounter_season_id",
        "encounter_config_id", "encounter_classifier_version",
        "encounter_policy_version", "encounter_zone_vnum", "encounter_group_key",
        "encounter_participant_subject_id", "encounter_participant_pid",
        "at_monotonic_usec", "at_utc_usec", "encounter_start_monotonic_usec",
        "encounter_start_utc_usec", "elapsed_usec", "participant_usec",
        "participant_count", "expected_credit_count", "encounter_quality_flags",
    )
    assert "CREATE TABLE" not in sql.upper()
    assert "CREATE TRIGGER" not in sql.upper()
    assert "uq_telemetry_encounter_event" in sql
    for column in required:
        assert column in sql
        assert column in verifier

    manifest = json.loads((ROOT / "migrations" / "migration_manifest.json").read_text())
    head = next(item for item in manifest["migrations"]
                if item["id"] == "0024_telemetry_encounters")
    assert head["id"] == "0024_telemetry_encounters"
    assert head["sequence"] == 24
    assert head["apply"] == "immutable/0024_telemetry_encounters.sql"
    assert head["verify"] == "immutable/0024_telemetry_encounters.sh"
    assert head["apply_checksum"] != "0" * 64
    assert head["verify_checksum"] != "0" * 64


def report_contract() -> None:
    import sys

    sys.path.insert(0, str(ROOT / "scripts" / "telemetry"))
    from encounter_definitions import (  # noqa: E402
        ATTEMPT_DENOMINATOR_OUTCOMES,
        ENCOUNTER_REPORT_DEFINITION,
        denominator_bucket,
    )

    assert ENCOUNTER_REPORT_DEFINITION["source_table"] == "telemetry_interval"
    assert "run_elapsed_usec" in ENCOUNTER_REPORT_DEFINITION["metrics"]
    assert "participant_usec" in ENCOUNTER_REPORT_DEFINITION["metrics"]
    assert "expected_credit_count" in ENCOUNTER_REPORT_DEFINITION["metrics"]
    assert len(ATTEMPT_DENOMINATOR_OUTCOMES) == 10
    assert denominator_bucket(1) == "successful"
    assert denominator_bucket(5) == "attempted_non_success"
    assert denominator_bucket(None) == "unclosed_tail"


def compile_and_run() -> None:
    compiler = shlex.split(os.environ.get("CXX", "g++"))
    with tempfile.TemporaryDirectory(prefix="telemetry-encounter-") as directory:
        binary = Path(directory) / "telemetry_encounter"
        command = compiler + [
            "-std=c++20", "-Wall", "-Wextra", "-Werror", "-pedantic", "-I", str(SRC),
            str(HARNESS), str(ENGINE), "-o", str(binary),
        ]
        subprocess.run(command, cwd=ROOT, check=True, timeout=120)
        subprocess.run([str(binary)], cwd=ROOT, check=True, timeout=30)


def main() -> None:
    source_contract()
    schema_contract()
    report_contract()
    compile_and_run()
    print("telemetry encounter lifecycle, schema and report contracts passed")


if __name__ == "__main__":
    main()
