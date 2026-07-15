#!/usr/bin/env python3
"""Modern Craft and Forge must share a config-backed execution owner."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
crafting_h = root / "src/crafting.h"
crafting_c = root / "src/crafting.c"
assert crafting_h.exists(), "missing src/crafting.h"
assert crafting_c.exists(), "missing src/crafting.c"
header = crafting_h.read_text()
source = crafting_c.read_text()
assert "void boot_crafting_system(void);" in header
assert "crafting_handle_command" in header
assert "crafting_build_plan" in source
assert "CRAFTING_MODE_CRAFT" in header
assert "CRAFTING_MODE_FORGE" in header
print("crafting module contract passed")
