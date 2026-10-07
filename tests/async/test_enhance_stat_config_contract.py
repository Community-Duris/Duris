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
assert "enhancement_prepare_superior_price(itemvalue(source), enhance_stat_platinum_base," in source
assert "enhance_stat_platinum_per_ival, &cost)" in source
prices = (SRC / "economy/enhancement_price.h").read_text()
assert "static_cast<int64_t>(base)" in prices
assert "static_cast<int64_t>(item_value) * per_value" in prices
assert "quote < 0 || quote > INT_MAX" in prices
print("superior stat config contract passed")
