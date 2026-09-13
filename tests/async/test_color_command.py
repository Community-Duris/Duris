#!/usr/bin/env python3
"""Exercise the compiled player command, renderer and preference service boundary."""
import os
from pathlib import Path
import subprocess
import tempfile
from _paths import ROOT

build = ROOT / "bin/tests"
build.mkdir(parents=True, exist_ok=True)
flags = (["-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"]
         if os.environ.get("SANITIZE") == "1" else [])
with tempfile.TemporaryDirectory(prefix="color-command-", dir=build) as directory:
    binary = Path(directory) / "harness"
    subprocess.run([
        "g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-Wpedantic", "-Og", "-g",
        "-D__NO_MYSQL__", *flags, f"-I{ROOT / 'src'}", f"-I{ROOT / 'src/no_mysql'}",
        str(ROOT / "tests/async/color_command_harness.cpp"),
        *(str(ROOT / "src" / path) for path in ("cmd/color_command.c",
            "net/output_profiles.c", "net/output_style.c", "net/ansi.c", "net/unicode.c")),
        "-lcjson", "-pthread", "-o", str(binary)
    ], check=True, timeout=120)
    subprocess.run([str(binary)], check=True, timeout=120)
