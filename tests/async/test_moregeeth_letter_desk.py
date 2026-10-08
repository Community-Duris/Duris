#!/usr/bin/env python3
"""Ungal's trapped letter desk must permit normal attempts to open its lock."""
import argparse
from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser()
parser.add_argument("--objects", type=Path, default=ROOT / "areas/obj/goblinht.obj")
args = parser.parse_args()


def bodies(path):
    return {int(m[1]): m[2] for m in re.finditer(
        r"^#(\d+)\s*\n([\s\S]*?)(?=^#\d+\s*$|\Z)",
        path.read_text(encoding="utf8"), re.M)}


objects = bodies(args.objects)
desk = objects[70064]
raw = desk.split("~", 4)[4].strip().splitlines()
header, values, economy = [list(map(int, line.split())) for line in raw[:3]]
# CONT_CLOSEABLE=1, CONT_CLOSED=4, CONT_LOCKED=8, CONT_PICKPROOF=16.
assert header == [15, 15, 4, 0, 7, 0, 10, 0, 0, 0, 0]
assert values[1] == 13 and not values[1] & 16
assert values[2] >= 0, "Letter desk has no keyhole: native PICK and KNOCK reject it"
assert values == [10, 13, 0, 0, 0, 0, 0, 0]
assert economy == [100, 10000, 100]
assert "T\n516 4 1 25" in desk, "Letter desk lost its opening trap"
resets = (ROOT / "areas/zon/goblinht.zon").read_text(encoding="utf8")
assert re.search(r"^O 0 70064 1 70212 100 0 0 0\b", resets, re.M)
assert re.search(r"^P 1 70065 1 70064 100 0 0 0\b", resets, re.M)
quests = bodies(ROOT / "areas/qst/goblinht.qst")
assert re.search(r"R I 70066\s+G I 70065\s+S", quests[70060])
assert "Dear Ungal:" in objects[70065] and "Dura" in objects[70065]
# Keep the guards responsible for interpreting the repaired prototype explicit.
for path, marker in [("src/cmd/actmove.c", "else if (obj->value[2] < 0)"),
                     ("src/magic/spell_item_enhancement.c",
                      "else if (found_obj->value[2] < 0)")]:
    assert marker in (ROOT / path).read_text(encoding="utf8")
print("Moregeeth: locked trapped desk permits normal PICK/KNOCK attempts; letter source and Tala's exact exchange retained.")
