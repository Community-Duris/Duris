#!/usr/bin/env python3
"""Contract and validation tests for item transfer double-entry accounting."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


class UniversalItemTransferAccountingContract(unittest.TestCase):
    def test_repository_wiring_contract(self):
        sql_source = (ROOT / "src/persistence/critical_command_repository.c").read_text(encoding="utf-8")
        self.assertNotIn("root_operation_id = command.operation_id;", sql_source)
        self.assertRegex(sql_source, r"item_transfer_repository_execute\(\s*connection,\s*command,")
        self.assertIn("economic_sql_item_transfer_record(", sql_source)
        self.assertIn("economic_sql_item_transfer_lock(", sql_source)
        self.assertIn("economic_sql_item_transfer_verify_retained(", sql_source)

    def test_sql_dispatch_fences_legacy_item_economy_before_transaction(self):
        source = (ROOT / "src/persistence/critical_command_repository.c").read_text()
        start = source.index("economic_sql_currency_writer_guard legacy_writer;")
        transaction = source.index('if (!execute(connection, "START TRANSACTION"))', start)
        gate = source[start:transaction]
        for kind in ("item_command", "coin_command", "auction_command", "collector_command",
                     "corpse_command", "restitution_command"):
            self.assertIn(kind, gate)
        self.assertIn("economic_sql_currency_writer_guard::acquire", gate)
        self.assertIn("return root_failure(error);", gate)
        guard = (ROOT / "src/persistence/economic_sql_lifecycle_guard.c").read_text()
        self.assertIn("active_epoch IS NOT NULL", guard)
        writer = guard[guard.index("economic_sql_currency_writer_guard::acquire("):]
        self.assertLess(writer.index("lock(connection, writer_lock"),
                        writer.index("staged_installation(connection, true)"))

    def test_flatfile_repository_wiring_contract(self):
        flatfile_source = (ROOT / "src/flatfile/flatfile_item_repository.c").read_text(encoding="utf-8")
        dispatch_source = (ROOT / "src/flatfile/flatfile_accounting_dispatch.c").read_text(encoding="utf-8")
        self.assertIn("flatfile_item_accounting_reference_append", flatfile_source)
        self.assertIn("ref.operation_id = command.operation_id;", flatfile_source)
        self.assertIn("ref.item_uid = payload.items[index].item_uid;", flatfile_source)
        self.assertIn("flatfile_item_accounting_reference_stage(", flatfile_source)
        self.assertIn("flatfile_accounting_item_transfer_transaction::stage(", flatfile_source)
        self.assertIn("flatfile_accounting_item_transfer_transaction::commit(", flatfile_source)
        self.assertIn("if (!accounted && !result_code && payload.item_count > 0)", flatfile_source)
        self.assertIn("command.type == critical_command_type::item_transfer", dispatch_source)

    def test_flatfile_accounting_roundtrip(self):
        with tempfile.TemporaryDirectory(prefix="duris-univ-item-test-") as directory:
            binary = Path(directory) / "univ_item_test"
            subprocess.run([
                *shlex.split(os.environ.get("CXX", "g++")),
                "-std=c++20", "-O1", "-g", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie",
                "-D__NO_MYSQL__", "-Isrc/no_mysql", "-Isrc",
                "tests/async/universal_item_transfer_accounting_test.cpp",
                "src/flatfile/flatfile_item_accounting_reference.c",
                "src/flatfile/flatfile_authority_transaction.c",
                "src/flatfile/flatfile_store.c",
                "src/item/economic_accounting_item_reference.c",
                "src/persistence/critical_command.c",
                "-lcrypto", "-o", str(binary),
            ], cwd=ROOT, check=True)
            flatfile_root = Path(directory) / "flatfile_root"
            flatfile_root.mkdir(mode=0o700)
            subprocess.run([str(binary), str(flatfile_root)], check=True, timeout=30, env=dict(
                os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1"))


if __name__ == "__main__":
    unittest.main()
