#!/usr/bin/env python3
"""Verify Redis credentials remain isolated and routed by subsystem."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SOURCE = (ROOT / "src" / "redis.c").read_text(encoding="ascii")
CLEAR = (ROOT / "scripts" / "clear-duris-redis-keys.sh").read_text(encoding="ascii")
WRAPPER = (ROOT / "scripts" / "clear-redis.sh").read_text(encoding="ascii")

for subsystem in ("WORLD", "PRESENCE", "CACHE", "DONATION", "MAINTENANCE"):
    assert f'"REDIS_{subsystem}_USERNAME"' in SOURCE
    assert f'"REDIS_{subsystem}_PASSWORD"' in SOURCE

assert "const bool production" in SOURCE
assert "strcmp(usernames[left], usernames[right])" in SOURCE
assert "redis_presence_settings" in SOURCE
assert "redis_cache_settings" in SOURCE
assert "redis_donation_settings" in SOURCE
assert "redis_maintenance_settings" in SOURCE
assert "const redis_floor_store_config floor_config = { redis_settings };" in SOURCE
assert "redis_report_cache_start(redis_cache_settings)" in SOURCE
assert "redis_connection_open(redis_maintenance_settings)" in SOURCE
assert "redis_ctx = maintenance;" in SOURCE
assert "redis_ctx = world_context;" in SOURCE

assert "REDIS_MAINTENANCE_USERNAME" in CLEAR
assert "REDIS_MAINTENANCE_PASSWORD" in CLEAR
assert "maintenance username and password must be configured together" in CLEAR
assert "REDIS_MAINTENANCE_USERNAME" in WRAPPER
assert "REDIS_MAINTENANCE_PASSWORD" in WRAPPER

print("Redis subsystem identity routing contracts passed")
