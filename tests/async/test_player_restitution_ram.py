#!/usr/bin/env python3
"""Atomic RAM death restitution, including artifact authority and lifetime."""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


class RestitutionRamTests(unittest.TestCase):
    def test_restoration_publication(self):
        with tempfile.TemporaryDirectory() as scratch:
            binary = Path(scratch) / "restitution_ram"
            sources = [
                "tests/async/player_restitution_ram_test.cpp", "src/player/player_restitution_ram.c", "src/persistence/player_death_restitution_command.c", "src/player/player_checkpoint_ram.c",
                "src/player/player_checkpoint_command.c", "src/player/player_snapshot_codec.c",
                "src/item/item_ownership_runtime.c", "src/item/item_transfer_command.c",
                "src/economy/collector_command.c", "src/persistence/corpse_lifecycle_command.c",
                "src/economy/collector_codec.c", "src/economy/collector_policy.c", "src/economy/currency_command.c",
                "src/persistence/critical_command.c",
                "src/player/artifact_restitution_ram.c",
            ]
            subprocess.run([
                os.environ.get("CXX", "g++"), "-std=c++20", "-Wall", "-Wextra", "-Werror",
                "-g", "-O1", "-Wno-missing-field-initializers", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
                f"-I{ROOT / 'src'}", "-I/usr/include/mysql",
                *[str(ROOT / source) for source in sources], "-lcrypto", "-o", str(binary),
            ], check=True)
            subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    unittest.main()
