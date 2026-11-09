#!/usr/bin/env python3
"""RAM corpse metadata and atomic resurrection decisions."""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


class CorpseRamTests(unittest.TestCase):
    def test_metadata_and_resurrection(self):
        with tempfile.TemporaryDirectory() as scratch:
            binary = Path(scratch) / "corpse_ram"
            sources = [
                "tests/async/corpse_ram_test.cpp", "src/persistence/corpse_ram.c",
                "src/persistence/corpse_lifecycle_command.c", "src/economy/collector_ram.c",
                "src/economy/collector_runtime.c", "src/economy/collector_policy.c",
                "src/economy/collector_codec.c", "src/economy/collector_command.c",
                "src/economy/economic_state.c", "src/economy/currency_command.c",
                "src/item/item_ownership_runtime.c", "src/item/item_transfer_command.c",
                "src/player/player_snapshot_codec.c", "src/persistence/critical_command.c",
                "src/player/artifact_restitution_ram.c",
                "src/persistence/player_death_restitution_command.c",
            ]
            subprocess.run([
                os.environ.get("CXX", "g++"), "-std=c++20", "-Wall", "-Wextra", "-Werror",
                "-g", "-O1", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
                f"-I{ROOT / 'src'}", "-I/usr/include/mysql",
                *[str(ROOT / source) for source in sources], "-lcrypto", "-o", str(binary),
            ], check=True)
            subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    unittest.main()
