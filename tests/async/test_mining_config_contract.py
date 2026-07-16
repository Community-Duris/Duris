#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
header = (ROOT / "src/mining_config.h").read_text()
source = (ROOT / "src/mining_config.c").read_text()
config = (ROOT / "lib/mining.cfg").read_text()
tradeskill = (ROOT / "src/tradeskill.c").read_text()
makefile = (ROOT / "src/Makefile").read_text()

assert 'fopen("lib/mining.cfg", "r")' in source
assert "mining_config_boot" in source
assert "mining_config_gem_vnum" in source
assert "mining_config_region_value" in source
assert "mining.gem.504.weight=8" in config
assert "mining.region.map.duration=9" in config
assert "int mining_config_gem_vnum(void);" in header
assert "mining_config_boot();" in tradeskill
assert "mining_config_gem_vnum()" in tradeskill
assert "mining_config.o" in makefile
print("mining configuration contract passed")
