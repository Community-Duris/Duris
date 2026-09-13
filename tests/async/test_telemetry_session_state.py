#!/usr/bin/env python3
"""Focused #264 bounded session-state harness and contract checks."""

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
SESSION_SOURCE = SRC / "telemetry" / "telemetry_session.c"
SESSION_HEADER = SRC / "telemetry" / "telemetry_session.h"
HARNESS = ROOT / "tests" / "async" / "telemetry_session_state_harness.cc"
GOLDEN_HARNESS = ROOT / "tests" / "async" / "telemetry_session_state_golden_harness.cc"
FIXTURES = ROOT / "tests" / "async" / "fixtures" / "telemetry" / "contract"

GOLDEN_FIXTURES = (
    "normal_interval.json",
    "detach_reconnect.json",
    "copyover_handoff.json",
)

EXPECTED_FIXTURES = {
    "checkpoint_revision_conflict.json",
    "copyover_handoff.json",
    "detach_reconnect.json",
    "drop_recovery_no_invented_context.json",
    "unclosed_crash_tail.json",
}


def check_source_boundary() -> None:
    source = SESSION_SOURCE.read_text(encoding="utf-8")
    header = SESSION_HEADER.read_text(encoding="utf-8")
    assert "telemetry_session_state" in source
    assert "telemetry_session_state_init" in header
    assert "telemetry_runtime_init(" not in source
    assert "telemetry_runtime_session_enter(" not in source
    assert "telemetry_runtime_connection_transition(" not in source
    assert "telemetry_runtime_update_counters(" not in source
    assert "MYSQL" not in source
    assert "std::vector" not in source
    assert "std::thread" not in source
    assert "malloc(" not in source
    assert "calloc(" not in source
    assert "realloc(" not in source
    assert "new telemetry" not in source
    assert "slots[TELEMETRY_SESSION_STATE_MAX_SLOTS]" in header
    assert "telemetry_session_record_key_allocator" in header
    assert "telemetry_record_sequence" not in source


def check_golden_facts() -> None:
    assert {path.name for path in FIXTURES.glob("*.json")} == EXPECTED_FIXTURES | {
        "commit_ambiguity_retry.json",
        "level_zone_config_boundary.json",
        "midnight_clock_jump.json",
        "normal_interval.json",
        "replay_identical_conflict.json",
    }
    reconnect = json.loads((FIXTURES / "detach_reconnect.json").read_text())
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
    copyover = json.loads((FIXTURES / "copyover_handoff.json").read_text())
    copy_metrics = copyover["expected"]["metrics"]["copyover"]
    assert copy_metrics["session_retained"] is True
    assert copy_metrics["producer_count"] == 2
    assert copy_metrics["downtime_invented_usec"] == 0
    assert copy_metrics["cross_process_duration_usec"] == 0
    assert copy_metrics["handoff_checkpoint_revision"] == 1
    assert copy_metrics["post_copyover_checkpoint_revision"] == 2
    drop = json.loads((FIXTURES / "drop_recovery_no_invented_context.json").read_text())
    coverage = drop["expected"]["metrics"]["coverage"]
    assert coverage["recovered_connected_usec"] == 120
    assert coverage["invented_context_usec"] == 0
    assert coverage["coverage_incomplete"] is True
    checkpoint = json.loads((FIXTURES / "checkpoint_revision_conflict.json").read_text())
    checkpoint_metrics = checkpoint["expected"]["metrics"]["checkpoint"]
    assert checkpoint_metrics["latest_revision"] == 3
    assert checkpoint_metrics["stale_outcome"] == "checkpoint_older"
    assert checkpoint_metrics["conflict_outcome"] == "duplicate_conflict"
    tail = json.loads((FIXTURES / "unclosed_crash_tail.json").read_text())
    tail_metrics = tail["expected"]["metrics"]["tail"]
    assert tail_metrics["claimed_tail_usec"] == 0
    assert tail_metrics["complete"] is False
    assert tail_metrics["unclosed_quality_flag"] is True


