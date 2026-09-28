#!/usr/bin/env python3
"""Contract and integration tests for coin transfer item custody accounting bridge."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


class CoinTransferItemAccountingTest(unittest.TestCase):
    def test_bridge_sources(self):
        critical_repo = (ROOT / "src/persistence/critical_command_repository.c").read_text(encoding="utf-8")
        self.assertIn("child_item_accounting.root_operation_id =", critical_repo)
        self.assertIn("command.operation_id", critical_repo)
        self.assertIn("child_item_accounting.child_index =", critical_repo)
        self.assertIn("item_transfer_repository_execute_coin", critical_repo)

        flatfile_repo = (ROOT / "src/flatfile/flatfile_item_repository.c").read_text(encoding="utf-8")
        self.assertIn("flatfile_item_accounting_reference_append(root, ref)", flatfile_repo)

    def test_compilation_and_execution(self):
        with tempfile.TemporaryDirectory(prefix="duris-coin-item-acc-") as directory:
            binary = Path(directory) / "coin_item_acc_test"
            test_storage = Path(directory) / "storage"
            subprocess.run([
                *shlex.split(os.environ.get("CXX", "g++")), "-std=c++20", "-O1", "-g",
                "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie",
                "-D__NO_MYSQL__", "-Isrc/no_mysql", "-Isrc",
                "tests/async/coin_transfer_item_accounting_test.cpp",
                "src/flatfile/flatfile_item_accounting_reference.c",
                "src/flatfile/flatfile_store.c",
                "src/item/economic_accounting_item_reference.c",
                "-o", str(binary),
            ], cwd=ROOT, check=True)

            subprocess.run([str(binary), str(test_storage)], check=True, timeout=30, env=dict(
                os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1"))


if __name__ == "__main__":
    unittest.main()
