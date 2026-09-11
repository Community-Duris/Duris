#!/usr/bin/env python3
"""Execute the production M-reset branch and batched SQL restore with fake world/DB I/O."""

from pathlib import Path
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
db = (ROOT / "src/world/db.c").read_text()
sql = (ROOT / "src/sql/sql_player.c").read_text()
helper_start = db.find("static bool room_has_shopkeeper(")
helper = "" if helper_start < 0 else db[helper_start:db.index("/* force_item_repop", helper_start)]
reset_start = db.index("case 'M': /* read a mobile */", db.index("void reset_zone("))
reset = db[reset_start:db.index("case 'O':", reset_start)]
restore_start = sql.index("// temp struct for batched shopkeeper loading")
restore = sql[restore_start:sql.index("void sql_save_dirty_shopkeepers(void)", restore_start)]
harness = (ROOT / "tests/async/shopkeeper_population_harness.cpp").read_text()
harness = harness.replace("// PRODUCTION_HELPER", helper)
harness = harness.replace("// PRODUCTION_RESET", reset)
harness = harness.replace("// PRODUCTION_RESTORE", restore)

build_root = ROOT / "bin/tests"
build_root.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix="shopkeeper-population-", dir=build_root) as temporary:
    source = Path(temporary) / "harness.cpp"
    binary = Path(temporary) / "harness"
    source.write_text(harness)
    subprocess.run([
        "g++", "-std=c++20", "-g", "-Wall", "-Wextra", "-Werror",
        "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie",
        str(source), "-o", str(binary),
    ], check=True)
    for scenario in ("reset", "shared", "duplicate", "invalid"):
        subprocess.run([str(binary), scenario], check=True, env={
            **os.environ, "ASAN_OPTIONS": "detect_leaks=1:halt_on_error=1",
        })
        print(f"shopkeeper population: {scenario} passed", flush=True)

assert "ORDER BY save_time DESC, id DESC" in restore, "duplicate rows must prefer the newest snapshot"
