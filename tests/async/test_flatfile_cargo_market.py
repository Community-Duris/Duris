#!/usr/bin/env python3

from _paths import SRC, rel
import pathlib
import subprocess
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[2]


with tempfile.TemporaryDirectory(prefix="duris-flat-cargo-") as temporary:
    temporary_path = pathlib.Path(temporary)
    binary = temporary_path / "flatfile_cargo_market_test"
    compile_result = subprocess.run(
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
            "-Isrc/ships",
            "-I/usr/include/libxml2",
            "tests/async/flatfile_cargo_market_harness.cpp",
            rel("ship_cargo.c"),
            rel("flatfile_store.c"),
            "-Wl,--gc-sections",
            "-lcrypto",
            "-pthread",
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
        [str(binary), str(temporary_path / "fixtures")],
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
        rel("ship_cargo.c"),
    ],
    cwd=ROOT,
    check=True,
    text=True,
    stdout=subprocess.PIPE,
).stdout
live_market_path = preprocess[preprocess.index("int read_cargo()") :]
for token in (
    "load_flat_cargo(&record, &error)",
    "save_flat_cargo(capture_flat_cargo(), &error)",
    "flatfile_cargo_maintenance_apply(const int64_t *values, size_t count)",
    'fatal_boot_error("ship_cargo", "flat cargo market authority could not be loaded")',
    "Reloading cargo mods from persistent storage",
    "Writing cargo mods to persistent storage",
):
    if token not in preprocess:
        raise SystemExit(f"client-free cargo route is missing {token}")
for database_call in (
    'qry("select type, port_id, cargo_type, modifier from ship_cargo_market_mods")',
    'qry("delete from ship_cargo_market_mods; delete from ship_cargo_prices;")',
    "sql_begin_transaction()",
    "sql_commit()",
    "sql_rollback()",
):
    if database_call in live_market_path:
        raise SystemExit(f"client-free cargo route retained database call: {database_call}")

maintenance = subprocess.run(
    [
        "g++",
        "-std=c++20",
        "-D__NO_MYSQL__",
        "-Isrc/no_mysql",
        "-Isrc",
        "-Isrc/ships",
        "-E",
        "-P",
        rel("maintenance_repository.c"),
    ],
    cwd=ROOT,
    check=True,
    text=True,
    stdout=subprocess.PIPE,
).stdout
execute = maintenance[maintenance.index("maintenance_result maintenance_repository_execute") :]
flat_apply = execute.index("flatfile_cargo_maintenance_apply(request.values.data()")
pool_acquire = execute.index("MYSQL *connection = sql_pool_acquire()")
if flat_apply > pool_acquire:
    raise SystemExit("client-free cargo maintenance reaches the SQL pool before flat persistence")

database_source = (SRC / "ships/ship_cargo.c").read_text()
for sql_token in (
    "select type, port_id, cargo_type, modifier from ship_cargo_market_mods",
    "delete from ship_cargo_market_mods; delete from ship_cargo_prices;",
):
    if sql_token not in database_source:
        raise SystemExit(f"MariaDB cargo behavior lost {sql_token}")
