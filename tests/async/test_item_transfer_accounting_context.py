#!/usr/bin/env python3
"""Contract and validation tests for item_transfer_accounting_context in item_transfer_repository."""
import os
import shlex
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


class ItemTransferAccountingContextContract(unittest.TestCase):
    def test_header_declarations(self):
        header = (ROOT / "src/item/item_transfer_repository.h").read_text(encoding="utf-8")
        self.assertIn("struct item_transfer_accounting_context", header)
        self.assertIn("const item_transfer_accounting_context *accounting_context", header)

    def test_cpp_compilation_and_execution(self):
        with tempfile.TemporaryDirectory(prefix="duris-item-ctx-test-") as directory:
            binary = Path(directory) / "item_ctx_test"
            subprocess.run([
                *shlex.split(os.environ.get("CXX", "g++")), "-std=c++20", "-O1", "-g", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie",
                "-D__NO_MYSQL__", "-Isrc/no_mysql", "-Isrc",
                "tests/async/item_transfer_accounting_context_test.cpp",
                "src/item/item_transfer_repository.c",
                "src/persistence/sql_room_item_payload.c",
                "src/item/economic_accounting_item_reference.c",
                "src/item/item_transfer_command.c", "src/item/craft_pouch_mutation.c", "src/combat/chaos_pouch_ledger.c",
                "src/economy/item_transfer_accounting.c",
                "src/economy/economic_accounting_intent.c",
                "src/economy/economic_accounting_plan.c", "src/economy/economic_source_event.c",
                "src/economy/economic_accounting_types.c",
                "src/persistence/critical_command.c",
                "src/player/player_snapshot_codec.c",
                "-lcrypto",
                "-o", str(binary),
            ], cwd=ROOT, check=True)
            subprocess.run([str(binary)], check=True, timeout=30, env=dict(
                os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1"))


if __name__ == "__main__":
    unittest.main()
