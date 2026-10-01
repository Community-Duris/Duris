#!/usr/bin/env python3
"""Run the issue #272 injected-repository fault/teardown matrix."""
from __future__ import annotations

import json
from pathlib import Path
import sys
import tempfile


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/telemetry"))

from benchmark_272 import (  # noqa: E402
    FAULT_CASES,
    FAULT_HARNESS,
    Toolchain,
    fault_measurements,
    source_contract,
)


def main() -> int:
    source = source_contract()
    (ROOT / "bin/tests").mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="telemetry-272-fault-", dir=ROOT / "bin/tests") as directory:
        output = Path(directory) / "telemetry_fault_272"
        toolchain = Toolchain.discover()
        toolchain.compile(output, FAULT_HARNESS)
        results = fault_measurements(toolchain, output)
    by_case = {value["case"]: value for value in results}
    assert set(by_case) == set(FAULT_CASES)
    saturation = by_case["queue_saturation"]
    assert saturation["queue_peak"] == 16
    assert saturation["dropped_detail"] > 0 and saturation["dropped_control"] > 0
    recovery = by_case["repository_recovery"]
    assert recovery["pending_zero"] and recovery["applied_records"] == 8
    ambiguous = by_case["ambiguous_commit"]
    assert ambiguous["pending_zero"] and ambiguous["ambiguous_commits"] > 0
    assert ambiguous["duplicate_records"] == 3
    slow = by_case["slow_write"]
    assert slow["pending_zero"] and slow["max_enqueue_ns"] < 100_000_000
    assert slow["stop_request_ns"] < 100_000_000
    copyover = by_case["copyover_shutdown"]
    assert copyover["pending_zero"] and copyover["rejected_during_quiesce"] == 1
    churn = by_case["churn"]
    assert churn["cycles"] == 20 and churn["pending_zero"]
    assert source["producer_heap_or_sql_calls"] is False
    print(
        "TELEMETRY_272_FAULT_MATRIX_PASS "
        f"cases={','.join(FAULT_CASES)} queue_bytes={saturation['queue_reserved_bytes']}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
