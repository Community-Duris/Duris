from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"
LIB = ROOT / "lib"

config = (LIB / "creation_availability.cfg").read_text()
impl = (SRC / "creation_availability_config.c").read_text()
header = (SRC / "creation_availability_config.h").read_text()
makefile = (SRC / "Makefile").read_text()
comm = (SRC / "comm.c").read_text()
nanny = (SRC / "nanny.c").read_text()
ws = (SRC / "ws_handlers.c").read_text()

assert "creation_availability_config.o" in makefile
assert "boot_creation_availability_config" in comm
assert "#ifndef CREATION_AVAILABILITY_CONFIG_H" in header
assert "creation_class_enabled" in header
assert "creation_race_enabled" in header
assert "creation.class.dragoon.enabled=false" in config
assert "creation.class.warrior.enabled=true" in config
assert "creation.race.human.enabled=true" in config
assert '"dragoon", 30' in impl
assert '"human", RACE_HUMAN' in impl
assert "extract_name" in impl
assert "creation_class_enabled(cls)" in nanny
assert "creation_class_enabled(flag2idx" in nanny
assert "creation_race_enabled(playable_races[i].race_id)" in nanny
assert "creation_race_enabled(GET_RACE" in nanny
assert "creation_class_enabled(j)" in ws
assert "creation_class_enabled(class_id)" in ws
assert "creation_race_enabled(race_id)" in ws
assert "creation_race_enabled(race)" in ws

print("creation availability configuration source contract passed")
