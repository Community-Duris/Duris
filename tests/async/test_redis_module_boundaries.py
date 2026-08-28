#!/usr/bin/env python3
"""Source contracts for Redis and player-checkpoint module ownership."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
REDIS_HEADER = (ROOT / "src/redis.h").read_text(encoding="ascii")
REDIS_SOURCE = (ROOT / "src/redis.c").read_text(encoding="ascii")
CHECKPOINT_HEADER = (ROOT / "src/persistence_checkpoint.h").read_text(encoding="ascii")
CHECKPOINT_SOURCE = (ROOT / "src/persistence_checkpoint.c").read_text(encoding="ascii")
DONATION_HEADER = (ROOT / "src/redis_donation_runtime.h").read_text(encoding="ascii")
DONATION_SOURCE = (ROOT / "src/redis_donation_runtime.c").read_text(encoding="ascii")
PRESENCE_HEADER = (ROOT / "src/redis_presence_runtime.h").read_text(encoding="ascii")
PRESENCE_SOURCE = (ROOT / "src/redis_presence_runtime.c").read_text(encoding="ascii")
REPORT_HEADER = (ROOT / "src/redis_report_cache.h").read_text(encoding="ascii")
REPORT_SOURCE = (ROOT / "src/redis_report_cache.c").read_text(encoding="ascii")
MAKEFILE = (ROOT / "src/Makefile").read_text(encoding="ascii")


CHECKPOINT_API = (
    "mark_player_dirty",
    "mark_player_dirty_components",
    "flush_dirty_players",
    "get_dirty_player_count",
    "persistence_dirty_save_snapshot_copy",
    "event_flush_dirty_players",
)

for symbol in CHECKPOINT_API:
    assert symbol in CHECKPOINT_HEADER
    assert symbol in CHECKPOINT_SOURCE
    assert symbol not in REDIS_HEADER
    assert symbol not in REDIS_SOURCE

for include in ("persistence_observability.h", "player_revision_state.h"):
    assert include in CHECKPOINT_HEADER
    assert include not in REDIS_HEADER

assert "player_save_pipeline.h" in CHECKPOINT_SOURCE
assert "player_save_worker.h" in CHECKPOINT_SOURCE
assert "player_save_pipeline.h" not in REDIS_SOURCE
assert "player_save_worker.h" not in REDIS_SOURCE
assert "persistence_checkpoint.o" in MAKEFILE

for symbol in (
    "redis_donation_runtime_enabled",
    "redis_donation_runtime_set_enabled",
    "event_check_donation_messages",
):
    assert symbol in DONATION_HEADER
    assert symbol in DONATION_SOURCE
    assert symbol not in REDIS_HEADER
assert "redis_donation_worker_take" in DONATION_SOURCE
assert "REDIS_DONATION_MAX_MESSAGES_PER_PULSE = 8" in DONATION_SOURCE
assert "redis_donation_runtime.o" in MAKEFILE
assert "redis_donation_enabled" not in REDIS_SOURCE
assert "broadcast_donation_nchat" not in REDIS_SOURCE

events = (ROOT / "src/new_events.c").read_text(encoding="utf-8")
assert '#include "redis_donation_runtime.h"' in events
assert "redis_donation_runtime_enabled()" in events

for symbol in (
    "redis_presence_runtime_enabled",
    "redis_presence_runtime_set_enabled",
    "redis_player_online",
    "redis_player_offline",
    "redis_clear_online_players",
):
    assert symbol in PRESENCE_HEADER
    assert symbol in PRESENCE_SOURCE
    assert symbol not in REDIS_HEADER
assert "redis_presence_payload_encode" in PRESENCE_SOURCE
assert "redis_presence_worker_submit_online" in PRESENCE_SOURCE
assert "redis_presence_worker_submit_offline" in PRESENCE_SOURCE
assert "redis_presence_worker_submit_clear" in PRESENCE_SOURCE
assert "redis_presence_runtime.o" in MAKEFILE
for filename in ("actoth.c", "nanny.c"):
    source = (ROOT / "src" / filename).read_text(encoding="utf-8")
    assert '#include "redis_presence_runtime.h"' in source
    assert '#include "redis.h"' not in source

for symbol in (
    "redis_report_cache_configure",
    "redis_report_cache_start",
    "redis_report_cache_cancel",
    "redis_report_cache_shutdown",
    "redis_cache_named_report",
    "redis_cache_fraglist",
    "redis_cache_epic_zones",
    "redis_cache_artifact_list",
):
    assert symbol in REPORT_HEADER
    assert symbol in REPORT_SOURCE
    assert symbol not in REDIS_HEADER
for token in (
    "redis_cache_store_set",
    "redis_cache_store_get",
    "redis_cache_store_transform",
    "redis_cache_store_delete",
    "redis_cache_store_seed",
    "generate_named_report",
    "generate_fraglist_cache_payload",
):
    assert token in REPORT_SOURCE
for token in (
    "redis_cache_store_set",
    "redis_cache_store_get",
    "redis_cache_store_transform",
    "redis_cache_store_seed",
    "generate_named_report",
    "generate_fraglist_cache_payload",
):
    assert token not in REDIS_SOURCE
assert REDIS_SOURCE.count("redis_cache_store_delete") == 1
assert "redis_report_cache.o" in MAKEFILE
assert "redis_report_cache_start(redis_cache_settings)" in REDIS_SOURCE
assert "redis_report_cache_cancel()" in REDIS_SOURCE
assert "redis_report_cache_shutdown(REDIS_CACHE_DRAIN_TIMEOUT_MSEC)" in REDIS_SOURCE

report_only_callers = (
    "artifact.c",
    "artifact_guild_transaction.c",
    "combat_outcome_transaction.c",
    "epic.c",
    "fraglist.c",
    "random.mob.c",
    "zone_touch_transaction.c",
)
for filename in report_only_callers:
    source = (ROOT / "src" / filename).read_text(encoding="utf-8")
    assert '#include "redis_report_cache.h"' in source
    assert '#include "redis.h"' not in source

checkpoint_only_callers = (
    "actinf.c",
    "auction_houses.c",
    "fight.c",
    "limits.c",
    "memorize.c",
    "nexus_stones.c",
    "utility.c",
    "world_quest.c",
)
for filename in checkpoint_only_callers:
    source = (ROOT / "src" / filename).read_text(encoding="utf-8")
    assert '#include "persistence_checkpoint.h"' in source
    assert '#include "redis.h"' not in source

redis_includers = []
for source_path in (ROOT / "src").rglob("*.[ch]"):
    if '#include "redis.h"' in source_path.read_text(encoding="utf-8"):
        redis_includers.append(source_path)
assert len(redis_includers) <= 13

print("Redis, checkpoint, donation, presence, and report-cache module boundaries passed")
