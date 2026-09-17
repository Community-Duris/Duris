#!/usr/bin/env python3
"""Run the offline telemetry performance/degradation gate for issue #272.

The gate deliberately uses only synthetic rows and an injected in-memory
repository.  It does not connect to SQL, start the game server, write a
database migration, or generate production traffic.  The output is a
sanitized JSON report suitable for attaching to the issue/PR.
"""
from __future__ import annotations

import argparse
from copy import deepcopy
from dataclasses import dataclass
import gc
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys
import tempfile
import time
import tracemalloc
from typing import Any, Callable, Sequence


ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"
CAPACITY_HARNESS = ROOT / "tests/async/telemetry_capacity_272_harness.cc"
FAULT_HARNESS = ROOT / "tests/async/telemetry_fault_272_harness.cc"
TRANSPORT_SOURCES = (
    SRC / "telemetry/telemetry_queue.c",
    SRC / "telemetry/telemetry_transport.c",
)
DEFAULT_WORKLOADS = (50, 200, 1000)
DEFAULT_REPETITIONS = 5
FAULT_CASES = (
    "queue_saturation",
    "repository_recovery",
    "ambiguous_commit",
    "slow_write",
    "copyover_shutdown",
    "churn",
)


class GateError(RuntimeError):
    """A reproducibility or evidence failure."""


@dataclass(frozen=True)
class Toolchain:
    """Compiler/runner pair that works from native Windows or WSL."""

    compiler: tuple[str, ...]
    use_wsl: bool

    @staticmethod
    def discover() -> "Toolchain":
        configured = os.environ.get("CXX")
        if configured:
            return Toolchain(tuple(shlex.split(configured)), False)
        if shutil.which("g++"):
            return Toolchain(("g++",), False)
        if shutil.which("wsl.exe"):
            return Toolchain(("g++",), True)
        raise GateError("g++ is not available and no wsl.exe fallback was found")

    @staticmethod
    def _wsl_path(path: Path) -> str:
        resolved = path.resolve()
        value = resolved.as_posix()
        if len(value) >= 2 and value[1] == ":":
            return f"/mnt/{value[0].lower()}{value[2:]}"
        return value

    def _wsl_shell(self, command: Sequence[str]) -> list[str]:
        return [
            "wsl.exe",
            "-e",
            "bash",
            "-lc",
            " ".join(shlex.quote(item) for item in command),
        ]

    def compile(self, output: Path, source: Path) -> None:
        command = [
            *self.compiler,
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-pedantic",
            "-pthread",
            "-I",
            str(SRC),
            "-O2",
            *(str(item) for item in TRANSPORT_SOURCES),
            str(source),
            "-o",
            str(output),
        ]
        if self.use_wsl:
            wsl_command = [
                *self.compiler,
                "-std=c++20",
                "-Wall",
                "-Wextra",
                "-Werror",
                "-pedantic",
                "-pthread",
                "-I",
                self._wsl_path(SRC),
                "-O2",
                *(self._wsl_path(item) for item in TRANSPORT_SOURCES),
                self._wsl_path(source),
                "-o",
                self._wsl_path(output),
            ]
            command = self._wsl_shell(wsl_command)
        run(command, timeout=120)

    def execute(self, executable: Path, arguments: Sequence[str]) -> str:
        command = [str(executable), *(str(item) for item in arguments)]
        if self.use_wsl:
            command = self._wsl_shell([self._wsl_path(executable), *arguments])
        return run(command, timeout=120).stdout


def run(command: Sequence[str], *, timeout: int) -> subprocess.CompletedProcess[str]:
    try:
        result = subprocess.run(
            list(command),
            cwd=ROOT,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            timeout=timeout,
            check=False,
        )
    except subprocess.TimeoutExpired as error:
        raise GateError(f"command timed out after {timeout}s: {' '.join(command)}") from error
    if result.returncode != 0:
        raise GateError(
            f"command failed with exit {result.returncode}: {' '.join(command)}\n{result.stdout}"
        )
    return result


def parse_json_line(output: str) -> dict[str, Any]:
    for line in reversed(output.splitlines()):
        line = line.strip()
        if line.startswith("{") and line.endswith("}"):
            value = json.loads(line)
            if not isinstance(value, dict):
                break
            return value
    raise GateError(f"harness did not produce a JSON object:\n{output}")


