#!/usr/bin/env python3
"""Native source capture on an explicitly owned disposable loopback schema."""
import argparse
import os
from pathlib import Path
import re
import shlex
import subprocess
import sys
import tempfile
from unittest import mock
import uuid

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--client-free-only", action="store_true",
                    help="run refusal/normalization checks without connecting to a database")
options = parser.parse_args()
if not options.client_free_only and (
    os.environ.get("ECONOMIC_ACCOUNTING_DISPOSABLE_SCHEMA") != "1" or
    os.environ.get("DB_HOST") != "127.0.0.1" or os.environ.get("DB_SOCKET") or
    not re.fullmatch(r"economic_schema_test_[A-Za-z0-9_]+", os.environ.get("DB_NAME", ""))
):
    raise SystemExit("explicit disposable loopback schema required")

SOURCES = ["tests/async/economic_sql_source_snapshot_test.cpp",
           "src/persistence/economic_sql_source_snapshot.c",
           "src/economy/economic_sql_source_normalize.c",
           "src/economy/economic_accounting_types.c",
           "src/item/item_transfer_command.c", "src/persistence/critical_command.c",
           "src/player/player_snapshot_codec.c"]


def run_mode(mode, environment, directory):
    binary = directory / mode
    flags = ["g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O1", "-g",
             "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie", "-Isrc"]
    if mode == "sql":
        flags += shlex.split(subprocess.check_output(["mysql_config", "--cflags"], text=True))
        flags += ["-Wl,--wrap=mysql_real_query,--wrap=mysql_errno,--wrap=_Znwm,--wrap=_Znam"]
    else:
        flags += ["-D__NO_MYSQL__", "-Isrc/no_mysql"]
    flags += SOURCES
    if mode == "sql":
        flags += shlex.split(subprocess.check_output(["mysql_config", "--libs"], text=True))
    subprocess.run(flags + ["-lcrypto", "-lz", "-pthread", "-o", str(binary)], cwd=ROOT, check=True)
    subprocess.run([str(binary)], cwd=ROOT,
                   env=dict(environment, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                            UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1"),
                   check=True, timeout=600)
    print(mode + " source snapshot passed", flush=True)


with tempfile.TemporaryDirectory(prefix="duris-sql-sources-") as temporary:
    directory = Path(temporary)
    if options.client_free_only:
        run_mode("client-free", os.environ, directory)
    else:
        sys.path.insert(0, str(ROOT / "migrations"))
        from verify_economy_accounting_schema import Client
        admin = Client()
        # The supplied guarded schema name need not exist. The administrative
        # session creates only this runner's random database below.
        assert admin.args[-1] == os.environ["DB_NAME"]
        admin.args = admin.args[:-1]
        name = "economic_schema_test_sources_" + uuid.uuid4().hex
        admin.sql("CREATE DATABASE `" + name + "` CHARACTER SET utf8mb4;")
        try:
            with mock.patch.dict(os.environ, {"DB_NAME": name}):
                client = Client()
            boot = subprocess.run(client.args,
                                  input=(ROOT / "migrations/bootstrap_multithread_safe.sql").read_text(),
                                  text=True, capture_output=True, env=client.env, timeout=120)
            if boot.returncode:
                raise RuntimeError("disposable source schema bootstrap failed")
            run_mode("sql", client.env, directory)
            run_mode("client-free", os.environ, directory)
        finally:
            assert re.fullmatch(r"economic_schema_test_sources_[0-9a-f]{32}", name)
            admin.sql("DROP DATABASE `" + name + "`;")
