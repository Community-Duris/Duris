#!/usr/bin/env python3

from _paths import rel
import pathlib
import subprocess
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[2]

with tempfile.TemporaryDirectory(prefix="duris-flat-item-repository-test-") as temporary:
    temporary_path = pathlib.Path(temporary)
    binary = temporary_path / "flatfile_item_repository_test"
    compile_result = subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Wpedantic",
            "-Werror",
            "-D__NO_MYSQL__",
            "-DDURIS_FLATFILE_AUTHORITY_FAULT_TEST",
            "-Isrc",
            "-Isrc/no_mysql",
            "tests/async/flatfile_item_repository_harness.cpp",
            rel("flatfile_item_repository.c"),
            rel("coin_transfer_command.c"),
            rel("flatfile_player_snapshot_file.c"),
            rel("flatfile_corpse_repository.c"),
            rel("flatfile_shop_trade_repository.c"),
            rel("flatfile_shop_trade_materialization.c"),
            rel("flatfile_locker_repository.c"),
            rel("flatfile_world_item_repository.c"),
            rel("flatfile_artifact_repository.c"),
            rel("flatfile_shopkeeper_repository.c"),
            rel("flatfile_auction_repository.c"),
            rel("flatfile_boon_repository.c"),
            rel("flatfile_player_domain_repository.c"),
            rel("flatfile_ip_activity_repository.c"),
            rel("flatfile_authority_transaction.c"),
            rel("flatfile_store.c"),
            rel("player_snapshot_codec.c"),
            rel("item_transfer_command.c"),
            rel("corpse_lifecycle_command.c"),
            rel("shop_trade_command.c"),
            rel("critical_command.c"),
            rel("epic_command.c"),
            rel("currency_command.c"),
            rel("auction_command.c"),
            rel("combat_outcome_command.c"),
            rel("boon_reward_command.c"),
            rel("boon_shop_command.c"),
            rel("persistence_mode.c"),
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

    state_root = temporary_path / "state"
    run_result = subprocess.run(
        [str(binary), str(state_root)],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if run_result.returncode:
        raise SystemExit(run_result.stdout)
    print(run_result.stdout.strip())
