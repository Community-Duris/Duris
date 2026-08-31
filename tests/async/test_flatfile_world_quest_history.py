#!/usr/bin/env python3

from _paths import rel
import pathlib
import subprocess
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[2]


with tempfile.TemporaryDirectory(prefix="duris-flatfile-world-quests-") as temporary:
    temporary_path = pathlib.Path(temporary)
    binary = temporary_path / "flatfile_world_quest_history_test"
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Wpedantic",
            "-Werror",
            "-D__NO_MYSQL__",
            "-ffunction-sections",
            "-fdata-sections",
            "-Isrc/no_mysql",
            "-Isrc",
            "tests/async/flatfile_world_quest_history_harness.cpp",
            rel("sql.c"),
            rel("flatfile_world_quest_history.c"),
            rel("flatfile_store.c"),
            "-Wl,--gc-sections",
            "-lcrypto",
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        check=True,
    )
    subprocess.run([str(binary), str(temporary_path / "state")], cwd=ROOT, check=True)

print("flat-file world-quest history runtime regression passed")
