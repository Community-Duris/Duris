#!/usr/bin/env python3
"""Source contract for truthful, bounded trusted persistence status."""

from _paths import SRC
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
actinf = (SRC / "actinf.c").read_text()
actoth = (SRC / "actoth.c").read_text()
checkpoint = (SRC / "persistence_checkpoint.c").read_text()
queue = (SRC / "persistence_queue.c").read_text()


def function_has_lock(name: str, mutex: str) -> bool:
    start = queue.index(name)
    block = queue[start:start + 900]
    return f"pthread_mutex_lock(&{mutex})" in block and f"pthread_mutex_unlock(&{mutex})" in block

checks = [
    ("persistence command occupies trusted value", "#define WORLD_PERSISTENCE 8" in actinf and '"persistence"' in actinf),
    ("existing trusted gate covers persistence", "choice = world_values[world_index]) > WORLD_ZONES" in actinf),
    ("fresh snapshot getters are inside renderer", all(token in actinf[actinf.index("static void show_world_persistence(P_char ch)"):actinf.index("void do_world", actinf.index("static void show_world_persistence(P_char ch)"))] for token in ["persistence_query_snapshot_copy", "persistence_item_event_health_snapshot_copy", "persistence_scalar_event_health_snapshot_copy", "persistence_large_event_health_snapshot_copy", "persistence_dirty_save_snapshot_copy", "persistence_deferred_save_snapshot_copy", "redis_shared_command_health_copy"])),
    ("flat shop materialization capacity is reported", all(token in actinf for token in ["flatfile_shop_trade_materialization_read_health", "shop_materialization state=%s", "reclaimable=%llu"])),
    ("top output is bounded with latency buckets", "top_site_limit = 8" in actinf and "site_index < rendered_sites" in actinf and "latency_buckets[7]" in actinf),
    ("status states are explicit", all(token in actinf for token in ["state=empty", "state=disabled", "state=unavailable", "heartbeat=unavailable", "registry_overflow", "failed_unscheduled", "oldest_save_age_ms"])),
    ("deferred retry state remains observable", "slot->retry_delay" in actoth and "snapshot.failures" in actoth),
    ("dirty state comes from revisioned pipeline and worker age", all(token in checkpoint for token in ["player_save_pipeline_health_copy", "player_save_pipeline_dirty_count", "player_save_worker_health_copy", "inflight_oldest_age_msec = worker.oldest_age_msec"])),
    ("all queue snapshots lock", all([function_has_lock("persistence_item_event_health_snapshot_copy", "persistence_item_event_queue_mutex"), function_has_lock("persistence_scalar_event_health_snapshot_copy", "persistence_scalar_event_queue_mutex"), function_has_lock("persistence_large_event_health_snapshot_copy", "persistence_large_event_queue_mutex")])),
    ("operator output contains no entity labels", all(token not in actinf[actinf.index("static void show_world_persistence(P_char ch)"):actinf.index("void do_world", actinf.index("static void show_world_persistence(P_char ch)"))] for token in ["player=", "account=", "item=", "ip=", "path=", "%p", "query="])),
]

failed = [name for name, ok in checks if not ok]
for name, ok in checks:
    print(f"[{'PASS' if ok else 'FAIL'}] {name}")
if failed:
    raise SystemExit("failed persistence status checks: " + ", ".join(failed))

print("persistence status contract checks passed")
