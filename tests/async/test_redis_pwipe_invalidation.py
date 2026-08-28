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
assert contains(pwipe_fn, "redis_donation_worker_shutdown()")
assert contains(pwipe_fn, "redis_clear_floor_drops_checked()")
for key in ("REDIS_LEGACY_FLOOR_DROPS", "REDIS_LEGACY_FLOOR_PICKUPS",
            "REDIS_LEGACY_ONLINE", "redis_presence_current_key",
            "REDIS_LEGACY_PRESENCE_CURRENT",
            "REDIS_LEGACY_WORLD_CURRENT", "REDIS_LEGACY_WORLD_TIMESTAMP",
            "REDIS_LEGACY_WORLD_SEQUENCE", "REDIS_LEGACY_WORLD_CHECKSUM",
            "REDIS_LEGACY_WORLD_COMPLETE", "REDIS_LEGACY_WORLD_FENCE"):
    assert contains(
        pwipe_fn,
        f"redis_delete_key_checked(REDIS_SHARED_SCOPE_MAINTENANCE, {key})",
    )
for pattern in ("redis_cache_pattern", "redis_presence_retry_pattern",
                "redis_presence_session_pattern", "REDIS_LEGACY_CACHE_PATTERN",
                "REDIS_LEGACY_PRESENCE_RETRY_PATTERN",
                "REDIS_LEGACY_PRESENCE_SESSION_PATTERN",
                "REDIS_LEGACY_WORLD_GENERATION_PATTERN"):
    assert contains(
        pwipe_fn,
        f"redis_clear_scan_match(REDIS_SHARED_SCOPE_MAINTENANCE, {pattern})",
    )
assert contains(pwipe_fn, "redis_clear_ship_snapshots()")
assert not contains(pwipe_fn, "FLUSHALL")
assert contains(source, "redis_clear_scan_match")
assert contains(source, "redis_scan_match_empty(scope, pattern)")
delete_checked = source[
    source.index("static bool redis_delete_key_checked") : source.index("// rnum to vnum")
]
assert contains(delete_checked, "REDIS_SHARED_COMMAND_READ")
assert contains(delete_checked, '"EXISTS %s"')
print("Redis pwipe invalidation checks passed")
