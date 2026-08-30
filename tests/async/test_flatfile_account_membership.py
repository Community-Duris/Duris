#!/usr/bin/env python3

import pathlib
import subprocess
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[2]
ACCOUNT = (ROOT / "src/account.c").read_text()
ADAPTER = (ROOT / "src/flatfile_account_adapter.c").read_text()
FILES = (ROOT / "src/files.c").read_text()

assert "flatfile_account_state_release(loaded)" in ACCOUNT
assert "str_dup(loaded->acct_name" in ACCOUNT
assert "void flatfile_account_state_release" in ADAPTER
assert "c->level = GET_LEVEL(player);" in ACCOUNT
assert "c->race = GET_RACE(player);" in ACCOUNT
assert "c->m_class = player->player.m_class;" in ACCOUNT
assert "sync_account_character_projection(P_char player" in ACCOUNT
assert "sync_account_character_projection(ch, projection_room, TRUE)" in ACCOUNT
assert "world[ch->in_room].number" in ACCOUNT
assert "sync_account_character_projection(ch, room, TRUE)" in FILES

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
