#!/usr/bin/env python3
"""Contracts for removal of retired Redis UID state and unused exports."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
REDIS = (ROOT / "src" / "redis.c").read_text(encoding="utf-8")
HEADER = (ROOT / "src" / "redis.h").read_text(encoding="utf-8")
ADMIN = (ROOT / "src" / "wizredis.c").read_text(encoding="utf-8")

for retired in (
    "mud:next_obj_uid",
    "redis_save_obj_uid_counter",
    "redis_load_obj_uid_counter",
    "redis_ping",
    "redis_publish(",
    "redis_get_string",
):
    assert retired not in REDIS, retired
    assert retired not in HEADER, retired

assert "mud:next_obj_uid" not in ADMIN
print("redis legacy API retirement tests passed")
