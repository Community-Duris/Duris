#!/usr/bin/env python3
"""Production SQL quest-erasure component: fake SQL boundary, no database service."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
compiler = shlex.split(os.environ.get("CXX", "g++"))
mysql_flags = shlex.split(subprocess.check_output(["mysql_config", "--cflags"], text=True))
sanitizers = []
if os.environ.get("DURIS_TEST_SANITIZERS"):
    sanitizers = ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
with tempfile.TemporaryDirectory(prefix="duris-sql-quest-erasure-") as temporary:
    binary = Path(temporary) / "erasure"
    subprocess.run([
        *compiler, "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
        "-g", "-O0", *sanitizers, "-Isrc", *mysql_flags,
        "tests/async/sql_zone_story_quest_erasure_harness.cpp",
        "src/sql/zone_story_quest_state_repository.c",
        "src/world/zone_story_quest_feature.c", "src/world/zone_story_quest_tracking.c",
        "src/world/zone_story_quest_catalog.c", "-Wl,--wrap=free", "-lz", "-o", str(binary),
    ], cwd=ROOT, check=True, timeout=180)
    subprocess.run([str(binary)], cwd=ROOT, check=True, timeout=60)
