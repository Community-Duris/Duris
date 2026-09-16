#!/usr/bin/env python3
"""Compile and run the production divine-refusal policy with injected RNG/time."""

from __future__ import annotations

import os
from pathlib import Path
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]
CXX = os.environ.get("CXX", "g++")


with tempfile.TemporaryDirectory(prefix="duris-divine-refusal-") as directory:
    binary = Path(directory) / "divine_refusal_policy_harness"
    subprocess.run(
        [
            CXX,
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Wpedantic",
            "-Werror",
            f"-I{ROOT / 'src'}",
            str(ROOT / "tests/async/divine_refusal_policy_harness.cpp"),
            str(ROOT / "src/cmd/divine_refusal_policy.c"),
            "-o",
            str(binary),
        ],
        check=True,
        cwd=ROOT,
    )
    subprocess.run([str(binary)], check=True, cwd=ROOT)

print("Divine refusal policy runtime checks passed.")
