#!/usr/bin/env python3
"""Configured superior-stat values must drive their documented mechanics."""
from _paths import SRC
from pathlib import Path

source = (SRC / "enhance.c").read_text()
for key in (
    "enhance_stat_cap_multiplier",
    "enhance_stat_platinum_base",
    "enhance_stat_platinum_per_ival",
    '"enhance_stat.cap.multiplier"',
    '"enhance_stat.platinum.base"',
    '"enhance_stat.platinum.per.ival"',
):
    assert key in source, key
assert "base_modifier * enhance_stat_cap_multiplier" in source
assert "static_cast<int64_t>(enhance_stat_platinum_base)" in source
assert "static_cast<int64_t>(itemvalue(source))" in source
assert "quoted_cost < 0 || quoted_cost > INT_MAX" in source
print("superior stat config contract passed")
