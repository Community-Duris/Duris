#!/usr/bin/env python3
"""Runtime and source contracts for redacted shared Redis command health."""

import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
REDIS = (ROOT / "src/redis.c").read_text()
WIZ = (ROOT / "src/wizredis.c").read_text()
ACTINF = (ROOT / "src/actinf.c").read_text()
WORLD_STORE = (ROOT / "src/redis_world_store.c").read_text()
WORKERS = {
    name: (ROOT / path).read_text()
    for name, path in {
        "presence": "src/redis_presence_worker.c",
        "cache": "src/redis_cache_store.c",
        "floor": "src/redis_floor_store.c",
        "donation": "src/redis_donation_worker.c",
        "world_publish": "src/world_recovery_pipeline.c",
    }.items()
}
WORKER_HEADERS = (
    ROOT / "src/redis_presence_worker.h",
    ROOT / "src/redis_cache_store.h",
    ROOT / "src/redis_floor_store.h",
    ROOT / "src/redis_donation_worker.h",
    ROOT / "src/world_recovery_pipeline.h",
)

HARNESS = r'''
#include "redis_command_observability.h"

#include <cassert>
#include <string>

int main()
{
    redis_shared_command_observability_reset(true);
    auto health = redis_shared_command_health_copy();
    assert(health.enabled);
    assert(!health.connection_available);
    redis_shared_connection_observability_record(false, true);

    redis_shared_command_observability_record(
        REDIS_SHARED_SCOPE_WORLD, REDIS_SHARED_COMMAND_READ,
        REDIS_SHARED_OUTCOME_SUCCESS, 10);
    redis_shared_command_observability_record(
        REDIS_SHARED_SCOPE_WORLD, REDIS_SHARED_COMMAND_READ,
        REDIS_SHARED_OUTCOME_SUCCESS, 20);
    redis_shared_command_observability_record(
        REDIS_SHARED_SCOPE_WORLD, REDIS_SHARED_COMMAND_WRITE,
        REDIS_SHARED_OUTCOME_TIMEOUT, 30);
    redis_shared_command_observability_record(
        REDIS_SHARED_SCOPE_FLOOR, REDIS_SHARED_COMMAND_READ,
        REDIS_SHARED_OUTCOME_UNAVAILABLE, 0);
    redis_shared_command_observability_record(
        REDIS_SHARED_SCOPE_WORLD, REDIS_SHARED_COMMAND_SCAN,
        REDIS_SHARED_OUTCOME_SUCCESS, 40);
    redis_shared_command_observability_record(
        REDIS_SHARED_SCOPE_CACHE, REDIS_SHARED_COMMAND_SCRIPT,
        REDIS_SHARED_OUTCOME_ERROR_REPLY, 50);
    redis_shared_command_observability_record(
        REDIS_SHARED_SCOPE_MAINTENANCE, REDIS_SHARED_COMMAND_SCAN,
        REDIS_SHARED_OUTCOME_TRANSPORT, 60);
    redis_shared_command_observability_record(
        REDIS_SHARED_SCOPE_MAINTENANCE, REDIS_SHARED_COMMAND_WRITE,
        REDIS_SHARED_OUTCOME_NO_REPLY, 70);

    health = redis_shared_command_health_copy();
    assert(health.connection_available);
    redis_shared_connection_observability_record(true, false);
    redis_shared_connection_observability_record(true, true);
    health = redis_shared_command_health_copy();

    const auto& world = health.scopes[REDIS_SHARED_SCOPE_WORLD];
    assert(world.calls == 4);
    assert(world.successes == 3);
    assert(world.failures == 1);
    assert(world.timeouts == 1);
    assert(world.total_latency_usec == 100);
    assert(world.last_latency_usec == 40);
    assert(world.max_latency_usec == 40);
    assert(world.consecutive_failures == 0);
    assert(world.last_success_available);
    assert(world.last_success_age_msec < 1000);

    assert(health.scopes[REDIS_SHARED_SCOPE_FLOOR].unavailable == 1);
    assert(health.scopes[REDIS_SHARED_SCOPE_CACHE].error_replies == 1);
    assert(health.scopes[REDIS_SHARED_SCOPE_MAINTENANCE].transport_failures == 1);
    assert(health.scopes[REDIS_SHARED_SCOPE_MAINTENANCE].no_replies == 1);
    assert(health.scopes[REDIS_SHARED_SCOPE_MAINTENANCE].consecutive_failures == 2);
    assert(health.command_kind_calls[REDIS_SHARED_COMMAND_READ] == 3);
    assert(health.command_kind_calls[REDIS_SHARED_COMMAND_WRITE] == 2);
    assert(health.command_kind_calls[REDIS_SHARED_COMMAND_SCAN] == 2);
    assert(health.command_kind_calls[REDIS_SHARED_COMMAND_SCRIPT] == 1);
    assert(health.connection_attempts == 1);
    assert(health.connection_failures == 0);
    assert(health.reconnect_attempts == 2);
    assert(health.reconnect_successes == 1);
    assert(health.reconnect_failures == 1);
    assert(health.recovery_transitions == 1);
    assert(health.connection_available);
    assert(redis_shared_command_scope_name(REDIS_SHARED_SCOPE_WORLD) ==
           std::string("world"));

    redis_worker_operation_health operations = {};
    redis_worker_operation_prepare_snapshot(&operations);
    assert(!operations.last_success_available);
    assert(operations.last_success_age_msec == 0);
    redis_worker_operation_record(
        &operations, REDIS_SHARED_OUTCOME_SUCCESS, 10);
    redis_worker_operation_record(
        &operations, REDIS_SHARED_OUTCOME_TIMEOUT, 20);
    redis_worker_operation_record(
        &operations, REDIS_SHARED_OUTCOME_ERROR_REPLY, 30);
    assert(operations.calls == 3);
    assert(operations.successes == 1);
    assert(operations.failures == 2);
    assert(operations.timeouts == 1);
    assert(operations.response_failures == 1);
    assert(operations.consecutive_failures == 2);
    assert(operations.total_latency_usec == 60);
    assert(operations.last_latency_usec == 30);
    assert(operations.max_latency_usec == 30);
    redis_worker_operation_prepare_snapshot(&operations);
    assert(operations.last_success_available);
    assert(operations.last_success_age_msec < 1000);
    redis_worker_operation_record(
        &operations, REDIS_SHARED_OUTCOME_SUCCESS, 40);
    assert(operations.calls == 4);
    assert(operations.successes == 2);
    assert(operations.failures == 2);
    assert(operations.consecutive_failures == 0);
    assert(operations.total_latency_usec == 100);
    assert(operations.last_latency_usec == 40);
    assert(operations.max_latency_usec == 40);

    redis_shared_command_observability_set_enabled(false);
    health = redis_shared_command_health_copy();
    assert(!health.enabled);
    assert(!health.connection_available);
    return 0;
}
'''

