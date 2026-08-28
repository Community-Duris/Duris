#!/usr/bin/env python3
"""Redis outage, child-watchdog, and floor-ack source contracts."""

from pathlib import Path

root = Path(__file__).resolve().parents[2]
text = (root / "src/redis.c").read_text()
checkpoint = (root / "src/persistence_checkpoint.c").read_text()
store = (root / "src/redis_world_store.c").read_text()
presence_worker = (root / "src/redis_presence_worker.c").read_text()
cache_store = (root / "src/redis_cache_store.c").read_text()
floor_store = (root / "src/redis_floor_store.c").read_text()
donation_worker = (root / "src/redis_donation_worker.c").read_text()
donation_runtime = (root / "src/redis_donation_runtime.c").read_text()
connection = (root / "src/redis_connection.c").read_text()
key_registry = (root / "src/redis_key_registry.def").read_text()
header = (root / "src/redis.h").read_text()
signals = (root / "src/signals.c").read_text()


def section(start: str, end: str, source: str = text) -> str:
    first = source.index(start)
    last = source.index(end, first)
    return source[first:last]


assert "redisCommand(" not in text
assert "redisConnect(" not in text
assert "redisConnectWithTimeout(" not in text
assert text.count("redisvCommand(") == 1
assert text.count("redisCommandArgv(") == 1
assert "redisvAppendCommand(" not in text
assert "redisGetReply(ctx," not in text
assert floor_store.count("redisAppendCommand(") == 6
assert floor_store.count("redisGetReply(context,") == 1
assert 'redisAppendCommand(context, "MULTI")' in floor_store
assert 'redisAppendCommand(context, "EXEC")' in floor_store
assert 'redisAppendCommand(context, "ZADD %b 0 %llu"' in floor_store
assert 'redisAppendCommand(context, "ZREM %b %llu"' in floor_store
assert "redisConnect(" not in store
assert "redisConnectWithTimeout(" not in store
assert store.count("redisvCommand(") == 1
command = section("static redisReply *redis_command", "static bool redis_scan_match_empty")
assert connection.count("redisConnectWithTimeout(") == 1
assert "redisInitiateSSL(context, ssl)" in connection
assert "X509_VERIFY_PARAM_set1_host" in connection
assert "X509_VERIFY_PARAM_set1_ip_asc" in connection
assert "redisSetTimeout" in connection
assert 'redisCommand(context, "AUTH %b %b"' in connection
assert 'redisCommand(context, "AUTH %b"' in connection
assert 'redisCommand(context, "SELECT %d"' in connection
assert "if (!ctx || ctx->err)" in command
assert "redis_command_outcome(ctx, false)" in command
assert "REDIS_REPLY_ERROR" in command and '"error_reply"' in command
assert '"timeout"' in command and '"transport"' in command and '"no_reply"' in command
assert "redis_shared_command_observability_record" in command
assert "redis_ctx = redis_connection_open(redis_settings);" in text
print("[PASS] all runtime Redis connections use bounded authenticated selected-database helpers")

assert "redisConnectWithTimeout(" not in donation_worker
assert "redis_connection_open(configured_connection)" in donation_worker
assert donation_worker.count("redisCommand(") == 1
for token in (
    "REDIS_DONATION_QUEUE_CAPACITY",
    "REDIS_DONATION_REPLAY_CAPACITY",
    "REDIS_DONATION_WORK_BATCH",
    "wait_for_retry(reconnect_delay_seconds)",
    "redis_donation_worker_take",
):
    assert token in donation_worker
donation_pulse = section(
    "void check_donation_messages", "} // namespace", donation_runtime
)
assert "redis_donation_worker_take" in donation_pulse
for forbidden in ("redis_command", "redis_ctx", "redisConnect", "redisGetReply", "poll("):
    assert forbidden not in donation_pulse
print("[PASS] donation connect, subscribe, validation, and replay work stay off the simulation thread")

