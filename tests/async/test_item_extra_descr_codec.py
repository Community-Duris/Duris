#!/usr/bin/env python3
"""Compile and execute the real item extra-description SQL codec."""

from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"

sql_player = (SRC / "sql/sql_player.c").read_text()
locker_async = (SRC / "persistence/locker_async.c").read_text()
player_load_repository = (SRC / "player/player_load_repository.c").read_text()
assert "sql_encode_item_extra_descr(ed->keyword, ed->description" in sql_player
assert "sql_encode_item_extra_descr(ed->keyword, ed->description" in locker_async
assert "la_esc(ed->description ? ed->description" not in locker_async
assert sql_player.count("sql_load_item_extra_descr_values(") == 4
assert "legacy_spellbook_corrupt" in sql_player
assert "sql_decode_stored_spellbook(" in sql_player
assert player_load_repository.count("append_loaded_extra_description(") == 3
assert "sql_item_extra_descr_is_spellbook_marker(keyword)" in player_load_repository
assert 'legacy_raw ? "SPELLBOOK" : keyword' in player_load_repository
assert 'legacy_raw ? "[]"' in player_load_repository

with tempfile.TemporaryDirectory(prefix="item-extra-descr-codec-") as tmp:
    binary = Path(tmp) / "item-extra-descr-codec"
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-D__NO_MYSQL__",
            f"-I{SRC}",
            str(SRC / "sql/item_extra_descr_codec.c"),
            str(ROOT / "tests/async/item_extra_descr_codec_harness.cpp"),
            "-o",
            str(binary),
        ],
        check=True,
        cwd=ROOT,
    )
    subprocess.run([str(binary)], check=True, cwd=ROOT)
