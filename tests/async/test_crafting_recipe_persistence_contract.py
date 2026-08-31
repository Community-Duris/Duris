#!/usr/bin/env python3
"""Craft and Forge must use one SQL-first recipe provider."""
from _paths import SRC
from pathlib import Path

root = Path(__file__).resolve().parents[2]
source = (SRC / "crafting.c").read_text()
header = (SRC / "crafting.h").read_text()
craft = (SRC / "actnew.c").read_text()
assert "int *crafting_get_player_recipes(P_char ch, int *count);" in header
assert "sql_get_player_recipes(GET_PID(ch), count)" in source
assert "sql_add_player_recipe(GET_PID(ch), recipe_vnum)" in source
assert "real_object(recipe_vnum) < 0" in source
assert "crafting_get_player_recipes(ch, &recipe_count)" in source
assert '"Players/Tradeskills/%c/%s.crafting"' not in craft
print("crafting recipe persistence contract passed")
