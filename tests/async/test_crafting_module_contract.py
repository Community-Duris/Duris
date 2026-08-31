#!/usr/bin/env python3
"""Modern Craft and Forge must be dispatched by the dedicated module."""
from _paths import SRC
from pathlib import Path

root = Path(__file__).resolve().parents[2]
header = (SRC / "crafting.h").read_text()
source = (SRC / "crafting.c").read_text()
craft = (SRC / "actnew.c").read_text()
forge = (SRC / "tradeskill.c").read_text()
config = (root / "lib/crafting.cfg").read_text()

assert "crafting_build_plan" in source
assert "CRAFTING_MODE_CRAFT" in header
assert "CRAFTING_MODE_FORGE" in header
assert "crafting_handle_craft_command" in source
assert "crafting_handle_forge_command" in source
assert "crafting_handle_command(ch, CRAFTING_MODE_CRAFT, argument);" in craft
assert "crafting_handle_command(ch, CRAFTING_MODE_FORGE, argument);" in forge
assert "crafting.craft.enabled=1" in config
assert "crafting.forge.enabled=1" in config
assert "crafting_essence_vnum(CRAFTING_MODE_CRAFT)" in source
assert "crafting_tool_vnum(CRAFTING_MODE_FORGE)" in source
print("crafting module contract passed")
