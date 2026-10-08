#!/usr/bin/env python3
"""Keep three Tribal Forest exit clues aligned with reciprocal native routes."""

from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "areas/wld/tribal.wld").read_text(encoding="utf8")
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
    (42204, 3, 42203, 1, "A dark forest continues to the west."),
    (42231, 3, 42232, 1, "A tiny path leads west into a dense forest."),
    (42261, 2, 42260, 0, "A village square continues to the south."),
):
    route = exit_to(room, direction, destination)
    assert route and exit_to(destination, reverse, room), f"missing reciprocal route at {room}"
    if " ".join(route[1].split()) != expected:
        failures.append(f"room {room}, D{direction}: exit clue disagrees with its actual route")
assert not failures, "\n".join(failures)

print("PASS: Tribal Forest's two western forest exits and southern village exit have correct clues.")
