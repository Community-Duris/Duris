#!/usr/bin/env python3
"""Inactive accounting blocks journals/dailies while native rewards survive restart."""
from pathlib import Path
import sys

import test_static_quest_reward_journey as journey

if len(sys.argv) != 2:
    raise SystemExit("Usage: test_discovered_zone_daily_journey.py <built-flatfile-server>")
journey.journey.build_inspector()
journey.run(Path(sys.argv[1]).resolve(), daily=True, inactive_accounting=True)
print("inactive accounting gate, native reward persistence, and cold reconnect passed")
