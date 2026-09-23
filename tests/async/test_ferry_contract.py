#!/usr/bin/env python3
"""Check ferryact.c's ferries[] table against its area data, and "look out".

init_ferries() creates one Ferry per table entry, and nothing else loads a
ferry: Ferry::init() puts the ship object at the first stop, binds
ferry_room_proc to the ship's rooms, and routes each leg with
dijkstra(valid_ship_edge).  Area content for a new ship therefore does nothing
until its entry is here, as happened to The Stromvok (issue #615).

The ids, ship objects, and ship rooms of different ferries must not collide,
because get_ferry(), get_ferry_from_obj(), and get_ferry_from_room() all
return the first match.  Every stop must be a room valid_ship_edge() lets a
ship enter, or that leg can never be routed.  Every ferry also puts a ticket
automat (FERRY_AUTOMAT_OBJ) at each stop, and the automat sells tickets
(FERRY_TICKET_VNUM) that ticket control looks for, so both objects must be in
the world.  Whether each leg actually has a path is checked by the full-world
boot journey, which fails on the server's "no path found!" and missing-automat
boot lines.

ferry_room_proc() also answers "look out" anywhere on board.
command_interpreter() skips the spaces after the command word before it calls
special(), so the argument arrives as "out".  The proc compared it with " out",
so "look out" failed on every ferry.  It must show the room the ferry is in
with new_look(), as the ferry does for PLR2_SHIPMAP passengers on every move.
Moving the passenger out and back instead runs char_from_room() in the ship
room and in the dock or ocean room, which cancels item actions aimed at them,
runs the outside room's CMD_FROMROOM hook, and can leave a bloodstain there.
"""

import re

from _paths import ROOT, extract_function, source
from _source_contract import strip_comments

ITEM_SHIP = 28
ROOM_DOCKABLE = 1 << 20  # BIT_21
SECT_WATER_NOSWIM = 7
SECT_OCEAN = 12

STROMVOK = {
    "name": "&+RThe Str&+Lom&+Rvok&N",
    "id": 7,
    "obj": 47018,
    "board": 47198,
    "rooms": (47199, 47215),
    "speed": 2,
    "wait": 180,
    "notice": 60,
    "price": 10000,
    "stops": [
        (22445, "&+WSto&+Lrm Port&N"),
        (66688, "&+YTorrhan&N"),
        (30929, "&+WStrathor&N"),
    ],
}


def parse_ferries():
    text = strip_comments(source("ferryact.c").read_text(encoding="utf-8"))
    rooms = {
        name: [int(value) for value in body.split(",")]
        for name, body in re.findall(r"static int (\w+)\[\] = \{([^}]*)\};", text)
    }
    stops = {
        name: [(int(vnum), label)
               for vnum, label in re.findall(r'\{\s*(\d+),\s*"([^"]*)"\s*\}', body)]
        for name, body in re.findall(
            r"static struct ferry_definition::stop_info (\w+)\[\] = \{(.*?)\{\s*\}\s*\};",
            text, re.S)
    }
    table = re.search(
        r"static const struct ferry_definition ferries\[\] = \{(.*?)\{\s*\}\s*\};", text, re.S)
    assert table, "ferryact.c no longer defines the ferries[] table"
    entries = re.findall(
        r'\{\s*"([^"]*)",' + r"\s*(\d+)," * 3 + r"\s*(\w+)," + r"\s*(\d+)," * 4 + r"\s*(\w+)\s*\}",
        table.group(1))
    assert len(entries) == len(re.findall(r'\{\s*"', table.group(1))), (
        "a ferries[] entry did not parse")
    ferries = []
    for name, fid, obj, board, room_array, speed, wait, notice, price, stop_array in entries:
        span = rooms[room_array]
        assert len(span) == 3 and span[2] == 0 and span[0] <= span[1], (
            f"{room_array} must be {{ first, last, 0 }}; create_ferry() reads a room range")
        ferries.append({
            "name": name, "id": int(fid), "obj": int(obj), "board": int(board),
            "rooms": tuple(span[:2]), "speed": int(speed), "wait": int(wait),
            "notice": int(notice), "price": int(price), "stops": stops[stop_array],
        })
    return ferries


def world_files(kind):
    """The area files make_all combines into the world the server boots."""
    directory = ROOT / "areas" / kind
    by_lower_name = {path.name.lower(): path for path in directory.iterdir()}
    for line in (ROOT / "areas/AREA").read_text(encoding="latin-1").splitlines():
        if line.startswith("*") or not line.split():
            continue
        name = f"{line.split()[0]}.{kind}"
        path = directory / name
        yield path if path.is_file() else by_lower_name[name.lower()]


def records(kind, vnums):
    """Each wanted vnum's record: the lines from its #vnum header to the next header."""
    found = {}
    for path in world_files(kind):
        text = path.read_text(encoding="latin-1")
        headers = list(re.finditer(r"(?m)^#(\d+)[ \t\r]*$", text))
        for index, header in enumerate(headers):
            vnum = int(header.group(1))
            if vnum in vnums and vnum not in found:
                end = headers[index + 1].start() if index + 1 < len(headers) else len(text)
                found[vnum] = text[header.end():end].split("\n")[1:]
    missing = sorted(set(vnums) - set(found))
    assert not missing, f"no {kind} record in the world for vnums {missing}"
    return found


