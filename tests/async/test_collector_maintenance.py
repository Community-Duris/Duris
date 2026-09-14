#!/usr/bin/env python3
"""Collector enable/disable reconciliation and scheduling regressions."""

from pathlib import Path
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]


def main() -> None:
    comm = (ROOT / "src/net/comm.c").read_text()
    assert "collector_maintenance_pulse();" in comm
    assert "collector_maintenance_shutdown();" in comm
    makefile = (ROOT / "src/Makefile").read_text()
    assert "economy/collector_maintenance.o" in makefile

    output = ROOT / "bin" / "tests"
    output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="collector-maintenance-", dir=output) as directory:
        binary = Path(directory) / "collector-maintenance"
        subprocess.run(
            [
                "g++",
                "-std=c++20",
                "-Wall",
                "-Wextra",
                "-Wpedantic",
                "-Werror",
                "-O2",
                "-I",
                str(ROOT / "src"),
                str(ROOT / "src/economy/collector_maintenance.c"),
                str(ROOT / "tests/async/collector_maintenance_harness.cpp"),
                "-o",
                str(binary),
            ],
            check=True,
        )
        subprocess.run([str(binary)], check=True, timeout=30)


if __name__ == "__main__":
    main()
