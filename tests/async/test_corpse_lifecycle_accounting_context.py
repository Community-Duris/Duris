#!/usr/bin/env python3
"""Contract and validation tests for item_transfer_accounting_context in corpse_lifecycle_repository."""
import os
import shlex
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


class CorpseLifecycleAccountingContextContract(unittest.TestCase):
    def test_repository_wiring_contract(self):
        source = (ROOT / "src/persistence/corpse_lifecycle_repository.c").read_text(encoding="utf-8")
        self.assertIn("execute_world_item_transfer", source)
        self.assertIn("const item_transfer_accounting_context *accounting_context", source)
        self.assertNotIn("root_operation_id = command.operation_id;", source)
        self.assertIn("item_transfer_repository_execute_at_offset(", source)
        self.assertIn("rollback_domain(connection)", source)

    def test_cpp_compilation_and_execution(self):
        with tempfile.TemporaryDirectory(prefix="duris-corpse-ctx-test-") as directory:
            binary = Path(directory) / "corpse_ctx_test"
            subprocess.run([
                *shlex.split(os.environ.get("CXX", "g++")), "-std=c++20", "-O1", "-g", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie",
                "-D__NO_MYSQL__", "-Isrc/no_mysql", "-Isrc",
                "tests/async/corpse_lifecycle_accounting_context_test.cpp",
                "src/persistence/corpse_lifecycle_repository.c",
                "src/persistence/corpse_lifecycle_command.c",
                "src/item/item_transfer_repository.c",
                "src/item/economic_accounting_item_reference.c",
                "src/item/item_transfer_command.c", "src/item/craft_pouch_mutation.c", "src/combat/chaos_pouch_ledger.c",
                "src/economy/economic_accounting_types.c",
                "src/economy/economic_accounting_plan.c",
                "src/economy/economic_accounting_intent.c",
                "src/economy/item_transfer_accounting.c",
                "src/persistence/critical_command.c",
                "src/player/player_snapshot_codec.c",
                "src/economy/collector_repository.c",
                "src/economy/collector_command.c",
                "src/economy/collector_codec.c",
                "src/economy/collector_policy.c",
                "src/economy/currency_command.c",
                "-lcrypto",
                "-o", str(binary),
            ], cwd=ROOT, check=True)
            subprocess.run([str(binary)], check=True, timeout=30, env=dict(
                os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1"))


if __name__ == "__main__":
    unittest.main()
