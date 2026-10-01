#!/usr/bin/env python3
"""Run the collector accounting transaction in an explicitly disposable schema."""

import os
from pathlib import Path
import re
import shlex
import subprocess
import sys
import tempfile


ROOT = Path(__file__).resolve().parents[2]
if (
    os.environ.get("ECONOMIC_ACCOUNTING_DISPOSABLE_SCHEMA") != "1"
    or os.environ.get("DB_HOST") != "127.0.0.1"
    or os.environ.get("DB_SOCKET")
    or not re.fullmatch(r"economic_schema_test_[A-Za-z0-9_]+", os.environ.get("DB_NAME", ""))
):
    raise SystemExit("explicit disposable loopback schema required")

legacy = (ROOT / "tests/async/run_collector_repository_schema_mysql.sh").read_text()
chunk = legacy.split("g++ -std=c++20", 1)[1].split(
    '"$ROOT/bin/tests/collector_repository_mysql_harness"', 1
)[0]
files = re.findall(r"(?:tests|src)/[A-Za-z0-9_/.-]+\.(?:cpp|c)", chunk)[1:]
files += [
    "tests/async/collector_sql_accounting_mysql_harness.cpp",
    "src/economy/collector_accounting.c",
    "src/persistence/economic_sql_collector_transaction.c",
    "src/item/economic_accounting_item_reference.c",
    "src/economy/coin_transfer_accounting.c",
    "src/economy/item_transfer_accounting.c",
    "src/persistence/economic_sql_item_transfer_transaction.c",
]
work = ROOT / "bin/tests/economic-sql-collector"
work.mkdir(parents=True, exist_ok=True)
compile_only = sys.argv[1:] == ["--compile-only"]
if sys.argv[1:] and not compile_only:
    raise SystemExit("usage: run_collector_sql_accounting_mysql.py [--compile-only]")
with tempfile.TemporaryDirectory(prefix="run-", dir=work) as temporary:
    executable = work / "collector" if compile_only else Path(temporary) / "collector"
    flags = [
        os.environ.get("CXX", "g++"),
        "-std=c++20",
        "-Wall",
        "-Wextra",
        "-Wpedantic",
        "-Werror",
        "-pthread",
        "-O1",
        "-g",
        "-Isrc",
    ]
    flags += shlex.split(subprocess.check_output(["mysql_config", "--cflags"], text=True))
    flags += files + shlex.split(
        subprocess.check_output(["mysql_config", "--libs"], text=True)
    )
    subprocess.run(flags + ["-lcrypto", "-o", str(executable)], cwd=ROOT, check=True)
    if not compile_only:
        subprocess.run([str(executable)], cwd=ROOT, check=True)