def source_contract() -> dict[str, Any]:
    transport = (SRC / "telemetry/telemetry_transport.c").read_text(encoding="utf-8")
    queue = (SRC / "telemetry/telemetry_queue.c").read_text(encoding="utf-8")
    queue_private = (SRC / "telemetry/telemetry_queue_private.h").read_text(encoding="utf-8")
    combined = transport + queue
    forbidden = (
        "std::mutex",
        "std::thread",
        "std::condition_variable",
        "std::vector",
        "malloc(",
        "calloc(",
        "realloc(",
        "new ",
        "fopen(",
        "mysql_",
    )
    for marker in forbidden:
        if marker in combined:
            raise GateError(f"producer/transport source contains forbidden marker {marker}")
    for marker in (
        "telemetry_transport_bind_for_tests",
        "telemetry_transport_quiesce_for_tests",
        "telemetry_transport_resume_for_tests",
        "telemetry_transport_clock_binding",
    ):
        if marker not in (transport + queue_private + (SRC / "telemetry/telemetry_transport_private.h").read_text(encoding="utf-8")):
            raise GateError(f"transport test seam missing: {marker}")
    return {
        "producer_heap_or_sql_calls": False,
        "fixed_queue_storage": True,
        "queue_private_atomic_spsc": "std::atomic" in queue_private,
        "queue_private_producer_head": "producer_head" in queue_private,
        "queue_private_consumer_tail": "consumer_tail" in queue_private,
    }


def percentile(samples: Sequence[int], percent: int) -> int:
    if not samples:
        return 0
    ordered = sorted(samples)
    rank = (len(ordered) * percent + 99) // 100
    return ordered[min(len(ordered) - 1, max(0, rank - 1))]


def distribution(samples: Sequence[int]) -> dict[str, int]:
    return {
        "sample_count": len(samples),
        "p50_ns": percentile(samples, 50),
        "p95_ns": percentile(samples, 95),
        "p99_ns": percentile(samples, 99),
        "p999_ns": percentile(samples, 999),
    }


def capacity_measurements(
    toolchain: Toolchain, executable: Path, workload: int, repetitions: int
) -> dict[str, dict[str, Any]]:
    output: dict[str, dict[str, Any]] = {}
    for mode in ("off", "capture_only", "capture_write"):
        value = parse_json_line(toolchain.execute(executable, (mode, str(workload), str(repetitions))))
        if value.get("schema_version") != 1 or value.get("mode") != mode:
            raise GateError(f"unexpected capacity result for {mode}: {value}")
        output[mode] = value
    return output


def synthetic_rollup_rows(count: int) -> list[dict[str, Any]]:
    fixture_path = ROOT / "tests/async/fixtures/telemetry/contract/normal_interval.json"
    sys.path.insert(0, str(ROOT / "tests/async"))
    from telemetry_rollup_fixtures import FIXTURE_DIR, golden_rows  # type: ignore

    del fixture_path
    _, base_rows = golden_rows(FIXTURE_DIR / "normal_interval.json")
    interval = next(row for row in base_rows if row["record_kind"] == 1)
    rows: list[dict[str, Any]] = []
    for index in range(1, count + 1):
        row = deepcopy(interval)
        row.update(
            boot_id=101,
            process_id=201,
            record_seq=index,
            ingest_id=index,
            session_boot_id=101,
            session_process_id=201,
            session_seq=index,
            connection_boot_id=101,
            connection_process_id=201,
            connection_seq=index,
            subject_id=9000 + index,
            pid=40 + index,
            start_monotonic_usec=index * 1_000,
            end_monotonic_usec=index * 1_000 + 100,
            duration_usec=100,
            start_utc_usec=index * 1_000_000,
            end_utc_usec=index * 1_000_000 + 100,
            occurrence_utc_usec=index * 1_000_000 + 100,
            ingested_utc_usec=index * 1_000_000 + 100,
        )
        rows.append(row)
    return rows


def python_stage_measure(
    function: Callable[[], Any], repetitions: int
) -> dict[str, Any]:
    samples: list[int] = []
    cpu_samples: list[int] = []
    peak_alloc = 0
    gc.collect()
    tracemalloc.start()
    total_started = time.perf_counter_ns()
    total_cpu_started = time.process_time_ns()
    for _ in range(repetitions):
        tracemalloc.reset_peak()
        started = time.perf_counter_ns()
        cpu_started = time.process_time_ns()
        function()
        cpu_finished = time.process_time_ns()
        finished = time.perf_counter_ns()
        samples.append(finished - started)
        cpu_samples.append(cpu_finished - cpu_started)
        _, current_peak = tracemalloc.get_traced_memory()
        peak_alloc = max(peak_alloc, current_peak)
    total_cpu_finished = time.process_time_ns()
    total_finished = time.perf_counter_ns()
    tracemalloc.stop()
    return {
        **distribution(samples),
        "wall_ns": total_finished - total_started,
        "cpu_ns": total_cpu_finished - total_cpu_started,
        "cpu_p99_ns": percentile(cpu_samples, 99),
        "peak_python_alloc_bytes": peak_alloc,
        "allocation_bytes": peak_alloc,
        "memory_bytes": peak_alloc,
        "database_calls": 0,
        "database_fixture": "in_memory_synthetic",
    }


