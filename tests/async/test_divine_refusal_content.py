#!/usr/bin/env python3
"""Compile and execute the validated authored divine-refusal content registry."""

from pathlib import Path
import subprocess
import sys
import tempfile

from _paths import ROOT, source


def main() -> int:
    harness = Path(__file__).with_name("divine_refusal_content_harness.cpp")
    with tempfile.TemporaryDirectory() as directory:
        binary = Path(directory) / "divine_refusal_content"
        subprocess.run(
            [
                "g++",
                "-std=c++20",
                "-Wall",
                "-Wextra",
                "-Wpedantic",
                "-Werror",
                "-g",
                "-fsanitize=address,undefined",
                "-I",
                str(ROOT / "src"),
                str(harness),
                str(source("divine_refusal_content.c")),
                "-lcjson",
                "-o",
                str(binary),
            ],
            check=True,
        )
        subprocess.run([str(binary)], check=True)
    print("Divine refusal content checks passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
