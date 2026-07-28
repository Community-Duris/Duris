#!/usr/bin/env python3
"""NPC essence balance supports global level gates and sparse zone overrides."""
from pathlib import Path
root = Path(__file__).resolve().parents[2]
src = (root / "src/enhance.c").read_text()
cfg = (root / "lib/enhance.cfg").read_text()
for marker in (
    "enhance_essence_minimum_level",
    "enhance_essence_maximum_level",
    "enhance_essence_zone_rules",
    "enhance_find_essence_zone_rule",
    '"essence_drop_zone"',
    "ROOM_ZONE_NUMBER(ch->in_room)",
):
    assert marker in src, marker
for key in (
    "enhance.essence_drop.minimum_level=1",
    "enhance.essence_drop.maximum_level=1000000",
    "[essence_drop_zone]",
):
    assert key in cfg, key
print("essence drop balance rules contract passed")
