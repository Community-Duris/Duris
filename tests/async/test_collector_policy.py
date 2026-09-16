#!/usr/bin/env python3
"""Executable collector policy regressions; not a server integration test."""

from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def main():
    output = ROOT / "bin/tests"
    output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="collector-policy-", dir=output) as directory:
        binary = Path(directory) / "collector-policy"
        subprocess.run([
            "g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
            "-O2", "-I", str(ROOT / "src"),
            str(ROOT / "src/economy/collector_policy.c"),
            str(ROOT / "tests/async/collector_policy_harness.cpp"),
            "-o", str(binary),
        ], check=True)
        subprocess.run([str(binary)], check=True, timeout=30)


if __name__ == "__main__":
    main()
