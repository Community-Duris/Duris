#!/usr/bin/env python3

from _paths import SRC, rel
import pathlib
import subprocess
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[2]

with tempfile.TemporaryDirectory(prefix="duris-item-codec-") as temporary:
    binary = pathlib.Path(temporary) / "player_item_snapshot_codec_test"
    compile_result = subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Wpedantic",
            "-Werror",
            "-Isrc",
            "tests/async/player_item_snapshot_codec_harness.cpp",
            rel("player_snapshot_codec.c"),
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
        [str(binary)],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if run_result.returncode:
        raise SystemExit(run_result.stdout)
    print(run_result.stdout.strip())

codec_source = (SRC / "player_snapshot_codec.c").read_text()
for token in (
    "encode_items(out, snapshot.items)",
    "decode_items(in, snapshot.items)",
    "player_item_snapshot_list_encode",
    "player_item_snapshot_list_decode",
):
    if token not in codec_source:
        raise SystemExit(f"shared item codec contract is missing {token}")
