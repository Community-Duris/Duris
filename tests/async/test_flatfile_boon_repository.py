#!/usr/bin/env python3

import pathlib
import subprocess
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[2]

with tempfile.TemporaryDirectory(prefix="duris-flat-boon-") as temporary:
    temporary_path = pathlib.Path(temporary)
    binary = temporary_path / "flatfile_boon_test"
    sources = [
        "tests/async/flatfile_boon_repository_harness.cpp",
        "src/flatfile_boon_repository.c",
        "src/flatfile_player_domain_repository.c",
        "src/flatfile_authority_transaction.c",
        "src/flatfile_store.c",
        "src/boon_reward_command.c",
        "src/boon_shop_command.c",
        "src/epic_command.c",
        "src/currency_command.c",
        "src/combat_outcome_command.c",
        "src/critical_command.c",
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

dispatcher = (ROOT / "src/flatfile_item_repository.c").read_text()
if "return flatfile_boon_repository_apply(root, command);" not in dispatcher:
    raise SystemExit("flat critical dispatcher does not route boon rewards")
if "return flatfile_boon_shop_repository_apply(root, command);" not in dispatcher:
    raise SystemExit("flat critical dispatcher does not route boon shop purchases")
