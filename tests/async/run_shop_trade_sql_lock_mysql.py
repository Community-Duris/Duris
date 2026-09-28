#!/usr/bin/env python3
"""Compile and run the inactive shop SQL authority fixture on a disposable schema."""

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
    "src/player/player_snapshot_codec.c",
    "src/persistence/economic_accounting_repository.c",
    "src/economy/shop_trade_accounting.c",
    "src/economy/shop_trade_command.c",
    "src/economy/currency_command.c",
    "src/economy/economic_accounting_intent.c",
    "src/economy/economic_accounting_plan.c",
    "src/economy/economic_accounting_types.c",
    "src/item/item_transfer_command.c",
    "src/persistence/critical_command.c",
]
common = ["g++-12", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
          "-O1", "-g", "-Isrc"]

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
    flags += ["tests/async/shop_trade_sql_lock_mysql_harness.cpp"] + sources
    flags += shlex.split(subprocess.check_output(["mysql_config", "--libs"], text=True))
    subprocess.run(flags + ["-lcrypto", "-o", str(executable)], cwd=ROOT, check=True)
    subprocess.run([str(executable)], cwd=ROOT, check=True)
else:
    raise SystemExit("usage: run_shop_trade_sql_lock_mysql.py [--client-free-only]")
