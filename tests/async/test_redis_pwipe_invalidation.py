from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
header = (ROOT / "src/redis.h").read_text()
source = (ROOT / "src/redis.c").read_text()
sql = (ROOT / "src/sql.c").read_text()

assert "bool redis_clear_pwipe_state(void);" in header
assert "extern int                  _pwipe;" in source
for signature in (
    "void redis_log_floor_pickup(unsigned long obj_uid)",
    "void redis_log_floor_drop(P_obj obj, int room_vnum)",
    "void mark_player_dirty(int pid)",
    "bool redis_cache_ship_snapshot(struct ShipData *ship)",
):
    start = source.index(signature)
    body = source[start:source.index("\n}", start) + 2]
    assert "if (_pwipe)" in body, signature

wipe = sql[sql.index("bool sql_pwipe(int code_verify)"):]
assert 'DELETE FROM persistence_item_events' in wipe
assert 'DELETE FROM persistence_scalar_events' in wipe
assert wipe.index("redis_clear_pwipe_state()") < wipe.rindex("return TRUE;")

# Season caches must be wiped so old-season scoreboards/lists cannot resurrect.
pwipe_fn = source[source.index("bool redis_clear_pwipe_state(void)"): source.index("void redis_cleanup(void)")]
assert "redis_invalidate_fraglist()" in pwipe_fn
assert "redis_invalidate_epic_zones()" in pwipe_fn
assert "redis_invalidate_artifact_cache()" in pwipe_fn
assert 'redis_cache_del("mud:cache:named")' in pwipe_fn
assert 'redis_clear_scan_match("mud:cache:*)' in pwipe_fn or 'redis_clear_scan_match("mud:cache:*")' in pwipe_fn
assert 'redis_clear_scan_match("mud:cache:artifact:*")' in pwipe_fn
assert "redis_clear_ship_snapshots()" in pwipe_fn
assert "FLUSHALL" not in pwipe_fn
assert "redis_clear_scan_match" in source
print("Redis pwipe invalidation checks passed")
