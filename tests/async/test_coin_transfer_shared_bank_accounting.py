#!/usr/bin/env python3
"""Focused native regression for shared-bank coin-transfer result validation."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


class CoinTransferSharedBankAccountingTest(unittest.TestCase):
    def test_shared_bank_result_validation(self):
        with tempfile.TemporaryDirectory(prefix="duris-coin-shared-bank-test-") as directory:
            binary = Path(directory) / "coin_transfer_shared_bank_accounting_test"
            subprocess.run([
                *shlex.split(os.environ.get("CXX", "g++")), "-std=c++20", "-O1", "-g",
                "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-ffunction-sections",
                "-fdata-sections", "-Wl,--gc-sections", "-fsanitize=address,undefined",
                "-fno-omit-frame-pointer", "-fno-pie", "-no-pie", "-D__NO_MYSQL__",
                "-Isrc/no_mysql", "-Isrc", "tests/async/coin_transfer_shared_bank_accounting_test.cpp",
                "src/economy/coin_transfer_command.c", "src/economy/currency_command.c",
                "src/economy/native_quest_cost.c", "src/economy/native_quest_coin_give.c",
                "src/economy/shop_trade_recovery_manifest.c", "src/item/lockpick_retirement_continuation.c",
                "src/item/item_transfer_command.c", "src/world/quest_mobile_native_reference.c", "src/economy/economic_source_event.c", "src/item/craft_pouch_mutation.c", "src/combat/chaos_pouch_ledger.c", "src/player/player_snapshot_codec.c", "src/persistence/critical_command.c",
                "-lcrypto", "-o", str(binary),
            ], cwd=ROOT, check=True)
            subprocess.run([str(binary)], check=True, timeout=30, env=dict(
                os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1"))


if __name__ == "__main__":
    unittest.main()
