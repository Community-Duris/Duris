#!/usr/bin/env python3
"""Run the issue #272 offline capacity/performance gate."""
from __future__ import annotations

import json
import os
from pathlib import Path
import sys


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/telemetry"))

from benchmark_272 import DEFAULT_WORKLOADS, Toolchain, build_report  # noqa: E402


def main() -> int:
    repetitions = int(os.environ.get("TELEMETRY_272_REPETITIONS", "3"))
    assert 1 <= repetitions <= 8
    report = build_report(DEFAULT_WORKLOADS, repetitions, Toolchain.discover())
    destination = Path(
        os.environ.get(
            "TELEMETRY_272_REPORT",
            str(ROOT / "bin/tests/telemetry_capacity_272.report.json"),
        )
    )
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    assert report["gate"]["status"] == "pass", report["gate"]
    assert [item["records"] for item in report["workloads"]] == list(DEFAULT_WORKLOADS)
    for workload in report["workloads"]:
        assert set(workload["modes"]) == {"off", "capture_only", "capture_write"}
        for mode in workload["modes"].values():
            for field in ("p50_ns", "p95_ns", "p99_ns", "p999_ns"):
                assert field in mode
        assert workload["rollup"]["database_calls"] == 0
        assert workload["report"]["database_calls"] == 0
    print(
        "TELEMETRY_272_CAPACITY_GATE_PASS "
        f"workloads={','.join(str(value) for value in DEFAULT_WORKLOADS)} "
        f"repetitions={repetitions} report={destination}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
