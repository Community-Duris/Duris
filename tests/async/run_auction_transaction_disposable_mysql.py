#!/usr/bin/env python3
"""Execute the auction transaction mysql harness against a disposable container."""

import os
from pathlib import Path
import re
import shlex
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
WORK = ROOT / "bin/tests/auction-transaction-schema"
WORK.mkdir(parents=True, exist_ok=True)

sources = [
    "src/persistence/critical_command.c",
    "src/world/epic_command.c",
    "src/economy/currency_command.c",
    "src/item/item_transfer_command.c", "src/item/craft_pouch_mutation.c", "src/combat/chaos_pouch_ledger.c",
    "src/item/item_transfer_repository.c",
    "src/item/economic_accounting_item_reference.c",
    "src/economy/auction_command.c",
    "src/economy/auction_repository.c",
    "src/combat/combat_outcome_command.c",
    "src/combat/combat_outcome_repository.c",
    "src/guild/artifact_guild_command.c",
    "src/guild/artifact_guild_repository.c",
    "src/economy/boon_reward_command.c",
    "src/economy/boon_reward_repository.c",
    "src/world/zone_touch_command.c",
    "src/world/zone_touch_repository.c",
    "src/account/session_audit_command.c",
    "src/account/session_audit_repository.c",
    "src/item/item_uid_allocator.c",
    "src/flatfile/flatfile_item_uid_allocator.c",
    "src/flatfile/flatfile_store.c",
    "src/persistence/persistence_mode.c",
    "src/economy/coin_transfer_command.c",
    "src/player/player_snapshot_codec.c",
    "src/economy/collector_command.c",
    "src/economy/collector_codec.c",
    "src/economy/collector_policy.c",
    "src/economy/collector_accounting.c",
    "src/economy/collector_repository.c",
    "src/persistence/corpse_lifecycle_command.c",
    "src/persistence/corpse_lifecycle_repository.c",
    "src/persistence/player_death_restitution_command.c",
    "src/persistence/player_death_restitution_repository.c",
    "src/persistence/economic_accounting_repository.c",
    "src/persistence/economic_sql_bank_transaction.c",
    "src/persistence/economic_sql_item_transfer_transaction.c",
    "src/economy/economic_currency_adapter.c",
    "src/economy/item_transfer_accounting.c",
    "src/economy/coin_transfer_accounting.c",
    "src/economy/economic_accounting_types.c",
    "src/economy/economic_accounting_plan.c",
    "src/economy/economic_accounting_intent.c",
    "src/economy/economic_command_admission.c",
    "src/persistence/economic_sql_lifecycle_guard.c",
    "src/persistence/critical_command_repository.c",
    "src/persistence/critical_command_journal.c",
    "src/persistence/critical_command_coordinator.c",
    "src/persistence/economic_sql_collector_transaction.c",
]

compiler = os.environ.get("CXX", "g++-12")
common = [compiler, "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
          "-pthread", "-Isrc"]

if sys.argv[1:] == []:
    if (os.environ.get("ECONOMIC_ACCOUNTING_DISPOSABLE_SCHEMA") != "1" or
        os.environ.get("DB_HOST") != "127.0.0.1" or os.environ.get("DB_SOCKET") or
        not re.fullmatch(r"auction_schema_test_[A-Za-z0-9_]+",
                         os.environ.get("AUCTION_TEST_DB_NAME", ""))):
        raise SystemExit("explicit disposable loopback schema required (AUCTION_TEST_DB_NAME)")
    
    executable = WORK / "auction_transaction_mysql_harness"
    flags = common + shlex.split(subprocess.check_output(["mysql_config", "--cflags"], text=True))
    flags += ["tests/async/auction_transaction_mysql_harness.cpp"] + sources
    flags += shlex.split(subprocess.check_output(["mysql_config", "--libs"], text=True))
    flags += ["-lcrypto", "-lz", "-o", str(executable)]
    subprocess.run(flags, cwd=ROOT, check=True)
    subprocess.run([str(executable)], cwd=ROOT, check=True)
    print("auction listing, stale bid, replay, settlement, refund, money claim, and item claim checks passed")
else:
    raise SystemExit("usage: run_auction_transaction_disposable_mysql.py")
