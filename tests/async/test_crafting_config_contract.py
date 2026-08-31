#!/usr/bin/env python3
"""Crafting balance must be boot-loaded from a documented config."""
from _paths import SRC
from pathlib import Path
from contract_text import contains

root = Path(__file__).resolve().parents[2]
source = (SRC / "crafting.c").read_text()
header = (SRC / "crafting.h").read_text()
comm = (SRC / "comm.c").read_text()
config = root / "lib/crafting.cfg"
config_text = config.read_text()
assert config.exists(), "missing lib/crafting.cfg"
assert contains(source, 'fopen("lib/crafting.cfg", "r")')
assert contains(source, "crafting.level.gate.multiplier")
assert contains(source, "crafting.material.quantity.multiplier")
assert contains(source, "crafting.experience.per.ival")
assert "crafting.experience.per.ival=1000" in config_text
assert "Commands accept either form" in config_text
assert contains(source, "i + 1, recipes[i]")
assert contains(source, "selected = recipes[choice2 - 1]")
assert contains(source, "objVnum = recipes[objVnum - 1]")
assert contains(source, "crafting.recipe.max.player.level")
assert contains(source, "crafting.recipe.examine.materials")
assert "crafting.recipe.max.player.level=56" in config_text
assert "crafting.recipe.examine.materials=1" in config_text
assert contains(source, "Discipline consumables")
assert contains(source, "is_salvageable(item)")
assert contains(source, "crafting.salvage.scientific.tools.vnum")
assert contains(header, "int crafting_scientific_tools_vnum(void);")
assert contains(header, "int crafting_level_gate_multiplier(void);")
assert contains(comm, "boot_crafting_system();")
print("crafting config contract passed")
