#!/usr/bin/env python3
"""Executable auction bid, outbid, and buy-now accounting regressions."""

import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def main():
    with tempfile.TemporaryDirectory(prefix="auction-bid-accounting-") as directory:
        binary = Path(directory) / "auction-bid-accounting"
        subprocess.run([
            os.environ.get("CXX", "g++"), "-std=c++20", "-O1", "-g", "-Wall", "-Wextra", "-Wpedantic",
            "-Werror", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
            "-D__NO_MYSQL__", "-Isrc/no_mysql", "-Isrc",
            "tests/async/auction_bid_accounting_test.cpp",
            "src/economy/auction_accounting.c",
            "src/economy/auction_command.c",
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