def rollup_measurement(workload: int, repetitions: int) -> dict[str, Any]:
    sys.path.insert(0, str(ROOT / "scripts/telemetry"))
    from rollup_definitions import RollupTarget  # type: ignore
    from rollup_engine import build_page_contributions  # type: ignore

    rows = synthetic_rollup_rows(workload)
    target = RollupTarget(1, 1, 8, 7)

    def run_rollup() -> Any:
        return build_page_contributions(
            rows,
            target,
            max_page_bytes=64 * 1024 * 1024,
            max_output_fanout=max(10_000, workload * 10),
        )

    result = python_stage_measure(run_rollup, repetitions)
    result.update({"stage": "rollup"})
    return result


def report_measurement(workload: int, repetitions: int) -> dict[str, Any]:
    sys.path.insert(0, str(ROOT / "scripts/telemetry"))
    sys.path.insert(0, str(ROOT / "tests/async"))
    from report import ReportDatabase, ReportRequest, _QueryPage  # type: ignore
    from telemetry_reports_fixtures import cohort_rows, state_row  # type: ignore

    base_rows = cohort_rows()
    rows = [dict(base_rows[index % len(base_rows)]) for index in range(workload)]

    class FixedDatabase(ReportDatabase):
        def __init__(self) -> None:
            super().__init__(object())

        def _begin_snapshot(self) -> None:
            return None

        def _read_published_state(self, _request: Any) -> dict[str, Any]:
            return state_row()

        def _read_page(self, _request: Any, _target: Any) -> Any:
            return _QueryPage(
                rows=tuple(rows),
                sentinel=None,
                truncated=False,
                next_cursor=None,
                source_rows_fetched=len(rows),
            )

        def close(self) -> None:
            return None

    request = ReportRequest(
        report_name="cohort",
        definition_version=1,
        environment_id=8,
        season_id=9,
        generation=7,
        max_rows=max(10_000, workload),
    )
    database = FixedDatabase()

    def run_report() -> Any:
        return database.read(request)

    result = python_stage_measure(run_report, repetitions)
    result.update(
        {
            "stage": "report",
            "response_rows": workload,
        }
    )
    return result


def fault_measurements(toolchain: Toolchain, executable: Path) -> list[dict[str, Any]]:
    results = []
    for fault_case in FAULT_CASES:
        result = parse_json_line(toolchain.execute(executable, (fault_case,)))
        if result.get("schema_version") != 1 or result.get("case") != fault_case:
            raise GateError(f"unexpected fault result for {fault_case}: {result}")
        results.append(result)
    return results


