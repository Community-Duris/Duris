#!/usr/bin/env python3
"""Verify Redis credentials remain isolated and routed by subsystem."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SOURCE = (ROOT / "src" / "redis.c").read_text(encoding="ascii")
WORLD = (ROOT / "src" / "redis_world_runtime.c").read_text(encoding="ascii")
CONFIG = (ROOT / "src" / "redis_runtime_config.c").read_text(encoding="ascii")
MAINTENANCE = (ROOT / "src" / "redis_maintenance.c").read_text(encoding="ascii")
CLEAR = (ROOT / "scripts" / "clear-duris-redis-keys.sh").read_text(encoding="ascii")
WRAPPER = (ROOT / "scripts" / "clear-redis.sh").read_text(encoding="ascii")

for subsystem in ("WORLD", "PRESENCE", "CACHE", "DONATION", "MAINTENANCE"):
    assert f'"REDIS_{subsystem}_USERNAME"' in CONFIG
    assert f'"REDIS_{subsystem}_PASSWORD"' in CONFIG

assert "const bool production" in CONFIG
assert "strcmp(usernames[left], usernames[right])" in CONFIG
for field in ("world", "presence", "cache", "donation", "maintenance"):
    assert f"configured.{field}" in CONFIG
assert "const redis_floor_store_config floor_config = { world_connection };" in WORLD
assert "redis_report_cache_start(redis_connections.cache)" in SOURCE
assert "redis_connections.maintenance" in SOURCE
assert "redis_connection_open(config->connection)" in MAINTENANCE
assert "redis_ctx = maintenance;" not in SOURCE
assert "redis_ctx = world_context;" not in SOURCE

assert "REDIS_MAINTENANCE_USERNAME" in CLEAR
assert "REDIS_MAINTENANCE_PASSWORD" in CLEAR
assert "maintenance username and password must be configured together" in CLEAR
assert "REDIS_MAINTENANCE_USERNAME" in WRAPPER
assert "REDIS_MAINTENANCE_PASSWORD" in WRAPPER

print("Redis subsystem identity routing contracts passed")
