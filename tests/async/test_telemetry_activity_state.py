#!/usr/bin/env python3
"""Focused #266 pure activity/context accounting tests."""

from __future__ import annotations

import json
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile
from typing import Any


ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"
TELEMETRY = SRC / "telemetry"
ACTIVITY_SOURCE = TELEMETRY / "telemetry_activity.c"
ACTIVITY_HEADER = TELEMETRY / "telemetry_activity.h"
PRIVATE_HEADER = TELEMETRY / "telemetry_activity_private.h"
STATE_HARNESS = ROOT / "tests" / "async" / "telemetry_activity_state_harness.cc"
GOLDEN_HARNESS = ROOT / "tests" / "async" / "telemetry_activity_b_golden_harness.cc"
SESSION_SOURCE = TELEMETRY / "telemetry_session.c"
INTEGRATION_HARNESS = ROOT / "tests" / "async" / "telemetry_activity_session_integration_harness.cc"
FIXTURES = ROOT / "tests" / "async" / "fixtures" / "telemetry" / "contract"

B_GOLDEN_FIXTURES = ("normal_interval.json", "detach_reconnect.json")


def _u64(value: int) -> str:
    return f"{value}ULL"


def _utc(value: int) -> str:
    if value == -(1 << 63):
        return "TELEMETRY_UTC_UNKNOWN"
    return f"static_cast<telemetry_utc_usec>({value})"


def _quality(value: int) -> str:
    return f"{value}U"


def _producer_expr(producer: dict[str, int]) -> str:
    return f"make_producer({_u64(producer['boot_id'])}, {_u64(producer['process_id'])})"


def _session_expr(session: dict[str, Any]) -> str:
    session_id = session["id"]
    producer = session_id["producer"]
    return (
        "make_session("
        + ", ".join(
            (
                _u64(producer["boot_id"]),
                _u64(producer["process_id"]),
                _u64(session_id["session_seq"]),
                _u64(session["subject_id"]),
                f"static_cast<telemetry_pid>({session['pid']})",
                _u64(session["season_id"]),
                _u64(session["environment_id"]),
            )
        )
        + ")"
    )


def _connection_expr(connection: dict[str, Any]) -> str:
    producer = connection["producer"]
    return (
        "make_connection("
        + ", ".join(
            (
                _u64(producer["boot_id"]),
                _u64(producer["process_id"]),
                _u64(connection["connection_seq"]),
            )
        )
        + ")"
    )


def _dimensions_expr(dimensions: dict[str, Any]) -> str:
    return (
        "make_dimensions("
        + ", ".join(
            (
                str(dimensions["level_band"]),
                str(dimensions["class_id"]),
                str(dimensions["race_id"]),
                str(dimensions["faction_id"]),
                str(dimensions["zone_vnum"]),
                str(dimensions["group_size"]),
            )
        )
        + ")"
    )


def _lifecycle_payload(fixture: dict[str, Any]) -> dict[str, Any]:
    for operation in fixture["operations"]:
        if operation["name"] == "enter":
            record = fixture["records"][operation["record_indexes"][0]]
            return record["payload"]["session_lifecycle"]
    raise AssertionError(f"{fixture['fixture_id']} has no enter operation")


def _interval_payloads(fixture: dict[str, Any]) -> list[dict[str, Any]]:
    names = (
        ["seal_interval"]
        if fixture["fixture_id"] == "normal_interval"
        else ["connected_interval", "linkdead_interval", "reconnected_interval"]
    )
    by_name = {operation["name"]: operation for operation in fixture["operations"]}
    result = []
    for name in names:
        operation = by_name[name]
        result.append(fixture["records"][operation["record_indexes"][0]]["payload"]["interval"])
    return result


def _context_flag(context: str) -> str:
    return {
        "combat": "TELEMETRY_ACTIVITY_CONTEXT_COMBAT",
        "travel": "TELEMETRY_ACTIVITY_CONTEXT_TRAVEL",
        "crafting": "TELEMETRY_ACTIVITY_CONTEXT_CRAFTING",
        "social": "TELEMETRY_ACTIVITY_CONTEXT_SOCIAL",
        "administration": "TELEMETRY_ACTIVITY_CONTEXT_ADMINISTRATION",
        "other": "TELEMETRY_ACTIVITY_CONTEXT_OTHER",
        "none": "TELEMETRY_ACTIVITY_CONTEXT_NONE",
        "unknown": "TELEMETRY_ACTIVITY_CONTEXT_UNKNOWN",
    }[context]


