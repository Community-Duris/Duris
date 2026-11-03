#!/usr/bin/env python3
"""Audit journal coverage of native creation locations and playable town areas."""
import argparse
import importlib.util
import json
import pathlib
import re


def required_areas(root):
    spec = importlib.util.spec_from_file_location("zone_catalog", root / "scripts/zone_story_quest_catalog.py")
    tool = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(tool)
    zones = tool.zone_registry(root)
    source = (root / "src/core/constant.c").read_text()
    source = re.sub(r"/\*.*?\*/|//[^\n]*", "", source, flags=re.S)

    def rows(name):
        match = re.search(r"const int " + name + r"\[\]\[[^\]]+\]\s*=\s*\{(.*?)\n\};", source, re.S)
        if not match:
            raise ValueError(f"could not read native {name} table")
        return [[int(n) for n in re.findall(r"-?\d+", row)] for row in re.findall(r"\{([^{}]*)\}", match[1])]

    available, locations = rows("avail_hometowns"), rows("guild_locations")
    names = re.search(r"const char \*town_name_list\[\]\s*=\s*\{(.*?)\};", source, re.S)
    names = re.findall(r'"([^"\n]*)"', names[1])
    if len(available) != len(locations) or len(names) != len(available) + 1:
        raise ValueError("native hometown tables have inconsistent dimensions")
    # Conservative union over native rows; configured race/class switches may
    # narrow the menu. Covering the union keeps journals valid when switches change.
    selected, all_rooms = {}, set()
    room_count = {}
    for zone in zones:
        path = root / "areas/wld" / f"{zone['source_area']}.wld"
        rooms = set(int(m[1]) for m in re.finditer(r"^#(\d+)\s*\n([^~]*)~", path.read_text(errors="replace"), re.M)
                    if m[2].strip() != "$") if path.is_file() else set()
        all_rooms |= rooms
        room_count[zone["source_area"]] = len(rooms)
    for home, permitted in enumerate(available):
        if not any(permitted):
            continue
        for room in set(v for v in locations[home] if v > 0):
            if room not in all_rooms:
                raise ValueError(f"creation location {room} ({names[home]}) is absent from active room sources")
            zone = next(z for z in zones if z["first_vnum"] <= room <= z["last_vnum"])
            record = selected.setdefault(zone["source_area"], {"zone": zone, "creation_homes": set(), "start_rooms": set(), "town": False})
            record["creation_homes"].add(names[home])
            record["start_rooms"].add(room)
    flag_name = re.search(r"#define\s+ZONE_TOWN\s+(BIT_\d+)", (root / "src/world/db.h").read_text())[1]
    town_flag = int(re.search(r"#define\s+" + flag_name + r"\s+(\d+)U", (root / "src/core/defines.h").read_text())[1])
    excluded = []
    for zone in zones:
        path = root / "areas/zon" / f"{zone['source_area']}.zon"
        header = re.search(r"^#-?\d+\s*\n.*?~\s*\n([^\n]+)", path.read_text(errors="replace"), re.M | re.S)
        flags = int(header[1].split()[2])
        if zone["discoverable"] and flags & town_flag:
            if not room_count[zone["source_area"]]:
                excluded.append(zone["source_area"])
                continue
            record = selected.setdefault(zone["source_area"], {"zone": zone, "creation_homes": set(), "start_rooms": set(), "town": False})
            record["town"] = True
    result = []
    for area, record in sorted(selected.items(), key=lambda pair: pair[1]["zone"]["zone_number"]):
        result.append({"source_area": area, "zone_number": record["zone"]["zone_number"], "name": record["zone"]["name"],
                       "town": record["town"], "creation_homes": sorted(record["creation_homes"]),
                       "start_rooms": sorted(record["start_rooms"]), "room_count": room_count[area],
                       "mapping": f"areas/story/{area}.story.json"})
    return {"areas": result, "excluded_empty_town_markers": excluded}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=pathlib.Path, default=pathlib.Path(__file__).resolve().parents[1])
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    report = required_areas(args.source_root)
    if args.check:
        missing = [r["source_area"] for r in report["areas"] if not (args.source_root / r["mapping"]).is_file()]
        if missing:
            raise SystemExit("Missing starter/town story mappings: " + ", ".join(missing))
        print(f"All {len(report['areas'])} starter/town areas have story mappings.")
    else:
        print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
