#!/usr/bin/env python3
"""Preserve the original fixed rift; quest prose must not enable a portable escape."""
import argparse
from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser()
parser.add_argument("--objects", type=Path, default=ROOT / "areas/obj/fields_between.obj")
args = parser.parse_args()


def bodies(path):
    return {int(m[1]): m[2] for m in re.finditer(
        r"^#(\d+)\s*\n([\s\S]*?)(?=^#\d+\s*$|\Z)",
        path.read_text(encoding="utf8"), re.M)}


objects = bodies(args.objects)
rift = objects[71030]
parts = rift.split("~", 4)
values = [list(map(int, line.split())) for line in parts[4].strip().splitlines()[:3]]
assert "easy to pick up" in parts[2]
assert values[0][0] == 25 and values[0][7] == 0, "Quest mapping must not enable ITEM_TAKE on this unlimited portal"
assert values[2][0] == 1000000, "Quest mapping must preserve the original rift weight"
assert values[1] == [71153, 7, -1, 0, 0, 0, 0, 0]
assert values[0][6] == 8392712 and values[0][8:] == [32768, 0, 0]
quests = bodies(ROOT / "areas/qst/fields_between.qst")
assert re.search(r"R I 71033\s+G I 71030\s+S", quests[71040])
resets = (ROOT / "areas/zon/fields_between.zon").read_text(encoding="utf8")
assert re.search(r"^O 0 71030 1 71126 100 0 0 0\b", resets, re.M)
assert not re.search(r"^[GEP] \d+ 71030\b", resets, re.M)
# Fixed scenery portals retain their original pickup restrictions and routes.
for vnum, destination in [(71001, 71104), (71002, 71014), (71031, 71001)]:
    raw = objects[vnum].split("~", 4)[4].strip().splitlines()
    header, teleport = list(map(int, raw[0].split())), list(map(int, raw[1].split()))
    assert header[0] == 25 and header[7] == 0
    assert teleport[:4] == [destination, 7, -1, 0]
rooms = bodies(ROOT / "areas/wld/fields_between.wld")
assert not re.search(r"\bD\d", rooms[71104])
assert len(re.findall(r"0 0 71153", rooms[71153])) == 6
print("Fields Between: original non-takeable/million-weight rift restored; fixed ENTER route, shaman recipe and other portal restrictions retained. Portable quest delivery remains unresolved.")
