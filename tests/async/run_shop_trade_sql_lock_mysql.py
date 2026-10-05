#!/usr/bin/env python3
"""Compile and run shop SQL authority and transaction checks on a disposable schema."""

import os
from pathlib import Path
import re
import shlex
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
WORK = ROOT / "bin/tests/economic-sql-shop-lock"
WORK.mkdir(parents=True, exist_ok=True)

sources = [
    "src/persistence/economic_sql_shop_trade_transaction.c",
    "src/item/economic_accounting_item_reference.c",
    "src/player/player_snapshot_codec.c",
    "src/persistence/economic_accounting_repository.c",
    "src/economy/shop_trade_accounting.c",
    "src/economy/shop_trade_command.c",
    "src/economy/currency_command.c",
    "src/economy/economic_accounting_intent.c",
    "src/economy/economic_accounting_plan.c", "src/economy/economic_source_event.c",
    "src/economy/economic_accounting_types.c",
    "src/item/item_transfer_command.c", "src/item/craft_pouch_mutation.c", "src/combat/chaos_pouch_ledger.c",
    "src/persistence/critical_command.c",
]
root_sources = [
    "src/persistence/critical_command_repository.c",
    "src/persistence/economic_sql_bank_transaction.c",
    "src/persistence/economic_sql_collector_transaction.c",
    "src/persistence/economic_sql_item_transfer_transaction.c",
    "src/persistence/economic_sql_lifecycle_guard.c",
    "src/economy/economic_currency_adapter.c",
    "src/economy/auction_repository.c",
    "src/economy/auction_command.c",
    "src/economy/collector_accounting.c",
    "src/economy/collector_repository.c",
    "src/economy/collector_command.c",
    "src/economy/collector_codec.c",
    "src/economy/collector_policy.c",
    "src/economy/boon_reward_repository.c",
    "src/economy/boon_reward_command.c",
    "src/economy/coin_transfer_accounting.c",
    "src/economy/coin_transfer_command.c",
    "src/economy/item_transfer_accounting.c",
    "src/item/item_transfer_repository.c",
    "src/world/epic_command.c",
    "src/persistence/corpse_lifecycle_repository.c",
    "src/persistence/corpse_lifecycle_command.c",
    "src/persistence/player_death_restitution_repository.c",
    "src/persistence/player_death_restitution_command.c",
    "src/combat/combat_outcome_repository.c",
    "src/combat/combat_outcome_command.c",
    "src/guild/artifact_guild_repository.c",
    "src/guild/artifact_guild_command.c",
    "src/world/zone_touch_repository.c",
    "src/world/zone_touch_command.c",
    "src/account/session_audit_repository.c",
    "src/account/session_audit_command.c",
    "src/player/player_load_topology.c",
]
common = [os.environ.get("CXX", "g++"), "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
          "-O1", "-g", "-ffunction-sections", "-fdata-sections",
          "-Wl,--gc-sections", "-Isrc"]

if sys.argv[1:] == ["--client-free-only"]:
    executable = WORK / "shop-lock-client-free"
    subprocess.run(common + ["-D__NO_MYSQL__", "-Isrc/no_mysql",
                    "tests/async/shop_trade_sql_lock_no_mysql_test.cpp"] + sources +
                   ["-lcrypto", "-o", str(executable)], cwd=ROOT, check=True)
    subprocess.run([str(executable)], cwd=ROOT, check=True)
    print("client-free SQL shop lock refusal passed")
elif sys.argv[1:] == []:
    if (os.environ.get("ECONOMIC_ACCOUNTING_DISPOSABLE_SCHEMA") != "1" or
        os.environ.get("DB_HOST") != "127.0.0.1" or os.environ.get("DB_SOCKET") or
        not re.fullmatch(r"economic_schema_test_[A-Za-z0-9_]+",
                         os.environ.get("DB_NAME", ""))):
        raise SystemExit("explicit disposable loopback schema required")
    executable = WORK / "shop-lock-mysql"
    flags = common + shlex.split(subprocess.check_output(["mysql_config", "--cflags"],
                                                         text=True))
    flags += ["tests/async/shop_trade_sql_lock_mysql_harness.cpp"] + sources + root_sources
    flags += shlex.split(subprocess.check_output(["mysql_config", "--libs"], text=True))
    subprocess.run(flags + ["-lcrypto", "-o", str(executable)], cwd=ROOT, check=True)
    subprocess.run([str(executable)], cwd=ROOT, check=True)
else:
    raise SystemExit("usage: run_shop_trade_sql_lock_mysql.py [--client-free-only]")
