#!/usr/bin/env python3
"""Regression contracts for verified legacy forge and modenhance defects."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
tradeskill = (root / "src/tradeskill.c").read_text()
forge_items = (root / "src/forge_items.c").read_text()
enhance = (root / "src/enhance.c").read_text()
crafting = (root / "src/crafting.c").read_text()

assert "Price tiers follow ore count" in tradeskill
assert "sizeof(forge_prices) / sizeof(forge_prices[0])" in tradeskill
assert "obj->type = ITEM_ARMOR;" in tradeskill
assert "crafting_validate_recipe_target" in crafting
assert "ITEM2_QUESTITEM" in crafting
# The material ival floor must stay tunable rather than a hard-coded 5.  This
# pins the live computation in enhance(); modenhance() computed the same value
# but never compared anything against it, so its copy was removed as dead.
assert "minval = itemvalue(source) - enhance_material_ival_delta;" in enhance
assert "if (itemvalue(material) < minval)" in enhance
assert "SUB_MONEY(ch, cost, 0);" in enhance  # `cost` must be the same amount gated and reported.
assert "cost = 20000;" in enhance
assert "cost = 100000;" in enhance
print("crafting and enhancement regression contract passed")
