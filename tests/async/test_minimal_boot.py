#!/usr/bin/env python3
"""Minimal boot must stay isolated from generated full-world data."""

import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
MINIMAL = ROOT / "areas_mini"
COMM = (ROOT / "src/comm.c").read_text()
DB = (ROOT / "src/db.c").read_text()
SHOP = (ROOT / "src/shop.c").read_text()
STUDIOPROC = (ROOT / "src/studioproc.c").read_text()
CYCLE = (ROOT / "scripts/cycle_mud.sh").read_text()

required = (
    "mini.mob",
    "mini.obj",
    "mini.qst",
    "mini.wld",
    "mini.zon",
    "world.shp",
    "world.tab",
    "world.weather",
)
for name in required:
    path = MINIMAL / name
    assert path.is_file() and path.stat().st_size > 0, f"missing minimal data: {name}"

for name in ("mini.mob", "mini.obj", "mini.wld"):
    records = [int(value) for value in re.findall(r"^#(\d+)$", (MINIMAL / name).read_text(), re.M)]
    assert records, f"no records in {name}"
    assert len(records) == len(set(records)), f"duplicate vnum in {name}"

object_vnums = {
    int(value) for value in re.findall(r"^#(\d+)$", (MINIMAL / "mini.obj").read_text(), re.M)
}
assert 7 in object_vnums, "configured test character's master spellbook prototype is missing"

zone_lines = [
    line
    for line in (MINIMAL / "mini.zon").read_text().splitlines()
    if line and not line.startswith(("*", "#", "$"))
]
assert zone_lines[-1] == "S"
assert not any(re.match(r"^[MOEPDGRFABCYS] ", line) for line in zone_lines[:-1]), (
    "minimal zone must not reset full-world prototypes"
)
assert "\n0 0 0\n" not in (MINIMAL / "mini.wld").read_text(), (
    "minimal world contains an exit to nonexistent room zero"
)

assert 'strcmp(argv[pos], "--minimal")' in COMM
assert '"areas_mini/world.weather"' in DB
assert '"areas_mini/world.tab"' in DB
assert 'fopen("areas_mini/world.shp", "r")' in SHOP
assert "STUDIOPROC: minimal world mode, proc engine idle." in STUDIOPROC

assert "--minimal)" in CYCLE
assert "MINIMAL_MODE=1" in CYCLE
assert 'SERVER_ARGS+=(--minimal)' in CYCLE
assert '"${SERVER_ARGS[@]}" "${MUD_PORT}"' in CYCLE
assert "skipping full world generation" in CYCLE
assert 'areas_mini/$MINIMAL_FILE' in CYCLE

print("minimal boot contracts OK")