def _category_enum(category: str) -> str:
    return f"telemetry_interval_category::{category}"


def _context_enum(context: str) -> str:
    return f"telemetry_activity_context::{context}"


def _context_quality_enum(quality: str) -> str:
    return f"telemetry_context_quality::{quality}"


def _enter_expr(payload: dict[str, Any]) -> str:
    return (
        "make_enter("
        + ", ".join(
            (
                _session_expr(payload["session"]),
                _connection_expr(payload["connection"]),
                _u64(payload["at_monotonic_usec"]),
                _utc(payload["at_utc_usec"]),
                _dimensions_expr(payload["dimensions"]),
                _u64(payload["config_id"]),
                f"{payload['classifier_version']}U",
                f"{payload['policy_version']}U",
                _quality(payload["quality_flags"]),
            )
        )
        + ")"
    )


def _evidence_expr(session: dict[str, Any], connection: dict[str, Any],
                   monotonic: int, utc: int, kind: str) -> str:
    return (
        "make_evidence("
        + ", ".join(
            (
                _session_expr(session),
                _connection_expr(connection),
                _u64(monotonic),
                _utc(utc),
                f"telemetry_activity_evidence_kind::{kind}",
            )
        )
        + ")"
    )


def _context_expr(payload: dict[str, Any], monotonic: int, utc: int) -> str:
    return (
        "make_context("
        + ", ".join(
            (
                _session_expr(payload["session"]),
                _connection_expr(payload["connection"]),
                _u64(monotonic),
                _utc(utc),
                _dimensions_expr(payload["dimensions"]),
                _context_flag(payload["context"]),
                _u64(payload["config_id"]),
                f"{payload['classifier_version']}U",
                f"{payload['policy_version']}U",
                _quality(payload["quality_flags"]),
            )
        )
        + ")"
    )


def _transition_expr(payload: dict[str, Any]) -> str:
    return (
        "make_transition("
        + ", ".join(
            (
                _session_expr(payload["session"]),
                _connection_expr(payload["connection"]),
                _u64(payload["at_monotonic_usec"]),
                _utc(payload["at_utc_usec"]),
                f"telemetry_connection_transition_kind::{payload['kind']}",
                _quality(payload["quality_flags"]),
            )
        )
        + ")"
    )


def _expected_interval_expr(payload: dict[str, Any]) -> str:
    window = payload["window"]
    return (
        "make_expected_interval("
        + ", ".join(
            (
                _session_expr(payload["session"]),
                _connection_expr(payload["connection"]),
                "telemetry_time_window{"
                + ", ".join(
                    (
                        _u64(window["start_monotonic_usec"]),
                        _u64(window["end_monotonic_usec"]),
                        _utc(window["start_utc_usec"]),
                        _utc(window["end_utc_usec"]),
                    )
                )
                + "}",
                _u64(payload["duration_usec"]),
                _category_enum(payload["category"]),
                _context_enum(payload["context"]),
                _context_quality_enum(payload["context_quality"]),
                _dimensions_expr(payload["dimensions"]),
                _u64(payload["config_id"]),
                f"{payload['classifier_version']}U",
                f"{payload['policy_version']}U",
                _quality(payload["quality_flags"]),
            )
        )
        + ")"
    )


def _cumulative_expr(metrics: dict[str, Any]) -> str:
    conservation = metrics["conservation"]
    return (
        "make_cumulative("
        + ", ".join(
            _u64(conservation[key])
            for key in (
                "connected_usec",
                "active_usec",
                "idle_usec",
                "unknown_usec",
                "resident_usec",
                "linkdead_usec",
            )
        )
        + ")"
    )


