#!/usr/bin/env python3
"""Regression contract: enhancement policy belongs to the enhance module."""
from _paths import SRC
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
header = (SRC / "enhance.h").read_text()
enhance = (SRC / "enhance.c").read_text()
fight = (SRC / "fight.c").read_text()
db = (SRC / "db.c").read_text()
comm = (SRC / "comm.c").read_text()

for symbol in (
    "boot_enhancement_system(void)",
    "enhance_on_eligible_npc_death(P_char ch, P_char killer)",
    "enhance_on_npc_item_reset_skipped(P_char mob, P_obj missing_item)",
):
    assert symbol in header, f"missing narrow enhancement API: {symbol}"

assert "void boot_enhancement_system(void)" in enhance
assert "void enhance_on_eligible_npc_death(P_char ch, P_char killer)" in enhance
assert "void enhance_on_npc_item_reset_skipped(P_char mob, P_obj missing_item)" in enhance

assert "enhancematload(" not in fight, "fight.c should use the enhancement death event API"
assert "enhance_on_eligible_npc_death(ch, killer);" in fight
assert "load_npc_missing_item_material" not in db
assert "enhance_stat_npc_material_fallback_enabled" not in db
assert "get_matstart(missing_item)" not in db
assert db.count("enhance_on_npc_item_reset_skipped(mob, obj);") == 2

assert "load_enhance_config();" not in comm
assert "load_enhance_index();" not in comm
assert "boot_enhancement_system();" in comm

# Reloading config must be fail-closed for allow-list masks rather than retaining
# permissions from an older file.
reload_start = enhance.index("void load_enhance_config(void)")
reload = enhance[reload_start:enhance.index("bool is_enhance_banned", reload_start)]
for mask in ("enhance_allow_mask", "enhance_allow_mask2", "enhance_allow_mask3", "enhance_allow_mask4", "enhance_allow_mask5"):
    assert f"{mask} = 0;" in reload

print("enhancement module boundary contract passed")
