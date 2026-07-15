#!/usr/bin/env python3
"""Crafting balance must be boot-loaded from a documented config."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
source = (root / "src/crafting.c").read_text()
header = (root / "src/crafting.h").read_text()
comm = (root / "src/comm.c").read_text()
config = root / "lib/crafting.cfg"
config_text = config.read_text()
assert config.exists(), "missing lib/crafting.cfg"
assert 'fopen("lib/crafting.cfg", "r")' in source
assert "crafting.level.gate.multiplier" in source
assert "crafting.material.quantity.multiplier" in source
assert "crafting.experience.per.ival" in source
assert "crafting.experience.per.ival=1000" in config_text
assert "Commands accept either form" in config_text
assert "i + 1, recipes[i]" in source
assert "selected = recipes[choice2 - 1]" in source
assert "objVnum = recipes[objVnum - 1]" in source
assert "crafting.recipe.max.player.level" in source
assert "crafting.recipe.examine.materials" in source
assert "crafting.recipe.max.player.level=56" in config_text
assert "crafting.recipe.examine.materials=1" in config_text
assert "Discipline consumables" in source
assert "is_salvageable(item)" in source
assert "int crafting_level_gate_multiplier(void);" in header
assert "boot_crafting_system();" in comm
print("crafting config contract passed")