def _golden_fact_expr(fixture: dict[str, Any]) -> str:
    enter = _lifecycle_payload(fixture)
    intervals = _interval_payloads(fixture)
    first = intervals[0]
    session = enter["session"]
    transitions = fixture.get("transitions", [])
    is_reconnect = fixture["fixture_id"] == "detach_reconnect"
    evidence = [
        _evidence_expr(session, first["connection"], enter["at_monotonic_usec"],
                       enter["at_utc_usec"], "player_action")
    ]
    contexts = [_context_expr(first, enter["at_monotonic_usec"], enter["at_utc_usec"])]
    if is_reconnect:
        last = intervals[-1]
        evidence.append(
            _evidence_expr(session, last["connection"], last["window"]["start_monotonic_usec"],
                           last["window"]["start_utc_usec"], "afk")
        )
        contexts.append(
            _context_expr(last, last["window"]["start_monotonic_usec"],
                          last["window"]["start_utc_usec"])
        )
    else:
        evidence.append("{}")
        contexts.append("{}")
    evidence_text = "{ " + ", ".join(evidence) + " }"
    context_text = "{ " + ", ".join(contexts) + " }"
    transition_text = "{ " + ", ".join(
        [_transition_expr(item) for item in transitions] + ["{}"] * (2 - len(transitions))
    ) + " }"
    pulses = [item["window"]["end_monotonic_usec"] for item in intervals]
    pulse_utc = [item["window"]["end_utc_usec"] for item in intervals]
    pulse_text = "{ " + ", ".join(_u64(value) for value in pulses + [0] * (3 - len(pulses))) + " }"
    pulse_utc_text = "{ " + ", ".join(
        _utc(value) for value in pulse_utc + [-(1 << 63)] * (3 - len(pulse_utc))
    ) + " }"
    interval_text = "{ " + ", ".join(_expected_interval_expr(item) for item in intervals) + " }"
    final_connection = intervals[-1]["connection"]
    return "{ " + ", ".join(
        (
            json.dumps(fixture["fixture_id"]),
            _enter_expr(enter),
            evidence_text,
            context_text,
            transition_text,
            pulse_text,
            pulse_utc_text,
            f"{len(intervals)}U",
            interval_text,
            f"{len(intervals)}U",
            _cumulative_expr(fixture["expected"]["metrics"]),
            _connection_expr(final_connection),
        )
    ) + " }"


def render_golden_fixture_include() -> str:
    rows = []
    for name in B_GOLDEN_FIXTURES:
        rows.append(_golden_fact_expr(json.loads((FIXTURES / name).read_text(encoding="utf-8"))))
    return (
        "/* Generated from the existing B contract golden fixtures. */\n"
        "static const activity_b_golden_fact ACTIVITY_B_GOLDEN_FACTS[] = {\n"
        + ",\n".join("\t" + row for row in rows)
        + "\n};\n"
        "static constexpr std::size_t ACTIVITY_B_GOLDEN_FACT_COUNT =\n"
        "\tsizeof(ACTIVITY_B_GOLDEN_FACTS) / sizeof(ACTIVITY_B_GOLDEN_FACTS[0]);\n"
    )


def check_source_boundary() -> None:
    source = ACTIVITY_SOURCE.read_text(encoding="utf-8")
    header = ACTIVITY_HEADER.read_text(encoding="utf-8")
    private = PRIVATE_HEADER.read_text(encoding="utf-8")
    assert "telemetry_activity_state_init" in source
    assert "telemetry_activity_state_pulse" in source
    assert "telemetry_activity_private.h" in source
    assert "telemetry_activity_state" in header
    assert "slots[TELEMETRY_ACTIVITY_STATE_MAX_SLOTS]" in header
    assert "telemetry_activity_record_key_allocator" in header
    assert "MYSQL" not in source
    assert "std::vector" not in source
    assert "std::thread" not in source
    assert "malloc(" not in source
    assert "calloc(" not in source
    assert "realloc(" not in source
    assert "operator new" not in source
    assert "new telemetry" not in source
    assert "std::string" not in source
    assert "TELEMETRY_ACTIVITY_MAX_PIECES_PER_OPERATION" in source
    assert "CONFIGURED_WINDOW_MAX_USEC" in private


def check_b_golden_facts() -> None:
    for name in B_GOLDEN_FIXTURES:
        assert (FIXTURES / name).is_file(), name
    normal = json.loads((FIXTURES / "normal_interval.json").read_text(encoding="utf-8"))
    assert normal["records"][1]["payload"]["interval"]["category"] == "connected_active"
    assert normal["records"][1]["payload"]["interval"]["context"] == "combat"
    assert normal["expected"]["metrics"]["conservation"] == {
        "interval_total_usec": 100,
        "connected_usec": 100,
        "active_usec": 100,
        "idle_usec": 0,
        "unknown_usec": 0,
        "resident_usec": 100,
        "linkdead_usec": 0,
        "connected_identity_holds": True,
        "resident_identity_holds": True,
    }
    reconnect = json.loads((FIXTURES / "detach_reconnect.json").read_text(encoding="utf-8"))
    assert reconnect["expected"]["metrics"]["conservation"] == {
        "interval_total_usec": 250,
        "connected_usec": 150,
        "active_usec": 100,
        "idle_usec": 50,
        "unknown_usec": 0,
        "resident_usec": 250,
        "linkdead_usec": 100,
        "connected_identity_holds": True,
        "resident_identity_holds": True,
    }
    assert [item["kind"] for item in reconnect["transitions"]] == ["detached", "attached"]


