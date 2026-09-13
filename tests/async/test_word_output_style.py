#!/usr/bin/env python3
"""Compile/run the production word renderer; SANITIZE=1 adds ASan and UBSan."""
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
BUILD = ROOT / "bin/tests"
BUILD.mkdir(parents=True, exist_ok=True)
flags = ["-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"] if os.environ.get("SANITIZE") == "1" else []
with tempfile.TemporaryDirectory(prefix="word-style-", dir=BUILD) as directory:
    binary = Path(directory) / "harness"
    subprocess.run([
        "g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-O1", "-g", *flags,
        f"-I{ROOT / 'src'}", str(ROOT / "tests/async/word_output_style_harness.cpp"),
        str(ROOT / "src/net/ansi.c"), str(ROOT / "src/net/unicode.c"),
        str(ROOT / "src/net/output_style.c"), "-o", str(binary)
    ], check=True, timeout=120)
    subprocess.run([str(binary)], check=True, timeout=120)
