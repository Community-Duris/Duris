#!/usr/bin/env python3
"""Contract and validation tests for artifact_guild_repository."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


class ArtifactGuildRepositoryTest(unittest.TestCase):
    def test_schema_contracts(self):
        sql = (ROOT / "migrations/artifact_guild_outcome.sql").read_text(encoding="utf-8")
        self.assertIn("CREATE TABLE IF NOT EXISTS artifact_delta_ledger", sql)
        self.assertIn("CREATE TABLE IF NOT EXISTS guild_outcome_ledger", sql)
        self.assertIn("uq_artifact_delta_revision", sql)
        self.assertIn("uq_guild_outcome_revision", sql)

    def test_repository_source_contracts(self):
        source = (ROOT / "src/guild/artifact_guild_repository.c").read_text(encoding="utf-8")
        self.assertIn("artifact_delta_ledger", source)
        self.assertIn("guild_outcome_ledger", source)
        self.assertIn("artifact_guild_outcome", source)
        self.assertIn("#ifdef __NO_MYSQL__", source)

    def test_compilation_and_execution(self):
        with tempfile.TemporaryDirectory(prefix="duris-art-guild-test-") as directory:
            binary = Path(directory) / "art_guild_test"
            subprocess.run([
                *shlex.split(os.environ.get("CXX", "g++")), "-std=c++20", "-O1", "-g",
                "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie",
                "-D__NO_MYSQL__", "-Isrc/no_mysql", "-Isrc",
                "tests/async/artifact_guild_repository_test.cpp",
                "src/guild/artifact_guild_command.c",
                "src/guild/artifact_guild_repository.c",
                "src/persistence/critical_command.c",
                "-lcrypto",
                "-o", str(binary),
            ], cwd=ROOT, check=True)
            subprocess.run([str(binary)], check=True, timeout=30, env=dict(
                os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1"))


if __name__ == "__main__":
    unittest.main()
