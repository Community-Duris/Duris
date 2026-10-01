#!/usr/bin/env python3
"""Run the auction listing accounting journey in a disposable SQL schema."""

import os
from pathlib import Path
import re
import shlex
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
if (
    os.environ.get("ECONOMIC_ACCOUNTING_DISPOSABLE_SCHEMA") != "1"
    or os.environ.get("DB_HOST") != "127.0.0.1"
    or os.environ.get("DB_SOCKET")
    or not re.fullmatch(r"economic_schema_test_[A-Za-z0-9_]+", os.environ.get("DB_NAME", ""))
):
    raise SystemExit("explicit disposable loopback schema required")

if sys.argv[1:] not in ([], ["--compile-only"]):
    raise SystemExit("usage: run_auction_listing_sql_accounting_mysql.py [--compile-only]")

work = ROOT / "bin/tests/economic-sql-auction-listing"
work.mkdir(parents=True, exist_ok=True)
executable = work / "auction-listing"
files = [
    "tests/async/auction_listing_sql_accounting_mysql_harness.cpp",
    "src/economy/auction_listing_accounting.c",
    "src/economy/auction_command.c",
    "src/economy/auction_repository.c",
    "src/economy/currency_command.c",
    "src/economy/economic_accounting_types.c",
    "src/economy/economic_accounting_plan.c",
    "src/economy/economic_accounting_intent.c",
    "src/item/item_transfer_command.c", "src/item/craft_pouch_mutation.c", "src/combat/chaos_pouch_ledger.c",
    "src/player/player_snapshot_codec.c",
    "src/item/economic_accounting_item_reference.c",
    "src/persistence/economic_accounting_repository.c",
    "src/persistence/economic_sql_auction_listing_transaction.c",
    "src/persistence/critical_command.c",
]
flags = [os.environ.get("CXX", "g++"), "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
         "-O1", "-g", "-Isrc"]
flags += shlex.split(subprocess.check_output(["mysql_config", "--cflags"], text=True))
flags += files + shlex.split(subprocess.check_output(["mysql_config", "--libs"], text=True))
subprocess.run(flags + ["-lcrypto", "-o", str(executable)], cwd=ROOT, check=True)
if not sys.argv[1:]:
    subprocess.run([str(executable)], cwd=ROOT, check=True)
