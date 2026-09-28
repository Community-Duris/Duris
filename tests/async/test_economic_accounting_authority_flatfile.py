#!/usr/bin/env python3
"""Client-free builds refuse SQL identity locking without altering output."""
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix="duris-sql-authority-refusal-") as directory:
    binary = Path(directory) / "authority"
    subprocess.run([
        "g++", "-std=c++20", "-O1", "-g", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
        "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie",
        "-D__NO_MYSQL__", "-Isrc/no_mysql", "-Isrc",
        "tests/async/economic_accounting_authority_flatfile_test.cpp",
        "src/persistence/economic_accounting_repository.c", "-o", str(binary),
    ], cwd=ROOT, check=True)
    subprocess.run([str(binary)], check=True, timeout=30, env=dict(
        os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
        UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1"))
print("SQL identity authority remains unavailable in client-free builds; output preserved")
