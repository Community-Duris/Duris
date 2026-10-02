#!/usr/bin/env python3
"""Real native offerings award two checklist entries and one bonus across restart."""
from pathlib import Path
import sys

import test_static_quest_reward_journey as journey

if len(sys.argv) != 2:
    raise SystemExit("Usage: test_discovered_zone_daily_journey.py <built-flatfile-server>")
journey.journey.build_inspector()
journey.run(Path(sys.argv[1]).resolve(), daily=True)
print("discovery, two native daily turn-ins, one bonus, and cold reconnect passed")
