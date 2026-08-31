from _paths import SRC
from pathlib import Path
from contract_text import contains

ROOT = Path(__file__).resolve().parents[2]
events = (SRC / "events.c").read_text()
db = (SRC / "db.c").read_text()
config = (SRC / "config.h").read_text()

assert contains(events, "zone_table[zone].age++;")
assert contains(events, "zone_table[zone].lifespan")
assert contains(events, "add_event(event_reset_zone, PULSES_IN_TICK")
assert contains(db, "zone_table[zone].age = 0;")
assert contains(db, "REQUIRED_FSCANF(fl, \"%d %d %d %d %d %d\\n\", &tmp1, &tmp2, &tmp3, &tmp4, &tmp5, &tmp6);")
assert contains(db, "zone_table[zon].lifespan_min = tmp4;")
assert contains(db, "zone_table[zon].lifespan_max = tmp5;")
assert contains(config, "#define WAIT_SEC           4")
assert contains(config, "#define WAIT_MIN           60 * WAIT_SEC")
assert contains(config, "#define PULSES_IN_TICK     300")
assert contains(events, "DURIS_ZONE_RESET_TRACE")
assert contains(events, "due_tick")
assert contains(events, "lateness_ticks")
assert contains(events, "event_bucket")

print("zone reset timing contract passed")