def read_string(lines, i):
    """fread_string(): the string ends on the first line whose last non-space is '~'."""
    parts = []
    while not lines[i].rstrip().endswith("~"):
        parts.append(lines[i])
        i += 1
    parts.append(lines[i].rstrip()[:-1])
    return "\n".join(parts), i + 1


def parse_room(lines):
    name, i = read_string(lines, 0)
    _, i = read_string(lines, i)
    _zone, flags, sector = (int(value) for value in lines[i].split()[:3])
    i += 1
    exits = {}
    while lines[i].strip() != "S":
        field = lines[i].strip()
        if field.startswith("D"):
            _, i = read_string(lines, i + 1)
            _, i = read_string(lines, i)
            exits[int(field[1:])] = int(lines[i].split()[2])
            i += 1
        elif field == "E":
            _, i = read_string(lines, i + 1)
            _, i = read_string(lines, i)
        elif field.split()[0] in ("F", "C"):  # fall chance; current speed and direction
            i += 1 if len(field.split()) > 1 else 2
        else:
            raise AssertionError(f"unexpected room field {field!r}")
    return {"name": name, "flags": flags, "sector": sector, "exits": exits}


def object_type(lines):
    i = 0
    for _ in range(4):  # keywords, short, long, and action descriptions
        _, i = read_string(lines, i)
    return int(lines[i].split()[0])


def strip_color(text):
    return re.sub(r"&(?:[+-].|=..|[nN])", "", text)


ferries = parse_ferries()
names = [strip_color(ferry["name"]) for ferry in ferries]
assert len(set(names)) == len(names), f"two ferries share a name: {names}"

stromvok = [ferry for ferry in ferries if ferry["obj"] == STROMVOK["obj"]]
assert stromvok == [STROMVOK], f"The Stromvok's ferries[] entry changed: {stromvok}"

for key in ("id", "obj"):
    values = [ferry[key] for ferry in ferries]
    assert len(set(values)) == len(values), f"two ferries share a ferry {key}: {values}"

ship_rooms = {
    name: {ferry["board"], *range(ferry["rooms"][0], ferry["rooms"][1] + 1)}
    for name, ferry in zip(names, ferries)
}
for index, first in enumerate(names):
    for second in names[index + 1:]:
        shared = ship_rooms[first] & ship_rooms[second]
        assert not shared, f"{first} and {second} share ship rooms {sorted(shared)}"

ferry_h = source("ferry.h").read_text(encoding="utf-8")
shared_objects = {
    macro: int(re.search(rf"(?m)^#define {macro} (\d+)$", ferry_h).group(1))
    for macro in ("FERRY_AUTOMAT_OBJ", "FERRY_TICKET_VNUM")
}
objects = records("obj", {ferry["obj"] for ferry in ferries} | set(shared_objects.values()))
for name, ferry in zip(names, ferries):
    kind = object_type(objects[ferry["obj"]])
    assert kind == ITEM_SHIP, (
        f"{name} object {ferry['obj']} is type {kind}; ferry_obj_proc boards ITEM_SHIP only")

stop_vnums = {vnum for ferry in ferries for vnum, _ in ferry["stops"]}
rooms = {vnum: parse_room(lines)
         for vnum, lines in records("wld", set().union(*ship_rooms.values(), stop_vnums)).items()}
for name, ferry in zip(names, ferries):
    assert len(ferry["stops"]) >= 2, f"{name} needs at least two stops"
    for vnum, label in ferry["stops"]:
        stop = rooms[vnum]
        assert strip_color(label).strip(), f"{name} stop {vnum} has no name"
        enterable = (stop["flags"] & ROOM_DOCKABLE or
                     stop["sector"] in (SECT_OCEAN, SECT_WATER_NOSWIM))
        assert enterable, (f"{name} stop {vnum} is neither ROOM_DOCKABLE nor open water; "
                           "valid_ship_edge() never enters it")

# The Stromvok is rooms 47198-47215; a passenger can reach no other room without
# disembarking, so ticket control, "look out", and "disembark" cover the whole ship.
stromvok_rooms = ship_rooms["The Stromvok"]
assert stromvok_rooms == set(range(47198, 47216)), sorted(stromvok_rooms)
assert strip_color(rooms[47198]["name"]) == "The Boarding Platform of The Stromvok"
for vnum in sorted(stromvok_rooms):
    for direction, to_room in rooms[vnum]["exits"].items():
        assert to_room in stromvok_rooms, (
            f"Stromvok room {vnum} exit {direction} leaves the ship for room {to_room}")
for vnum, _ in STROMVOK["stops"]:
    assert rooms[vnum]["flags"] & ROOM_DOCKABLE, f"Stromvok stop {vnum} is not ROOM_DOCKABLE"

room_proc = strip_comments(extract_function("ferryact.c", "int ferry_room_proc("))
assert 'str_cmp(skip_spaces(arg), "out")' in room_proc, (
    "ferry_room_proc must match 'look out' after the interpreter has skipped the spaces")
look_out = room_proc[room_proc.index("if (cmd == CMD_LOOK)"):
                     room_proc.index("if (cmd == CMD_DISEMBARK)")]
assert "new_look(ch, 0, CMD_LOOKOUT, ferry->obj->loc.room);" in look_out, (
    "ferry_room_proc must answer 'look out' with new_look(CMD_LOOKOUT) on the ferry's room")
for call in ("char_from_room(", "char_to_room("):
    assert call not in look_out, (
        f"ferry 'look out' must not move the passenger: {call} runs room departure/arrival logic")

print(f"ferry contract passed ({len(ferries)} ferries)")
