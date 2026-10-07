#!/usr/bin/env python3
"""Build the guarded native settlement-to-money-claim qualification entrypoint."""
import argparse
import os
from pathlib import Path
import shlex
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def qualify_native_target_guard(executable):
    base = {"PATH": os.environ.get("PATH", ""), "TEST_DB_DISPOSABLE": "1",
        "ECONOMIC_ACCOUNTING_DISPOSABLE_SCHEMA": "1", "DB_HOST": "127.0.0.1", "DB_PORT": "3306",
        "DB_USER": "synthetic_guard", "DB_PASSWD": "synthetic_guard", "DB_NAME": "economic_schema_test_guard",
        "ASAN_OPTIONS": "detect_leaks=0:halt_on_error=1"}
    for changed in ({"TEST_DB_DISPOSABLE": "0"}, {"ECONOMIC_ACCOUNTING_DISPOSABLE_SCHEMA": "0"},
                    {"DB_HOST": "192.0.2.1"}, {"DB_SOCKET": "/tmp/unowned-socket"},
                    {"DB_NAME": "unowned_database"}, {"DB_PORT": "0"}):
        result = subprocess.run(["bash", "-c", 'ulimit -c 0 && ulimit -s 65536 && exec "$1"', "auction-target-guard", str(executable)],
            cwd=ROOT, env=dict(base, **changed), capture_output=True, timeout=10)
        if result.returncode == 0 or b"require_telemetry_disposable_target" not in result.stderr:
            raise AssertionError("native auction target guard did not refuse before connection")


def compile_auction(executable):
    executable = Path(executable).resolve()
    if not executable.is_relative_to((ROOT / "bin/tests").resolve()):
        raise ValueError("auction qualification binary must stay under bin/tests")
    executable.parent.mkdir(parents=True, exist_ok=True)
    files = ["tests/async/auction_settlement_sql_accounting_mysql_harness.cpp",
        "src/economy/auction_settlement_accounting.c", "src/economy/auction_item_claim_accounting.c",
        "src/economy/auction_money_claim_accounting.c", "src/economy/auction_command.c", "src/economy/auction_repository.c",
        "src/economy/currency_command.c", "src/economy/economic_accounting_types.c", "src/economy/economic_accounting_plan.c",
        "src/economy/economic_accounting_intent.c", "src/item/item_transfer_command.c", "src/item/craft_pouch_mutation.c",
        "src/combat/chaos_pouch_ledger.c", "src/player/player_snapshot_codec.c", "src/item/economic_accounting_item_reference.c",
        "src/persistence/economic_accounting_repository.c", "src/persistence/economic_sql_pending_claim_source.c",
        "src/persistence/economic_sql_auction_settlement_transaction.c", "src/persistence/economic_sql_auction_item_claim_transaction.c",
        "src/persistence/economic_sql_auction_money_claim_transaction.c", "src/persistence/critical_command.c"]
    command = [os.environ.get("CXX", "g++"), "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
        "-O1", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
        "-DDURIS_TELEMETRY_AUCTION_QUALIFICATION", "-Isrc"]
    command += shlex.split(subprocess.check_output(["mysql_config", "--cflags"], text=True))
    command += files + shlex.split(subprocess.check_output(["mysql_config", "--libs"], text=True))
    subprocess.run(command + ["-lcrypto", "-o", str(executable)], cwd=ROOT, check=True)
    qualify_native_target_guard(executable)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--compile-only", type=Path, required=True)
    compile_auction(parser.parse_args().compile_only)
