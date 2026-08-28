#!/usr/bin/env python3
"""Runtime and source contracts for redacted shared Redis command health."""

import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
REDIS = (ROOT / "src/redis.c").read_text()
WIZ = (ROOT / "src/wizredis.c").read_text()
ACTINF = (ROOT / "src/actinf.c").read_text()

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
print("shared Redis command observability runtime and source contracts passed")