with tempfile.TemporaryDirectory(prefix="duris-redis-command-health-") as temp_dir:
    source = Path(temp_dir) / "redis_command_health_test.cpp"
    binary = Path(temp_dir) / "redis_command_health_test"
    source.write_text(HARNESS)
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-Isrc",
            str(source),
            "src/redis_command_observability.c",
            "-pthread",
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        check=True,
        capture_output=True,
        text=True,
    )
    subprocess.run([str(binary)], check=True)

for token in (
    "REDIS_SHARED_SCOPE_WORLD",
    "REDIS_SHARED_SCOPE_FLOOR",
    "REDIS_SHARED_SCOPE_CACHE",
    "REDIS_SHARED_SCOPE_MAINTENANCE",
    "REDIS_SHARED_COMMAND_READ",
    "REDIS_SHARED_COMMAND_WRITE",
    "REDIS_SHARED_COMMAND_SCAN",
    "REDIS_SHARED_COMMAND_SCRIPT",
    "redis_shared_command_observability_record",
):
    assert token in REDIS

assert "redis_shared_command_health_copy" in WIZ
assert "Redis is not queried" in WIZ
assert "redis_shared_command_health_copy" in ACTINF
assert "redis_shared state=" in ACTINF
for worker_name, worker_source in WORKERS.items():
    assert "redis_worker_operation_record" in worker_source, worker_name
    assert "redis_worker_operation_prepare_snapshot" in worker_source, worker_name
for header in WORKER_HEADERS:
    assert "redis_worker_operation_health" in header.read_text(), header
for worker_name in WORKERS:
    assert f'"{worker_name}"' in ACTINF
assert WIZ.count("redis_append_operation_health") == 6
assert "redis_world_store_publish_observed" in WORLD_STORE
assert "context->err == REDIS_ERR_TIMEOUT" in WORLD_STORE
print("shared and worker Redis command observability contracts passed")
