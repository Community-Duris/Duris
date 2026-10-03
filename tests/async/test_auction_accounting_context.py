#!/usr/bin/env python3
"""Contract and validation tests for auction double-entry item custody accounting."""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


class AuctionAccountingContextContract(unittest.TestCase):
    def test_repository_wiring_contract(self):
        sql_source = (ROOT / "src/economy/auction_repository.c").read_text(encoding="utf-8")
        self.assertNotIn("economic_accounting_item_reference_insert(", sql_source)
        self.assertIn("INSERT INTO item_ownership_ledger", sql_source)

    def test_flatfile_repository_wiring_contract(self):
        flatfile_source = (ROOT / "src/flatfile/flatfile_auction_repository.c").read_text(encoding="utf-8")
        self.assertIn("flatfile_item_accounting_reference_stage", flatfile_source)
        self.assertIn("flatfile_authority_transaction_commit_operations", flatfile_source)
        self.assertIn("ref.operation_id = command.operation_id;", flatfile_source)
        self.assertIn("ref.item_uid = item_mutation.item_uids[index];", flatfile_source)
        self.assertIn("ref.after_revision = item_mutation.item_revisions[index];", flatfile_source)
        self.assertIn("ref.child_index = accounted ? 0 : 1;", flatfile_source)

    def test_auction_accounting_roundtrip(self):
        with tempfile.TemporaryDirectory(prefix="duris-auction-acc-test-") as directory:
            binary = Path(directory) / "auction_acc_test"
            subprocess.run([
                os.environ.get("CXX", "g++"), "-std=c++20", "-O1", "-g", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie",
                "-ffunction-sections", "-fdata-sections", "-Wl,--gc-sections",
                "-D__NO_MYSQL__", "-Isrc/no_mysql", "-Isrc",
                "tests/async/auction_accounting_context_test.cpp",
                "src/flatfile/flatfile_item_accounting_reference.c",
                "src/flatfile/flatfile_store.c",
                "src/item/economic_accounting_item_reference.c",
                "-o", str(binary),
            ], cwd=ROOT, check=True)
            flatfile_root = Path(directory) / "flatfile_root"
            flatfile_root.mkdir(mode=0o700)
            subprocess.run([str(binary), str(flatfile_root)], check=True, timeout=30, env=dict(
                os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1"))


if __name__ == "__main__":
    unittest.main()
