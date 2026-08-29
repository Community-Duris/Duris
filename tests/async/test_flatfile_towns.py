#!/usr/bin/env python3

import pathlib
import subprocess
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[2]


with tempfile.TemporaryDirectory(prefix="duris-flatfile-towns-") as temporary:
    temporary_path = pathlib.Path(temporary)
    binary = temporary_path / "flatfile_towns_test"
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-D__NO_MYSQL__",
            "-ffunction-sections",
            "-fdata-sections",
            "-Isrc/no_mysql",
            "-Isrc",
            "tests/async/flatfile_town_harness.cpp",
            "src/sql_player.c",
            "src/flatfile_store.c",
            "-Wl,--gc-sections",
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        check=True,
    )
    subprocess.run([str(binary), str(temporary_path / "fixtures")], check=True)

defaults = (ROOT / "defaults/towns").read_text().splitlines()
assert len(defaults) == 80
assert defaults[::8] == [
    "tharnadia",
    "charing",
    "ashrumite",
    "kimordril",
    "woodseer",
    "khildarak",
    "shady",
    "ghore",
    "faang",
    "goblinht",
]
assert defaults[1:8] == [
    "0 0 0",
    "FALSE",
    "132677 20 132618",
    "FALSE",
    "0 0 0",
    "FALSE",
    "0 0",
]

source = (ROOT / "src/siege.c").read_text()
assert "if (!sql_load_towns())" in source
assert "exit(EXIT_FAILURE);" in source

preprocessed = subprocess.run(
    [
        "g++",
        "-std=c++20",
        "-D__NO_MYSQL__",
        "-Isrc/no_mysql",
        "-Isrc",
        "-E",
        "-P",
        "src/sql_player.c",
    ],
    cwd=ROOT,
    check=True,
    text=True,
    stdout=subprocess.PIPE,
).stdout
for sql_fragment in (
    "DELETE FROM towns",
    "INSERT INTO towns",
    "SELECT zone_filename, resources, defense, offense",
):
    assert sql_fragment not in preprocessed, (
        f"client-free town path retained SQL: {sql_fragment}"
    )

print("flat-file town regression passed")
