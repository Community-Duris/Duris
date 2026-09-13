#!/usr/bin/env python3
"""Exercise the production profile parser, resolver, reload, and snapshot ownership."""
import os
from pathlib import Path
import subprocess
import tempfile
from _paths import ROOT

build = ROOT / "bin/tests"
build.mkdir(parents=True, exist_ok=True)
flags = ["-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"] if os.environ.get("SANITIZE") == "1" else []
with tempfile.TemporaryDirectory(prefix="output-profiles-", dir=build) as directory:
    binary = Path(directory) / "harness"
    subprocess.run([
        "g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-Wpedantic", "-Og", "-g",
        "-D__NO_MYSQL__", *flags, f"-I{ROOT / 'src'}", f"-I{ROOT / 'src/no_mysql'}",
        str(ROOT / "tests/async/output_profiles_harness.cpp"),
        str(ROOT / "src/net/output_profiles.c"), str(ROOT / "src/net/output_style.c"),
        str(ROOT / "src/net/ansi.c"), str(ROOT / "src/net/unicode.c"),
        "-lcjson", "-pthread", "-o", str(binary)
    ], check=True, timeout=120)
    subprocess.run([str(binary), str(ROOT / "docs/examples/output-profiles-v1.json"), directory],
                   check=True, timeout=120)

# The optional file boundary belongs to boot, before accepting gameplay.
comm = (ROOT / "src/net/comm.c").read_text()
assert comm.count('getenv("DURIS_OUTPUT_PROFILES_FILE")') == 1
assert comm.index('getenv("DURIS_OUTPUT_PROFILES_FILE")') < comm.index("run_the_game(port, sslport);")
assert '.reload_file(' not in comm[comm.index("void send_to_char(const char *messg"):]
