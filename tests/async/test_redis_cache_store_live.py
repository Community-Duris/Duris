#!/usr/bin/env python3
"""Exercise local cache reads and bounded asynchronous Redis publication."""

from __future__ import annotations

from _paths import SRC
import shutil
import socket
import subprocess
import tempfile
import time
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


def free_port() -> int:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as listener:
        listener.bind(("127.0.0.1", 0))
        return int(listener.getsockname()[1])


def main() -> None:
    if not shutil.which("redis-server") or not shutil.which("redis-cli"):
        raise SystemExit("redis-server and redis-cli are required")

    harness = r'''
#include "redis/redis_cache_store.h"
#include "redis/redis_connection.h"

#include <hiredis/hiredis.h>

#include <cassert>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>

static redisReply *run(redisContext *context, const char *command)
{
    redisReply *reply = (redisReply *)redisCommand(context, command);
    assert(reply);
    return reply;
}

static char *append_suffix(const char *value, void *context)
{
    const char *suffix = static_cast<const char *>(context);
    const size_t size = strlen(value) + strlen(suffix);
    char *result = static_cast<char *>(malloc(size + 1));
    assert(result);
    strcpy(result, value);
    strcat(result, suffix);
    return result;
}

int main(int argc, char **argv)
{
    assert(argc == 3);
    const int live_port = atoi(argv[1]);
    const int unavailable_port = atoi(argv[2]);
    redis_connection_options options = {
        "127.0.0.1", live_port, 100, 100, 0, nullptr, nullptr, false, nullptr, nullptr, false, nullptr};
    redis_connection_settings *settings = redis_connection_settings_create(&options);
    assert(settings);
    redis_cache_store_config config = {settings};
    assert(redis_cache_store_init(&config));

    assert(redis_cache_store_seed("mud:cache:test", "warm", 30));
    char *local = redis_cache_store_get("mud:cache:test");
    assert(local && !strcmp(local, "warm"));
    free(local);
    local = redis_cache_store_transform(
        "mud:cache:test", append_suffix, const_cast<char *>("-rendered"));
    assert(local && !strcmp(local, "warm-rendered"));
    free(local);
    assert(!redis_cache_store_transform("mud:cache:test", nullptr, nullptr));

    // Local reads and repeated publication remain available before Redis starts.
    for (int value = 1; value <= 100; ++value)
    {
        const std::string payload = "value-" + std::to_string(value);
        assert(redis_cache_store_set("mud:cache:test", payload.c_str(), 0));
    }
    local = redis_cache_store_get("mud:cache:test");
    assert(local && !strcmp(local, "value-100"));
    free(local);
    redis_cache_store_health health = redis_cache_store_health_copy();
    assert(health.coalesced > 0 && health.queued <= 2);
    assert(redis_cache_store_drain(5000));

    redisContext *context = redisConnect("127.0.0.1", live_port);
    assert(context && !context->err);
    redisReply *reply = run(context, "GET mud:cache:test");
    assert(reply->type == REDIS_REPLY_STRING && !strcmp(reply->str, "value-100"));
    freeReplyObject(reply);

    assert(redis_cache_store_set("mud:cache:ttl", "short-lived", 1));
    local = redis_cache_store_get("mud:cache:ttl");
    assert(local && !strcmp(local, "short-lived"));
    free(local);
    assert(redis_cache_store_drain(1000));
    std::this_thread::sleep_for(std::chrono::milliseconds(1100));
    assert(!redis_cache_store_get("mud:cache:ttl"));
    reply = run(context, "EXISTS mud:cache:ttl");
    assert(reply->type == REDIS_REPLY_INTEGER && reply->integer == 0);
    freeReplyObject(reply);
    assert(redis_cache_store_delete("mud:cache:test"));
    assert(!redis_cache_store_get("mud:cache:test"));
    assert(redis_cache_store_drain(1000));
    reply = run(context, "EXISTS mud:cache:test");
    assert(reply->type == REDIS_REPLY_INTEGER && reply->integer == 0);
    freeReplyObject(reply);
    health = redis_cache_store_health_copy();
    assert(health.connection_failures >= 1);
    assert(health.operations.calls >= 3);
    assert(health.operations.successes == health.operations.calls);
    assert(health.operations.failures == 0);
    assert(health.operations.consecutive_failures == 0);
    assert(health.operations.last_success_available);
    assert(health.operations.last_success_age_msec < 1000);
    redisFree(context);
    assert(redis_cache_store_shutdown(1000));
    redis_connection_settings_destroy(settings);

    redis_cache_store_reset_for_tests();
    options.port = unavailable_port;
    settings = redis_connection_settings_create(&options);
    assert(settings);
    config.connection = settings;
    assert(redis_cache_store_init(&config));
    std::string oversized(REDIS_CACHE_MAX_VALUE_BYTES + 1, 'x');
    assert(!redis_cache_store_set("mud:cache:oversized", oversized.c_str(), 0));
    std::string maximum(REDIS_CACHE_MAX_VALUE_BYTES, 'y');
    size_t byte_rejections = 0;
    for (int index = 0; index < 8; ++index)
    {
        const std::string key = "mud:cache:bytes:" + std::to_string(index);
        if (!redis_cache_store_set(key.c_str(), maximum.c_str(), 0))
            ++byte_rejections;
    }
    health = redis_cache_store_health_copy();
    assert(byte_rejections > 0);
    assert(health.queued_bytes <= REDIS_CACHE_QUEUE_MAX_BYTES);
    redis_cache_store_cancel();

    redis_cache_store_reset_for_tests();
    assert(redis_cache_store_init(&config));
    size_t queue_rejections = 0;
    for (size_t index = 0; index < REDIS_CACHE_QUEUE_CAPACITY * 2; ++index)
    {
        const std::string key = "mud:cache:delete:" + std::to_string(index);
        if (!redis_cache_store_delete(key.c_str()))
            ++queue_rejections;
    }
    health = redis_cache_store_health_copy();
    assert(queue_rejections > 0);
    assert(health.queued <= REDIS_CACHE_QUEUE_CAPACITY);
    redis_cache_store_cancel();
    redis_cache_store_reset_for_tests();
    redis_connection_settings_destroy(settings);
    return 0;
}
'''

    with tempfile.TemporaryDirectory(prefix="redis-cache-store-") as temp_dir:
        temp = Path(temp_dir)
        live_port = free_port()
        unavailable_port = free_port()
        source = temp / "harness.cpp"
        binary = temp / "harness"
        source.write_text(harness, encoding="ascii")
        subprocess.run(
            [
                "g++",
                "-std=c++20",
                "-Wall",
                "-Wextra",
                "-Werror",
                "-fsanitize=address,undefined",
                "-fno-omit-frame-pointer",
                "-I",
                str(SRC),
                str(SRC / "redis_connection.c"),
                str(SRC / "redis_cache_store.c"),
                str(SRC / "redis_command_observability.c"),
                str(source),
                "-lhiredis",
                "-lhiredis_ssl",
                "-lssl",
                "-lcrypto",
                "-pthread",
                "-o",
                str(binary),
            ],
            check=True,
        )
        harness_process = subprocess.Popen(
            [str(binary), str(live_port), str(unavailable_port)]
        )
        time.sleep(0.25)
        server = subprocess.Popen(
            [
                "redis-server",
                "--bind",
                "127.0.0.1",
                "--port",
                str(live_port),
                "--save",
                "",
                "--appendonly",
                "no",
                "--dir",
                str(temp),
                "--dbfilename",
                "isolated.rdb",
            ],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        try:
            for _ in range(50):
                ready = subprocess.run(
                    ["redis-cli", "-h", "127.0.0.1", "-p", str(live_port), "PING"],
                    capture_output=True,
                    text=True,
                    check=False,
                )
                if ready.returncode == 0 and ready.stdout.strip() == "PONG":
                    break
                time.sleep(0.02)
            else:
                raise AssertionError("isolated redis-server did not start")
            assert harness_process.wait(timeout=15) == 0
        finally:
            if harness_process.poll() is None:
                harness_process.terminate()
                harness_process.wait(timeout=5)
            server.terminate()
            server.wait(timeout=5)

    print("live local cache and asynchronous Redis publication bounds passed")


if __name__ == "__main__":
    main()
