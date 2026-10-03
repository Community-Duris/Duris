#!/usr/bin/env python3
"""Fault-inject read transaction protocol failures against production cleanup code."""

from pathlib import Path
import os
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]

with tempfile.TemporaryDirectory(prefix="player-read-transaction-cleanup-") as directory:
    binary = Path(directory) / "read-transaction-cleanup"
    mysql_cflags = shlex.split(subprocess.check_output(["mysql_config", "--cflags"], text=True))
    mysql_libs = shlex.split(subprocess.check_output(["mysql_config", "--libs"], text=True))
    subprocess.run(
        [
            os.environ.get("CXX", "g++"),
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Wpedantic",
            "-Werror",
            "-pthread",
            "-ffunction-sections",
            "-fdata-sections",
            "-Isrc",
            *mysql_cflags,
            "tests/async/player_read_transaction_cleanup_harness.cpp",
            "src/player/player_death_recovery_query.c",
            "src/player/player_load_pipeline.c",
            "src/sql/sql_pool.c",
            "src/persistence/persistence_observability.c",
            "-Wl,--gc-sections",
            *mysql_libs,
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        check=True,
    )
    subprocess.run([str(binary)], cwd=ROOT, check=True, timeout=15)

print("player read transaction cleanup fault-injection checks passed")
