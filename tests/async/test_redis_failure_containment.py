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
command = section("static redisReply *redis_command", "static bool redis_reply_status_ok")
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
assert 'redis_restore_dirty_snapshot("mud:dirty_players:flushing")' in init
snapshot = section(
    "struct persistence_dirty_save_snapshot redis_dirty_save_snapshot_copy",
    "void event_flush_dirty_players",
)
assert "snapshot.enabled = redis_enabled" in snapshot
assert "redis_ctx && !redis_ctx->err" in snapshot
dirty_count = section("int get_dirty_player_count(void)", "struct persistence_dirty_save_snapshot")
assert dirty_count.count("return redis_local_dirty_count;") >= 2
assert "count += (int)reply->integer;" in dirty_count
print("[PASS] enabled and transient availability states remain truthful")

mark = section("void mark_player_dirty(int pid)", "void flush_dirty_players(void)")
assert "if (!redis_enabled || pid <= 0)" in mark
assert mark.index("redis_local_dirty_add(pid);") < mark.index("redis_reconnect()")
assert mark.index("redis_dirty_metric_mark_active(pid);") < mark.index("redis_reconnect()")
assert "sql_save_player" not in mark
assert "sql_begin_transaction" not in mark
assert "redis_enabled = false" not in mark
local_flush = section("static bool redis_flush_local_dirty", "enum redis_child_poll_result")
assert "redis_local_dirty_remove(pid);" in local_flush
assert local_flush.index("redis_command(") < local_flush.index("redis_local_dirty_remove(pid);")
print("[PASS] null/reconnect/command failures retain local dirty intent without SQL stalls")

restore = section("static bool redis_restore_dirty_snapshot", "void mark_player_dirty")
assert "SUNIONSTORE mud:dirty_players 2 mud:dirty_players %s" in restore
assert restore.index("SUNIONSTORE") < restore.index('redis_command(redis_ctx, "DEL %s"')
flush = section("void flush_dirty_players(void)", "int get_dirty_player_count(void)")
assert flush.index("if (!redis_restore_dirty_snapshot(inflight_key))") < flush.index(
    '"RENAME mud:dirty_players %s"'
)
assert flush.count("redis_restore_dirty_snapshot(inflight_key);") >= 6
fork_failure = flush[flush.index("if (pid < 0)"):flush.index("if (pid == 0)")]
assert "sql_save_player" not in fork_failure
assert "redis_restore_dirty_snapshot(inflight_key);" in fork_failure
print("[PASS] boot, stale inflight, pre-fork, fork, and child failures merge without overwrite")

watchdog = section(
    "static enum redis_child_poll_result redis_poll_child", "static void redis_terminate_child"
)
assert "waitpid(pid, &status, WNOHANG)" in watchdog
assert "time(NULL) - *started >= timeout_sec" in watchdog
assert "kill(pid, SIGKILL)" in watchdog
assert "waitpid(pid, &status, 0)" in watchdog
assert "WIFEXITED(status) && WEXITSTATUS(status) == 0" in watchdog
assert watchdog.count("take_reaped_child_status(") >= 2
assert "alarm(REDIS_DIRTY_CHILD_TIMEOUT_SEC);" in flush
assert flush.index("signal(SIGALRM, SIG_DFL);") < flush.index(
    "alarm(REDIS_DIRTY_CHILD_TIMEOUT_SEC);"
)
world = section("bool redis_save_world_state(void)", "bool redis_has_world_state(void)")
assert "alarm(REDIS_WORLD_CHILD_TIMEOUT_SEC);" in world
assert world.index("signal(SIGALRM, SIG_DFL);") < world.index(
    "alarm(REDIS_WORLD_CHILD_TIMEOUT_SEC);"
)
assert "redis_poll_child(" in world
cleanup = section("void redis_cleanup(void)", "bool redis_ping(void)")
assert cleanup.count("redis_terminate_child(") == 2
assert "waitpid(-1, &status, WNOHANG)" in signals
assert "bool take_reaped_child_status(pid_t pid, int *status)" in signals
print("[PASS] dirty-save and world-snapshot children are bounded, reaped, and status-checked")

world_json = section(
    "static bool redis_save_world_state_json", "static bool redis_save_world_state_sync"
)
assert world_json.count("redis_reply_status_ok(reply)") == 3
world_sync = section("static bool redis_save_world_state_sync", "// forks child")
assert "redis_reply_status_ok(valid_reply)" in world_sync
assert "_exit(success ? 0 : 1);" in world
print("[PASS] null, timeout, error reply, or incomplete world writes force child failure")

floor_flush = section("bool redis_flush_floor_drops(void)", "void redis_remove_floor_drop")
assert "world_state_save_pid > 0 || world_state_snapshot_pending_ack" in floor_flush
assert floor_flush.index("return false;") < floor_flush.index("floor_drop_remove_count = 0;")
assert floor_flush.index("return false;") < floor_flush.index("floor_drop_batch_count = 0;")
assert "bool redis_flush_floor_drops(void);" in header
assert world.index("child_result == REDIS_CHILD_SUCCEEDED") < world.index(
    "world_state_snapshot_pending_ack = true;"
)
ack = world[world.index("if (world_state_snapshot_pending_ack)"):]
assert ack.index("redis_clear_floor_drops_checked()") < ack.index(
    "world_state_snapshot_pending_ack = false;"
)
assert ack.index("world_state_snapshot_pending_ack = false;") < ack.index(
    "redis_flush_floor_drops()"
)
event = section("void event_save_world_state", "bool redis_cache_set")
assert "redis_clear_floor_drops" not in event
print("[PASS] floor deltas clear only after exact snapshot success; newer deltas remain queued")

print("redis failure containment source contracts passed")
