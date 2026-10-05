#!/usr/bin/env python3
"""Contract and validation tests for typed item-transfer accounting intent."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


class ItemTransferAccountingIntentContract(unittest.TestCase):
    def test_cpp_compilation_and_execution(self):
        with tempfile.TemporaryDirectory(prefix="duris-item-accounting-test-") as directory:
            binary = Path(directory) / "item_accounting_test"
            subprocess.run([
                *shlex.split(os.environ.get("CXX", "g++")), "-std=c++20", "-O1", "-g",
                "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-fsanitize=address,undefined",
                "-fno-omit-frame-pointer", "-fno-pie", "-no-pie", "-Isrc",
                "tests/async/item_transfer_accounting_test.cpp",
                "src/economy/item_transfer_accounting.c",
                "src/economy/economic_accounting_intent.c",
                "src/economy/economic_accounting_plan.c", "src/economy/economic_source_event.c",
                "src/economy/economic_accounting_types.c",
                "src/item/item_transfer_command.c", "src/world/quest_mobile_native_reference.c", "src/item/craft_pouch_mutation.c", "src/combat/chaos_pouch_ledger.c", "src/player/player_snapshot_codec.c",
                "src/persistence/critical_command.c", "-lcrypto", "-o", str(binary),
            ], cwd=ROOT, check=True)
            subprocess.run([str(binary)], check=True, timeout=30, env=dict(
                os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1"))


if __name__ == "__main__":
    unittest.main()
