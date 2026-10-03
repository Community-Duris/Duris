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
        self.assertIn("item_transfer_repository_execute_coin", critical_repo)
        self.assertIn("&item_failure_stage, nullptr)", critical_repo)

        coin_accounting = (ROOT / "src/economy/coin_transfer_accounting.c").read_text(encoding="utf-8")
        self.assertLess(
            coin_accounting.index('insert(connection, "economic_accounting_operation", operation)'),
            coin_accounting.index('verify_item_reference_rows(connection, root, value, result, *plan, true)'),
        )

        flatfile_repo = (ROOT / "src/flatfile/flatfile_item_repository.c").read_text(encoding="utf-8")
        self.assertIn("flatfile_item_accounting_reference_append(root, ref)", flatfile_repo)

    def test_active_give_resolves_morph_wallet_and_refuses_unmapped_cash(self):
        source = (ROOT / "src/cmd/actobj.c").read_text(encoding="utf-8")
        give = source[source.index("void do_give("):]
        guard = give.index("economic_gameplay_authority::active()")
        self.assertIn("!(IS_PC(vict) || IS_MORPH(vict))", give)
        self.assertIn("GET_PID(GET_PLYR(vict)) <= 0", give)
        self.assertIn("GET_LEVEL(ch) >= MAXLVL", give)
        self.assertLess(guard, give.index("begin_coin_give_credit(ch, vict, context, false)"))
        self.assertLess(guard, give.index("submit_coin_debit(ch, context)"))

        debit = source[source.index("bool submit_coin_debit("):]
        self.assertLess(debit.index("economic_gameplay_authority::active()"),
                        debit.index("currency_transaction_submit_wallet_value("))
        peer = source[source.index("bool submit_coin_give("):]
        self.assertIn("P_char recipient_wallet = GET_PLYR(recipient)", peer)
        self.assertIn("currency_transaction_coin_wallet_exact(recipient_wallet, context.coin_type", peer)

        transaction = (ROOT / "src/economy/currency_transaction.c").read_text(encoding="utf-8")
        self.assertIn("P_char wallet = GET_PLYR(desc->character)", transaction)
        self.assertIn("P_char character = live_wallet_by_pid(wallet.pid)", transaction)

    def test_compilation_and_execution(self):
        with tempfile.TemporaryDirectory(prefix="duris-coin-item-acc-") as directory:
            binary = Path(directory) / "coin_item_acc_test"
            test_storage = Path(directory) / "storage"
            subprocess.run([
                *shlex.split(os.environ.get("CXX", "g++")), "-std=c++20", "-O1", "-g",
                "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                "-ffunction-sections", "-fdata-sections", "-Wl,--gc-sections",
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
