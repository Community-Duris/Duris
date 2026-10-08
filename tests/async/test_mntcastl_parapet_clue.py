#!/usr/bin/env python3
"""Du'Maathe's northwest parapet must agree with its title and wall roads."""
import argparse
from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser()
parser.add_argument("--world", type=Path, default=ROOT / "areas/wld/mntcastl.wld")
args = parser.parse_args()
rooms = {int(m[1]): m[2] for m in re.finditer(
    r"^#(\d+)\s*\n([\s\S]*?)(?=^#\d+\s*$|\Z)",
    args.world.read_text(encoding="utf8"), re.M)}


def exits(room):
    return {int(m[1]): (int(m[4]), int(m[5]), int(m[6])) for m in re.finditer(
        r"\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)",
        rooms[room], re.S)}


title, prose, _ = rooms[37248].split("~", 2)
assert "North Western Parapet" in title
assert "entry to the north western parapet" in " ".join(prose.split())
assert "south western parapet" not in prose
assert exits(37248) == {1: (0, 0, 37249), 2: (0, 0, 37247)}
assert exits(37249)[3] == (0, 0, 37248)
assert exits(37247)[0] == (0, 0, 37248)
assert "northern castle wall road" in re.sub(
    r"&(?:\+[A-Za-z]|[A-Za-z0-9])", "", rooms[37249]).lower()
assert "western castle wall road" in re.sub(
    r"&(?:\+[A-Za-z]|[A-Za-z0-9])", "", rooms[37247]).lower()
# The distinct southwestern corner must retain its correct original clue.
assert "South Western Parapet" in rooms[37238].split("~")[0]
assert "entry to the south western parapet" in rooms[37238].split("~")[1]
assert exits(37238) == {0: (0, 0, 37239), 1: (0, 0, 37237)}
print("Du'Maathe: northwest clue agrees with title and both reciprocal wall roads; southwestern clue retained.")
