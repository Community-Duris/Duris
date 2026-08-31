#!/usr/bin/env python3

from _paths import rel
import pathlib
import subprocess
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[2]

with tempfile.TemporaryDirectory(prefix="duris-flat-shopkeeper-") as temporary:
    temporary_path = pathlib.Path(temporary)
    binary = temporary_path / "flatfile_shopkeeper_test"
    compile_result = subprocess.run(
        [
            "g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-Isrc",
            "tests/async/flatfile_shopkeeper_repository_harness.cpp",
            rel("flatfile_shopkeeper_repository.c"), rel("player_snapshot_codec.c"),
            rel("shop_trade_command.c"), rel("item_transfer_command.c"),
            rel("currency_command.c"), rel("critical_command.c"),
            rel("flatfile_authority_transaction.c"), rel("flatfile_store.c"),
            "-lcrypto", "-pthread", "-o", str(binary),
        ],
        cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
    )
    if compile_result.returncode:
        raise SystemExit(compile_result.stdout)
    run_result = subprocess.run(
        [str(binary), str(temporary_path / "state")], cwd=ROOT, text=True,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
    )
    if run_result.returncode:
        raise SystemExit(run_result.stdout)
    print(run_result.stdout.strip())
