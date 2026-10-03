#!/usr/bin/env python3
"""Execute the native bounded payload codec/admission contract without services."""
from pathlib import Path
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix="sql-room-item-payload-") as directory:
    binary = Path(directory) / "payload-test"
    flags = ["-std=c++20", "-Wall", "-Wextra", "-Werror", "-D__NO_MYSQL__",
             "-ffunction-sections", "-fdata-sections", "-Wl,--gc-sections"]
    if os.environ.get("DURIS_TEST_SANITIZERS") == "1":
        flags += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"]
    subprocess.run(["g++", *flags, f"-I{ROOT / 'src'}",
                    str(ROOT / "src/persistence/sql_room_item_payload.c"),
                    str(ROOT / "src/player/player_snapshot_codec.c"),
                    str(ROOT / "tests/async/sql_room_item_payload_test.cpp"), "-o", str(binary)],
                   cwd=ROOT, check=True)
    subprocess.run([str(binary)], cwd=ROOT, check=True)
