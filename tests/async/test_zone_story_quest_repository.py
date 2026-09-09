#!/usr/bin/env python3

import pathlib
import subprocess
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[2]

with tempfile.TemporaryDirectory(prefix="duris-zone-story-quest-repository-") as temporary:
    binary = pathlib.Path(temporary) / "zone_story_quest_repository_test"
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Wpedantic",
            "-Werror",
            "-Isrc",
            "tests/async/zone_story_quest_repository_harness.cpp",
            "src/world/zone_story_quest_tracking.c",
            "src/world/zone_story_quest_repository.c",
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        check=True,
    )
    subprocess.run([str(binary)], cwd=ROOT, check=True)

print("zone-story quest repository contract runtime regression passed")
