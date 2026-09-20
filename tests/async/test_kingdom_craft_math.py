#!/usr/bin/env python3
"""Compile and run the guild store's arithmetic harness.

src/kingdom/kingdom_craft_math.h is pure integer code, so this needs no server
and no database: it builds tests/async/kingdom_craft_math_harness.cpp with the
tree's own warning discipline and fails on any worked example that moved."""

import pathlib
import subprocess
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[2]

with tempfile.TemporaryDirectory(prefix="duris-kingdom-craft-math-") as temporary:
    binary = pathlib.Path(temporary) / "kingdom_craft_math"
    compiled = subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Wpedantic",
            "-Werror",
            "-Isrc",
            "tests/async/kingdom_craft_math_harness.cpp",
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if compiled.returncode:
        raise SystemExit(compiled.stdout)
    ran = subprocess.run(
        [str(binary)], cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT
    )
    print(ran.stdout.strip())
    if ran.returncode:
        raise SystemExit(1)
