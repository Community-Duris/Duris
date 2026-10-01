#!/usr/bin/env python3
"""Executable shop cash, issuance, and custody accounting regressions."""

import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def main():
    work = ROOT / "bin/tests/economic-shop-trade"
    work.mkdir(parents=True, exist_ok=True)
    binary = work / "shop-trade-accounting"
    subprocess.run([
        os.environ.get("CXX", "g++"), "-std=c++20", "-O1", "-g", "-Wall", "-Wextra", "-Wpedantic",
        "-Werror", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
        "-D__NO_MYSQL__", "-Isrc/no_mysql", "-Isrc",
        "tests/async/shop_trade_typed_accounting_test.cpp",
        "src/economy/shop_trade_accounting.c",
        "src/economy/shop_trade_command.c",
        "src/economy/currency_command.c",
        "src/economy/economic_accounting_types.c",
        "src/economy/economic_accounting_plan.c",
        "src/economy/economic_accounting_intent.c",
        "src/item/item_transfer_command.c", "src/player/player_snapshot_codec.c",
        "src/persistence/critical_command.c",
        "-lcrypto", "-o", str(binary),
    ], cwd=ROOT, check=True)
    subprocess.run([str(binary)], cwd=ROOT, check=True, timeout=30)


if __name__ == "__main__":
    main()
