#!/usr/bin/env python3
"""Every fall in the world data lands.

`falling_step()` follows open down exits and lands only where there is none. A down exit
back onto the room itself, or onto the room the step just left, lands too. Any longer loop
of down exits is a fall that never ends, and a character in one can do nothing but petition
until a god moves them. Walk every fall entry (a no-ground sector or a fall chance) the way
the server does and refuse such a loop.
"""

from pathlib import Path
import re

from _paths import ROOT


NO_GROUND_SECTORS = {8, 18}  # SECT_NO_GROUND, SECT_UNDRWLD_NOGROUND
DOWN = 5


def read_rooms(text: str) -> dict[int, tuple[int, int, dict[int, int]]]:
    """Map vnum to (sector, fall chance, exits by direction), as the loader reads them."""
    lines = text.split("\n")
    count = len(lines)

    def skip_string(index: int) -> int:  # a string ends on the line whose last mark is '~'
        while index < count and not lines[index].rstrip().endswith("~"):
            index += 1
        return index + 1

    rooms = {}
    index = 0
    while index < count:
        header = re.fullmatch(r"#(\d+)", lines[index].rstrip())
        index += 1
        if not header:
            continue
        vnum = int(header.group(1))
        index = skip_string(skip_string(index))  # name, description
        fields = None
        while index < count:  # "zone flags sector"; art ending in a stray '~' comes first
            fields = lines[index].split()
            index += 1
            if len(fields) >= 3 and all(re.fullmatch(r"-?\d+", f) for f in fields[:3]):
                break
        if not fields or len(fields) < 3 or not re.fullmatch(r"-?\d+", fields[2]):
            continue
        exits = {}
        chance = 0
        while index < count:
            line = lines[index].rstrip()
            if line == "S" or line.startswith("#"):
                index += 1
                break
            if re.fullmatch(r"D\d", line):
                index = skip_string(skip_string(index + 1))  # description, keywords
                numbers = lines[index].split()
                index += 1
                exits[int(line[1])] = int(numbers[2]) if len(numbers) >= 3 else -1
            elif line == "E":
                index = skip_string(skip_string(index + 1))
            elif line == "F":
                chance = int(lines[index + 1].split()[0])
                index += 2
            else:
                index += 1
        rooms[vnum] = (int(fields[2]), chance, exits)
    return rooms


rooms = {}
for entry in (ROOT / "areas/AREA").read_text(encoding="latin-1").splitlines():
    if entry.strip() and not entry.startswith("*"):
        path = ROOT / "areas/wld" / f"{entry.split()[0]}.wld"
        rooms.update(read_rooms(path.read_text(encoding="latin-1")))

entries = [vnum for vnum, (sector, chance, _) in rooms.items()
           if sector in NO_GROUND_SECTORS or chance > 0]
assert len(rooms) > 200_000 and len(entries) > 1_000, (len(rooms), len(entries))

loops = {}
longest = 0
for start in entries:
    path = [start]
    while True:
        below = rooms[path[-1]][2].get(DOWN, -1)
        if below not in rooms or below == path[-1] or (len(path) > 1 and below == path[-2]):
            break  # lands: no way down, or a down exit back onto this room or the last one
        if below in path:
            loop = path[path.index(below):]
            loops.setdefault(min(loop), " -> ".join(map(str, loop + [below])))
            break
        path.append(below)
    longest = max(longest, len(path))

assert not loops, "a fall through these rooms never lands:\n" + "\n".join(
    f"  {text}" for _, text in sorted(loops.items()))
assert longest >= 20, longest  # the walk saw the long real falls, so the parser read the exits
print(f"{len(entries)} fall entries over {len(rooms)} rooms land; the longest fall is {longest} rooms")