def evaluate_gate(
    workloads: Sequence[dict[str, Any]], faults: Sequence[dict[str, Any]], source: dict[str, Any]
) -> dict[str, Any]:
    failures: list[str] = []
    budgets = {
        "normal_capture_write_p99_ns_max": 1_000_000,
        "normal_capture_write_p999_ns_max": 5_000_000,
        "normal_worker_p999_ns_max": 50_000_000,
        "rollup_p999_ns_max": 5_000_000_000,
        "report_p999_ns_max": 5_000_000_000,
        "slow_write_enqueue_max_ns": 100_000_000,
        "stop_request_max_ns": 100_000_000,
    }
    for workload in workloads:
        size = workload["records"]
        modes = workload["modes"]
        write = modes["capture_write"]
        if write["dropped"] != 0:
            failures.append(f"{size}: normal capture-write dropped records")
        if write["queue_peak"] > 128:
            failures.append(f"{size}: normal queue peak exceeded the 128-record batch gate")
        if write["p99_ns"] > budgets["normal_capture_write_p99_ns_max"]:
            failures.append(f"{size}: capture-write p99 exceeded 1ms local guard")
        if write["p999_ns"] > budgets["normal_capture_write_p999_ns_max"]:
            failures.append(f"{size}: capture-write p99.9 exceeded 5ms local guard")
        if write["worker_p999_ns"] > budgets["normal_worker_p999_ns_max"]:
            failures.append(f"{size}: in-memory worker p99.9 exceeded 50ms local guard")
        if workload["rollup"]["p999_ns"] > budgets["rollup_p999_ns_max"]:
            failures.append(f"{size}: rollup p99.9 exceeded 5s local guard")
        if workload["report"]["p999_ns"] > budgets["report_p999_ns_max"]:
            failures.append(f"{size}: report p99.9 exceeded 5s local guard")
    for fault in faults:
        name = fault["case"]
        if name == "queue_saturation":
            if fault["queue_peak"] != 16 or fault["dropped_detail"] == 0 or fault["dropped_control"] == 0:
                failures.append("queue saturation did not preserve explicit bounded-loss evidence")
        elif name == "repository_recovery":
            if not fault["pending_zero"] or fault["applied_records"] != 8:
                failures.append("repository recovery did not catch up the retained batch")
        elif name == "ambiguous_commit":
            if not fault["pending_zero"] or fault["ambiguous_commits"] == 0 or fault["duplicate_records"] != 3:
                failures.append("ambiguous commit did not reconcile as duplicates")
        elif name == "slow_write":
            if not fault["pending_zero"] or fault["max_enqueue_ns"] > budgets["slow_write_enqueue_max_ns"]:
                failures.append("slow write blocked producer admission or left a tail")
            if fault["stop_request_ns"] > budgets["stop_request_max_ns"]:
                failures.append("stop request exceeded the local nonblocking guard")
        elif name == "copyover_shutdown":
            if not fault["pending_zero"] or fault["rejected_during_quiesce"] != 1:
                failures.append("copyover-style quiesce/resume was not bounded")
        elif name == "churn" and (fault["cycles"] != 20 or not fault["pending_zero"]):
            failures.append("repeated init/drain/shutdown churn retained state")
    if (
        source["producer_heap_or_sql_calls"]
        or not source["fixed_queue_storage"]
        or not source["queue_private_atomic_spsc"]
        or not source["queue_private_producer_head"]
        or not source["queue_private_consumer_tail"]
    ):
        failures.append("source contract did not prove fixed/no-SQL producer path")
    return {
        "status": "pass" if not failures else "fail",
        "budgets": budgets,
        "failures": failures,
        "ci": "not_run_by_request",
        "live_database": False,
        "production_load": False,
    }


def build_report(workloads: Sequence[int], repetitions: int, toolchain: Toolchain) -> dict[str, Any]:
    source = source_contract()
    (ROOT / "bin/tests").mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="telemetry-272-", dir=ROOT / "bin/tests") as directory:
        output_dir = Path(directory)
        capacity_binary = output_dir / "telemetry_capacity_272"
        fault_binary = output_dir / "telemetry_fault_272"
        toolchain.compile(capacity_binary, CAPACITY_HARNESS)
        toolchain.compile(fault_binary, FAULT_HARNESS)
        measured_workloads = []
        for size in workloads:
            measured_workloads.append(
                {
                    "records": size,
                    "repetitions": repetitions,
                    "modes": capacity_measurements(toolchain, capacity_binary, size, repetitions),
                    "rollup": rollup_measurement(size, repetitions),
                    "report": report_measurement(size, repetitions),
                }
            )
        faults = fault_measurements(toolchain, fault_binary)
    gate = evaluate_gate(measured_workloads, faults, source)
    return {
        "schema_version": 1,
        "issue": 272,
        "scope": "offline_synthetic_local_gate",
        "source_contract": source,
        "toolchain": {
            "compiler": list(toolchain.compiler),
            "wsl_runner": toolchain.use_wsl,
            "python": sys.version.split()[0],
        },
        "workloads": measured_workloads,
        "faults": faults,
        "gate": gate,
    }


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--workloads", nargs="+", type=int, default=list(DEFAULT_WORKLOADS))
    parser.add_argument("--repetitions", type=int, default=DEFAULT_REPETITIONS)
    parser.add_argument("--output", type=Path)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    if not args.workloads or any(value <= 0 or value > 1000 for value in args.workloads):
        raise GateError("workloads must be positive and no greater than 1000")
    if args.repetitions <= 0 or args.repetitions > 8:
        raise GateError("repetitions must be in 1..8")
    toolchain = Toolchain.discover()
    report = build_report(args.workloads, args.repetitions, toolchain)
    encoded = json.dumps(report, indent=2, sort_keys=True) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(encoded, encoding="utf-8")
    print(encoded, end="")
    return 0 if report["gate"]["status"] == "pass" else 1


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (GateError, AssertionError, subprocess.CalledProcessError) as error:
        print(f"telemetry #272 benchmark: FAIL: {error}", file=sys.stderr)
        raise SystemExit(2)
