#!/usr/bin/env python3

from _paths import SRC, rel
import pathlib
import subprocess
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[2]


with tempfile.TemporaryDirectory(prefix="duris-flatfile-siege-") as temporary:
    temporary_path = pathlib.Path(temporary)
    binary = temporary_path / "flatfile_siege_test"
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-D__NO_MYSQL__",
            "-DSIEGE_ENABLED",
            "-ffunction-sections",
            "-fdata-sections",
            "-Isrc/no_mysql",
            "-Isrc",
            "-Isrc/ships",
            "tests/async/flatfile_siege_harness.cpp",
            rel("flatfile_store.c"),
            rel("player_snapshot_codec.c"),
            "-Wl,--gc-sections",
            "-lcrypto",
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        check=True,
    )
    subprocess.run([str(binary), str(temporary_path / "fixtures")], check=True)

source = (SRC / "siege.c").read_text()
remove_start = source.index("void remove_siege")
remove_end = source.index("void save_siege_list", remove_start)
assert "save_siege_list();" in source[remove_start:remove_end]
assert "sql_save_siege_item(siege->obj, world[siege->obj->loc.room].number)" in source

preprocessed = subprocess.run(
    [
        "g++",
        "-std=c++20",
        "-D__NO_MYSQL__",
        "-DSIEGE_ENABLED",
        "-Isrc/no_mysql",
        "-Isrc",
        "-Isrc/ships",
        "-E",
        "-P",
        rel("siege.c"),
    ],
    cwd=ROOT,
    check=True,
    text=True,
    stdout=subprocess.PIPE,
).stdout
live_siege_path = preprocessed[preprocessed.rindex("void save_siege_list()") :]
for database_call in (
    "sql_begin_transaction()",
    "sql_save_siege_list()",
    "sql_save_siege_item(",
    "sql_load_siege_list()",
):
    assert database_call not in live_siege_path, (
        f"client-free siege path retained database call: {database_call}"
    )

print("flat-file siege regression passed")
