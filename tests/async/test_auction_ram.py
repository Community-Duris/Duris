#!/usr/bin/env python3
"""Native offline-wallet and atomic RAM auction decision regressions."""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


class RamAuctionTests(unittest.TestCase):
    def test_runtime_decisions(self):
        with tempfile.TemporaryDirectory() as scratch:
            binary = Path(scratch) / "auction_ram"
            sources = [
                "tests/async/auction_ram_test.cpp",
                "src/economy/economic_state.c",
                "src/economy/auction_ram.c",
                "src/economy/auction_ram_runtime.c",
                "src/economy/auction_website.c",
                "src/economy/auction_command.c",
                "src/economy/currency_command.c",
                "src/persistence/critical_command.c",
                "src/item/item_transfer_command.c",
                "src/player/player_snapshot_codec.c",
            ]
            subprocess.run([
                os.environ.get("CXX", "g++"), "-std=c++20", "-Wall", "-Wextra", "-Werror",
                "-g", "-O1", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
                f"-I{ROOT / 'src'}", *[str(ROOT / name) for name in sources],
                "-lcrypto", "-o", str(binary),
            ], check=True)
            subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    unittest.main()
