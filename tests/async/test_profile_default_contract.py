#!/usr/bin/env python3
"""Keep the high-overhead legacy profiler opt-in rather than boot-enabled."""
from pathlib import Path

from contract_text import contains

ROOT = Path(__file__).resolve().parents[2]
events = (ROOT / "src/new_events.c").read_text()
debug = (ROOT / "src/debug.c").read_text()

assert contains(events, "bool do_profile = FALSE;")
assert not contains(events, "bool do_profile = TRUE;")
assert contains(debug, "do_profile = true;")
assert contains(debug, "do_profile = false;")

print("legacy profiler opt-in contract passed")
