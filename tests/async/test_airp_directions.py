#!/usr/bin/env python3
"""Keep Tempest Court's war-chamber and prison clues aligned with their exits."""

from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "areas/wld/airp.wld").read_text(encoding="utf8")
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


war_return = exit_to(131628, 1, 131627)
assert war_return and exit_to(131627, 3, 131628)
assert "way to the east" in war_return[1], "Si'Ciltron's return clue disagrees with its eastern exit"
assert exit_to(131740, 0, 131741) and exit_to(131741, 2, 131740)
assert exit_to(131740, 2, 131742) and exit_to(131742, 0, 131740)
prison_prose = " ".join(re.sub(r"&[+-][A-Za-z]|&[nN]", "", rooms[131740].split("~")[1]).split())
assert "To the north and south" in prison_prose, "the prison clue disagrees with the two cell exits"

print("PASS: Tempest Court's war-chamber return and prison-cell clues match reciprocal exits.")
