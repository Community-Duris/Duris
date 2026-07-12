from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
header = (ROOT / "src/redis.h").read_text()
source = (ROOT / "src/redis.c").read_text()

assert "bool redis_clear_pwipe_state(void);" in header
stub_start = source.index("#ifdef __NO_MYSQL__")
stub_end = source.index("#endif", stub_start)
assert "bool redis_clear_ship_snapshots(void)" in source[stub_start:stub_end]
assert "return true;" in source[stub_start:stub_end]
assert '"SCAN %s MATCH ship:snapshot:* COUNT 256"' in source
clear = source[source.index("bool redis_clear_pwipe_state(void)"):]
assert "redis_clear_ship_snapshots()" in clear
print("Redis ship-snapshot namespace invalidation checks passed")
