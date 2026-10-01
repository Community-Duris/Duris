#!/usr/bin/env python3
"""Contract and validation tests for collector double-entry item custody accounting."""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


class CollectorAccountingContextContract(unittest.TestCase):
    def test_repository_wiring_contract(self):
        sql_source = (ROOT / "src/economy/collector_repository.c").read_text(encoding="utf-8")
        self.assertNotIn("economic_accounting_item_reference_insert(", sql_source)
        self.assertIn("INSERT INTO item_ownership_ledger", sql_source)

    def test_flatfile_repository_wiring_contract(self):
        flatfile_source = (ROOT / "src/flatfile/flatfile_collector_repository.c").read_text(encoding="utf-8")
        self.assertIn("flatfile_item_accounting_reference_stage", flatfile_source)
        self.assertLess(flatfile_source.index("flatfile_item_accounting_reference_stage"),
                        flatfile_source.index("flatfile_accounting_storage::commit("))
        self.assertIn("ref.operation_id = command.operation_id;", flatfile_source)
        self.assertIn("ref.item_uid = payload.items[index].item_uid;", flatfile_source)
        self.assertIn("ref.before_revision = payload.items[index].expected_item_revision;", flatfile_source)
        self.assertIn("ref.after_revision = ref.before_revision + 1;", flatfile_source)

    def test_collector_accounting_roundtrip(self):
        with tempfile.TemporaryDirectory(prefix="duris-collector-acc-test-") as directory:
            binary = Path(directory) / "collector_acc_test"
            subprocess.run([
                os.environ.get("CXX", "g++"), "-std=c++20", "-O1", "-g", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie",
                "-ffunction-sections", "-fdata-sections", "-Wl,--gc-sections",
                "-D__NO_MYSQL__", "-Isrc/no_mysql", "-Isrc",
                "tests/async/collector_accounting_context_test.cpp",
                "src/flatfile/flatfile_item_accounting_reference.c",
                "src/flatfile/flatfile_authority_transaction.c",
                "src/flatfile/flatfile_store.c",
                "src/item/economic_accounting_item_reference.c",
                "src/persistence/critical_command.c", "-lcrypto", "-pthread",
                "-o", str(binary),
            ], cwd=ROOT, check=True)
            flatfile_root = Path(directory) / "flatfile_root"
            flatfile_root.mkdir(mode=0o700)
            subprocess.run([str(binary), str(flatfile_root)], check=True, timeout=30, env=dict(
                os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1"))


if __name__ == "__main__":
    unittest.main()
