#!/usr/bin/env python3

import pathlib
import subprocess
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[2]

with tempfile.TemporaryDirectory(prefix="duris-flat-offline-") as temporary:
    temporary_path = pathlib.Path(temporary)
    binary = temporary_path / "flatfile_offline_test"
    sources = [
        "tests/async/flatfile_offline_message_repository_harness.cpp",
        "src/flatfile_offline_message_repository.c",
        "src/flatfile_authority_transaction.c",
        "src/flatfile_store.c",
    ]
    compile_result = subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Wpedantic",
            "-Werror",
            "-Isrc",
            *sources,
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

sql_source = (ROOT / "src/sql.c").read_text()
no_mysql = sql_source[
    sql_source.index("#ifdef __NO_MYSQL__") : sql_source.index(
        "#else", sql_source.index("#ifdef __NO_MYSQL__")
    )
]
for token in (
    "flatfile_offline_message_enqueue",
    "flatfile_offline_message_list",
    "flatfile_offline_message_acknowledge",
):
    if token not in no_mysql:
        raise SystemExit(f"client-free offline route is missing {token}")
if "void send_to_pid_offline(const char * /*msg*/" in no_mysql:
    raise SystemExit("client-free offline message route is still a no-op")
