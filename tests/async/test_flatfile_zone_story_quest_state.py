#!/usr/bin/env python3

import pathlib
import subprocess
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[2]

with tempfile.TemporaryDirectory(prefix="duris-zone-story-flatfile-") as temporary:
    binary = pathlib.Path(temporary) / "flatfile_zone_story_quest_state_test"
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Wpedantic",
            "-Werror",
            "-D__NO_MYSQL__",
            "-Isrc",
            "-Isrc/no_mysql",
            "tests/async/flatfile_zone_story_quest_state_harness.cpp",
            "src/flatfile/flatfile_store.c",
            "src/flatfile/flatfile_zone_story_quest_state.c",
            "-lcrypto",
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        check=True,
    )
    subprocess.run([str(binary)], cwd=ROOT, check=True)

print("zone-story flat-file state runtime regression passed")
