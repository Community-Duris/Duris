#!/usr/bin/env python3
"""Redis administrator commands must remain truthful and network-free online."""

from _paths import SRC
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
WIZ = (SRC / "wizredis.c").read_text()
REDIS = (SRC / "redis.c").read_text()
DONATION = (SRC / "redis_donation_runtime.c").read_text()
HEADER = (SRC / "redis.h").read_text()
REPORT = (SRC / "redis_report_cache.c").read_text()
REPORT_HEADER = (SRC / "redis_report_cache.h").read_text()


def section(source: str, start: str, end: str) -> str:
    return source[source.index(start) : source.index(end, source.index(start))]


simple = section(WIZ, "static void redis_status_simple", "static void redis_status_detailed")
detailed = section(WIZ, "static void redis_status_detailed", "static void redis_clear_cache")
clear = section(WIZ, "static void redis_clear_cache", "void do_redis")

for status in (simple, detailed):
    for forbidden in (
        "redis_has_world_state",
        "redis_world_state_timestamp",
        "redis_key_exists",
        "redis_get_ttl",
        "redis_hlen",
        "redis_scard",
        "redis_command",
    ):
        assert forbidden not in status
    for local_snapshot in (
        "world_recovery_pipeline_health_copy",
        "redis_presence_worker_health_copy",
        "redis_cache_store_health_copy",
        "redis_floor_store_health_copy",
        "redis_donation_worker_health_copy",
        "redis_shared_command_health_copy",
    ):
        assert local_snapshot in status

assert "Redis is not queried" in simple
assert "Redis is not queried" in detailed
assert "redis_clear_world_state" not in clear
assert "redis_world_recovery_quiesce" not in clear
assert "redis_clear_floor_drops" not in clear
assert "redis_clear_floor_pickups" not in clear
assert "Refused online:" in clear
assert "maintenance clear workflow" in clear
assert "Queued:" in clear and "Rejected:" in clear and "Partial:" in clear

for retired in (
    "redis_world_state_timestamp",
    "redis_key_exists",
    "redis_get_ttl",
    "redis_hlen",
    "redis_scard",
):
    assert retired not in HEADER
    assert retired not in REDIS

for signature in (
    "bool redis_invalidate_named_report(void)",
    "bool redis_invalidate_fraglist(void)",
    "bool redis_invalidate_epic_zones(void)",
    "bool redis_invalidate_artifact_cache(void)",
):
    assert signature in REPORT_HEADER
    assert signature in REPORT

donation_pulse = section(
    DONATION, "void check_donation_messages", "} // namespace"
)
assert "redis_donation_worker_take" in donation_pulse
for forbidden in (
    "redisConnect",
    "redis_command",
    "redisGetReply",
    "redisBufferRead",
    "poll(",
):
    assert forbidden not in donation_pulse

print("Redis administrator nonblocking and truthful-result contracts passed")
