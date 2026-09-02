#!/usr/bin/env python3
"""Exercise the flat-file half of src/kingdom/kingdom_db.c.

The MariaDB half is reached by the server's own boot; the __NO_MYSQL__ half
was, until this test, compiled by CI and executed by nothing. The harness
links kingdom_db.c against the real flatfile_store.c under a temporary root
and drives save, load, delete and flush through corruption and merge cases.
Modelled on test_flatfile_nexus_repository.py.
"""

from _paths import SRC, rel
import pathlib
import subprocess
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[2]

with tempfile.TemporaryDirectory(prefix="duris-flat-kingdom-") as temporary:
    temporary_path = pathlib.Path(temporary)
    binary = temporary_path / "flatfile_kingdom_test"
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
            "tests/async/flatfile_kingdom_repository_harness.cpp",
            rel("kingdom_db.c"),
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

# The client-free build must reach the flat store and nothing else: a MariaDB
# call surviving preprocessing here would be a link error in production, and a
# missing flat call would mean the half above was never the half that ran.
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
        rel("kingdom_db.c"),
    ],
    cwd=ROOT,
    text=True,
    stdout=subprocess.PIPE,
    stderr=subprocess.STDOUT,
)
if preprocess.returncode:
    raise SystemExit(preprocess.stdout)
for token in (
    "flatfile_read(",
    "flatfile_atomic_write(",
    "flatfile_lock_acquire(",
    "flatfile_lock_release(",
    "persistence_mode_flatfile_root()",
    "bool kingdom_db_load_all(void)",
    "bool kingdom_db_save_realm(const kingdom_realm &realm)",
    "bool kingdom_db_delete_realm(int assoc_id)",
    "void kingdom_db_flush_dirty(void)",
):
    if token not in preprocess.stdout:
        raise SystemExit(f"client-free kingdom store is missing {token}")
for database_call in (
    "FROM kingdom_realms",
    "INSERT INTO kingdom_realms",
    "DELETE FROM kingdom_realms",
    "mysql_fetch_row(",
    "mysql_store_result(",
    "db_query(",
):
    if database_call in preprocess.stdout:
        raise SystemExit(f"client-free kingdom store retained database call: {database_call}")

source = (SRC / "kingdom_db.c").read_text()
for database_call in (
    "SELECT %s FROM kingdom_realms ORDER BY assoc_id",
    "INSERT INTO kingdom_realms (%s) VALUES ",
    "DELETE FROM kingdom_realms WHERE assoc_id=%d",
):
    if database_call not in source:
        raise SystemExit(f"MariaDB kingdom behavior was removed: {database_call}")

makefile = (SRC / "Makefile").read_text()
if "kingdom/kingdom_db.o" not in makefile:
    raise SystemExit("kingdom_db is not linked into the server")
