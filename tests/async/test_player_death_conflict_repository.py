#!/usr/bin/env python3
"""Compile the real repository; SQL execution requires a disposable fixture."""
from pathlib import Path
import os
import shlex
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


def compile_sql(output, repository_source=None, snapshot_source=None, extra_flags=(), harness_source=None):
    flags = shlex.split(subprocess.check_output(["mysql_config", "--cflags"], text=True))
    libs = shlex.split(subprocess.check_output(["mysql_config", "--libs"], text=True))
    subprocess.run([os.environ.get("CXX", "g++"), "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-pthread",
                    "-ffunction-sections", "-fdata-sections", "-Wl,--gc-sections", "-Isrc", *flags, *extra_flags,
                    str(harness_source or ROOT / "tests/async/player_death_conflict_repository_mysql_harness.cpp"),
                    str(repository_source or ROOT / "src/player/player_death_conflict_repository.c"),
                    str(snapshot_source or ROOT / "src/player/player_snapshot_repository.c"), "src/player/player_snapshot_codec.c",
                    "src/player/player_save_journal.c",
                    "src/sql/item_extra_descr_codec.c", "src/persistence/persistence_observability.c",
                    "src/persistence/economic_sql_lifecycle_guard.c",
                    *libs, "-lcrypto", "-o", str(output)], cwd=ROOT, check=True)


class DeathConflictRepository(unittest.TestCase):
    def test_flatfile_is_explicitly_unsupported(self):
        with tempfile.TemporaryDirectory(prefix="death-conflict-flat-") as directory:
            directory = Path(directory)
            source = directory / "probe.cpp"
            source.write_text('''#include "player/player_death_conflict_repository.h"
#include <cerrno>
int main() {
    player_snapshot request = {}, output = {}; output.pid = 999;
    critical_operation_id operation = {};
    std::vector<player_death_conflict_case> cases(1);
    return player_death_conflict_retain(nullptr,request).error_code != ENOTSUP ||
        player_death_conflict_read(nullptr,7,operation,&output).error_code != ENOTSUP || output.pid != 999 ||
        player_death_conflict_list(nullptr,7,0,&cases).error_code != ENOTSUP || cases.size() != 1;
}
''')
            binary = directory / "probe"
            subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-D__NO_MYSQL__",
                            "-Isrc", "-Isrc/no_mysql", str(source),
                            "src/player/player_death_conflict_repository.c", "-o", str(binary)], cwd=ROOT, check=True)
            subprocess.run([str(binary)], check=True)

    def test_mysql_compile_and_optional_disposable_runtime(self):
        with tempfile.TemporaryDirectory(prefix="death-conflict-sql-") as directory:
            binary = Path(directory) / "probe"
            compile_sql(binary)
            if os.environ.get("TEST_DB_DISPOSABLE") != "1":
                self.skipTest("SQL compiled; runtime NOT run: TEST_DB_DISPOSABLE=1 and a dedicated fixture are required")
            subprocess.run([str(binary)], cwd=ROOT, check=True)


if __name__ == "__main__":
    unittest.main()
