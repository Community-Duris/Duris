#!/usr/bin/env python3

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
            "src/flatfile_item_repository.c",
            "src/flatfile_corpse_repository.c",
            "src/flatfile_shop_trade_repository.c",
            "src/flatfile_shop_trade_materialization.c",
            "src/flatfile_locker_repository.c",
            "src/flatfile_world_item_repository.c",
            "src/flatfile_artifact_repository.c",
            "src/flatfile_shopkeeper_repository.c",
            "src/flatfile_auction_repository.c",
            "src/flatfile_boon_repository.c",
            "src/flatfile_player_domain_repository.c",
            "src/flatfile_ip_activity_repository.c",
            "src/flatfile_authority_transaction.c",
            "src/flatfile_store.c",
            "src/player_snapshot_codec.c",
            "src/item_transfer_command.c",
            "src/corpse_lifecycle_command.c",
            "src/shop_trade_command.c",
            "src/critical_command.c",
            "src/epic_command.c",
            "src/currency_command.c",
            "src/auction_command.c",
            "src/combat_outcome_command.c",
            "src/boon_reward_command.c",
            "src/boon_shop_command.c",
            "src/persistence_mode.c",
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
