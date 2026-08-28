from pathlib import Path
from contract_text import contains

ROOT = Path(__file__).resolve().parents[2]
header = (ROOT / "src/redis_maintenance.h").read_text()
source = (ROOT / "src/redis.c").read_text()
maintenance = (ROOT / "src/redis_maintenance.c").read_text()
floor = (ROOT / "src/redis_floor_runtime.c").read_text()
checkpoint = (ROOT / "src/persistence_checkpoint.c").read_text()
sql = (ROOT / "src/sql.c").read_text()

assert contains(header, "bool redis_clear_pwipe_state(void);")
assert contains(header, "bool redis_validate_pwipe_state(void);")
assert contains(source, "extern int                  _pwipe;")
for signature in ("void redis_log_floor_drop(P_obj obj, int room_vnum)",):
    start = floor.index(signature)
    body = floor[start:floor.index("\n}", start) + 2]
    assert contains(body, "if (_pwipe)"), signature
signature = "void mark_player_dirty(int pid)"
start = checkpoint.index(signature)
body = checkpoint[start:checkpoint.index("\n}", start) + 2]
assert contains(body, "if (_pwipe)"), signature

wipe = sql[sql.index("bool sql_pwipe(int code_verify)"):]
assert wipe.index("redis_validate_pwipe_state()") < wipe.index("sql_begin_pwipe_epoch()")
assert contains(wipe, 'DELETE FROM persistence_item_events')
assert contains(wipe, 'DELETE FROM persistence_scalar_events')
assert wipe.index("redis_clear_pwipe_state()") < wipe.rindex("return TRUE;")

# Season caches must be wiped so old-season scoreboards/lists cannot resurrect.
pwipe_fn = source[source.index("bool redis_clear_pwipe_state(void)"): source.index("void redis_cleanup(void)")]
assert contains(pwipe_fn, "redis_donation_worker_shutdown()")
assert contains(pwipe_fn, "redis_world_recovery_quiesce()")
assert contains(pwipe_fn, "redis_maintenance_clear(&config)")
for key in ("REDIS_LEGACY_FLOOR_DROPS", "REDIS_LEGACY_FLOOR_PICKUPS",
            "REDIS_LEGACY_ONLINE", "config->presence_current_key",
            "REDIS_LEGACY_PRESENCE_CURRENT",
            "REDIS_LEGACY_WORLD_CURRENT", "REDIS_LEGACY_WORLD_TIMESTAMP",
            "REDIS_LEGACY_WORLD_SEQUENCE", "REDIS_LEGACY_WORLD_CHECKSUM",
            "REDIS_LEGACY_WORLD_COMPLETE", "REDIS_LEGACY_WORLD_FENCE"):
    assert contains(
        maintenance,
        f"delete_key_checked(context, {key})",
    )
for pattern in ("config->report_cache_pattern", "config->presence_retry_pattern",
                "config->presence_session_pattern", "REDIS_LEGACY_CACHE_PATTERN",
                "REDIS_LEGACY_PRESENCE_RETRY_PATTERN",
                "REDIS_LEGACY_PRESENCE_SESSION_PATTERN",
                "REDIS_LEGACY_WORLD_GENERATION_PATTERN"):
    assert contains(
        maintenance,
        f"clear_scan_match(context, {pattern})",
    )
assert contains(maintenance, "redis_clear_ship_snapshots(context)")
assert not contains(maintenance, "FLUSHALL")
assert contains(maintenance, "clear_scan_match")
assert contains(maintenance, "scan_match_empty(context, pattern)")
delete_checked = maintenance[
    maintenance.index("bool delete_key_checked") : maintenance.index(
        "bool delete_pair_checked"
    )
]
assert contains(delete_checked, "REDIS_SHARED_COMMAND_READ")
assert contains(delete_checked, '"EXISTS %s"')
print("Redis pwipe invalidation checks passed")
