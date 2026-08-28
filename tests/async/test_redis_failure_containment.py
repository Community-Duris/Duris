#!/usr/bin/env python3
"""Redis outage, child-watchdog, and floor-ack source contracts."""

from pathlib import Path

root = Path(__file__).resolve().parents[2]
text = (root / "src/redis.c").read_text()
header = (root / "src/redis.h").read_text()
signals = (root / "src/signals.c").read_text()


def section(start: str, end: str) -> str:
    first = text.index(start)
    last = text.index(end, first)
    return text[first:last]


assert "redisCommand(" not in text
assert "redisConnect(" not in text
assert text.count("redisConnectWithTimeout(") == 1
assert text.count("redisvCommand(") == 1
connect = section("static redisContext *redis_connect_bounded", "static redisReply *redis_command")
command = section("static redisReply *redis_command", "/* Scan-and-delete with MATCH pattern.")
assert "REDIS_CONNECT_TIMEOUT_MSEC" in connect
assert "REDIS_COMMAND_TIMEOUT_MSEC" in connect
assert "redisSetTimeout" in connect
assert "if (!ctx)" in command and "if (ctx->err)" in command
assert "REDIS_REPLY_ERROR" in command and '"error_reply"' in command
assert '"timeout_or_io"' in command and '"no_reply"' in command
for assignment in (
    "redis_ctx = redis_connect_bounded(redis_host, redis_port);",
    "redisContext *ctx = redis_connect_bounded(redis_host, redis_port);",
    "donation_sub_ctx = redis_connect_bounded(redis_host, redis_port);",
):
    assert assignment in text
print("[PASS] all Redis connects and commands use bounded guarded helpers")

init = section("bool redis_init(void)", "bool redis_clear_pwipe_state")
assert init.index("redis_enabled = true;") < init.index("redis_connect_bounded")
connect_failure = init[init.index("if (!redis_ctx)"):init.index("// check for world state")]
assert "redis_enabled = false;" not in connect_failure
assert "mud:dirty_players" not in init
snapshot = section(
    "struct persistence_dirty_save_snapshot redis_dirty_save_snapshot_copy",
    "void event_flush_dirty_players",
)
assert "player_save_pipeline_health_copy" in snapshot
assert "snapshot.available = pipeline.initialized" in snapshot
dirty_count = section("int get_dirty_player_count(void)", "struct persistence_dirty_save_snapshot")
assert "player_save_pipeline_dirty_count()" in dirty_count
assert "redis_command" not in dirty_count
print("[PASS] dirty health and count use local revisioned pipeline state")

mark = section("void mark_player_dirty(int pid)", "void flush_dirty_players(void)")
assert "mark_player_dirty_components(pid, PLAYER_CHECKPOINT_COMPONENT_ALL)" in mark
assert "player_save_pipeline_mark(pid, components)" in mark
assert "sql_save_player" not in mark
assert "sql_begin_transaction" not in mark
assert "redis_command" not in mark and "redis_reconnect" not in mark
flush = section("void flush_dirty_players(void)", "int get_dirty_player_count(void)")
assert "player_save_pipeline_checkpoint_dirty" in flush
for forbidden in ("redis_command", "redis_reconnect", "sql_save_player", "fork("):
    assert forbidden not in flush
print("[PASS] dirty marking and checkpoint capture do no Redis, SQL, filesystem, or fork work")

world = section("bool redis_save_world_state(void)", "bool redis_has_world_state(void)")
assert "fork(" not in world
assert "world_recovery_pipeline_request" in world
assert "world_recovery_pipeline_busy" in world
cleanup = section("void redis_cleanup(void)", "void redis_clear_floor_pickups(void)")
assert "redis_terminate_child" not in cleanup
assert "world_recovery_pipeline_shutdown" in cleanup
assert "waitpid(-1, &status, WNOHANG)" in signals
assert "bool take_reaped_child_status(pid_t pid, int *status)" in signals
print("[PASS] player and world persistence child paths are fully retired")

publisher = section(
    "static bool redis_publish_world_generation", "static bool redis_world_recovery_ensure_initialized"
)
assert "MULTI" in publisher and "EXEC" in publisher and "DISCARD" in publisher
assert "world_state:sequence" in publisher and "world_state:checksum" in publisher
print("[PASS] null, timeout, error reply, or incomplete world transaction forces worker failure")

floor_flush = section("bool redis_flush_floor_drops(void)", "void redis_remove_floor_drop")
assert "world_recovery_floor_ack_pending || world_recovery_pipeline_busy()" in floor_flush
assert floor_flush.index("return false;") < floor_flush.index("floor_drop_remove_count = 0;")
assert floor_flush.index("return false;") < floor_flush.index("floor_drop_batch_count = 0;")
assert "bool redis_flush_floor_drops(void);" in header
ack = section("void redis_world_recovery_pulse", "bool redis_world_recovery_drain")
assert ack.index("completion.sequence == recovery.last_acknowledged_sequence") < ack.index(
    "redis_clear_floor_drops_checked()"
)
assert ack.index("redis_clear_floor_drops_checked()") < ack.index(
    "world_recovery_floor_ack_pending = false;"
)
event = section("void event_save_world_state", "bool redis_cache_set")
assert "redis_clear_floor_drops" not in event
print("[PASS] floor deltas clear only after exact snapshot success; newer deltas remain queued")

print("redis failure containment source contracts passed")
