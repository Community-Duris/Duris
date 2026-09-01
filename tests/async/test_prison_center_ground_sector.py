#!/usr/bin/env python3
"""Keep the Planar Prisons center compatible with its grounded encounters."""

from pathlib import Path
import re


root = Path(__file__).resolve().parents[2]
world = (root / "areas/wld/prison.wld").read_text(encoding="utf-8")
room = re.search(r"(?ms)^#7460\n(.*?)(?=^#\d+\n|\Z)", world)

assert room is not None, "Planar Prisons room 7460 is missing"
assert re.search(r"(?m)^73 32768 15$", room.group(1)), (
    "room 7460 must remain UD-Inside because its encounters and loot have no "
    "valid downward exit"
)
