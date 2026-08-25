"""Every player race must start in a town whose guards will not attack it.

find_hometown() falls back to Tharnadia when avail_hometowns[] has no entry for
a race, and Tharnadia's militia carry AGGR_UNDEAD_RACE / AGGR_EVIL_RACE, so a
race with no hometown of its own spawned into a town that immediately killed
it.  This checks every player race against the mobs its hometown zone loads.
"""

import collections
import re
from pathlib import Path
from contract_text import split_at

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"
AREAS = ROOT / "areas"

defines = (SRC / "defines.h").read_text()
constant = (SRC / "constant.c").read_text()
utils = (SRC / "utils.h").read_text()

race_ids = {
    n.replace("RACE_", ""): int(v)
    for n, v in re.findall(r"#define (RACE_[A-Z_]+)\s+(\d+)", defines)
}
home_ids = {
    int(v): n.replace("HOME_", "")
    for n, v in re.findall(r"#define (HOME_[A-Z_]+)\s+(\d+)", defines)
}
player_max = race_ids[
    re.search(r"#define RACE_PLAYER_MAX\s+(RACE_[A-Z_]+)", defines).group(1).replace("RACE_", "")
]

# --- hometown start rooms and the race/town matrix --------------------------
hb = split_at(constant, "const int hometown[] = {", 1)[1].split("\n};", 1)[0]
home_rooms = [int(m.group(1)) for m in re.finditer(r"^\s*(\d+),", hb, re.M)]

ab = split_at(constant, "const int        avail_hometowns[][LAST_RACE + 1] = {", 1)[1]
ab = ab.split("\n};", 1)[0]
avail = []
# Brace groups, not lines: clang-format wraps a row once it passes the column
# limit, so a per-line match would drop the continuation.
for m in re.finditer(r"\{([0-9,\s]*)\}", ab):
    avail.append([int(x) for x in m.group(1).split(",") if x.strip()])

assert len(avail) == len(home_rooms), "hometown[] and avail_hometowns[] disagree"


# --- which racewar side each race lands on ----------------------------------
def macro_races(name):
    block = utils.split("#define " + name, 1)[1].split("\n\n", 1)[0]
    return {n.replace("RACE_", "") for n in re.findall(r"race == (RACE_[A-Z_]+)", block)}


good = macro_races("OLD_RACE_GOOD")
evil = macro_races("OLD_RACE_EVIL")
undead = macro_races("OLD_RACE_PUNDEAD")
neutral = macro_races("OLD_RACE_NEUTRAL")


def sides(name):
    if name in undead:
        return ["UNDEAD"]
    if name in neutral:
        return ["GOOD", "EVIL"]  # alignment choice decides at creation
    if name in good:
        return ["GOOD"]
    if name in evil:
        return ["EVIL"]
    return []  # no racewar side; nothing can aggro on it by race


# --- world data: mob aggro flags and which zone loads them ------------------
mobs = {}
lines = (AREAS / "world.mob").read_text(encoding="latin-1").split("\n")
i = 0
while i < len(lines):
    if lines[i].startswith("#") and lines[i][1:].strip().isdigit():
        vnum = int(lines[i][1:].strip())
        i += 1
        tildes = 0
        while i < len(lines) and tildes < 4:
            if lines[i].rstrip().endswith("~"):
                tildes += 1
            i += 1
        while i < len(lines) and not lines[i].startswith("#"):
            m = re.match(r"\s*(-?\d+(?:\s+-?\d+)+)\s*([A-Z])\s*$", lines[i])
            if m:
                nums = [int(x) for x in m.group(1).split()]
                mobs[vnum] = nums[1] if len(nums) >= 9 else 0
                break
            i += 1
    else:
        i += 1

