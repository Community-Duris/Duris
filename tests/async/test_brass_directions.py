#!/usr/bin/env python3
"""Keep Brass's eastern Imix Avenue clue aligned with its native route."""

from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "areas/wld/brass.wld").read_text(encoding="utf8")
rooms = {
    int(m[1]): m[2]
    for m in re.finditer(r"^#(\d+)\s*\n(.*?)(?=^#\d+|^\$|\Z)", source, re.M | re.S)
}


def exit_to(room, direction, destination):
    return re.search(
        rf"\bD{direction}\s+([^~]*)~[^~]*~\s+\d+ -?\d+ {destination}\b",
        rooms[room],
        re.S,
    )


failures = []
for room, direction, destination, reverse, expected in (
    (139017, 1, 139016, 3, "Imix Ave runs to the east in that direction."),
):
    route = exit_to(room, direction, destination)
    assert route and exit_to(destination, reverse, room), f"missing reciprocal route at {room}"
    clue = re.sub(r"&(?:\+.|[nN])", "", route[1])
    if " ".join(clue.split()) != expected:
        failures.append(f"room {room}, D{direction}: exit clue disagrees with its actual route")
assert not failures, "\n".join(failures)

print("PASS: Brass's eastern Imix Avenue exit has the correct clue.")