def _u64(value: int) -> str:
    return f"{value}ULL"


def _utc(value: int) -> str:
    return f"static_cast<telemetry_utc_usec>({value})"


def _quality(value: int) -> str:
    return f"{value}U"


def _producer_args(producer: dict[str, int]) -> str:
    return f"{_u64(producer['boot_id'])}, {_u64(producer['process_id'])}"


def _producer_expr(producer: dict[str, int]) -> str:
    return f"make_producer({_producer_args(producer)})"


def _session_args(session: dict[str, Any]) -> str:
    session_id = session["id"]
    assert isinstance(session_id, dict)
    producer = session_id["producer"]
    assert isinstance(producer, dict)
    return ", ".join(
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


def _session_expr(session: dict[str, Any]) -> str:
    return f"make_session({_session_args(session)})"


def _connection_args(connection: dict[str, Any]) -> str:
    producer = connection["producer"]
    assert isinstance(producer, dict)
    return ", ".join(
        (
            _u64(producer["boot_id"]),
            _u64(producer["process_id"]),
            _u64(connection["connection_seq"]),
        )
    )


def _connection_expr(connection: dict[str, Any]) -> str:
    return f"make_connection({_connection_args(connection)})"


def _enter_expr(payload: dict[str, Any]) -> str:
    return (
        "make_enter("
        + ", ".join(
            (
                _session_args(payload["session"]),
                _connection_args(payload["connection"]),
                _u64(payload["at_monotonic_usec"]),
                _utc(payload["at_utc_usec"]),
                str(payload["dimensions"]["level_band"]),
                str(payload["dimensions"]["class_id"]),
                str(payload["dimensions"]["race_id"]),
                str(payload["dimensions"]["faction_id"]),
                str(payload["dimensions"]["zone_vnum"]),
                str(payload["dimensions"]["group_size"]),
                _u64(payload["config_id"]),
                f"{payload['classifier_version']}U",
                f"{payload['policy_version']}U",
                _quality(payload["quality_flags"]),
            )
        )
        + ")"
    )


def _counter_expr(payload: dict[str, Any]) -> str:
    duration = payload["duration_usec"]
    category = payload["category"]
    if category == "connected_active":
        connected, active, idle, unknown, resident, linkdead = (duration, duration, 0, 0, duration, 0)
    elif category == "connected_idle":
        connected, active, idle, unknown, resident, linkdead = (duration, 0, duration, 0, duration, 0)
    elif category == "resident_linkdead":
        connected, active, idle, unknown, resident, linkdead = (0, 0, 0, 0, duration, duration)
    else:
        raise AssertionError(f"unsupported direct golden interval category: {category}")
    window = payload["window"]
    connection = payload["connection"]
    return (
        "make_counter("
        + ", ".join(
            (
                _session_args(payload["session"]),
                _connection_args(connection),
                _u64(window["end_monotonic_usec"]),
                _u64(connected),
                _u64(active),
                _u64(idle),
                _u64(unknown),
                _u64(resident),
                _u64(linkdead),
                _quality(payload["quality_flags"]),
            )
        )
        + ")"
    )


TRANSITION_ENUMS = {
    "attached": "attached",
    "detached": "detached",
    "copyover_resumed": "copyover_resumed",
}


def _transition_expr(payload: dict[str, Any]) -> str:
    return (
        "make_transition("
        + ", ".join(
            (
                _session_args(payload["session"]),
                _connection_args(payload["connection"]),
                _u64(payload["at_monotonic_usec"]),
                _utc(payload["at_utc_usec"]),
                f"telemetry_connection_transition_kind::{TRANSITION_ENUMS[payload['kind']]}",
                _quality(payload["quality_flags"]),
            )
        )
        + ")"
    )


def _operation_record(fixture: dict[str, Any], name: str) -> dict[str, Any]:
    for operation in fixture["operations"]:
        if operation["name"] == name:
            indexes = operation["record_indexes"]
            assert len(indexes) == 1, f"{fixture['fixture_id']} operation {name} is not singular"
            return fixture["records"][indexes[0]]
    raise AssertionError(f"{fixture['fixture_id']} is missing operation {name}")


def _transition(fixture: dict[str, Any], kind: str) -> dict[str, Any]:
    matches = [item for item in fixture.get("transitions", []) if item["kind"] == kind]
    assert len(matches) == 1, f"{fixture['fixture_id']} must contain one {kind} transition"
    return matches[0]


def _lifecycle_payload(record: dict[str, Any]) -> dict[str, Any]:
    payload = record["payload"]["session_lifecycle"]
    assert payload["lifecycle"] in {"session_entered", "connection_attached"}
    return payload


def _interval_payload(record: dict[str, Any]) -> dict[str, Any]:
    payload = record["payload"]["interval"]
    assert payload["category"] in {"connected_active", "connected_idle", "resident_linkdead"}
    return payload


def _checkpoint_payload(record: dict[str, Any]) -> dict[str, Any]:
    return record["payload"]["session_checkpoint"]


def _golden_calls(fixture: dict[str, Any]) -> tuple[list[str], dict[str, Any]]:
    calls: list[str] = []
    last_interval: dict[str, Any] | None = None
    for operation in fixture["operations"]:
        name = operation["name"]
        record = _operation_record(fixture, name)
        if name in {"enter", "old_session_enter"}:
            calls.append(f"make_enter_call(0U, {_enter_expr(_lifecycle_payload(record))})")
        elif name in {
            "seal_interval",
            "connected_interval",
            "linkdead_interval",
            "reconnected_interval",
            "old_interval",
            "new_process_interval",
        }:
            process = 1 if name == "new_process_interval" else 0
            last_interval = _interval_payload(record)
            calls.append(f"make_counter_call({process}U, {_counter_expr(last_interval)})")
        elif name == "detach":
            transition = _transition(fixture, "detached")
            calls.append(f"make_transition_call(0U, {_transition_expr(transition)})")
        elif name == "reconnect":
            transition = _transition(fixture, "attached")
            calls.append(f"make_transition_call(0U, {_transition_expr(transition)})")
        elif name in {"old_checkpoint", "post_copyover_checkpoint"}:
            checkpoint = _checkpoint_payload(record)
            calls.append(
                "make_checkpoint_call("
                f"{1 if name == 'post_copyover_checkpoint' else 0}U, "
                f"{_session_expr(checkpoint['session'])}, "
                f"{_u64(checkpoint['at_monotonic_usec'])}, {_utc(checkpoint['at_utc_usec'])})"
            )
        elif name == "copyover_resume":
            resume_payload = _lifecycle_payload(record)
            copyover_transition = _transition(fixture, "copyover_resumed")
            assert resume_payload["session"] == copyover_transition["session"]
            assert resume_payload["connection"] == copyover_transition["connection"]
            assert resume_payload["at_monotonic_usec"] == copyover_transition["at_monotonic_usec"]
            assert resume_payload["at_utc_usec"] == copyover_transition["at_utc_usec"]
            old_checkpoint = _checkpoint_payload(_operation_record(fixture, "old_checkpoint"))
            calls.append(
                "make_handoff_call("
                f"0U, {_session_expr(old_checkpoint['session'])}, "
                f"{_u64(old_checkpoint['at_monotonic_usec'])}, {_utc(old_checkpoint['at_utc_usec'])})"
            )
            calls.append(f"make_resume_call(1U, {_enter_expr(resume_payload)})")
        else:
            raise AssertionError(f"unsupported direct golden operation: {fixture['fixture_id']}:{name}")

    assert last_interval is not None or fixture["fixture_id"] == "copyover_handoff"
    if fixture["fixture_id"] in {"normal_interval", "detach_reconnect"}:
        assert last_interval is not None
        calls.append(
            "make_checkpoint_call(0U, "
            f"{_session_expr(last_interval['session'])}, "
            f"{_u64(last_interval['window']['end_monotonic_usec'])}, "
            f"{_utc(last_interval['window']['end_utc_usec'])})"
        )

    metrics = fixture["expected"]["metrics"]
    conservation = metrics["conservation"]
    cumulative = {
        key: conservation[key]
        for key in (
            "connected_usec",
            "active_usec",
            "idle_usec",
            "unknown_usec",
            "resident_usec",
            "linkdead_usec",
        )
    }
    first_enter = _lifecycle_payload(_operation_record(
        fixture, "old_session_enter" if fixture["fixture_id"] == "copyover_handoff" else "enter"
    ))
    if fixture["fixture_id"] == "copyover_handoff":
        post_checkpoint = _checkpoint_payload(_operation_record(fixture, "post_copyover_checkpoint"))
        copyover = metrics["copyover"]
        assert cumulative == copyover["post_copyover_checkpoint_cumulative"]
        final_connection = post_checkpoint["connection"]
        new_producer = copyover["new_producer"]
        handoff_cumulative = copyover["handoff_checkpoint_cumulative"]
        handoff_revision = copyover["handoff_checkpoint_revision"]
        final_revision = copyover["post_copyover_checkpoint_revision"]
        has_handoff = 1
    else:
        final_connection = last_interval["connection"]
        new_producer = {"boot_id": 0, "process_id": 0}
        handoff_cumulative = {
            "connected_usec": 0,
            "active_usec": 0,
            "idle_usec": 0,
            "unknown_usec": 0,
            "resident_usec": 0,
            "linkdead_usec": 0,
        }
        handoff_revision = 0
        final_revision = 1
        has_handoff = 0
    expected = {
        "session": first_enter["session"],
        "final_connection": final_connection,
        "cumulative": cumulative,
        "old_producer": first_enter["session"]["id"]["producer"],
        "new_producer": new_producer,
        "handoff_cumulative": handoff_cumulative,
        "handoff_revision": handoff_revision,
        "final_revision": final_revision,
        "has_handoff": has_handoff,
    }
    return calls, expected


def _cumulative_expr(cumulative: dict[str, int]) -> str:
    return (
        "make_cumulative("
        + ", ".join(_u64(cumulative[key]) for key in (
            "connected_usec",
            "active_usec",
            "idle_usec",
            "unknown_usec",
            "resident_usec",
            "linkdead_usec",
        ))
        + ")"
    )


def _expected_expr(expected: dict[str, Any]) -> str:
    return "{" + ", ".join(
        (
            _session_expr(expected["session"]),
            _connection_expr(expected["final_connection"]),
            _cumulative_expr(expected["cumulative"]),
            _producer_expr(expected["old_producer"]),
            _producer_expr(expected["new_producer"]),
            _cumulative_expr(expected["handoff_cumulative"]),
            _u64(expected["handoff_revision"]),
            _u64(expected["final_revision"]),
            f"{expected['has_handoff']}U",
        )
    ) + "}"


def render_golden_fixture_include() -> str:
    lines = [
        "/* Generated in a bin/tests temporary directory from the JSON contract fixtures. */",
    ]
    fixture_rows: list[str] = []
    for fixture_name in GOLDEN_FIXTURES:
        fixture = json.loads((FIXTURES / fixture_name).read_text(encoding="utf-8"))
        calls, expected = _golden_calls(fixture)
        symbol = fixture["fixture_id"].upper()
        lines.append(f"static const golden_call {symbol}_CALLS[] = {{")
        lines.extend(f"\t{{ {call} }}," for call in calls)
        lines.append("};")
        fixture_rows.append(
            "\t{ "
            f"{json.dumps(fixture['fixture_id'])}, {symbol}_CALLS, "
            f"sizeof({symbol}_CALLS) / sizeof({symbol}_CALLS[0]), {_expected_expr(expected)} "
            "},"
        )
    lines.extend(
        (
            "static const golden_fixture GOLDEN_FIXTURES[] = {",
            *fixture_rows,
            "};",
            "static constexpr std::size_t GOLDEN_FIXTURE_COUNT =",
            "\tsizeof(GOLDEN_FIXTURES) / sizeof(GOLDEN_FIXTURES[0]);",
        )
    )
    return "\n".join(lines) + "\n"


def compile_and_run(flags: list[str], label: str) -> None:
    compiler = shlex.split(os.environ.get("CXX", "g++"))
    artifact_root = ROOT / "bin" / "tests"
    artifact_root.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix=f"telemetry-session-{label}-", dir=artifact_root) as directory:
        binary = Path(directory) / "telemetry_session_state"
        command = compiler + [
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-pedantic",
            "-fno-exceptions",
            "-fno-rtti",
            *flags,
            "-I",
            str(SRC),
            str(SESSION_SOURCE),
            str(HARNESS),
            "-o",
            str(binary),
        ]
        compiled = subprocess.run(command, cwd=ROOT, text=True, capture_output=True)
        if compiled.returncode != 0:
            sys.stderr.write(f"[{label}] compile failed: {' '.join(command)}\n")
            sys.stderr.write(compiled.stdout)
            sys.stderr.write(compiled.stderr)
            raise SystemExit(compiled.returncode)
        ran = subprocess.run([str(binary)], cwd=ROOT, text=True, capture_output=True)
        if ran.returncode != 0:
            sys.stderr.write(f"[{label}] harness failed with {ran.returncode}\n")
            sys.stderr.write(ran.stdout)
            sys.stderr.write(ran.stderr)
            raise SystemExit(ran.returncode)
        assert ran.stdout.strip() == "telemetry session state harness passed", ran.stdout


