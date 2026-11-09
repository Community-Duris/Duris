#!/usr/bin/env python3
"""Qualify ordered completion capacity and indefinite retry retention against the production coordinator."""
from pathlib import Path
import subprocess
import tempfile
from _paths import ROOT, rel

with tempfile.TemporaryDirectory(prefix="duris-critical_completion_capacity-") as temporary:
    binary = Path(temporary) / "harness"
    subprocess.run([
        "g++", "-std=c++20", "-g", "-Og", "-Wall", "-Wextra", "-Wpedantic",
        "-Werror", "-pthread", "-fsanitize=address,undefined",
        "-fno-omit-frame-pointer", "-fno-pie", "-no-pie", "-Isrc",
        "tests/async/critical_completion_capacity_test.cpp", rel("critical_command.c"),
        rel("critical_command_coordinator.c"), "-Wl,--wrap=_Znwm", "-lcrypto", "-o", str(binary),
    ], cwd=ROOT, check=True)
    subprocess.run([str(binary)], check=True, timeout=30)
print("ordered completion capacity and indefinite retry retention checks passed")
