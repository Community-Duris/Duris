"""Contracts for the Plane of Life paladin's one-time racewar reward."""

from _paths import SRC
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SOURCE = (SRC / "specs.mobile.c").read_text()

paladin = SOURCE.split("int newbie_paladin(", 1)[1]
paladin = paladin.split("int Malevolence(", 1)[0]

assert "VOBJ_NEWBIE2_SWORD_BLESSED" in paladin
assert paladin.count("obj_to_char(sword, pl)") == 1

# New characters already receive their complete starter kit before entering the
# world. The paladin must not promise or attempt a second kit after handing out
# the unique Plane of Life sword.
assert "load_obj_to_newbies" not in paladin
assert "gives a lot of stuff" not in paladin
assert "items crafted by slaves" not in paladin
