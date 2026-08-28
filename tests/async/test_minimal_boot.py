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
WEATHER = (ROOT / "src/weather.c").read_text()
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

weather_rows = [
    [int(value) for value in line.split()]
    for line in (MINIMAL / "world.weather").read_text().splitlines()
]
assert len(weather_rows) == 100
assert all(len(row) == 12 for row in weather_rows)
for row in weather_rows:
    for index in (1, 4, 7, 10):
        assert 0 <= row[index] <= 8, "weather precipitation index is out of bounds"

assert 'strcmp(argv[pos], "--minimal")' in COMM
assert COMM.count("no_random = 1;") >= 2
assert '"areas_mini/world.weather"' in DB
assert '"areas_mini/world.tab"' in DB
assert "Skipping full-world state restoration in mini mode." in DB
assert 'fopen("areas_mini/world.shp", "r")' in SHOP
assert "STUDIOPROC: minimal world mode, proc engine idle." in STUDIOPROC

assert "Skipping persistence worker startup in mini mode." not in COMM
pipeline_startup = COMM[
    COMM.index('logit(LOG_STATUS, "Entering game loop.");') : COMM.index("latency_trace_reset();")
]
assert "if (!mini_mode)\n\t\tlocker_async_init();" in pipeline_startup
assert "player_save_pipeline_init" in pipeline_startup
assert "critical_command_coordinator_init" in pipeline_startup
assert "Skipping zone database publication in mini mode." in COMM

flower_start = WEATHER.index("static void replace_hour_flower")
flower_end = WEATHER.index("void event_another_hour", flower_start)
flower_body = WEATHER[flower_start:flower_end]
assert "flowerroom < 0 || flowerroom > top_of_world" in flower_body
assert "replacement_rnum < 0" in flower_body
assert "flower->R_num < 0 || flower->R_num > top_of_objt" in flower_body
assert flower_body.index("read_object(replacement_rnum") < flower_body.index(
    "extract_obj(flower)"
)

assert "--minimal)" in CYCLE
assert "MINIMAL_MODE=1" in CYCLE
assert 'SERVER_ARGS+=(--minimal)' in CYCLE
assert '"${SERVER_ARGS[@]}" "${MUD_PORT}"' in CYCLE
assert "skipping full world generation" in CYCLE
assert 'areas_mini/$MINIMAL_FILE' in CYCLE

print("minimal boot contracts OK")
