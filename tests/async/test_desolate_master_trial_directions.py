#!/usr/bin/env python3
"""Keep Desolate's trial directions aligned with placed switches and real exits."""

from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[2]


def records(kind):
    source = (ROOT / "areas" / kind / f"desolate.{kind}").read_text(encoding="utf8")
    return {
        int(match[1]): match[2]
        for match in re.finditer(r"^#(\d+)\s*\n(.*?)(?=^#\d+|^\$|\Z)", source, re.M | re.S)
    }


world, objects = records("wld"), records("obj")
resets = (ROOT / "areas/zon/desolate.zon").read_text(encoding="utf8")
directions = ("northern", "eastern", "southern", "western", "upper", "lower")
for room, button, destination in ((22311, 22247, 22312), (22313, 22236, 22314)):
    fields = objects[button].split("~")
    values = list(map(int, fields[4].strip().splitlines()[1].split()))
    assert values[0:2] == [270, room], (button, values)  # PUSH in the actual source room.
    direction = values[2]
    wall = directions[direction] + " wall"
    assert wall in fields[2] and wall in fields[5], (button, wall)
    assert wall in world[room].split("~")[1], (room, "room prose disagrees with the control", wall)
    assert re.search(rf"\bD{direction}\s+[^~]*~[^~]*~\s+8 0 {destination}\b", world[room], re.S)
    assert re.search(rf"^O 0 {button} \d+ {room} 100\b", resets, re.M), (button, "switch not placed")
    assert re.search(rf"^D 0 {room} {direction} 8 100\b", resets, re.M), (room, "blocked exit reset missing")

print("PASS: Desolate trial prose matches the placed PUSH controls and blocked progression exits.")
