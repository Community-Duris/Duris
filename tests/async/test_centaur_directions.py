#!/usr/bin/env python3
"""Keep Centaur Villages' quest and travel clues aligned with actual exits."""

import argparse
import pathlib
import re


def check(root):
    world = (root / "areas/wld/centaur_zone.wld").read_text(encoding="utf8")
    rooms = {
        int(match[1]): match[2]
        for match in re.finditer(r"^#(\d+)\s*\n(.*?)(?=^#\d+|^\$|\Z)", world, re.M | re.S)
    }
    errors = []

    def require(condition, message):
        if not condition:
            errors.append(message)

    def prose(body):
        return " ".join(re.sub(r"&[+-][a-zA-Z]|&[nN]", "", body).split())

    def exit_to(vnum, direction, target, clue=None):
        match = re.search(
            rf"\bD{direction}\s+([^~]*)~[^~]*~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)",
            rooms[vnum],
            re.S,
        )
        require(match is not None and int(match[4]) == target,
                f"room {vnum} exit {direction} must lead to {target}")
        if clue:
            require(match is not None and clue in prose(match[1]),
                    f"room {vnum} exit {direction} clue must say {clue}")

    exit_to(93310, 3, 93311, "heads west")
    exit_to(93311, 1, 93310)
    exit_to(93313, 3, 93396)
    require("forest opens up to the west" in prose(rooms[93313].split("~")[1]),
            "room 93313 must place the forest to the west")
    exit_to(93319, 3, 93357)
    require("wilderness to the west" in prose(rooms[93319].split("~")[1]),
            "room 93319 must place the wilderness to the west")
    exit_to(93381, 1, 93382, "intersection to the east")
    exit_to(93382, 3, 93381)
    exit_to(93393, 1, 93392)
    require("enters from the east" in prose(rooms[93393].split("~")[1]),
            "room 93393 must describe its entrance from the east")
    exit_to(93326, 1, 93399, "cave entrance stands to the east")
    exit_to(93399, 3, 93326)
    require("peak to the east" in prose(rooms[93326].split("~")[1]),
            "Banitoor's rock must be on the eastern side of the plains")
    quests = (root / "areas/qst/centaur_zone.qst").read_text(encoding="utf8")
    tamilea = re.search(r"^#93308\s*\n(.*?)(?=^#\d+|\Z)", quests, re.M | re.S)[1]
    require("cave near the eastern edge of my cousin's plains" in prose(tamilea),
            "Tamilea must direct players to Banitoor's actual eastern cave")
    assert not errors, "\n".join(errors)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--source-root", type=pathlib.Path,
                        default=pathlib.Path(__file__).resolve().parents[2])
    check(parser.parse_args().source_root)
    print("Centaur Villages quest and travel direction regression passed")
