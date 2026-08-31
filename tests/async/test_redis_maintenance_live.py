#!/usr/bin/env python3
"""Exercise isolated, scoped pwipe cleanup against a disposable Redis server."""

from _paths import rel
from pathlib import Path
import shutil
import socket
import subprocess
import tempfile
import time


ROOT = Path(__file__).resolve().parents[2]

HARNESS = r'''
#include "redis/redis_connection.h"
#include "redis/redis_maintenance.h"
#include "redis/redis_command_observability.h"

#include <hiredis/hiredis.h>

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <initializer_list>

bool redis_cache_store_delete(const char *)
{
    return true;
}

static void command_ok(redisContext *context, const char *format, int value)
{
    redisReply *reply = static_cast<redisReply *>(redisCommand(context, format, value));
    assert(reply && reply->type == REDIS_REPLY_STATUS);
    freeReplyObject(reply);
}

static void command_ok(redisContext *context, const char *format, const char *value)
{
    redisReply *reply = static_cast<redisReply *>(redisCommand(context, format, value));
    assert(reply && reply->type == REDIS_REPLY_STATUS);
    freeReplyObject(reply);
}

static long long count(redisContext *context, const char *pattern)
{
    redisReply *reply = static_cast<redisReply *>(redisCommand(context, "KEYS %s", pattern));
    assert(reply && reply->type == REDIS_REPLY_ARRAY);
    const long long result = static_cast<long long>(reply->elements);
    freeReplyObject(reply);
    return result;
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    redis_connection_options options = {
        "127.0.0.1", atoi(argv[1]), 250, 250, 0, nullptr, nullptr, false,
        nullptr, nullptr, false, nullptr};
    redis_connection_settings *settings = redis_connection_settings_create(&options);
    assert(settings);
    redisContext *context = redis_connection_open(settings);
    assert(context && !context->err);

    for (int index = 0; index < 600; ++index)
    {
        command_ok(context, "SET duris:local:test:season:7:world_state:generation:%d value", index);
        command_ok(context, "SET duris:local:test:season:7:presence:session:%d value", index);
        command_ok(context, "SET duris:local:test:season:7:presence:retry:%d value", index);
        command_ok(context, "SET duris:local:test:season:7:cache:item:%d value", index);
    }
    for (const char *key : {
             "duris:local:test:season:7:world_state:current",
             "duris:local:test:season:7:world_state:timestamp",
             "duris:local:test:season:7:world_state:sequence",
             "duris:local:test:season:7:world_state:checksum",
             "duris:local:test:season:7:world_state:complete",
             "duris:local:test:season:7:world_state:clean_shutdown",
             "duris:local:test:season:7:floor_drops",
             "duris:local:test:season:7:floor_drop_index",
             "duris:local:test:season:7:presence:current",
             "mud:floor_drops", "mud:floor_pickups", "mud:online",
             "mud:presence:current", "mud:presence:session:old",
             "mud:presence_op:old", "mud:world_state:generation:old",
             "mud:world_state:current", "mud:world_state:timestamp",
             "mud:world_state:sequence", "mud:world_state:checksum",
             "mud:world_state:complete", "mud:world_state:writer_fence",
             "mud:cache:old", "ship:snapshot:old", "unrelated:key"})
        command_ok(context, "SET %s value", key);

    const redis_maintenance_config config = {
        settings,
        "duris:local:test",
        7,
        "duris:local:test:season:7:presence:current",
        "duris:local:test:season:7:presence:session:*",
        "duris:local:test:season:7:presence:retry:*",
        "duris:local:test:season:7:cache:*",
    };
    redis_shared_command_observability_reset(true);
    assert(!redis_maintenance_validate(nullptr));
    assert(redis_maintenance_validate(&config));
    assert(redis_maintenance_clear(&config));

    for (const char *pattern : {
             "duris:local:test:season:7:world_state:generation:*",
             "duris:local:test:season:7:world_state:current",
             "duris:local:test:season:7:world_state:timestamp",
             "duris:local:test:season:7:world_state:sequence",
             "duris:local:test:season:7:world_state:checksum",
             "duris:local:test:season:7:world_state:complete",
             "duris:local:test:season:7:world_state:clean_shutdown",
             "duris:local:test:season:7:floor_drops",
             "duris:local:test:season:7:floor_drop_index",
             "duris:local:test:season:7:presence:*",
             "duris:local:test:season:7:cache:*",
             "mud:floor_drops", "mud:floor_pickups", "mud:online",
             "mud:presence:*", "mud:presence_op:*", "mud:world_state:*", "mud:cache:*",
             "ship:snapshot:*"})
        assert(count(context, pattern) == 0);
    assert(count(context, "unrelated:key") == 1);

    const redis_shared_command_health health = redis_shared_command_health_copy();
    const redis_shared_scope_health &maintenance =
        health.scopes[REDIS_SHARED_SCOPE_MAINTENANCE];
    assert(maintenance.calls > 2400 && maintenance.successes == maintenance.calls);
    assert(health.command_kind_calls[REDIS_SHARED_COMMAND_SCAN] > 8);
    assert(health.command_kind_calls[REDIS_SHARED_COMMAND_WRITE] > 2400);

    redisFree(context);
    redis_connection_settings_destroy(settings);
    return 0;
}
'''


def free_port() -> int:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as listener:
        listener.bind(("127.0.0.1", 0))
        return int(listener.getsockname()[1])


for executable in ("redis-server", "redis-cli", "g++"):
    if not shutil.which(executable):
        raise SystemExit(f"{executable} is required")

with tempfile.TemporaryDirectory(prefix="duris-redis-maintenance-") as temp_dir:
    temp = Path(temp_dir)
    source = temp / "maintenance_test.cpp"
    binary = temp / "maintenance_test"
    source.write_text(HARNESS, encoding="ascii")
    subprocess.run(
        [
            "g++", "-std=c++20", "-Wall", "-Wextra", "-Werror",
            "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-Isrc",
            str(source), rel("redis_maintenance.c"), rel("redis_ship_legacy.c"),
            rel("redis_connection.c"), rel("redis_command_observability.c"),
            rel("redis_namespace.c"), "-lhiredis", "-lhiredis_ssl", "-lssl",
            "-lcrypto", "-pthread", "-o", str(binary),
        ],
        cwd=ROOT,
        check=True,
    )
    port = free_port()
    server = subprocess.Popen(
        [
            "redis-server", "--bind", "127.0.0.1", "--port", str(port),
            "--save", "", "--appendonly", "no", "--dir", str(temp),
            "--dbfilename", "isolated.rdb",
        ],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    try:
        for _ in range(100):
            probe = subprocess.run(
                ["redis-cli", "-h", "127.0.0.1", "-p", str(port), "PING"],
                capture_output=True,
                text=True,
                check=False,
            )
            if probe.returncode == 0 and probe.stdout.strip() == "PONG":
                break
            time.sleep(0.02)
        else:
            raise AssertionError("isolated redis-server did not start")
        subprocess.run([str(binary), str(port)], check=True)
    finally:
        server.terminate()
        server.wait(timeout=5)

print("Scoped Redis maintenance cleanup checks passed")
