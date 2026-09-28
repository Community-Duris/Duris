#!/usr/bin/env python3
"""Contract and storage tests for flatfile item accounting reference."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


class FlatfileItemAccountingReferenceContract(unittest.TestCase):
    def test_header_declarations(self):
        header = (ROOT / "src/flatfile/flatfile_item_accounting_reference.h").read_text(encoding="utf-8")
        self.assertIn("FLATFILE_ITEM_ACCOUNTING_REFERENCE_RECORD_BYTES = 66", header)
        self.assertIn("flatfile_item_accounting_reference_encode", header)
        self.assertIn("flatfile_item_accounting_reference_decode", header)
        self.assertIn("flatfile_item_accounting_reference_append", header)
        self.assertIn("flatfile_item_accounting_reference_find_by_legacy", header)
        self.assertIn("flatfile_item_accounting_reference_find_by_item", header)
        self.assertIn("flatfile_item_accounting_reference_find_by_operation", header)
        self.assertIn("flatfile_item_accounting_reference_find_history", header)


    def test_cpp_compilation_and_execution(self):
        with tempfile.TemporaryDirectory(prefix="duris-ff-item-ref-") as directory:
            binary = Path(directory) / "ff_item_ref_test"
            test_storage = Path(directory) / "storage"
            subprocess.run([
                *shlex.split(os.environ.get("CXX", "g++")), "-std=c++20", "-O1", "-g", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie",
                "-D__NO_MYSQL__", "-Isrc/no_mysql", "-Isrc",
                "tests/async/flatfile_item_accounting_reference_test.cpp",
                "src/flatfile/flatfile_item_accounting_reference.c",
                "src/flatfile/flatfile_authority_transaction.c",
                "src/flatfile/flatfile_store.c",
                "src/persistence/critical_command.c",
                "src/item/economic_accounting_item_reference.c",
                "-lcrypto", "-pthread",
                "-o", str(binary),
            ], cwd=ROOT, check=True)
            subprocess.run([str(binary), str(test_storage)], check=True, timeout=30, env=dict(
                os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1"))


if __name__ == "__main__":
    unittest.main()
