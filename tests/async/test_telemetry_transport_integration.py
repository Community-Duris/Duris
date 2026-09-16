#!/usr/bin/env python3
"""Narrow disabled-build journey linking actual C repository and D transport."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]

def main():
    with tempfile.TemporaryDirectory(prefix="issue262-disabled-") as directory:
        executable = str(Path(directory) / "disabled-integration")
        command = [
            "g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-pthread",
            "-D__NO_MYSQL__", "-I", str(ROOT / "src"),
            str(ROOT / "tests/async/telemetry_transport_disabled_integration.cc"),
            str(ROOT / "src/telemetry/telemetry_transport.c"),
            str(ROOT / "src/telemetry/telemetry_queue.c"),
            str(ROOT / "src/telemetry/telemetry_repository.c"),
            "-o", executable,
        ]
        subprocess.run(command, cwd=ROOT, check=True, timeout=60)
        subprocess.run([executable], cwd=ROOT, check=True, timeout=15)

if __name__ == "__main__":
    main()
