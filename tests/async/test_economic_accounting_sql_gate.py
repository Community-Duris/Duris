#!/usr/bin/env python3
"""Test direct SQL entrypoint gates without a database or credentials."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[2]
work=ROOT/'bin/tests/economic-accounting-sql-gate';work.mkdir(parents=True,exist_ok=True)
with tempfile.TemporaryDirectory(prefix='run-',dir=work) as temporary:
    executable=Path(temporary)/'gate'
    command=shlex.split(os.environ.get('CXX','g++'))+[
        '-std=c++20','-Wall','-Wextra','-Wpedantic','-Werror','-O1','-g',
        '-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie',
        '-ffunction-sections','-fdata-sections','-Wl,--gc-sections',
        '-Wl,--wrap=mysql_server_init,--wrap=mysql_thread_init','-pthread','-I'+str(ROOT/'src')]
    command+=shlex.split(subprocess.check_output(['mysql_config','--cflags'],text=True))
    command += [str(ROOT/name) for name in (
        'tests/async/economic_accounting_sql_gate_test.cpp',
        'src/persistence/critical_command.c','src/persistence/economic_accounting_repository.c','src/persistence/economic_sql_bank_transaction.c','src/economy/economic_currency_adapter.c','src/economy/economic_accounting_types.c','src/economy/economic_accounting_plan.c','src/economy/economic_accounting_intent.c','src/persistence/critical_command_repository.c',
        'src/economy/currency_command.c','src/item/item_transfer_command.c','src/world/epic_command.c',
        'src/item/item_transfer_repository.c','src/player/player_snapshot_codec.c',
        'src/economy/auction_repository.c','src/economy/auction_command.c','src/economy/collector_repository.c','src/economy/collector_command.c','src/economy/boon_reward_repository.c','src/economy/boon_reward_command.c','src/persistence/corpse_lifecycle_repository.c','src/persistence/corpse_lifecycle_command.c','src/persistence/player_death_restitution_repository.c','src/persistence/player_death_restitution_command.c','src/combat/combat_outcome_repository.c','src/combat/combat_outcome_command.c','src/guild/artifact_guild_repository.c','src/guild/artifact_guild_command.c','src/world/zone_touch_repository.c','src/world/zone_touch_command.c','src/account/session_audit_repository.c','src/account/session_audit_command.c','src/economy/collector_codec.c','src/economy/collector_policy.c','src/economy/coin_transfer_command.c','src/player/player_load_topology.c')]
    command+=shlex.split(subprocess.check_output(['mysql_config','--libs'],text=True))
    command+=['-lcrypto','-o',str(executable)]
    subprocess.run(command,check=True)
    environment=dict(os.environ,ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
    subprocess.run([str(executable)],check=True,env=environment)
