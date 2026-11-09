#!/usr/bin/env python3
"""Qualify RAM admission reservation, refusal and uncertainty against the production coordinator."""
from pathlib import Path
import subprocess
import tempfile
from _paths import ROOT, rel

with tempfile.TemporaryDirectory(prefix="duris-critical_command_admission-") as temporary:
    binary = Path(temporary) / "harness"
    subprocess.run([
        "g++", "-std=c++20", "-g", "-Og", "-Wall", "-Wextra", "-Wpedantic",
        "-Werror", "-pthread", "-fsanitize=address,undefined",
        "-fno-omit-frame-pointer", "-fno-pie", "-no-pie", "-Isrc",
        "tests/async/critical_admission_test.cpp", rel("critical_command.c"),
        rel("critical_command_coordinator.c"), "-lcrypto", "-o", str(binary),
    ], cwd=ROOT, check=True)
    subprocess.run([str(binary)], check=True, timeout=30)
print("RAM admission reservation, refusal and uncertainty checks passed")
