#!/usr/bin/env python3
"""Source contracts for Redis and player-checkpoint module ownership."""

from _paths import SRC
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
REDIS_HEADER = (SRC / "redis.h").read_text(encoding="ascii")
REDIS_SOURCE = (SRC / "redis.c").read_text(encoding="ascii")
LIFECYCLE_HEADER = (SRC / "redis_lifecycle.h").read_text(encoding="ascii")
WORLD_RUNTIME_HEADER = (SRC / "redis_world_runtime.h").read_text(encoding="ascii")
WORLD_RUNTIME_SOURCE = (SRC / "redis_world_runtime.c").read_text(encoding="ascii")
WIZARD_HEADER = (SRC / "redis_wizard.h").read_text(encoding="ascii")
CHECKPOINT_HEADER = (SRC / "persistence_checkpoint.h").read_text(encoding="ascii")
CHECKPOINT_SOURCE = (SRC / "persistence_checkpoint.c").read_text(encoding="ascii")
DONATION_HEADER = (SRC / "redis_donation_runtime.h").read_text(encoding="ascii")
DONATION_SOURCE = (SRC / "redis_donation_runtime.c").read_text(encoding="ascii")
PRESENCE_HEADER = (SRC / "redis_presence_runtime.h").read_text(encoding="ascii")
PRESENCE_SOURCE = (SRC / "redis_presence_runtime.c").read_text(encoding="ascii")
REPORT_HEADER = (SRC / "redis_report_cache.h").read_text(encoding="ascii")
REPORT_SOURCE = (SRC / "redis_report_cache.c").read_text(encoding="ascii")
RUNTIME_CONFIG_HEADER = (SRC / "redis_runtime_config.h").read_text(encoding="ascii")
RUNTIME_CONFIG_SOURCE = (SRC / "redis_runtime_config.c").read_text(encoding="ascii")
SHIP_HEADER = (SRC / "redis_ship_legacy.h").read_text(encoding="ascii")
SHIP_SOURCE = (SRC / "redis_ship_legacy.c").read_text(encoding="ascii")
MAINTENANCE_HEADER = (SRC / "redis_maintenance.h").read_text(encoding="ascii")
MAINTENANCE_SOURCE = (SRC / "redis_maintenance.c").read_text(encoding="ascii")
FLOOR_RUNTIME_HEADER = (SRC / "redis_floor_runtime.h").read_text(encoding="ascii")
FLOOR_RUNTIME_SOURCE = (SRC / "redis_floor_runtime.c").read_text(encoding="ascii")
MAKEFILE = (SRC / "Makefile").read_text(encoding="ascii")


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

events = (SRC / "new_events.c").read_text(encoding="utf-8")
assert '#include "redis/redis_donation_runtime.h"' in events
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
    source = (SRC / filename).read_text(encoding="utf-8")
    assert '#include "redis/redis_presence_runtime.h"' in source
    assert '#include "redis/redis.h"' not in source

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
assert "redis_cache_store_delete" not in REDIS_SOURCE
assert "redis_report_cache.o" in MAKEFILE
assert "redis_report_cache_start(redis_connections.cache)" in REDIS_SOURCE
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
    source = (SRC / filename).read_text(encoding="utf-8")
    assert '#include "redis/redis_report_cache.h"' in source
    assert '#include "redis/redis.h"' not in source

for symbol in (
    "redis_floor_runtime_configure",
    "redis_floor_runtime_set_enabled",
    "redis_floor_runtime_set_quiesced",
    "redis_floor_runtime_set_materializing",
    "redis_log_floor_drop",
    "redis_remove_floor_drop",
    "redis_flush_floor_drops",
):
    assert symbol in FLOOR_RUNTIME_HEADER
    assert symbol in FLOOR_RUNTIME_SOURCE
    assert symbol not in REDIS_HEADER
assert "redis_floor_store_submit" in FLOOR_RUNTIME_SOURCE
assert "world_recovery_write_object_to_buffer" in FLOOR_RUNTIME_SOURCE
assert "max_floor_drop_batch = 1024" in FLOOR_RUNTIME_SOURCE
assert "redis_floor_runtime.o" in MAKEFILE
for filename in (
    "actobj.c",
    "handler.c",
    "persistence_checkpoint.c",
    "world_recovery_pipeline.c",
):
    source = (SRC / filename).read_text(encoding="utf-8")
    assert '#include "redis/redis_floor_runtime.h"' in source
    assert '#include "redis/redis.h"' not in source

