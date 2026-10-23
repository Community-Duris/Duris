#!/usr/bin/env python3
"""Exercise the native discovery adapter with the real service and a failing store."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix="duris-zone-arrival-") as temporary:
    binary = Path(temporary) / "arrival"
    subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-Isrc",
                    "-ffunction-sections", "-fdata-sections", "-DTEST_MUD", "-D__NO_TESTS__", "-D__NO_MYSQL__",
                    "tests/async/zone_story_quest_arrival_harness.cpp",
                    "src/world/zone_story_quest_feature.c", "src/world/zone_story_quest_catalog.c",
                    "src/world/zone_story_quest_tracking.c", "src/flatfile/flatfile_zone_story_quest_state.c",
                    "src/flatfile/flatfile_store.c", "src/flatfile/flatfile_authority_transaction.c",
                    "-lcrypto", "-Wl,--gc-sections", "-o", str(binary)],
                   cwd=ROOT, check=True)
    subprocess.run([str(binary)], cwd=ROOT, check=True)
