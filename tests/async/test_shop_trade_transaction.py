#!/usr/bin/env python3
"""Completion publication regression for composite shop trades."""

from _paths import SRC, rel
import pathlib
import subprocess
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[2]

COMM = (SRC / "comm.c").read_text()
NANNY = (SRC / "nanny.c").read_text()
if "shop_trade_transaction_handle_completions(completions, count);" not in COMM:
    raise SystemExit("main loop does not dispatch shop trade completions")
if NANNY.count("shop_trade_transaction_player_ready(") != 2:
    raise SystemExit("login/reconnect do not release retained shop trade completions")

with tempfile.TemporaryDirectory(prefix="duris-shop-trade-transaction-") as temporary:
    binary = pathlib.Path(temporary) / "shop_trade_transaction_test"
    sources = [
        "tests/async/shop_trade_transaction_harness.cpp",
        rel("shop_trade_transaction.c"),
        rel("shop_trade_command.c"),
        rel("item_transfer_command.c"),
        rel("currency_command.c"),
        rel("critical_command.c"),
    ]
    compiled = subprocess.run(
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
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if compiled.returncode:
        raise SystemExit(compiled.stdout)
    run = subprocess.run(
        [str(binary)],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if run.returncode:
        raise SystemExit(run.stdout)

print("shop trade completion publication passed")