def compile_golden_and_run(flags: list[str]) -> None:
    compiler = shlex.split(os.environ.get("CXX", "g++"))
    artifact_root = ROOT / "bin" / "tests"
    artifact_root.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="telemetry-session-golden-", dir=artifact_root) as directory:
        temporary_root = Path(directory)
        fixture_include = temporary_root / "telemetry_session_state_golden_fixture.inc"
        fixture_include.write_text(render_golden_fixture_include(), encoding="utf-8")
        binary = temporary_root / "telemetry_session_state_golden"
        command = compiler + [
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-pedantic",
            "-fno-exceptions",
            "-fno-rtti",
            "-I",
            str(SRC),
            "-I",
            str(temporary_root),
            *flags,
            str(SESSION_SOURCE),
            str(GOLDEN_HARNESS),
            "-o",
            str(binary),
        ]
        compiled = subprocess.run(command, cwd=ROOT, text=True, capture_output=True)
        if compiled.returncode != 0:
            sys.stderr.write(f"[golden] compile failed: {' '.join(command)}\n")
            sys.stderr.write(compiled.stdout)
            sys.stderr.write(compiled.stderr)
            raise SystemExit(compiled.returncode)
        ran = subprocess.run([str(binary)], cwd=ROOT, text=True, capture_output=True)
        if ran.returncode != 0:
            sys.stderr.write(f"[golden] harness failed with {ran.returncode}\n")
            sys.stderr.write(ran.stdout)
            sys.stderr.write(ran.stderr)
            raise SystemExit(ran.returncode)
        assert ran.stdout.strip() == "telemetry session state golden harness passed", ran.stdout


def main() -> int:
    check_source_boundary()
    check_golden_facts()
    compile_and_run([], "sql")
    compile_and_run(["-D__NO_MYSQL__"], "no-mysql")
    compile_golden_and_run([])
    compile_golden_and_run(["-D__NO_MYSQL__"])
    print("telemetry session state: SQL, __NO_MYSQL__, and B golden harnesses passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
