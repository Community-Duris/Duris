#!/usr/bin/env python3
"""Compile the real load/query/archive stack; run only on explicitly disposable SQL."""
from pathlib import Path
import os
import shlex
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


def compile_sql(binary, *, load_source=None, query_source=None, extra_flags=()):
    flags = shlex.split(subprocess.check_output(['mysql_config', '--cflags'], text=True))
    libs = shlex.split(subprocess.check_output(['mysql_config', '--libs'], text=True))
    subprocess.run([
        os.environ.get('CXX', 'g++-14'), '-std=c++20', '-Wall', '-Wextra', '-Wpedantic',
        '-Werror', '-pthread', '-ffunction-sections', '-fdata-sections', '-Isrc', *flags,
        *extra_flags, 'tests/async/player_death_recovery_mysql_harness.cpp',
        str(load_source or ROOT / 'src/player/player_load_repository.c'),
        str(query_source or ROOT / 'src/player/player_death_recovery_query.c'),
        'src/player/player_load_topology.c', 'src/player/player_death_conflict_repository.c',
        'src/persistence/critical_command.c',
        'src/persistence/quest_reward_obligation_repository.c',
        'src/item/item_transfer_command.c', "src/item/craft_pouch_mutation.c", "src/combat/chaos_pouch_ledger.c", 'src/economy/currency_command.c',
        'src/player/player_snapshot_repository.c', 'src/player/player_snapshot_codec.c',
        'src/player/player_save_journal.c',
        'src/sql/item_extra_descr_codec.c', 'src/persistence/player_death_restitution_command.c',
        'src/persistence/persistence_observability.c', 'src/persistence/economic_sql_lifecycle_guard.c',
        '-Wl,--gc-sections', '-Wl,--wrap=mysql_real_query', *libs, '-lcrypto', '-o', str(binary)
    ], cwd=ROOT, check=True)


class DeathRecoverySQL(unittest.TestCase):
    def test_real_stack_compile_and_optional_runtime(self):
        with tempfile.TemporaryDirectory(prefix='death-recovery-sql-') as directory:
            binary = Path(directory) / 'probe'
            compile_sql(binary)
            if os.environ.get('TEST_DB_DISPOSABLE') != '1':
                self.skipTest('SQL compiled; runtime NOT run without an explicit fresh disposable fixture')
            subprocess.run([str(binary), '--seed-and-check'], cwd=ROOT, check=True)


if __name__ == '__main__':
    unittest.main()
