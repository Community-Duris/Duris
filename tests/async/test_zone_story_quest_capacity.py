#!/usr/bin/env python3
"""Measure a bounded pilot, including retained completion/telemetry history."""
from pathlib import Path
import subprocess
import tempfile
import argparse

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--socket")
parser.add_argument("--database")
args = parser.parse_args()
if bool(args.socket) != bool(args.database) or (args.database and not args.database.startswith("zone_daily_")):
    raise SystemExit("SQL capacity tests require an explicit socket and disposable zone_daily_* database")
extra = []
if args.socket:
    extra = ["-DZSQ_TEST_SQL", "src/sql/zone_story_quest_state_repository.c", *subprocess.check_output(["mysql_config", "--cflags", "--libs"], text=True).split(), "-lz"]
with tempfile.TemporaryDirectory(prefix="duris-zone-capacity-") as temporary:
    binary = Path(temporary) / "capacity"
    subprocess.run(["g++", "-O2", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-Isrc",
                    "tests/async/zone_story_quest_capacity_harness.cpp", "src/world/zone_story_quest_feature.c",
                    "src/world/zone_story_quest_catalog.c", "src/world/zone_story_quest_tracking.c",
                    "src/flatfile/flatfile_zone_story_quest_state.c", "src/flatfile/flatfile_store.c",
                    "src/flatfile/flatfile_authority_transaction.c",
                    *extra, "-lcrypto", "-o", str(binary)], cwd=ROOT, check=True)
    subprocess.run([str(binary), str(Path(temporary) / "state"), *([args.socket, args.database] if args.socket else [])], cwd=ROOT, check=True)
