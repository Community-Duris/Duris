#!/usr/bin/env python3
"""Regression contracts for verified legacy forge and modenhance defects."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
tradeskill = (root / "src/tradeskill.c").read_text()
forge_items = (root / "src/forge_items.c").read_text()
enhance = (root / "src/enhance.c").read_text()

assert "Price tiers follow ore count" in tradeskill
assert "sizeof(forge_prices) / sizeof(forge_prices[0])" in tradeskill
assert "obj->type = ITEM_ARMOR;" in tradeskill
assert "int minval = itemvalue(source) - enhance_material_ival_delta;" in enhance
assert "SUB_MONEY(ch, cost, 0);" in enhance  # `cost` must be the same amount gated and reported.
assert "cost = 20000;" in enhance
assert "cost = 100000;" in enhance
print("crafting and enhancement regression contract passed")
