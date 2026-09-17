#!/usr/bin/env python3
"""Focused #267 progression fact, boundary and migration contract tests."""

from __future__ import annotations

import json
from pathlib import Path
import os
import shlex
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"
HARNESS = ROOT / "tests" / "async" / "telemetry_progression_harness.cc"
PROGRESSION = SRC / "telemetry" / "telemetry_progression.c"
LIMITS = SRC / "world" / "limits.c"
MIGRATION = ROOT / "migrations" / "immutable" / "0022_telemetry_progression.sql"
VERIFIER = ROOT / "migrations" / "immutable" / "0022_telemetry_progression.sh"


def source_contract() -> None:
    limits = LIMITS.read_text()
    gain = limits[limits.index("int gain_exp(") : limits.index("int gain_condition(")]

    storage = gain.index("GET_EXP(ch) += (int)XP_final")
    capture = gain.index("telemetry_runtime_game_progression", storage)
    display = gain.index("display_gain(ch", capture)
    assert storage < capture < display
    assert gain.index("const int computed_xp") < gain.index("BOUNDED", gain.index("const int computed_xp"))
    assert gain.index("TELEMETRY_PROGRESSION_MODIFIER_FINAL_CAP") > gain.index("BOUNDED")
    assert "progression_source_for_type(type)" in gain
    assert "progression_reason_for_type(type)" in gain
    assert "static_cast<std::int64_t>(before_exp)" in gain
    assert "static_cast<std::int64_t>(after_exp)" in gain
    assert "lose_level_impl(ch," in gain
    assert "static_cast<std::uint64_t>(new_exp_table[GET_LEVEL(ch)])" in gain
    assert "progression_source_for_type(type), progression_reason_for_type(type)" in gain

    level = limits[limits.index("static void advance_level_impl") : limits.index("void clear_title")]
    assert level.count("telemetry_runtime_game_progression") == 2
    assert "telemetry_progression_kind::level_advanced" in level
    assert "telemetry_progression_kind::level_lost" in level
    assert "telemetry_progression_reason::level_threshold" in level
    assert "telemetry_progression_reason::system_adjustment" in level
    assert "void lose_level(P_char ch)" in level

    fight = (SRC / "combat" / "fight.c").read_text()
    assert "telemetry_runtime_game_progression" not in fight


def schema_contract() -> None:
    sql = MIGRATION.read_text()
    columns = (
        "progression_kind",
        "progression_source",
        "progression_reason",
        "progression_observation_status",
        "progression_modifier_flags",
        "progression_requested_xp",
        "progression_computed_xp",
        "progression_applied_xp",
        "progression_before_exp",
        "progression_after_exp",
        "progression_before_level",
        "progression_after_level",
        "progression_threshold_xp",
    )
    assert sql.count("ALTER TABLE telemetry_interval ADD COLUMN") == len(columns)
    for column in columns:
        assert f"column_name='{column}'" in sql
        assert f"ADD COLUMN {column} " in sql
        assert f"column_name='{column}'" in VERIFIER.read_text()
    assert "CREATE TABLE" not in sql.upper()
    assert "CREATE TRIGGER" not in sql.upper()

    manifest = json.loads((ROOT / "migrations" / "migration_manifest.json").read_text())
    head = manifest["migrations"][-1]
    assert head["id"] == "0022_telemetry_progression"
    assert head["sequence"] == 22
    assert head["apply"] == "immutable/0022_telemetry_progression.sql"
    assert head["verify"] == "immutable/0022_telemetry_progression.sh"
    assert set(head) >= {"apply_checksum", "verify_checksum"}
    assert head["apply_checksum"] != "0" * 64
    assert head["verify_checksum"] != "0" * 64


def compile_and_run() -> None:
    compiler = shlex.split(os.environ.get("CXX", "g++"))
    with tempfile.TemporaryDirectory(prefix="telemetry-progression-") as directory:
        binary = Path(directory) / "telemetry_progression"
        command = compiler + [
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-pedantic",
            "-pthread",
            "-I",
            str(SRC),
            str(HARNESS),
            str(PROGRESSION),
            "-o",
            str(binary),
        ]
        subprocess.run(command, check=True)
        subprocess.run([str(binary)], check=True, timeout=10)


def main() -> None:
    source_contract()
    schema_contract()
    compile_and_run()
    print("telemetry progression source, schema and arithmetic contracts passed")


if __name__ == "__main__":
    main()
