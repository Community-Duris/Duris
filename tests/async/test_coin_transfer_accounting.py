#!/usr/bin/env python3
"""Flatfile-build refusal checks for the SQL-only coin accounting root."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


class CoinTransferAccountingTest(unittest.TestCase):
    def test_flatfile_refusal(self):
        with tempfile.TemporaryDirectory(prefix="duris-coin-acc-test-") as directory:
            binary = Path(directory) / "coin_acc_flatfile_test"
            subprocess.run([
                *shlex.split(os.environ.get("CXX", "g++")), "-std=c++20", "-O1", "-g",
                "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-ffunction-sections",
                "-fdata-sections", "-Wl,--gc-sections", "-fsanitize=address,undefined",
                "-fno-omit-frame-pointer", "-fno-pie", "-no-pie", "-D__NO_MYSQL__",
                "-Isrc/no_mysql", "-Isrc", "tests/async/coin_transfer_accounting_test.cpp",
                "src/economy/coin_transfer_accounting.c", "-o", str(binary),
            ], cwd=ROOT, check=True)
            subprocess.run([str(binary)], check=True, timeout=30, env=dict(
                os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1"))


if __name__ == "__main__":
    unittest.main()
