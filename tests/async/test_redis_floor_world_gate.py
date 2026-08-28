#!/usr/bin/env python3
"""Floor-delta work exists only when Redis world recovery is enabled."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
REDIS = (ROOT / "src" / "redis.c").read_text(encoding="ascii")
WORLD = (ROOT / "src" / "redis_world_runtime.c").read_text(encoding="ascii")
FLOOR = (ROOT / "src" / "redis_floor_runtime.c").read_text(encoding="ascii")
CHECKPOINT = (ROOT / "src" / "persistence_checkpoint.c").read_text(encoding="ascii")
HEADER = (ROOT / "src" / "redis.h").read_text(encoding="ascii")
FLOOR_HEADER = (ROOT / "src" / "redis_floor_runtime.h").read_text(encoding="ascii")
ACTOBJ = (ROOT / "src" / "actobj.c").read_text(encoding="utf-8")


def section(start: str, end: str, source: str = WORLD) -> str:
    first = source.index(start)
    return source[first : source.index(end, first)]


assert "redis_log_floor_pickup" not in HEADER
assert "redis_check_floor_pickup" not in HEADER
assert "redis_check_floor_drop" not in HEADER
assert "redis_log_floor_pickup" not in REDIS + WORLD
assert "redis_check_floor_pickup" not in REDIS + WORLD
assert "redis_check_floor_drop" not in REDIS + WORLD
assert "redis_log_floor_pickup" not in ACTOBJ
assert "SADD mud:floor_pickups" not in WORLD
assert "SISMEMBER mud:floor_pickups" not in WORLD
assert "HEXISTS mud:floor_drops" not in WORLD

drop = section("void redis_log_floor_drop", "bool redis_flush_floor_drops", FLOOR)
flush = section("bool redis_flush_floor_drops", "void redis_remove_floor_drop", FLOOR)
remove = FLOOR[FLOOR.index("void redis_remove_floor_drop") :]
restore = section("bool redis_read_floor_records", "bool redis_world_runtime_start")
periodic = CHECKPOINT[CHECKPOINT.index("void event_flush_dirty_players") :]

assert "!floor_runtime_enabled" in drop
assert "!floor_runtime_enabled" in flush
assert "!floor_runtime_enabled" in remove
assert "!world_enabled" in restore
assert "world_recovery_floor_object_root_uid" in restore
assert "WORLD_RECOVERY_FLOOR_PREFIX_BYTES" in restore
assert "ZCARD" in restore and "HLEN" in restore
assert "ZRANGE" in restore and "HMGET" in restore
assert "HGETALL" not in restore
assert "WORLD_RECOVERY_MAX_FLOOR_RECORDS" in restore
assert "WORLD_RECOVERY_MAX_FLOOR_BYTES" in restore
assert "read_object(" not in restore
assert "obj_to_room(" not in restore
assert "if (redis_floor_runtime_enabled())" in periodic
assert "if (redis_enabled)" not in periodic
assert "redis_namespace_season_key" in FLOOR
assert "redis_season_key" not in flush
for symbol in ("redis_log_floor_drop", "redis_remove_floor_drop", "redis_flush_floor_drops"):
    assert symbol in FLOOR_HEADER
    assert symbol not in HEADER

# Cleanup intentionally remains available for the retired pickup key and the
# current season-scoped floor hash.
assert "REDIS_LEGACY_FLOOR_PICKUPS" in WORLD
assert "redis_epoch_key(floor_key, sizeof floor_key" in WORLD
assert "REDIS_FLOOR_DROP_INDEX_SUFFIX" in WORLD
clear_floor = section("bool redis_clear_floor_drops", "bool redis_read_floor_records")
assert "REDIS_SHARED_SCOPE_FLOOR" in clear_floor
assert "REDIS_SHARED_COMMAND_WRITE" in clear_floor
assert '"DEL %s %s"' in clear_floor

print("Redis floor deltas are gated on world recovery")
