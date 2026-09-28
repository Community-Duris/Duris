#!/usr/bin/env python3
"""Contract and validation tests for economic_accounting_item_reference."""
import os
import shlex
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


class EconomicAccountingItemReferenceContract(unittest.TestCase):
    def test_schema_constraints_match_contract(self):
        sql_schema = (ROOT / "migrations/economy_accounting.sql").read_text(encoding="utf-8")
        self.assertIn("CREATE TABLE IF NOT EXISTS economic_accounting_item_reference", sql_schema)
        self.assertIn("uq_item_ledger_accounting_reference", sql_schema)
        self.assertIn("chk_economic_item_indexes", sql_schema)
        self.assertIn("economic_item_operation_fk", sql_schema)
        self.assertIn("economic_item_legacy_fk", sql_schema)

    def test_cpp_validation_and_no_mysql_refusal(self):
        with tempfile.TemporaryDirectory(prefix="duris-item-ref-test-") as directory:
            binary = Path(directory) / "item_ref_test"
            subprocess.run([
                *shlex.split(os.environ.get("CXX", "g++")), "-std=c++20", "-O1", "-g", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie",
                "-D__NO_MYSQL__", "-Isrc/no_mysql", "-Isrc",
                "tests/async/economic_accounting_item_reference_test.cpp",
                "src/item/economic_accounting_item_reference.c",
                "-o", str(binary),
            ], cwd=ROOT, check=True)
            subprocess.run([str(binary)], check=True, timeout=30, env=dict(
                os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1"))


if __name__ == "__main__":
    unittest.main()
