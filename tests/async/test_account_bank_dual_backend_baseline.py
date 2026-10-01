#!/usr/bin/env python3
"""Dual-backend account bank balance verification and baseline audit contracts."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


class AccountBankDualBackendBaselineTest(unittest.TestCase):
    def test_account_bank_contracts(self):
        bank_header = (ROOT / "src/economy/account_bank_balances.h").read_text(encoding="utf-8")
        self.assertIn("struct AccountBankBalances", bank_header)
        self.assertIn("int copper;", bank_header)
        self.assertIn("int silver;", bank_header)
        self.assertIn("int gold;", bank_header)
        self.assertIn("int platinum;", bank_header)
        self.assertIn("total_copper()", bank_header)
        self.assertIn("is_valid()", bank_header)
        self.assertIn("operator==", bank_header)

        utility = (ROOT / "src/core/utility.c").read_text(encoding="utf-8")
        self.assertIn("publish_account_bank_balances_revision", utility)
        self.assertIn("balances->copper < 0", utility)

        sql_bank = (ROOT / "src/persistence/economic_sql_bank_transaction.c").read_text(encoding="utf-8")
        self.assertIn("economic_sql_bank_command_supported", sql_bank)

        flatfile_dispatch = (ROOT / "src/flatfile/flatfile_accounting_dispatch.c").read_text(encoding="utf-8")
        self.assertIn("flatfile_accounting_apply_selected", flatfile_dispatch)
        self.assertIn("CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION", flatfile_dispatch)

    def test_compilation_and_execution(self):
        with tempfile.TemporaryDirectory(prefix="duris-bank-dual-") as directory:
            binary = Path(directory) / "dual_bank_test"
            subprocess.run([
                *shlex.split(os.environ.get("CXX", "g++")), "-std=c++20", "-O1", "-g",
                "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie",
                "-D__NO_MYSQL__", "-Isrc/no_mysql", "-Isrc",
                "tests/async/account_bank_dual_backend_test.cpp",
                "src/persistence/economic_sql_bank_transaction.c",
                "src/economy/economic_currency_adapter.c",
                "src/economy/economic_accounting_intent.c",
                "src/economy/economic_accounting_plan.c",
                "src/economy/economic_accounting_types.c",
                "src/economy/currency_command.c",
                "src/persistence/critical_command.c",
                "src/item/item_transfer_command.c", "src/player/player_snapshot_codec.c",
                "src/flatfile/flatfile_accounting_dispatch.c",
                "-lcrypto",
                "-o", str(binary),
            ], cwd=ROOT, check=True)
            subprocess.run([str(binary)], check=True, timeout=30, env=dict(
                os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1"))


if __name__ == "__main__":
    unittest.main()
