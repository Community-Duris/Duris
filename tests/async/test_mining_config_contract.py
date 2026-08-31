#!/usr/bin/env python3
from _paths import SRC
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
header = (SRC / "mining_config.h").read_text()
source = (SRC / "mining_config.c").read_text()
config = (ROOT / "lib/mining.cfg").read_text()
mining = (SRC / "mining.c").read_text()
makefile = (SRC / "Makefile").read_text()

assert 'fopen("lib/mining.cfg", "r")' in source
assert "mining_config_boot" in source
assert "mining_config_gem_vnum" in source
assert "mining_config_region_value" in source
assert "mining.gem.504.weight=8" in config
assert "mining.region.map.duration=9" in config
assert "int mining_config_gem_vnum(int mine_quality);" in header
assert "mining_config_boot();" in mining
# the gem roll is biased by the mine's quality, like the sibling ore roll
assert "mining_config_gem_vnum(mine_quality)" in mining
assert "mining_config.o" in makefile
print("mining configuration contract passed")
