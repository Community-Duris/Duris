#!/usr/bin/env python3
"""Boon progress and stat purchases publish only a sealed RAM outcome."""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


class RamBoonTests(unittest.TestCase):
    def test_runtime_decisions(self):
        with tempfile.TemporaryDirectory() as scratch:
            binary = Path(scratch) / "boon_ram"
            sources = [
                "tests/async/boon_ram_test.cpp", "src/economy/boon_ram.c",
                "src/economy/boon_reward_command.c", "src/economy/boon_shop_command.c",
                "src/persistence/critical_command.c", "src/player/player_snapshot_codec.c",
            ]
            subprocess.run([
                os.environ.get("CXX", "g++"), "-std=c++20", "-Wall", "-Wextra", "-Werror",
                "-g", "-O1", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
                f"-I{ROOT / 'src'}", "-I/usr/include/mysql",
                *[str(ROOT / name) for name in sources], "-lcrypto", "-o", str(binary),
            ], check=True)
            subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    unittest.main()
