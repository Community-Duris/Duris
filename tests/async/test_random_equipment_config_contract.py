#!/usr/bin/env python3
from _paths import SRC
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
header = SRC / "random_equipment_config.h"
source = SRC / "random_equipment_config.c"
config = ROOT / "lib/random_equipment.cfg"
assert header.exists() and source.exists() and config.exists()

h = header.read_text()
s = source.read_text()
c = config.read_text()
r = (SRC / "randomeq.c").read_text()
properties = (ROOT / "lib/duris.properties").read_text()
m = (SRC / "Makefile").read_text()
comm = (SRC / "comm.c").read_text()

assert "struct random_equipment_config" in h
assert "boot_random_equipment_config" in h
assert 'fopen("lib/random_equipment.cfg", "r")' in s
assert "random_equipment_stat_max" in s
assert "random_equipment_config.o" in m
assert "boot_random_equipment_config();" in comm
assert '#include "item/random_equipment_config.h"' in r
assert "random_equipment_config_get()" in r

# Checked-in defaults must preserve the currently active duris.properties values.
for line in (
    "drop.piece.percentage=15.0",
    "drop.equipment.percentage=8.0",
    "drop.luck.divisor=4.0",
    "quality.level.multiplier=1.0",
    "stat.secondary.roll.max=2",
    "stat.tertiary.roll.max=5",
    "stat.primary.divisor=46",
):
    assert line in c, line

# Runtime tuning must come through the module rather than duplicated properties.
assert 'get_property("random.drop.' not in r
for obsolete in (
    "random.drop.piece.percentage",
    "random.drop.equip.percentage",
    "random.drop.modifier.quality",
    "random.drop.luck.divisor",
):
    assert obsolete not in properties
print("random equipment config contract passed")
