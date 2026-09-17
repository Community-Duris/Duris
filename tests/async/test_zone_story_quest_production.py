#!/usr/bin/env python3

import pathlib
import subprocess
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[2]

with tempfile.TemporaryDirectory(prefix="duris-zone-story-quest-production-") as temporary:
    binary = pathlib.Path(temporary) / "zone_story_quest_production_test"
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Wpedantic",
            "-Werror",
            "-D__NO_MYSQL__",
            "-DTEST_MUD",
            "-D__NO_TESTS__",
            "-Isrc",
            "tests/async/zone_story_quest_production_harness.cpp",
            "src/world/zone_story_quest_tracking.c",
            "src/world/zone_story_quest_catalog.c",
            "src/world/zone_story_quest_production.c",
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        check=True,
    )
    subprocess.run([str(binary)], cwd=ROOT, check=True)

print("zone-story runtime production catalog regression passed")
