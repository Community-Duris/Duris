from pathlib import Path
from contract_text import contains

ROOT = Path(__file__).resolve().parents[2]
header = (ROOT / "src/redis.h").read_text()
source = (ROOT / "src/redis.c").read_text()
sql = (ROOT / "src/sql.c").read_text()

assert contains(header, "bool redis_clear_pwipe_state(void);")
assert contains(header, "bool redis_validate_pwipe_state(void);")
assert contains(source, "extern int                  _pwipe;")
for signature in (
    "void redis_log_floor_drop(P_obj obj, int room_vnum)",
    "void mark_player_dirty(int pid)",
):
    start = source.index(signature)
    body = source[start:source.index("\n}", start) + 2]
    assert contains(body, "if (_pwipe)"), signature

wipe = sql[sql.index("bool sql_pwipe(int code_verify)"):]
assert wipe.index("redis_validate_pwipe_state()") < wipe.index("sql_begin_pwipe_epoch()")
assert contains(wipe, 'DELETE FROM persistence_item_events')
assert contains(wipe, 'DELETE FROM persistence_scalar_events')
assert wipe.index("redis_clear_pwipe_state()") < wipe.rindex("return TRUE;")

# Season caches must be wiped so old-season scoreboards/lists cannot resurrect.
pwipe_fn = source[source.index("bool redis_clear_pwipe_state(void)"): source.index("void redis_cleanup(void)")]
assert contains(pwipe_fn, "redis_clear_floor_drops_checked()")
for key in ("mud:floor_drops", "mud:floor_pickups", "mud:online",
            "mud:presence:current",
            "mud:world_state:current", "mud:world_state:timestamp",
            "mud:world_state:sequence", "mud:world_state:checksum",
            "mud:world_state:complete", "mud:world_state:writer_fence"):
    assert contains(pwipe_fn, f'redis_delete_key_checked("{key}")')
assert contains(pwipe_fn, 'redis_clear_scan_match("mud:cache:*)') or contains(pwipe_fn, 'redis_clear_scan_match("mud:cache:*")')
assert contains(pwipe_fn, 'redis_clear_scan_match("mud:presence_op:*")')
assert contains(pwipe_fn, 'redis_clear_scan_match("mud:presence:session:*")')
assert contains(pwipe_fn, 'redis_clear_scan_match("mud:world_state:generation:*")')
assert contains(pwipe_fn, "redis_clear_ship_snapshots()")
assert not contains(pwipe_fn, "FLUSHALL")
assert contains(source, "redis_clear_scan_match")
print("Redis pwipe invalidation checks passed")
