#!/usr/bin/env python3
"""Configured superior-stat values must drive their documented mechanics."""
from pathlib import Path

source = (Path(__file__).resolve().parents[2] / "src/enhance.c").read_text()
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
assert "enhance_stat_platinum_base + itemvalue(source) * enhance_stat_platinum_per_ival" in source
print("superior stat config contract passed")
