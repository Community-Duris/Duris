#!/usr/bin/env python3
"""Focused telemetry health observer and production wiring checks."""

from pathlib import Path
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"


def verify_wiring() -> None:
    runtime = (SRC / "telemetry/telemetry_runtime.c").read_text()
    comm = (SRC / "net/comm.c").read_text()
    actinf = (SRC / "cmd/actinf.c").read_text()
    makefile = (SRC / "Makefile").read_text()
    assert "telemetry_runtime_health_observe" in runtime
    assert "telemetry_health event=%s" in comm
    assert "telemetry_runtime_health_observe(telemetry_pulse_now)" in comm
    assert "world_keywords" in actinf and '"telemetry"' in actinf
    assert "show_world_telemetry" in actinf
    assert "telemetry/telemetry_health.o" in makefile
    for field in (
        "last_commit_monotonic_us", "last_commit_age_ms", "last_failure_monotonic_us",
        "failure class=%s error=%u", "record_kinds=%s", "queue depth=%llu capacity=%u",
        "retry inflight=%u repository=%u", "advisory_lock=%s", "coverage gaps=%llu",
    ):
        assert field in actinf
    assert "(!IS_TRUSTED(ch)" in actinf and "> WORLD_ZONES" in actinf
    log_format = comm[comm.index('"telemetry_health event=%s'):comm.index(
        "telemetry_health_event_kind_name", comm.index('"telemetry_health event=%s'))]
    for field in (
        "record_kinds=%s", "error=%u", "last_commit_monotonic_us=%llu",
        "last_failure_monotonic_us=%llu", "queue=%llu/%u", "affected_seq=%llu-%llu",
    ):
        assert field in log_format
    for forbidden in ("player_name", "credentials", "raw_sql", "payload="):
        assert forbidden not in log_format


def compile_and_run(directory: Path, label: str, flags: list[str]) -> None:
    binary = directory / f"telemetry-health-{label}"
    command = [
        os.environ.get("CXX", "g++"),
        "-std=c++20", "-Wall", "-Wextra", "-Werror", "-pedantic",
        "-I", str(SRC), *flags,
        str(ROOT / "tests/async/telemetry_health_harness.cc"),
        str(SRC / "telemetry/telemetry_health.c"),
        "-o", str(binary),
    ]
    subprocess.run(command, cwd=ROOT, check=True, timeout=60)
    environment = {**os.environ, "ASAN_OPTIONS": "detect_leaks=0:abort_on_error=1"}
    completed = subprocess.run([str(binary)], cwd=ROOT, check=False, text=True,
                               stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                               env=environment, timeout=30)
    print(completed.stdout, end="")
    completed.check_returncode()
    assert "telemetry health warning/critical/rate-limit/recovery passed" in completed.stdout
    print(f"telemetry health {label}: PASS")


def main() -> None:
    verify_wiring()
    with tempfile.TemporaryDirectory(prefix="telemetry-health-") as temporary:
        directory = Path(temporary)
        compile_and_run(directory, "normal", ["-O2"])
        compile_and_run(directory, "asan-ubsan", ["-O1", "-g", "-fsanitize=address,undefined",
                                                   "-fno-omit-frame-pointer"])


if __name__ == "__main__":
    main()