zones = []
cur = None
for line in (AREAS / "world.zon").read_text(encoding="latin-1").split("\n"):
    m = re.match(r"^#(\d+)\s*$", line)
    if m:
        cur = {"num": int(m.group(1)), "name": None, "file": None, "top": None, "mobs": []}
        zones.append(cur)
        continue
    if cur is None:
        continue
    if cur["name"] is None and line.endswith("~"):
        cur["name"] = line[:-1]
        continue
    if cur["file"] is None and line.endswith("~"):
        cur["file"] = line[:-1]
        continue
    if cur["top"] is None:
        mm = re.match(r"^\s*(\d+)\s+\d+", line)
        if mm:
            cur["top"] = int(mm.group(1))
        continue
    mm = re.match(r"^\s*M\s+\d+\s+(\d+)\s+\d+\s+(\d+)", line)
    if mm:
        cur["mobs"].append((int(mm.group(1)), int(mm.group(2))))

zones = [z for z in zones if z["top"] is not None and z["num"] != 4294967295]
zones.sort(key=lambda z: z["top"])
ranges = []
low = 0
for z in zones:
    ranges.append((low, z["top"], z))
    low = z["top"] + 1


def zone_of(room):
    for lo, hi, z in ranges:
        if lo <= room <= hi:
            return z
    return None


AGGR_BY_SIDE = {"GOOD": 64, "EVIL": 128, "UNDEAD": 256}  # AGGR_*_RACE bits

# How close a hostile mob may load to the start room, in vnum distance.
NEAR_START = 12
# A hometown whose population is this hostile is not a home, it is a dungeon.
HOSTILE_SHARE = 0.05
# Ambient AGGR_ALL wildlife is normal (shipped towns run 0-48%, Faang the
# highest), but a zone that is mostly monsters is a ruin: the Ancient City
# Ruins are 73% and killed every undead character sent there.
AGGR_ALL_SHARE = 0.60

for name, rid in sorted(race_ids.items(), key=lambda kv: kv[1]):
    if not 0 < rid <= player_max or name == "PLAYER_MAX":
        continue

    towns = [h for h in range(len(avail)) if rid < len(avail[h]) and avail[h][rid] == 1]
    assert towns, f"{name} has no hometown; find_hometown() would fall back to Tharnadia"

    for h in towns:
        z = zone_of(home_rooms[h])
        assert z, f"{name}: hometown {home_ids.get(h)} room {home_rooms[h]} is in no zone"

        # A working hometown may hold the odd flavour enemy -- the drow city
        # keeps a captive grey elf -- so what matters is whether racewar
        # hostiles are near where the character lands, and whether the town's
        # population is hostile at large.  Tharnadia for an undead character is
        # 31% hostile with 19 of them in or beside the start room; the shipped
        # evil towns sit under 1% with the nearest 63+ rooms away.
        #
        # Only AGGR_*_RACE is checked. AGGR_ALL wildlife attacks residents too
        # and every shipped town has some, so it is ambient danger rather than
        # "the guards reject what you are"; several are asleep and never aggro.
        hostile = collections.defaultdict(list)
        for vnum, room in z["mobs"]:
            flags = mobs.get(vnum, 0)
            near = abs(room - home_rooms[h]) <= NEAR_START
            for side, bit in AGGR_BY_SIDE.items():
                if flags & bit:
                    hostile[side].append((vnum, room, near))

        total = len(z["mobs"]) or 1

        aggro_all = sum(1 for vnum, _ in z["mobs"] if mobs.get(vnum, 0) & 1)
        assert aggro_all / total <= AGGR_ALL_SHARE, (
            f"{name} starts in {home_ids.get(h)}, where {aggro_all}/{total} "
            f"({aggro_all / total:.0%}) of the mobs attack everyone on sight"
        )

        for side in sides(name):
            entries = hostile.get(side, [])
            near_start = [(v, r) for v, r, n in entries if n]
            assert not near_start, (
                f"{name} ({side}) starts in {home_ids.get(h)} room {home_rooms[h]} "
                f"beside mob(s) that aggro its racewar: {near_start[:3]}"
            )
            share = len(entries) / total
            assert share <= HOSTILE_SHARE, (
                f"{name} ({side}) starts in {home_ids.get(h)}, where "
                f"{len(entries)}/{total} ({share:.0%}) of the mobs aggro its racewar"
            )

print("hometown racewar safety contracts passed")
