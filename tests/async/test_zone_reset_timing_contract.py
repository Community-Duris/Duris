from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
events = (ROOT / "src" / "events.c").read_text()
db = (ROOT / "src" / "db.c").read_text()
config = (ROOT / "src" / "config.h").read_text()

assert "zone_table[zone].age++;" in events
assert "zone_table[zone].lifespan" in events
assert "add_event(event_reset_zone, PULSES_IN_TICK" in events
assert "zone_table[zone].age = 0;" in db
assert "fscanf(fl, \"%d %d %d %d %d %d\\n\", &tmp1, &tmp2, &tmp3, &tmp4, &tmp5, &tmp6);" in db
assert "zone_table[zon].lifespan_min = tmp4;" in db
assert "zone_table[zon].lifespan_max = tmp5;" in db
assert "#define WAIT_SEC           4" in config
assert "#define WAIT_MIN           60 * WAIT_SEC" in config
assert "#define PULSES_IN_TICK     300" in config

print("zone reset timing contract passed")
