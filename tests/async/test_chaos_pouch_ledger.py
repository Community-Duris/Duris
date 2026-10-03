#!/usr/bin/env python3
"""Immutable pouch counter mutation and preservation, with native sanitizers."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


class ChaosPouchLedger(unittest.TestCase):
    def test_native_mutations(self):
        with tempfile.TemporaryDirectory(prefix="duris-pouch-ledger-") as directory:
            binary = Path(directory) / "pouch_ledger"
            subprocess.run([
                *shlex.split(os.environ.get("CXX", "g++")), "-std=c++20", "-O1", "-g",
                "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-fsanitize=address,undefined",
                "-fno-omit-frame-pointer", "-fno-pie", "-no-pie", "-Isrc",
                "tests/async/chaos_pouch_ledger_test.cpp", "src/combat/chaos_pouch_ledger.c",
                "src/item/craft_pouch_mutation.c",
                "src/player/player_snapshot_codec.c", "-o", str(binary),
            ], cwd=ROOT, check=True)
            subprocess.run([str(binary)], check=True, timeout=30, env=dict(
                os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1"))

    def test_publication_allocation_failure_and_replay(self):
        with tempfile.TemporaryDirectory(prefix="duris-pouch-publication-") as directory:
            binary = Path(directory) / "pouch_publication"
            subprocess.run([
                *shlex.split(os.environ.get("CXX", "g++")), "-std=c++20", "-O1", "-g",
                "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-fsanitize=address,undefined",
                "-fno-omit-frame-pointer", "-fno-pie", "-no-pie", "-Isrc",
                "tests/async/chaos_pouch_publication_test.cpp", "src/combat/chaos_pouch_publication.c",
                "src/combat/chaos_pouch_ledger.c", "src/item/craft_pouch_mutation.c",
                "src/player/player_snapshot_codec.c", "-o", str(binary),
            ], cwd=ROOT, check=True)
            subprocess.run([str(binary)], check=True, timeout=30, env=dict(
                os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1"))


if __name__ == "__main__":
    unittest.main()
