#!/usr/bin/env python3
"""Exercise durable atomic outcomes, outage retention and cold replay."""
from pathlib import Path
import os
import shlex
import subprocess
import tempfile

from _paths import ROOT

with tempfile.TemporaryDirectory(prefix="operation-log-") as scratch:
    binary = Path(scratch) / "operation-log-test"
    subprocess.run(shlex.split(os.environ.get("CXX", "g++")) + [
        "-std=c++20", "-Wall", "-Wextra", "-Werror", "-pthread", "-I", str(ROOT / "src"),
        str(ROOT / "tests/async/operation_log_test.cpp"),
        str(ROOT / "src/persistence/critical_command.c"),
        str(ROOT / "src/persistence/critical_command_journal.c"),
        str(ROOT / "src/persistence/operation_log.c"),
        str(ROOT / "src/player/player_checkpoint_command.c"),
        str(ROOT / "src/player/player_snapshot_codec.c"),
        "-Wl,--wrap=fsync", "-lcrypto", "-lz", "-o", str(binary),
    ], check=True)
    subprocess.run([str(binary), str(Path(scratch) / "journal")], check=True)
print("operation log regression passed")
