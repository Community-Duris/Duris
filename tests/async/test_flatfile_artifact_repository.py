#!/usr/bin/env python3

import pathlib
import subprocess
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[2]

with tempfile.TemporaryDirectory(prefix="duris-flat-artifact-") as temporary:
    temporary_path = pathlib.Path(temporary)
    binary = temporary_path / "flatfile_artifact_test"
    compile_result = subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Wpedantic",
            "-Werror",
            "-Isrc",
            "tests/async/flatfile_artifact_repository_harness.cpp",
            "src/flatfile_artifact_repository.c",
            "src/player_snapshot_codec.c",
            "src/item_transfer_command.c",
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

    runtime_binary = temporary_path / "flatfile_artifact_bind_runtime_test"
    runtime_compile_result = subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Wpedantic",
            "-Werror",
            "-D__NO_MYSQL__",
            "-ffunction-sections",
            "-fdata-sections",
            "-Isrc/no_mysql",
            "-Isrc",
            "tests/async/flatfile_artifact_bind_runtime_harness.cpp",
            "src/sql.c",
            "src/flatfile_artifact_repository.c",
            "src/player_snapshot_codec.c",
            "src/item_transfer_command.c",
            "src/critical_command.c",
            "src/flatfile_authority_transaction.c",
            "src/flatfile_store.c",
            "-Wl,--gc-sections",
            "-lcrypto",
            "-pthread",
            "-o",
            str(runtime_binary),
        ],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if runtime_compile_result.returncode:
        raise SystemExit(runtime_compile_result.stdout)
    runtime_result = subprocess.run(
        [str(runtime_binary), str(temporary_path / "runtime-state")],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if runtime_result.returncode:
        raise SystemExit(runtime_result.stdout)
    print(runtime_result.stdout.strip())

    gameplay_binary = temporary_path / "flatfile_artifact_gameplay_runtime_test"
    gameplay_compile_result = subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Wpedantic",
            "-Werror",
            "-D__NO_MYSQL__",
            "-ffunction-sections",
            "-fdata-sections",
            "-Isrc/no_mysql",
            "-Isrc",
            "tests/async/flatfile_artifact_runtime_harness.cpp",
            "src/artifact.c",
            "src/flatfile_artifact_repository.c",
            "src/player_snapshot_codec.c",
            "src/item_transfer_command.c",
            "src/critical_command.c",
            "src/flatfile_authority_transaction.c",
            "src/flatfile_store.c",
            "-Wl,--gc-sections",
            "-lcrypto",
            "-pthread",
            "-o",
            str(gameplay_binary),
        ],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if gameplay_compile_result.returncode:
        raise SystemExit(gameplay_compile_result.stdout)
    gameplay_result = subprocess.run(
        [str(gameplay_binary), str(temporary_path / "gameplay-state")],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if gameplay_result.returncode:
        raise SystemExit(gameplay_result.stdout)
    print(gameplay_result.stdout.strip())

    artifact_source = (ROOT / "src/artifact.c").read_text()
    expiry_start = artifact_source.index("void event_artifact_check_poof_sql(")
    expiry_end = artifact_source.index("\nvoid event_artifact_wars_sql(", expiry_start)
    expiry_body = artifact_source[expiry_start:expiry_end]
    if "flatfile_artifact_find_next_expired(" not in expiry_body:
        raise AssertionError("client-free expiry event does not select from flat authority")
    if "flatfile_artifact_expire(" not in expiry_body:
        raise AssertionError("client-free expiry event does not clear flat authority")
