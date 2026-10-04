#!/usr/bin/env python3
"""Measured selected-control accumulator/codec cost, plus maintained transport gate."""
import argparse
import json
from pathlib import Path
import subprocess
import sys
import tempfile

from test_telemetry_battle_contributions import ROOT, compile_harness


def run(output):
    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="control-performance-", dir=ROOT / "bin/tests") as temporary:
        native = Path(temporary) / "controls"
        compile_harness(native, optimize=True)
        profiles = []
        for repetition in range(5):
            result = subprocess.check_output([str(native), "--control-performance"], text=True, timeout=60)
            profiles.extend(dict(json.loads(line), repetition=repetition) for line in result.splitlines())
        for value in profiles:
            assert value["event_allocation_bytes"] == 0 and value["state_bytes"] == 213048
            if value["stage"] == "control_capture_encode":
                assert value["p99_ns"] <= 1_000_000 and value["p999_ns"] <= 5_000_000
        transport = Path(temporary) / "transport.json"
        subprocess.run([sys.executable, "scripts/telemetry/benchmark_272.py", "--workloads", "50", "200", "1000",
            "--repetitions", "5", "--output", str(transport)], cwd=ROOT, check=True, timeout=600)
        report = dict(status="passed", profiles=profiles, fixed_control_state_bytes=213048,
            compiler=subprocess.check_output(["g++", "--version"], text=True).splitlines()[0],
            compile_optimization="-O2", python_version=sys.version,
            target_capacity=512, overflow="explicit source gap; executable capacity cases in the control harness",
            event_allocation_bytes=0, capture_p99_budget_ns=1_000_000, capture_p999_budget_ns=5_000_000,
            measurement="native accumulator plus portable encoding; runtime callback and SQL costs excluded",
            transport=json.loads(transport.read_text(encoding="utf-8")))
        output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
        print(json.dumps({key: report[key] for key in ("status", "fixed_control_state_bytes", "target_capacity", "event_allocation_bytes")}))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    run(parser.parse_args().output)
