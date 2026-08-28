#!/usr/bin/env python3
"""Every active Redis surface is bound to one boot-captured SQL season epoch."""

from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[2]
REGISTRY = (ROOT / "src" / "redis_key_registry.def").read_text(encoding="ascii")
REDIS = (ROOT / "src" / "redis.c").read_text(encoding="ascii")
WORLD = (ROOT / "src" / "redis_world_runtime.c").read_text(encoding="ascii")
MAINTENANCE = (ROOT / "src" / "redis_maintenance.c").read_text(encoding="ascii")
REPORT = (ROOT / "src" / "redis_report_cache.c").read_text(encoding="ascii")
PRESENCE = (ROOT / "src" / "redis_presence_worker.c").read_text(encoding="ascii")
PRESENCE_HEADER = (ROOT / "src" / "redis_presence_worker.h").read_text(encoding="ascii")
DONATION = (ROOT / "src" / "redis_donation_worker.c").read_text(encoding="ascii")
DONATION_HEADER = (ROOT / "src" / "redis_donation_worker.h").read_text(encoding="ascii")
NAMESPACE = (ROOT / "src" / "redis_namespace.c").read_text(encoding="ascii")

surfaces = re.findall(
    r'^REDIS_SURFACE\(([A-Z0-9_]+), "([^"]+)", "([^"]+)", '
    r'[A-Z0-9_]+, "([^"]+)", "([^"]+)"\)$',
    REGISTRY,
    re.MULTILINE,
)
active = [(name, token, pattern, kind) for name, token, pattern, kind, state in surfaces
          if state == "active"]
assert active
for name, token, pattern, kind in active:
    assert pattern.startswith("<namespace>:season:<epoch>:"), (name, pattern)
    if name != "SEASON_INFIX":
        assert not token.startswith("mud:"), (name, token)
    assert kind in {"key", "key_format", "key_pattern", "key_prefix", "channel"}

assert REDIS.count("redis_configure_epoch_surfaces(sql_season_epoch())") == 1
assert "redis_runtime_epoch = epoch" in REDIS
assert "world_writer_epoch = world_runtime_epoch" in WORLD
assert "redis_configure_namespace()" in REDIS
assert "REDIS_SEASON_INFIX" in NAMESPACE
assert "redis_namespace_season_key(redis_key_namespace" in REDIS
assert "config.key_namespace = world_key_namespace.c_str()" in WORLD
assert REDIS.count("sql_season_epoch()") == 1

for field in ("current_key", "session_prefix", "retry_prefix", "event_channel"):
    assert field in PRESENCE_HEADER
    assert f"config->{field}" in PRESENCE
assert "REDIS_PRESENCE_CURRENT" not in PRESENCE
assert "REDIS_PRESENCE_EVENT_CHANNEL" not in PRESENCE

assert "const char *channel" in DONATION_HEADER
assert "config->channel" in DONATION
assert "configured_channel.c_str()" in DONATION
assert "REDIS_DONATION_CHANNEL" not in DONATION

for wrapper in ("cache_set_ex", "cache_get", "cache_delete"):
    start = REPORT.index(f"{wrapper}(")
    body = REPORT[start:REPORT.index("\n}", start)]
    assert "resolve_key" in body
for key in ("named_key", "fraglist_key", "epic_zones_key", "artifact_keys"):
    assert key in REPORT
assert "redis_report_cache_configure(redis_key_namespace, epoch)" in REDIS
assert "redis_namespace_season_key(key_namespace, epoch" in REPORT

pwipe = REDIS[REDIS.index("bool redis_clear_pwipe_state"):REDIS.index("void redis_cleanup")]
for scoped in ("redis_presence_current_key", "redis_presence_session_pattern",
               "redis_presence_retry_pattern", "redis_report_cache_pattern()"):
    assert scoped in pwipe
for legacy in ("REDIS_LEGACY_PRESENCE_CURRENT",
               "REDIS_LEGACY_PRESENCE_SESSION_PATTERN",
               "REDIS_LEGACY_PRESENCE_RETRY_PATTERN", "REDIS_LEGACY_CACHE_PATTERN"):
    assert legacy in MAINTENANCE
assert "redis_namespace_season_key(config->key_namespace, config->season_epoch" in MAINTENANCE

print("all active Redis surfaces use the boot-captured SQL season epoch")
