#!/usr/bin/env python3
"""Real native offerings award two checklist entries and one bonus across restart."""
from pathlib import Path
import subprocess
import sys

import test_static_quest_reward_journey as journey

ROOT = Path(__file__).resolve().parents[2]
if len(sys.argv) != 2:
    raise SystemExit("Usage: test_discovered_zone_daily_journey.py <built-flatfile-server>")
subprocess.run(["python3", "tests/async/test_flatfile_player_repository.py", "--build-inspector",
                str(journey.journey.INSPECTOR)], cwd=ROOT, check=True, timeout=180)
journey.run(Path(sys.argv[1]).resolve(), daily=True)
print("discovery, two native daily turn-ins, one bonus, and cold reconnect passed")
