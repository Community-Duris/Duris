#!/usr/bin/env python3

from _paths import SRC, rel
import pathlib
import subprocess
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[2]

with tempfile.TemporaryDirectory(prefix="duris-flat-ship-") as temporary:
    temporary_path = pathlib.Path(temporary)
    binary = temporary_path / "flatfile_ship_test"
    compile_result = subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Wpedantic",
            "-Werror",
            "-Isrc",
            "tests/async/flatfile_ship_repository_harness.cpp",
            rel("flatfile_ship_repository.c"),
            rel("flatfile_authority_transaction.c"),
            rel("flatfile_store.c"),
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
        rel("ship_base.c"),
    ],
    cwd=ROOT,
    text=True,
    stdout=subprocess.PIPE,
    stderr=subprocess.STDOUT,
)
if preprocess.returncode:
    raise SystemExit(preprocess.stdout)
for token in (
    "flatfile_player_identity_pid(ship->ownername, &owner_pid, error)",
    "CAP(ship->ownername);",
    "const auto result = root ? flatfile_ship_remove(",
    'panic_corruption("shutdown_ships", "flat write_ship failed")',
    'fatal_boot_error("ship_base", "flat ship authority could not be loaded")',
    'flatfile_ship_upsert(root ? root : "", &record, &error)',
    "flatfile_ship_list(root, &records, &error)",
    'flatfile_ship_import_legacy(root, "Ships"',
    "flatfile_ship_establish(root, {}, &error)",
    "flat_ship_materialize(record, &error)",
):
    if token not in preprocess.stdout:
        raise SystemExit(f"client-free ship runtime route is missing {token}")

live_ship_path = preprocess.stdout[preprocess.stdout.index("void initialize_ships()") :]
for database_call in (
    "sql_begin_transaction()",
    "sql_commit()",
    "sql_rollback()",
    "sql_save_ship(ship)",
    "sql_load_all_ships()",
    "sql_delete_ship(ship->ownername)",
):
    if database_call in live_ship_path:
        raise SystemExit(f"client-free ship runtime retained database call: {database_call}")

files_source = (SRC / "files.c").read_text()
delete_start = files_source.index("int deleteCharacter(")
flat_start = files_source.index(
    "if (persistence_mode_get() == PERSISTENCE_MODE_FLATFILE_PRIMARY)", delete_start
)
flat_delete = files_source[
    flat_start : files_source.index("char *tmp;", flat_start)
]
if "delete_ship(GET_NAME(ch));" not in flat_delete:
    raise SystemExit("flat character deletion does not remove the committed live ship")
