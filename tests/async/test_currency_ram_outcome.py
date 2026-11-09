#!/usr/bin/env python3
"""Validate RAM currency decisions, bounds, revisions and nonmutating rejection."""
from pathlib import Path
import subprocess
import tempfile

from _paths import ROOT

with tempfile.TemporaryDirectory(prefix="currency-ram-") as scratch:
    binary = Path(scratch) / "currency-ram-test"
    subprocess.run([
        "g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-Isrc",
        "tests/async/currency_ram_outcome_test.cpp",
        "src/economy/currency_command.c", "src/persistence/critical_command.c",
        "-lcrypto", "-o", str(binary),
    ], cwd=ROOT, check=True)
    subprocess.run([str(binary)], check=True)
print("RAM currency outcome regression passed")
