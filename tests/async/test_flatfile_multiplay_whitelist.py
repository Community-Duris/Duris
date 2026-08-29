#!/usr/bin/env python3

import pathlib
import subprocess
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[2]


with tempfile.TemporaryDirectory(prefix="duris-flatfile-multiplay-whitelist-") as temporary:
    temporary_path = pathlib.Path(temporary)
    binary = temporary_path / "flatfile_multiplay_whitelist_test"
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
            "tests/async/flatfile_multiplay_whitelist_harness.cpp",
            "src/multiplay_whitelist.c",
            "src/flatfile_store.c",
            "-Wl,--gc-sections",
            "-lcrypto",
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        check=True,
    )
    subprocess.run([str(binary), str(temporary_path / "state")], check=True)

print("flat-file multiplay whitelist regression passed")
