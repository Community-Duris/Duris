#!/usr/bin/env python3
"""Qualify the coordinator against the actual ordered persistence writer.

Journal fault/cold-replay coverage lives in test_operation_log.py and
test_critical_command_journal_faults.py. This harness intentionally exercises
the current single-writer contract rather than the retired parallel worker pool.
"""
from pathlib import Path
import subprocess
import tempfile

from _paths import ROOT, rel

with tempfile.TemporaryDirectory(prefix="duris-critical-writer-") as temporary:
    binary = Path(temporary) / "coordinator"
    subprocess.run([
        "g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
        "-pthread", "-Isrc", "tests/async/critical_coordinator_writer_test.cpp",
        rel("critical_command.c"), rel("critical_command_coordinator.c"),
        rel("player_save_worker.c"), rel("persistence_observability.c"),
        "-lmysqlclient", "-lcrypto", "-o", str(binary),
    ], cwd=ROOT, check=True)
    subprocess.run([str(binary)], check=True, timeout=30)
print("critical coordinator ordered writer, publication fence, retry, capacity and lifecycle checks passed")
