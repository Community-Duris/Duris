#!/usr/bin/env python3
"""Floor-delta work exists only when Redis world recovery is enabled."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
REDIS = (ROOT / "src" / "redis.c").read_text(encoding="ascii")
HEADER = (ROOT / "src" / "redis.h").read_text(encoding="ascii")
ACTOBJ = (ROOT / "src" / "actobj.c").read_text(encoding="utf-8")


def section(start: str, end: str) -> str:
    first = REDIS.index(start)
    return REDIS[first : REDIS.index(end, first)]


assert "redis_log_floor_pickup" not in HEADER
assert "redis_check_floor_pickup" not in HEADER
assert "redis_check_floor_drop" not in HEADER
assert "redis_log_floor_pickup" not in REDIS
assert "redis_check_floor_pickup" not in REDIS
assert "redis_check_floor_drop" not in REDIS
assert "redis_log_floor_pickup" not in ACTOBJ
assert "SADD mud:floor_pickups" not in REDIS
assert "SISMEMBER mud:floor_pickups" not in REDIS
assert "HEXISTS mud:floor_drops" not in REDIS

drop = section("void redis_log_floor_drop", "bool redis_flush_floor_drops")
flush = section("bool redis_flush_floor_drops", "void redis_remove_floor_drop")
remove = section("void redis_remove_floor_drop", "static bool redis_clear_floor_drops_checked")
restore = section("static bool redis_read_floor_records", "void mark_player_dirty")
periodic = section("void event_flush_dirty_players", "bool redis_save_world_state")

assert "!redis_world_state_enabled" in drop
assert "!redis_world_state_enabled" in flush
assert "!redis_world_state_enabled" in remove
assert "!redis_world_state_enabled" in restore
assert "world_recovery_floor_object_root_uid" in restore
assert "WORLD_RECOVERY_FLOOR_PREFIX_BYTES" in restore
assert "ZCARD" in restore and "HLEN" in restore
assert "ZRANGE" in restore and "HMGET" in restore
assert "HGETALL" not in restore
assert "WORLD_RECOVERY_MAX_FLOOR_RECORDS" in restore
assert "WORLD_RECOVERY_MAX_FLOOR_BYTES" in restore
assert "read_object(" not in restore
assert "obj_to_room(" not in restore
assert "if (redis_world_state_enabled)" in periodic
assert "if (redis_enabled)" not in periodic

# Cleanup intentionally remains available for the retired pickup key and the
# current season-scoped floor hash.
assert 'DEL mud:floor_pickups' in REDIS
assert "redis_season_key(floor_key, sizeof floor_key, REDIS_FLOOR_DROPS_SUFFIX)" in REDIS
assert "REDIS_FLOOR_DROP_INDEX_SUFFIX" in REDIS
assert 'redis_command(redis_ctx, "DEL %s %s", floor_key, floor_index_key)' in REDIS

print("Redis floor deltas are gated on world recovery")
