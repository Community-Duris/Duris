#!/usr/bin/env python3

import pathlib
import subprocess
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[2]

with tempfile.TemporaryDirectory(prefix="duris-flat-character-delete-test-") as temporary:
    temporary_path = pathlib.Path(temporary)
    binary = temporary_path / "flatfile_character_delete_test"
    sources = [
        "tests/async/flatfile_character_delete_harness.cpp",
        "src/flatfile_character_delete.c",
        "src/flatfile_artifact_repository.c",
        "src/flatfile_association_repository.c",
        "src/flatfile_frag_leaderboard_repository.c",
        "src/flatfile_player_repository.c",
        "src/flatfile_identity_repository.c",
        "src/flatfile_item_repository.c",
        "src/flatfile_locker_repository.c",
        "src/flatfile_auction_repository.c",
        "src/flatfile_boon_repository.c",
        "src/flatfile_offline_message_repository.c",
        "src/flatfile_player_domain_repository.c",
        "src/flatfile_recipe_repository.c",
        "src/flatfile_spellbook_repository.c",
        "src/flatfile_ship_repository.c",
        "src/flatfile_world_item_repository.c",
        "src/flatfile_authority_transaction.c",
        "src/player_snapshot_codec.c",
        "src/flatfile_store.c",
        "src/item_transfer_command.c",
        "src/critical_command.c",
        "src/epic_command.c",
        "src/currency_command.c",
        "src/auction_command.c",
        "src/combat_outcome_command.c",
        "src/boon_reward_command.c",
        "src/boon_shop_command.c",
        "src/persistence_observability.c",
        "src/persistence_mode.c",
    ]
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
