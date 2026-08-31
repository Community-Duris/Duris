#!/usr/bin/env python3

from _paths import SRC, rel
import pathlib
import subprocess
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[2]


with tempfile.TemporaryDirectory(prefix="duris-flatfile-polls-") as temporary:
    temporary_path = pathlib.Path(temporary)
    binary = temporary_path / "flatfile_polls_test"
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-D__NO_MYSQL__",
            "-ffunction-sections",
            "-fdata-sections",
            "-Isrc/no_mysql",
            "-Isrc",
            "tests/async/flatfile_poll_harness.cpp",
            rel("poll.c"),
            rel("flatfile_store.c"),
            "-Wl,--gc-sections",
            "-lcrypto",
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        check=True,
    )
    subprocess.run([str(binary), str(temporary_path / "state")], check=True)

poll_source = (SRC / "poll.c").read_text()
websocket_source = (SRC / "ws_handlers.c").read_text()
assert "Database not available" not in websocket_source
assert "return record_flat_poll_votes(acct_name, char_name, poll_id, choices);" in poll_source
assert "return create_flat_poll(poll);" in poll_source
assert "close_flat_poll(poll_id);" in poll_source
assert "expire_flat_polls();" in poll_source

preprocessed = subprocess.run(
    [
        "g++",
        "-std=c++20",
        "-D__NO_MYSQL__",
        "-Isrc/no_mysql",
        "-Isrc",
        "-E",
        "-P",
        rel("poll.c"),
    ],
    cwd=ROOT,
    check=True,
    text=True,
    stdout=subprocess.PIPE,
).stdout
for sql_fragment in (
    "SELECT id FROM poll_votes",
    "SELECT id, question, created_by",
    "INSERT INTO polls",
    "UPDATE polls SET is_active",
    "INSERT IGNORE INTO poll_votes",
):
    assert sql_fragment not in preprocessed, f"client-free poll path retained SQL: {sql_fragment}"

print("flat-file poll regression passed")
