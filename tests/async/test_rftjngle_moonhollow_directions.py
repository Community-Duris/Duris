#!/usr/bin/env python3
"""Check Moonhollow's route descriptions against the real reciprocal exits."""

from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "areas/wld/rftjngle.wld").read_text(encoding="utf8")
rooms = {
    int(m[1]): m[2]
    for m in re.finditer(r"^#(\d+)\s*\n(.*?)(?=^#\d+|^\$|\Z)", source, re.M | re.S)
}
for room, direction, destination, reverse, phrase in (
    (80325, 2, 80326, 0, "south into an opening in the cliff face"),
    (80327, 3, 80326, 1, "open doorway to the west that leads back to the guardroom"),
    (80335, 1, 80336, 3, "open doorway to the east is framed"),
    (80337, 2, 80336, 0, "back into the armory to the south"),
    (80346, 0, 80347, 2, "small elven home to the north requires"),
):
    assert re.search(rf"\bD{direction}\s+[^~]*~[^~]*~\s+[01] 0 {destination}\b", rooms[room], re.S), room
    assert re.search(rf"\bD{reverse}\s+[^~]*~[^~]*~\s+[01] 0 {room}\b", rooms[destination], re.S), destination
    prose = " ".join(rooms[room].split("~")[1].split())
    assert phrase in prose, (room, "prose disagrees with the real route", phrase)

print("PASS: five Moonhollow route descriptions match actual reciprocal exits.")
