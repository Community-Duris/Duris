#!/usr/bin/env python3
"""Contracts for authority-first, rollback-capable Redis world recovery."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
PIPELINE = (ROOT / "src/world_recovery_pipeline.c").read_text(encoding="ascii")
HEADER = (ROOT / "src/world_recovery_pipeline.h").read_text(encoding="ascii")
REDIS = (ROOT / "src/redis.c").read_text(encoding="ascii")
SQL = (ROOT / "src/sql.c").read_text(encoding="ascii")
COMM = (ROOT / "src/comm.c").read_text(encoding="ascii")


def section(text: str, start: str, end: str) -> str:
    first = text.index(start)
    return text[first : text.index(end, first)]


for token in (
    "uint64_t item_uid",
    "uint64_t root_item_uid",
    "uint64_t parent_item_uid",
    "int32_t vnum",
    "int32_t values[8]",
    "int64_t timers[6]",
    "WORLD_RECOVERY_MAX_ITEM_TREE = 12",
):
    assert token in HEADER

capture_tree = section(PIPELINE, "bool capture_item_tree", "int write_object_record")
for token in (
    "object->contains",
    "root_item_uid",
    "parent_item_uid",
    "WORLD_RECOVERY_MAX_ITEM_TREE",
):
    assert token in capture_tree

plan = section(PIPELINE, "bool add_object_record", "bool build_recovery_plan")
for token in (
    "real_room(record.room_vnum)",
    "real_object(item.vnum)",
    "plan->item_uids.insert",
    "parents.find(item.parent_item_uid)",
    "existing_tree_matches",
    "plan->authority_items.push_back",
):
    assert token in plan

reconcile_start = SQL.rindex("bool sql_persistence_reconcile_world_recovery_items")
reconcile = SQL[reconcile_start : SQL.index("bool sql_hydrate_item_owner_revisions", reconcile_start)]
for token in (
    "QUERY_BATCH_SIZE = 256",
    "MAX_RECOVERY_ITEMS = 262144",
    "item_current_owner",
    "item_owner_revision",
    "current_item.item_uid IN (",
    "entry.root_item_uid != planned.root_item_uid",
    "entry.parent_item_uid != planned.parent_item_uid",
    "entry.owner.type != item_owner_type::room",
    "entry.owner.id != static_cast<uint64_t>(planned.room_vnum)",
    "entry.vnum != planned.vnum",
    "entry.state != item_custody_state::active",
):
    assert token in reconcile
assert reconcile.index("sql_begin_transaction") < reconcile.index("db_query")
assert reconcile.index("authoritative_count == count") < reconcile.index("sql_commit")
assert "item_ownership_runtime_hydrate" not in reconcile

materialize = section(PIPELINE, "bool materialize_plan", "} // namespace")
assert materialize.index("copyover_restore_mob_from_buffer") < materialize.index(
    "materialize_object"
)
for token in (
    "consumed != planned.record.size()",
    "rollback_materialized",
    "created_objects.push_back",
    "item_ownership_runtime_hydrate_many_atomic",
):
    assert token in materialize
assert materialize.index("created_objects.push_back") < materialize.index(
    "item_ownership_runtime_hydrate_many_atomic"
) < materialize.index("for (const copyover_room &door")

load = section(REDIS, "bool redis_load_world_state", "void event_save_world_state")
assert load.index("redis_read_floor_records") < load.index("world_recovery_restore_with_floor")
assert "redis_restore_floor_drops" not in load
reader = section(REDIS, "static bool redis_read_floor_records", "void mark_player_dirty")
assert 'memcmp(encoded, "WRF2:", 5)' in reader
assert "read_object(" not in reader and "obj_to_room(" not in reader

recovery = section(COMM, "// redis crash recovery", "PROFILES(RESET)")
assert "applying full normal zone boot" in recovery
assert "reset_zone(zone, 2)" in recovery

print("authority-first transactional world recovery contracts passed")