assert "redisConnectWithTimeout(" not in presence_worker
assert "redis_connection_open(configured_connection)" in presence_worker
assert presence_worker.count("redisvCommand(") == 1
assert presence_worker.count("redisCommandArgv(") == 1
for token in (
    "REDIS_PRESENCE_QUEUE_CAPACITY",
    "REDIS_PRESENCE_MAX_PAYLOAD_BYTES",
    "pending_jobs.size() >= REDIS_PRESENCE_QUEUE_CAPACITY",
    "reconnect_delay_msec = std::min(reconnect_delay_msec * 2, 60000U)",
    "PRESENCE_SCRIPT",
    "PRESENCE_HEARTBEAT_SCRIPT",
    "REDIS_PRESENCE_HEARTBEAT_BATCH",
    "configured_heartbeat_interval_msec",
    "active_sessions",
    "REDIS_PRESENCE_MAX_COMMAND_ATTEMPTS",
    "redis_presence_worker_drain",
    "redis_presence_worker_cancel",
):
    assert token in presence_worker
online = section("void redis_player_online", "void redis_player_offline")
offline = section("void redis_player_offline", "void redis_clear_online_players")
for path in (online, offline):
    assert "redis_command" not in path and "redis_ctx" not in path
assert "redis_presence_worker_cancel();" in section(
    "bool redis_clear_pwipe_state", "bool redis_validate_pwipe_state"
)
assert "redis_presence_worker_shutdown" in section(
    "void redis_cleanup", "void redis_clear_floor_pickups"
)
print("[PASS] presence writes and lease refreshes use a bounded healing worker outside the simulation thread")

assert "redisConnectWithTimeout(" not in cache_store
assert "redis_connection_open(configured_connection)" in cache_store
assert cache_store.count("redisvCommand(") == 1
for token in (
    "REDIS_CACHE_QUEUE_CAPACITY",
    "REDIS_CACHE_QUEUE_MAX_BYTES",
    "REDIS_CACHE_LOCAL_CAPACITY",
    "REDIS_CACHE_MAX_VALUE_BYTES",
    "pending_jobs.size() >= REDIS_CACHE_QUEUE_CAPACITY",
    "pending_bytes > REDIS_CACHE_QUEUE_MAX_BYTES - bytes",
    "reconnect_delay_msec = std::min(reconnect_delay_msec * 2, 60000U)",
    "redis_cache_store_drain",
    "redis_cache_store_cancel",
):
    assert token in cache_store
cache_helpers = section("bool redis_cache_set", "#ifndef __NO_MYSQL__\nstatic void redis_ship_cache_key")
for forbidden in ("redis_command", "redis_ctx", "redis_reconnect"):
    assert forbidden not in cache_helpers
for token in ("redis_cache_store_set", "redis_cache_store_get", "redis_cache_store_delete"):
    assert token in cache_helpers
assert "redis_cache_store_cancel();" in section(
    "bool redis_clear_pwipe_state", "bool redis_validate_pwipe_state"
)
assert "redis_cache_store_shutdown" in section(
    "void redis_cleanup", "void redis_clear_floor_pickups"
)
prime = section("static void redis_prime_artifact_caches", "static bool redis_scan_match_empty")
assert "PTTL" in prime and "redis_cache_store_seed" in prime
print("[PASS] report caches use bounded local reads and asynchronous Redis publication")

init = section("bool redis_init(void)", "bool redis_clear_pwipe_state")
assert init.index("redis_enabled = true;") < init.index("redis_connection_open")
connect_failure = init[init.index("if (!redis_ctx)"):init.index("// check for world state")]
assert "redis_enabled = false;" not in connect_failure
assert "mud:dirty_players" not in init
snapshot = section(
    "struct persistence_dirty_save_snapshot persistence_dirty_save_snapshot_copy",
    "void event_flush_dirty_players",
    checkpoint,
)
assert "player_save_pipeline_health_copy" in snapshot
assert "snapshot.available = pipeline.initialized" in snapshot
dirty_count = section(
    "int get_dirty_player_count(void)",
    "struct persistence_dirty_save_snapshot",
    checkpoint,
)
assert "player_save_pipeline_dirty_count()" in dirty_count
assert "redis_command" not in dirty_count
print("[PASS] dirty health and count use local revisioned pipeline state")

