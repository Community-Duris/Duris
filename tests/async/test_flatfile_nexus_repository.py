#!/usr/bin/env python3

from _paths import SRC, rel
import pathlib
import subprocess
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[2]

with tempfile.TemporaryDirectory(prefix="duris-flat-nexus-") as temporary:
    temporary_path = pathlib.Path(temporary)
    binary = temporary_path / "flatfile_nexus_test"
    compile_result = subprocess.run(
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
            "-Isrc/ships",
            "-I/usr/include/libxml2",
            "-ffunction-sections",
            "-fdata-sections",
            "tests/async/flatfile_nexus_repository_harness.cpp",
            rel("nexus_stones.c"),
            rel("flatfile_nexus_repository.c"),
            rel("flatfile_store.c"),
            "-lcrypto",
            "-pthread",
            "-Wl,--gc-sections",
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if compile_result.returncode:
        raise SystemExit(compile_result.stdout)
    run_result = subprocess.run(
        [str(binary), str(temporary_path / "state")],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if run_result.returncode:
        raise SystemExit(run_result.stdout)
    print(run_result.stdout.strip())

preprocess = subprocess.run(
    [
        "g++",
        "-std=c++20",
        "-D__NO_MYSQL__",
        "-Isrc/no_mysql",
        "-Isrc",
        "-Isrc/ships",
        "-I/usr/include/libxml2",
        "-E",
        "-P",
        rel("nexus_stones.c"),
    ],
    cwd=ROOT,
    text=True,
    stdout=subprocess.PIPE,
    stderr=subprocess.STDOUT,
)
if preprocess.returncode:
    raise SystemExit(preprocess.stdout)
for token in (
    'fatal_boot_error("nexus_stones", "flat nexus authority could not be loaded")',
    "flatfile_nexus_establish(root, {}, error)",
    "flatfile_nexus_list(root, records, error)",
    "flatfile_nexus_find(root, stone_id, record, error)",
    "flatfile_nexus_update_state(root, stone_id, align, time(nullptr), &error)",
    "bool nexus_stone_touch(P_obj stone, P_char ch)",
    "void nexus_stone_list(P_char ch)",
    "bool nexus_stone_expired(int stone_id)",
    "epic_transaction_submit(pl, -cost",
):
    if token not in preprocess.stdout:
        raise SystemExit(f"client-free nexus runtime route is missing {token}")
for database_call in (
    'SELECT id, name, room_vnum, align FROM nexus_stones',
    'SELECT name, room_vnum, align, stat_affect, affect_amount',
    'UPDATE nexus_stones SET align',
    "mysql_store_result(DB)",
):
    if database_call in preprocess.stdout:
        raise SystemExit(f"client-free nexus runtime retained database call: {database_call}")

source = (SRC / "nexus_stones.c").read_text()
for database_call in (
    'SELECT id, name, room_vnum, align FROM nexus_stones',
    'UPDATE nexus_stones SET align',
    'SELECT id, align FROM nexus_stones',
):
    if database_call not in source:
        raise SystemExit(f"MariaDB nexus behavior was removed: {database_call}")
for mapping in (
    "info->affect_amount = atoi(row[4]);",
    "info->last_touched_at = row[5] ? atoi(row[5]) : 0;",
):
    if mapping not in source:
        raise SystemExit(f"MariaDB nexus info mapping is incomplete: {mapping}")

makefile = (SRC / "Makefile").read_text()
if "flatfile_nexus_repository.o" not in makefile:
    raise SystemExit("flat nexus repository is not linked into the server")