def _run(command: list[str], label: str) -> None:
    compiled = subprocess.run(command, cwd=ROOT, text=True, capture_output=True)
    if compiled.returncode != 0:
        sys.stderr.write(f"[{label}] compile failed: {' '.join(command)}\n")
        sys.stderr.write(compiled.stdout)
        sys.stderr.write(compiled.stderr)
        raise SystemExit(compiled.returncode)
    ran = subprocess.run([command[-1]], cwd=ROOT, text=True, capture_output=True)
    if ran.returncode != 0:
        sys.stderr.write(f"[{label}] harness failed with {ran.returncode}\n")
        sys.stderr.write(ran.stdout)
        sys.stderr.write(ran.stderr)
        raise SystemExit(ran.returncode)
    expected = {
        "state-sql": "telemetry activity state harness passed",
        "state-no-mysql": "telemetry activity state harness passed",
        "golden-sql": "telemetry activity B golden harness passed",
        "golden-no-mysql": "telemetry activity B golden harness passed",
        "integration-sql": "telemetry activity/session integration harness passed",
        "integration-no-mysql": "telemetry activity/session integration harness passed",
    }[label]
    assert ran.stdout.strip() == expected, ran.stdout


def compile_state(flags: list[str], label: str) -> None:
    compiler = shlex.split(os.environ.get("CXX", "g++"))
    with tempfile.TemporaryDirectory(prefix=f"telemetry-activity-{label}-",
                                      dir=ROOT / "bin" / "tests") as directory:
        binary = str(Path(directory) / "telemetry_activity_state")
        _run(
            compiler
            + [
                "-std=c++20", "-Wall", "-Wextra", "-Werror", "-pedantic",
                "-fno-exceptions", "-fno-rtti", *flags, "-I", str(SRC),
                str(ACTIVITY_SOURCE), str(STATE_HARNESS), "-o", binary,
            ],
            label,
        )


def compile_golden(flags: list[str], label: str) -> None:
    compiler = shlex.split(os.environ.get("CXX", "g++"))
    with tempfile.TemporaryDirectory(prefix=f"telemetry-activity-{label}-",
                                      dir=ROOT / "bin" / "tests") as directory:
        temporary_root = Path(directory)
        include = temporary_root / "telemetry_activity_b_golden_fixture.inc"
        include.write_text(render_golden_fixture_include(), encoding="utf-8")
        binary = str(temporary_root / "telemetry_activity_b_golden")
        _run(
            compiler
            + [
                "-std=c++20", "-Wall", "-Wextra", "-Werror", "-pedantic",
                "-fno-exceptions", "-fno-rtti", *flags, "-I", str(SRC),
                "-I", str(temporary_root), str(ACTIVITY_SOURCE),
                str(GOLDEN_HARNESS), "-o", binary,
            ],
            label,
        )


def compile_integration(flags: list[str], label: str) -> None:
    compiler = shlex.split(os.environ.get("CXX", "g++"))
    with tempfile.TemporaryDirectory(prefix=f"telemetry-activity-{label}-",
                                      dir=ROOT / "bin" / "tests") as directory:
        binary = str(Path(directory) / "telemetry_activity_session_integration")
        _run(
            compiler
            + [
                "-std=c++20", "-Wall", "-Wextra", "-Werror", "-pedantic",
                "-fno-exceptions", "-fno-rtti", *flags, "-I", str(SRC),
                str(ACTIVITY_SOURCE), str(SESSION_SOURCE),
                str(INTEGRATION_HARNESS), "-o", binary,
            ],
            label,
        )


def main() -> int:
    (ROOT / "bin" / "tests").mkdir(parents=True, exist_ok=True)
    check_source_boundary()
    check_b_golden_facts()
    compile_state([], "state-sql")
    compile_state(["-D__NO_MYSQL__"], "state-no-mysql")
    compile_golden([], "golden-sql")
    compile_golden(["-D__NO_MYSQL__"], "golden-no-mysql")
    compile_integration([], "integration-sql")
    compile_integration(["-D__NO_MYSQL__"], "integration-no-mysql")
    print("telemetry activity: SQL, __NO_MYSQL__, B golden, and real session integration harnesses passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
