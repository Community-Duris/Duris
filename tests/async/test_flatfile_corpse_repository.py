#!/usr/bin/env python3

import pathlib
import subprocess
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[2]
DISPATCHER = (ROOT / "src/flatfile_item_repository.c").read_text()
for token in (
    "command.type == critical_command_type::corpse_lifecycle",
    "flatfile_corpse_repository_apply(root, command)",
):
    if token not in DISPATCHER:
        raise SystemExit(f"flat critical-command dispatcher is missing {token}")

with tempfile.TemporaryDirectory(prefix="duris-flat-corpse-") as temporary:
    temporary_path = pathlib.Path(temporary)
    binary = temporary_path / "flatfile_corpse_test"
    compile_result = subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Wpedantic",
            "-Werror",
            "-DDURIS_FLATFILE_AUTHORITY_FAULT_TEST",
            "-Isrc",
            "tests/async/flatfile_corpse_repository_harness.cpp",
            "src/flatfile_corpse_repository.c",
            "src/flatfile_world_item_repository.c",
            "src/corpse_lifecycle_command.c",
            "src/item_transfer_command.c",
            "src/player_snapshot_codec.c",
            "src/critical_command.c",
            "src/flatfile_authority_transaction.c",
            "src/flatfile_store.c",
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