mark = section(
    "void mark_player_dirty(int pid)", "void flush_dirty_players(void)", checkpoint
)
assert "mark_player_dirty_components(pid, PLAYER_CHECKPOINT_COMPONENT_ALL)" in mark
assert "player_save_pipeline_mark(pid, components)" in mark
assert "sql_save_player" not in mark
assert "sql_begin_transaction" not in mark
assert "redis_command" not in mark and "redis_reconnect" not in mark
flush = section(
    "void flush_dirty_players(void)", "int get_dirty_player_count(void)", checkpoint
)
assert "player_save_pipeline_checkpoint_dirty" in flush
for forbidden in ("redis_command", "redis_reconnect", "sql_save_player", "fork("):
    assert forbidden not in flush
print("[PASS] dirty marking and checkpoint capture do no Redis, SQL, filesystem, or fork work")

world = section("bool redis_save_world_state(void)", "bool redis_has_world_state(void)")
assert "fork(" not in world
assert "world_recovery_pipeline_request" in world
assert "world_recovery_pipeline_busy" in world
assert "redis_floor_store_request_barrier" in world
cleanup = section("void redis_cleanup(void)", "void redis_clear_floor_pickups(void)")
assert "redis_terminate_child" not in cleanup
assert "world_recovery_pipeline_shutdown" in cleanup
assert "waitpid(-1, &status, WNOHANG)" in signals
assert "bool take_reaped_child_status(pid_t pid, int *status)" in signals
print("[PASS] player and world persistence child paths are fully retired")

publisher = store[store.index("bool redis_world_store_publish"):]
assert "WORLD_PUBLISH_SCRIPT" in publisher and "EVAL %b 9" in publisher
assert "redis.call('GET',KEYS[1])~=ARGV[1]" in store
assert "current~=ARGV[2]" in store
assert "reply->type == REDIS_REPLY_INTEGER && reply->integer == 1" in publisher
assert "REDIS_WORLD_SEQUENCE_SUFFIX" in store and "REDIS_WORLD_CHECKSUM_SUFFIX" in store
assert '"world_state:sequence"' in key_registry and '"world_state:checksum"' in key_registry
assert "redis.call('DEL',KEYS[8],KEYS[9])" in store and "PEXPIRE" in store
print("[PASS] null, timeout, error reply, or rejected world CAS forces worker failure")

floor_flush = section("bool redis_flush_floor_drops(void)", "void redis_remove_floor_drop")
assert "redis_floor_store_submit" in floor_flush
assert "world_recovery_floor_ack_pending" not in floor_flush
assert floor_flush.index("return false;") < floor_flush.index("floor_drop_remove_count = 0;")
assert floor_flush.index("return false;") < floor_flush.index("floor_drop_batch_count = 0;")
assert "redis_append_command" not in floor_flush
assert "redis_collect_integer_replies" not in floor_flush
assert floor_flush.count("redis_command") == 0
assert "bool redis_flush_floor_drops(void);" in header
ack = section("void redis_world_recovery_pulse", "bool redis_world_recovery_drain")
assert "redis_clear_floor_drops_checked()" not in ack
assert "world_recovery_floor_ack_pending" not in ack
for token in ("redis_floor_store_take_barrier", "world_recovery_pipeline_request",
              "redis_floor_store_resume"):
    assert token in ack
for token in ("REDIS_FLOOR_QUEUE_CAPACITY", "REDIS_FLOOR_QUEUE_MAX_BYTES",
              "redis_floor_store_request_barrier", "redis_floor_store_take_barrier"):
    assert token in floor_store
event = section("void event_save_world_state", "bool redis_cache_set")
assert "redis_clear_floor_drops" not in event
print("[PASS] floor deltas use a bounded background pipeline and ordered snapshot barrier")

print("redis failure containment source contracts passed")
