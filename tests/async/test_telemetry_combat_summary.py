#!/usr/bin/env python3
"""Focused #276 combat summary contract, reconciliation, and schema checks."""

from __future__ import annotations

import json
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"
HARNESS = ROOT / "tests" / "async" / "telemetry_combat_summary_harness.cc"
ENGINE = SRC / "telemetry" / "telemetry_combat_summary.c"
MIGRATION = ROOT / "migrations" / "immutable" / "0025_telemetry_combat_summaries.sql"
VERIFIER = ROOT / "migrations" / "immutable" / "0025_telemetry_combat_summaries.sh"


def source_contract() -> None:
    types = (SRC / "telemetry" / "telemetry_types.h").read_text()
    for marker in (
        "telemetry_record_kind::combat_summary",
        "telemetry_combat_summary_payload",
        "TELEMETRY_QUALITY_CARDINALITY_OVERFLOW",
        "telemetry_combat_summary_payload_is_valid",
    ):
        assert marker in types
    assert "telemetry_record_kind_is_control" in types

    summary_header = (SRC / "telemetry" / "telemetry_combat_summary.h").read_text()
    for marker in (
        "TELEMETRY_COMBAT_SUMMARY_MAX_ACTIVE",
        "TELEMETRY_COMBAT_SUMMARY_MAX_ACTORS",
        "telemetry_combat_summary_record_damage",
        "telemetry_combat_summary_record_healing",
        "telemetry_combat_summary_record_control",
        "telemetry_combat_summary_cast_attempt",
        "telemetry_combat_summary_close",
    ):
        assert marker in summary_header

    runtime = (SRC / "telemetry" / "telemetry_runtime.c").read_text()
    for marker in (
        "telemetry_combat_summary_state combat_summary",
        "emit_combat_summary",
        "telemetry_runtime_game_combat_damage",
        "telemetry_runtime_game_combat_healing",
        "telemetry_runtime_game_combat_cast_attempt",
        "telemetry_runtime_game_combat_context",
    ):
        assert marker in runtime

    fight = (SRC / "combat" / "fight.c").read_text()
    for marker in (
        "telemetry_runtime_game_combat_damage",
        "telemetry_runtime_game_combat_healing",
        "telemetry_runtime_game_combat_context",
    ):
        assert marker in fight
    sparser = (SRC / "net" / "sparser.c").read_text()
    for marker in (
        "telemetry_runtime_game_combat_cast_attempt",
        "telemetry_runtime_game_combat_cast_complete",
        "telemetry_runtime_game_combat_cast_abort",
    ):
        assert marker in sparser
    repository = (SRC / "telemetry" / "telemetry_repository.c").read_text()
    assert "combat_summary_fields" in repository
    assert "case telemetry_record_kind::combat_summary" in repository
    assert "combat_classifier_version" in repository
    assert "telemetry/telemetry_combat_summary.o" in (SRC / "Makefile").read_text()


def schema_contract() -> None:
    sql = MIGRATION.read_text()
    verifier = VERIFIER.read_text()
    required = (
        "combat_encounter_boot_id", "combat_encounter_process_id", "combat_encounter_seq",
        "combat_mode", "combat_outcome", "combat_revision", "combat_environment_id",
        "combat_season_id", "combat_config_id", "combat_classifier_version",
        "combat_policy_version", "combat_zone_vnum", "combat_group_key", "combat_actor_id",
        "combat_actor_pid", "combat_owner_subject_id", "combat_actor_kind",
        "combat_unique_player_count", "combat_participant_count",
        "combat_dropped_participant_count", "combat_power_band",
        "combat_opponent_power_band", "combat_opponent_count", "combat_modifier_flags",
        "combat_start_monotonic_usec", "combat_end_monotonic_usec", "combat_start_utc_usec",
        "combat_end_utc_usec", "combat_damage_dealt", "combat_damage_taken",
        "combat_healing_attempted", "combat_effective_healing", "combat_overhealing",
        "combat_control_applications", "combat_casting_attempts",
        "combat_casting_completions", "combat_casting_aborts",
        "combat_casting_elapsed_usec", "combat_tanking_usec", "combat_quality_flags",
    )
    assert "CREATE TABLE" not in sql.upper()
    assert "CREATE TRIGGER" not in sql.upper()
    assert "uq_telemetry_combat_summary" in sql
    for column in required:
        assert column in sql
        assert column in verifier

    manifest = json.loads((ROOT / "migrations" / "migration_manifest.json").read_text())
    head = next(item for item in manifest["migrations"]
                if item["id"] == "0025_telemetry_combat_summaries")
    assert head["id"] == "0025_telemetry_combat_summaries"
    assert head["sequence"] == 25
    assert head["apply"] == "immutable/0025_telemetry_combat_summaries.sql"
    assert head["verify"] == "immutable/0025_telemetry_combat_summaries.sh"
    assert len(head["apply_checksum"]) == 64
    assert len(head["verify_checksum"]) == 64


def documentation_contract() -> None:
    docs = (ROOT / "docs" / "telemetry" / "COMBAT_METRICS.md").read_text()
    for marker in (
        "one row for each retained actor",
        "unique_player_count",
        "Casting attempts are the denominator",
        "TELEMETRY_QUALITY_UNCLOSED_TAIL",
        "does not compare popularity with strength",
        "default-off",
    ):
        assert marker in docs


def compile_and_run() -> None:
    compiler = shlex.split(os.environ.get("CXX", "g++"))
    with tempfile.TemporaryDirectory(prefix="telemetry-combat-summary-") as directory:
        binary = Path(directory) / "telemetry_combat_summary"
        command = compiler + [
            "-std=c++20", "-Wall", "-Wextra", "-Werror", "-pedantic", "-I", str(SRC),
            str(HARNESS), str(ENGINE), "-o", str(binary),
        ]
        subprocess.run(command, cwd=ROOT, check=True, timeout=120)
        subprocess.run([str(binary)], cwd=ROOT, check=True, timeout=30)


def main() -> None:
    source_contract()
    schema_contract()
    documentation_contract()
    compile_and_run()
    print("telemetry combat summary contract and reconciliation passed")


if __name__ == "__main__":
    main()
