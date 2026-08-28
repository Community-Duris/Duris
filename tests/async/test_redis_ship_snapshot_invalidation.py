from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
header = (ROOT / "src/redis.h").read_text()
source = (ROOT / "src/redis.c").read_text()
sql = (ROOT / "src/sql_player.c").read_text()

assert "bool redis_clear_pwipe_state(void);" in header
assert "redis_load_ship_snapshot" not in header
assert "redis_cache_ship_snapshot" not in header
assert "redis_load_ship_snapshot" not in source
assert "redis_cache_ship_snapshot" not in source

load_start = sql.index("P_ship sql_load_ship(const char *owner_name)")
load_end = sql.index("bool sql_load_all_ships()", load_start)
load = sql[load_start:load_end]
assert "from ships where owner_name" in load
assert "redis_" not in load

save_start = sql.index("bool sql_save_ship(P_ship ship)")
save_end = sql.index("static bool sql_load_ship_armor", save_start)
assert "redis_" not in sql[save_start:save_end]

stub_start = source.index("#ifdef __NO_MYSQL__")
stub_end = source.index("#endif", stub_start)
assert "bool redis_clear_ship_snapshots(void)" in source[stub_start:stub_end]
assert "return true;" in source[stub_start:stub_end]
assert '"SCAN %s MATCH %s COUNT 256"' in source
assert "REDIS_SHIP_SNAPSHOT_PATTERN" in source
clear = source[source.index("bool redis_clear_pwipe_state(void)"):]
assert "redis_clear_ship_snapshots()" in clear
print("MySQL ship authority and legacy Redis invalidation checks passed")
