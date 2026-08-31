#!/usr/bin/env python3

from _paths import SRC, rel
import pathlib
import subprocess
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[2]

with tempfile.TemporaryDirectory(prefix="duris-flat-boon-") as temporary:
    temporary_path = pathlib.Path(temporary)
    binary = temporary_path / "flatfile_boon_test"
    sources = [
        "tests/async/flatfile_boon_repository_harness.cpp",
        rel("flatfile_boon_repository.c"),
        rel("flatfile_player_domain_repository.c"),
        rel("flatfile_authority_transaction.c"),
        rel("flatfile_store.c"),
        rel("boon_reward_command.c"),
        rel("boon_shop_command.c"),
        rel("epic_command.c"),
        rel("currency_command.c"),
        rel("combat_outcome_command.c"),
        rel("critical_command.c"),
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

dispatcher = (SRC / "flatfile_item_repository.c").read_text()
if "return flatfile_boon_repository_apply(root, command);" not in dispatcher:
    raise SystemExit("flat critical dispatcher does not route boon rewards")
if "return flatfile_boon_shop_repository_apply(root, command);" not in dispatcher:
    raise SystemExit("flat critical dispatcher does not route boon shop purchases")