for symbol in (
    "redis_runtime_connections_configure",
    "redis_runtime_connections_destroy",
):
    assert symbol in RUNTIME_CONFIG_HEADER
    assert symbol in RUNTIME_CONFIG_SOURCE
for token in (
    "REDIS_WORLD_USERNAME",
    "REDIS_PRESENCE_USERNAME",
    "REDIS_CACHE_USERNAME",
    "REDIS_DONATION_USERNAME",
    "REDIS_MAINTENANCE_USERNAME",
    "redis_connection_settings_create",
):
    assert token in RUNTIME_CONFIG_SOURCE
    assert token not in REDIS_SOURCE
assert "redis_runtime_config.o" in MAKEFILE
assert "redis_runtime_connections_configure(redis_donation_runtime_enabled()" in REDIS_SOURCE

for symbol in (
    "redis_ship_legacy_worker_init",
    "redis_ship_legacy_worker_drain",
    "redis_ship_legacy_worker_shutdown",
    "redis_ship_legacy_worker_cancel",
    "redis_invalidate_ship_snapshot",
    "redis_clear_ship_snapshots",
):
    assert symbol in SHIP_HEADER
    assert symbol in SHIP_SOURCE
    assert symbol not in REDIS_HEADER
assert "redis_cache_store_delete" not in SHIP_SOURCE
assert "redis_connection_open(configured_connection)" in SHIP_SOURCE
assert "redis_ship_legacy_worker_init(redis_connections.maintenance)" in REDIS_SOURCE
assert "REDIS_SHIP_SNAPSHOT_PATTERN" in SHIP_SOURCE
assert "redis_ship_legacy.o" in MAKEFILE
for filename in ("sql_player.c", "ships/ship_base.c"):
    source = (SRC / filename).read_text(encoding="utf-8")
    assert '#include "redis/redis_ship_legacy.h"' in source
    assert '#include "redis/redis.h"' not in source

for symbol in ("redis_clear_pwipe_state", "redis_validate_pwipe_state"):
    assert symbol in MAINTENANCE_HEADER
    assert symbol not in REDIS_HEADER
for symbol in ("redis_maintenance_clear", "redis_maintenance_validate"):
    assert symbol in MAINTENANCE_HEADER
    assert symbol in MAINTENANCE_SOURCE
assert "redis_maintenance.o" in MAKEFILE
sql = (SRC / "sql.c").read_text(encoding="utf-8")
assert '#include "redis/redis_maintenance.h"' in sql
assert '#include "redis/redis.h"' not in sql

assert "#error \"redis.h is retired" in REDIS_HEADER
for symbol in ("redis_init", "redis_cleanup", "redis_runtime_enabled"):
    assert symbol in LIFECYCLE_HEADER
for symbol in (
    "redis_world_runtime_start",
    "redis_world_runtime_shutdown",
    "redis_world_runtime_enabled",
    "redis_save_world_state",
    "redis_world_recovery_pulse",
    "redis_world_recovery_drain",
    "redis_world_recovery_quiesce",
):
    assert symbol in WORLD_RUNTIME_HEADER
    assert symbol in WORLD_RUNTIME_SOURCE
assert "do_redis" in WIZARD_HEADER
for token in (
    "redis_connection_open(world_connection)",
    "redis_world_store_claim_fence",
    "redis_floor_store_request_barrier",
    "world_recovery_pipeline_request",
    "world_recovery_restore_with_floor",
):
    assert token in WORLD_RUNTIME_SOURCE
    assert token not in REDIS_SOURCE
assert "redis_world_runtime.o" in MAKEFILE
for filename, header in (
    ("comm.c", "redis/redis_world_runtime.h"),
    ("copyover.c", "redis/redis_world_runtime.h"),
    ("new_events.c", "redis/redis_world_runtime.h"),
    ("wizredis.c", "redis/redis_world_runtime.h"),
    ("interp.c", "redis/redis_wizard.h"),
):
    source = (SRC / filename).read_text(encoding="utf-8")
    assert f'#include "{header}"' in source
    assert '#include "redis/redis.h"' not in source

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
    source = (SRC / filename).read_text(encoding="utf-8")
    assert '#include "persistence/persistence_checkpoint.h"' in source
    assert '#include "redis/redis.h"' not in source

redis_includers = []
for source_path in SRC.rglob("*.[ch]"):
    if '#include "redis/redis.h"' in source_path.read_text(encoding="utf-8"):
        redis_includers.append(source_path)
assert redis_includers == []

print("Redis, checkpoint, donation, presence, report-cache, and floor-runtime boundaries passed")
