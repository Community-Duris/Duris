#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "src/mining.c").read_text()
header = (ROOT / "src/mining.h").read_text()
legacy = (ROOT / "src/tradeskill.c").read_text()
legacy_header = (ROOT / "src/tradeskill.h").read_text()
makefile = (ROOT / "src/Makefile").read_text()

required = (
    "initialize_mining",
    "mines_properties",
    "get_pick",
    "mine_friendly",
    "random_ore",
    "get_ore_from_mine",
    "get_gem_from_mine",
    "int mine(",
    "event_mine_check",
    "invalid_mine_room",
    "load_one_mine",
    "load_mines",
    "event_load_mines",
    "do_mine",
)
for symbol in required:
    assert symbol in source, symbol

for definition in (
    "int mines_properties(",
    "P_obj get_pick(",
    "bool mine_friendly(",
    "int random_ore(",
    "P_obj get_ore_from_mine(",
    "P_obj get_gem_from_mine(",
    "int mine(",
    "void event_mine_check(",
    "bool invalid_mine_room(",
    "bool load_one_mine(",
    "void load_mines(",
    "void event_load_mines(",
    "void do_mine(",
):
    assert definition not in legacy, definition

assert "initialize_mining();" in legacy
assert "mining.o" in makefile
assert '"mining.h"' in legacy_header
assert "void initialize_mining();" in header
assert "#define LOWEST_ORE_VNUM" in header
assert "#define MINES_MAP_SURFACE" in header
assert "#define LOWEST_ORE_VNUM" not in legacy_header
print("mining module extraction contract passed")
