#!/usr/bin/env python3

import pathlib
import subprocess
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[2]

with tempfile.TemporaryDirectory(prefix="duris-flat-player-test-") as temporary:
    temporary_path = pathlib.Path(temporary)
    binary = temporary_path / "flatfile_player_test"
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
            "tests/async/flatfile_player_repository_harness.cpp",
            "src/flatfile_player_repository.c",
            "src/flatfile_identity_repository.c",
            "src/flatfile_item_repository.c",
            "src/flatfile_player_domain_repository.c",
            "src/flatfile_authority_transaction.c",
            "src/player_snapshot_codec.c",
            "src/flatfile_store.c",
            "src/item_transfer_command.c",
            "src/critical_command.c",
            "src/epic_command.c",
            "src/currency_command.c",
            "src/combat_outcome_command.c",
            "src/persistence_observability.c",
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
