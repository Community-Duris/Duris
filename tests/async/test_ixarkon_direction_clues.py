#!/usr/bin/env python3
"""Ixarkon's route descriptions must agree with reciprocal native exits."""
import argparse
from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser()
parser.add_argument("--world", type=Path, default=ROOT / "areas/wld/ixarkon.wld")
args = parser.parse_args()
rooms = {int(m[1]): m[2] for m in re.finditer(
    r"^#(\d+)\s*\n([\s\S]*?)(?=^#\d+\s*$|\Z)",
    args.world.read_text(encoding="utf8"), re.M)}


def exits(room):
    return {int(m[1]): (int(m[4]), int(m[5]), int(m[6])) for m in re.finditer(
        r"\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)",
        rooms[room], re.S)}


# Each clue identifies a real exit and the matching way back. Prose-only repairs
# preserve all direction numbers, destinations, flags, keys and reset commands.
cases = [
    (96426, "pens to the west", 3, 96421, 1),
    (96427, "pens to the west", 3, 96422, 1),
    (96448, "continues to the south and west", 2, 96447, 0),
    (96453, "western wall", 3, 96451, 1),
    (96489, "small depression to the north", 0, 96490, 2),
    (96491, "cavern floor to the east", 1, 96490, 3),
    (96492, "depression in the cavern to the south", 2, 96490, 0),
    (96495, "eastern wall", 1, 96475, 3),
    (96512, "tunnel continues to the west", 3, 96511, 1),
    (96531, "dwelling is directly to the west", 3, 96530, 1),
    (96531, "cavern continues to the east", 1, 96532, 3),
    (96600, "cavern continues back to the southwest", 7, 96470, 8),
]
failures = []
for room, clue, direction, target, reverse in cases:
    prose = " ".join(re.sub(r"&(?:\+[A-Za-z]|[A-Za-z0-9])", "",
                           rooms[room].split("~")[1]).split())
    if clue not in prose:
        failures.append(f"{room}: missing corrected direction clue {clue!r}")
    route = exits(room).get(direction)
    back = exits(target).get(reverse)
    if route is None or back is None or route[2] != target or back[2] != room:
        failures.append(f"{room}: clue lacks reciprocal route {direction}->{target}")
    elif route[:2] != back[:2]:
        failures.append(f"{room}: reciprocal door flags or keys disagree")
assert not failures, "\n".join(failures)
print(f"Ixarkon: {len(cases)} direction clues match reciprocal exits in 11 rooms.")
