#!/usr/bin/env python3

import pathlib
import subprocess
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[2]


with tempfile.TemporaryDirectory(prefix="duris-flatfile-timers-") as temporary:
    temporary_path = pathlib.Path(temporary)
    binary = temporary_path / "flatfile_timers_test"
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-D__NO_MYSQL__",
            "-Isrc/no_mysql",
            "-Isrc",
            "tests/async/flatfile_timers_harness.cpp",
            "src/timers.c",
            "src/flatfile_store.c",
            "-lcrypto",
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        check=True,
    )
    subprocess.run([str(binary), str(temporary_path / "state")], check=True)

print("flat-file timer regression passed")
