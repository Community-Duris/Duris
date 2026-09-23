#!/usr/bin/env python3
"""Keep Grumbar's dispersal room on the same sector as the rooms it feeds.

Room 131376 loads zone 1312 mobs. Their vnums fall inside IS_RANDOM_MOB's
range, and mundane wandering never moves such a mob into a room of another
sector, so an exit into a different sector strands everything loaded there.
"""

import re

from _paths import ROOT

DISP_ROOM = 131376
world = (ROOT / "areas/wld/earthp.wld").read_text(encoding="utf-8")


def sector_and_exits(vnum):
    room = re.search(rf"(?ms)^#{vnum}\n(.*?)(?=^#\d+\n|\Z)", world)
    assert room is not None, f"earthp.wld room {vnum} is missing"
    # Skip the ~-terminated name and description to reach "zone flags sector".
    body = room.group(1).split("~\n", 2)[2]
    exits = re.findall(r"(?ms)^D\d+\n.*?~\n.*?~\n-?\d+ -?\d+ (-?\d+)$", body)
    return int(body.split()[2]), [int(to_room) for to_room in exits]


sector, exits = sector_and_exits(DISP_ROOM)
assert exits, f"room {DISP_ROOM} must keep its exits into Grumbar's Domain"
mismatched = {
    to_room: to_sector
    for to_room in exits
    if (to_sector := sector_and_exits(to_room)[0]) != sector
}
assert not mismatched, (
    f"room {DISP_ROOM} is sector {sector}, but exits lead to other sectors "
    f"{mismatched}; its zone 1312 mobs could never wander out"
)
