#!/usr/bin/env python3
"""Run only against an explicitly supplied disposable zone_daily_* database."""
from pathlib import Path
import argparse
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser()
parser.add_argument("--socket", required=True)
parser.add_argument("--database", required=True)
args = parser.parse_args()
if not args.database.startswith("zone_daily_"):
    raise SystemExit("This test requires a disposable zone_daily_* database")
with tempfile.TemporaryDirectory(prefix="duris-zone-sql-") as temporary:
    binary = Path(temporary) / "sql-store"
    flags = subprocess.check_output(["mysql_config", "--cflags", "--libs"], text=True).split()
    subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-Isrc",
                    "tests/async/zone_story_quest_sql_store_harness.cpp",
                    "src/sql/zone_story_quest_state_repository.c", "src/world/zone_story_quest_feature.c",
                    "src/world/zone_story_quest_catalog.c", "src/world/zone_story_quest_tracking.c",
                    *flags, "-lz", "-o", str(binary)],
                   cwd=ROOT, check=True)
    subprocess.run([str(binary), args.socket, args.database], cwd=ROOT, check=True)
