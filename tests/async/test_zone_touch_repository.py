#!/usr/bin/env python3
"""Contract and validation tests for zone_touch_repository."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


class ZoneTouchRepositoryTest(unittest.TestCase):
    def test_schema_contracts(self):
        sql = (ROOT / "migrations/boon_reward_zone_outcome.sql").read_text(encoding="utf-8")
        self.assertIn("CREATE TABLE IF NOT EXISTS zone_touch_outcome", sql)
        self.assertIn("CREATE TABLE IF NOT EXISTS zone_touch_outcome_participant", sql)
        self.assertIn("uq_zone_touch_operation_pid", sql)
        self.assertIn("zone_touch_outcome_operation_fk", sql)
        self.assertIn("zone_touch_participant_operation_fk", sql)

    def test_repository_source_contracts(self):
        source = (ROOT / "src/world/zone_touch_repository.c").read_text(encoding="utf-8")
        self.assertIn("zone_touch_outcome", source)
        self.assertIn("zone_touch_outcome_participant", source)
        self.assertIn("zone_touches", source)
        self.assertIn("#ifdef __NO_MYSQL__", source)

    def test_accounting_context_and_derivation_contracts(self):
        header = (ROOT / "src/world/zone_touch_command.h").read_text(encoding="utf-8")
        self.assertIn("struct zone_touch_accounting_context", header)
        self.assertIn("ZONE_TOUCH_AWARD_DERIVATION_DOMAIN", header)
        self.assertIn("zone_touch_derive_award_id", header)

        command_src = (ROOT / "src/world/zone_touch_command.c").read_text(encoding="utf-8")
        self.assertIn("zone_touch_derive_award_id", command_src)
        self.assertIn("ZONE_TOUCH_AWARD_DERIVATION_DOMAIN", command_src)

    def test_compilation_and_execution(self):
        with tempfile.TemporaryDirectory(prefix="duris-zone-touch-test-") as directory:
            binary = Path(directory) / "zone_touch_test"
            subprocess.run([
                *shlex.split(os.environ.get("CXX", "g++")), "-std=c++20", "-O1", "-g",
                "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie",
                "-D__NO_MYSQL__", "-Isrc/no_mysql", "-Isrc",
                "tests/async/zone_touch_repository_test.cpp",
                "src/world/zone_touch_command.c",
                "src/world/zone_touch_repository.c",
                "src/world/epic_command.c",
                "src/persistence/critical_command.c",
                "-lcrypto",
                "-o", str(binary),
            ], cwd=ROOT, check=True)
            subprocess.run([str(binary)], check=True, timeout=30, env=dict(
                os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1"))


if __name__ == "__main__":
    unittest.main()
