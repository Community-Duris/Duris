#!/usr/bin/env python3
"""Compile and run the production casting-pulse policy under ASan/UBSan."""

from __future__ import annotations

import os
from pathlib import Path
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]
CXX = os.environ.get("CXX", "g++")


with tempfile.TemporaryDirectory(prefix="duris-casting-pulse-") as directory:
    binary = Path(directory) / "casting_pulse_policy_harness"
    subprocess.run(
        [
            CXX,
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Wpedantic",
            "-Werror",
            "-fsanitize=address,undefined",
            "-g",
            f"-I{ROOT / 'src'}",
            str(ROOT / "tests/async/casting_pulse_policy_harness.cpp"),
            str(ROOT / "src/net/casting_pulse_policy.c"),
            "-o",
            str(binary),
        ],
        check=True,
        cwd=ROOT,
    )
    environment = os.environ.copy()
    environment.setdefault("ASAN_OPTIONS", "detect_leaks=1")
    subprocess.run([str(binary)], check=True, cwd=ROOT, env=environment)

print("Casting pulse policy sanitizer checks passed.")
