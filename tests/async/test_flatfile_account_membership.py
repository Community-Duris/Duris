#!/usr/bin/env python3

import pathlib
import subprocess
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[2]

with tempfile.TemporaryDirectory(prefix="duris-flat-membership-test-") as temporary:
    temporary_path = pathlib.Path(temporary)
    binary = temporary_path / "flatfile_membership_test"
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
            "tests/async/flatfile_account_membership_harness.cpp",
            "src/flatfile_account_adapter.c",
            "src/flatfile_account_repository.c",
            "src/flatfile_identity_repository.c",
            "src/flatfile_authority_transaction.c",
            "src/flatfile_store.c",
            "src/persistence_mode.c",
            "src/flatfile_ip_activity_repository.c",
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
