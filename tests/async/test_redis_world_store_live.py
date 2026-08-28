#!/usr/bin/env python3
"""Exercise fenced world publication against an isolated local Redis server."""

from __future__ import annotations

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
#include "redis_world_store.h"
#include "redis_connection.h"
#include "world_recovery_pipeline.h"
#include <hiredis/hiredis.h>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <vector>

static redisReply *run(redisContext *context, const char *command)
{
    redisReply *reply = (redisReply *)redisCommand(context, command);
    assert(reply);
    return reply;
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    const int live_port = atoi(argv[1]);
    redis_connection_options options = {
        "127.0.0.1", live_port, 250, 100, 0, nullptr, nullptr, false, nullptr, nullptr, false, nullptr};
    redis_connection_settings *settings = redis_connection_settings_create(&options);
    assert(settings);
    constexpr const char *recovery_secret =
        "world-recovery-authentication-secret-0001";
    constexpr const char *rotated_secret =
        "world-recovery-authentication-secret-0002";
    constexpr const char *wrong_secret =
        "world-recovery-authentication-secret-wrong";
    redis_world_store_config config = {
        settings, "mud", recovery_secret, nullptr, 42, 3600};
    constexpr const char *writer_a = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    constexpr const char *writer_b = "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";
    constexpr uint64_t lease = 600000;
    const unsigned char first[] = "generation-one";
    const unsigned char second[] = "generation-two";

    assert(!redis_world_store_publish(&config, writer_a, lease, first,
                                      WORLD_RECOVERY_MAX_BYTES + 1, 1,
                                      time(nullptr), 11));

    assert(redis_world_store_claim_fence(&config, writer_a, lease));
    assert(!redis_world_store_claim_fence(&config, writer_b, lease));
    assert(redis_world_store_renew_fence(&config, writer_a, lease));
    assert(!redis_world_store_renew_fence(&config, writer_b, lease));

    redisContext *context = redisConnect("127.0.0.1", live_port);
    assert(context && !context->err);
    freeReplyObject(run(context, "HSET mud:season:42:floor_drops 100 delta"));
    freeReplyObject(run(context, "ZADD mud:season:42:floor_drop_index 0 100"));

    assert(!redis_world_store_publish(&config, writer_b, lease, first, sizeof(first) - 1,
                                      1, time(nullptr), 11));
    redisReply *reply = run(context, "EXISTS mud:season:42:world_state:current");
    assert(reply->type == REDIS_REPLY_INTEGER && reply->integer == 0);
    freeReplyObject(reply);
    reply = run(context, "HLEN mud:season:42:floor_drops");
    assert(reply->type == REDIS_REPLY_INTEGER && reply->integer == 1);
    freeReplyObject(reply);
    reply = run(context, "ZCARD mud:season:42:floor_drop_index");
    assert(reply->type == REDIS_REPLY_INTEGER && reply->integer == 1);
    freeReplyObject(reply);

    assert(redis_world_store_publish(&config, writer_a, lease, first, sizeof(first) - 1,
                                     1, time(nullptr), 11));
    reply = run(context, "GET mud:season:42:world_state:current");
    assert(reply->type == REDIS_REPLY_STRING && !strcmp(reply->str, "1"));
    freeReplyObject(reply);
    reply = run(context, "EXISTS mud:season:42:floor_drops");
    assert(reply->type == REDIS_REPLY_INTEGER && reply->integer == 0);
    freeReplyObject(reply);
    reply = run(context, "EXISTS mud:season:42:floor_drop_index");
    assert(reply->type == REDIS_REPLY_INTEGER && reply->integer == 0);
    freeReplyObject(reply);

    assert(!redis_world_store_publish(&config, writer_b, lease, second,
                                      sizeof(second) - 1, 1, time(nullptr), 22));
    std::vector<unsigned char> loaded;
    assert(redis_world_store_read_generation(&config, 1, &loaded));
    assert(loaded.size() == sizeof(first) - 1 &&
           !memcmp(loaded.data(), first, loaded.size()));

    freeReplyObject(run(context, "HSET mud:season:42:floor_drops 200 newer-delta"));
    freeReplyObject(run(context, "ZADD mud:season:42:floor_drop_index 0 200"));
    assert(!redis_world_store_publish(&config, writer_b, lease, second,
                                      sizeof(second) - 1, 2, time(nullptr), 22));
    reply = run(context, "HLEN mud:season:42:floor_drops");
    assert(reply->type == REDIS_REPLY_INTEGER && reply->integer == 1);
    freeReplyObject(reply);

    assert(redis_world_store_release_fence(&config, writer_a));
    assert(redis_world_store_claim_fence(&config, writer_b, lease));
    assert(!redis_world_store_renew_fence(&config, writer_a, lease));
    assert(redis_world_store_renew_fence(&config, writer_b, lease));
    assert(redis_world_store_publish(&config, writer_b, lease, second, sizeof(second) - 1,
                                     2, time(nullptr), 22));
    reply = run(context, "MGET mud:season:42:world_state:current mud:season:42:world_state:generation:1 mud:season:42:world_state:generation:2");
    assert(reply->type == REDIS_REPLY_ARRAY && reply->elements == 3);
    assert(reply->element[0]->type == REDIS_REPLY_STRING && !strcmp(reply->element[0]->str, "2"));
    assert(reply->element[1]->type == REDIS_REPLY_NIL);
    assert(reply->element[2]->type == REDIS_REPLY_STRING &&
           reply->element[2]->len == REDIS_WORLD_GENERATION_MANIFEST_BYTES);
    freeReplyObject(reply);
    assert(redis_world_store_read_generation(&config, 2, &loaded));
    assert(loaded.size() == sizeof(second) - 1 &&
           !memcmp(loaded.data(), second, loaded.size()));
    reply = run(context, "TTL mud:season:42:world_state:generation:2");
    assert(reply->type == REDIS_REPLY_INTEGER && reply->integer > 3500 && reply->integer <= 3600);
    freeReplyObject(reply);
    assert(!redis_world_store_mark_clean_shutdown(&config, writer_a));
    assert(redis_world_store_mark_clean_shutdown(&config, writer_b));
    reply = run(context, "TTL mud:season:42:world_state:clean_shutdown");
    assert(reply->type == REDIS_REPLY_INTEGER && reply->integer > 3500 && reply->integer <= 3600);
    freeReplyObject(reply);
    assert(redis_world_store_consume_clean_shutdown(&config) == 2);
    assert(redis_world_store_consume_clean_shutdown(&config) == 0);
    assert(!redis_world_store_consume_generation(&config, writer_a, 2));
    assert(!redis_world_store_consume_generation(&config, writer_b, 1));
    assert(redis_world_store_consume_generation(&config, writer_b, 2));
    reply = run(context, "EXISTS mud:season:42:world_state:current mud:season:42:world_state:generation:2");
    assert(reply->type == REDIS_REPLY_INTEGER && reply->integer == 0);
    freeReplyObject(reply);
    reply = run(context, "SCAN 0 MATCH mud:season:42:world_state:generation:2:upload:* COUNT 100");
    assert(reply->type == REDIS_REPLY_ARRAY && reply->elements == 2 &&
           reply->element[1]->type == REDIS_REPLY_ARRAY && reply->element[1]->elements == 0);
    freeReplyObject(reply);
    reply = run(context, "EXISTS mud:season:42:floor_drops");
    assert(reply->type == REDIS_REPLY_INTEGER && reply->integer == 0);
    freeReplyObject(reply);
    reply = run(context, "EXISTS mud:season:42:floor_drop_index");
    assert(reply->type == REDIS_REPLY_INTEGER && reply->integer == 0);
    freeReplyObject(reply);
    assert(!redis_world_store_release_fence(&config, writer_a));

    redis_world_store_config next_season = config;
    next_season.season_epoch = 43;
    assert(redis_world_store_claim_fence(&next_season, writer_a, lease));
    reply = run(context, "EXISTS mud:season:43:world_state:current");
    assert(reply->type == REDIS_REPLY_INTEGER && reply->integer == 0);
    freeReplyObject(reply);
    assert(redis_world_store_publish(&next_season, writer_a, lease, first,
                                     sizeof(first) - 1, 1, time(nullptr), 33));
    reply = run(context, "GET mud:season:43:world_state:generation:1");
    assert(reply->type == REDIS_REPLY_STRING &&
           reply->len == REDIS_WORLD_GENERATION_MANIFEST_BYTES);
    std::vector<unsigned char> replayed_manifest(reply->str, reply->str + reply->len);
    freeReplyObject(reply);
    reply = (redisReply *)redisCommand(
        context, "SET mud:season:42:world_state:generation:1 %b",
        replayed_manifest.data(), replayed_manifest.size());
    assert(reply && reply->type == REDIS_REPLY_STATUS);
    freeReplyObject(reply);
    assert(!redis_world_store_read_generation(&config, 1, &loaded));
    reply = (redisReply *)redisCommand(
        context, "SET mud:season:42:world_state:generation:99 %b",
        replayed_manifest.data(), replayed_manifest.size());
    assert(reply && reply->type == REDIS_REPLY_STATUS);
    freeReplyObject(reply);
    assert(!redis_world_store_read_generation(&config, 99, &loaded));
    freeReplyObject(run(context, "DEL mud:season:42:world_state:generation:1 mud:season:42:world_state:generation:99"));
    assert(redis_world_store_publish(&config, writer_b, lease, second, sizeof(second) - 1,
                                     3, time(nullptr), 44));
    std::vector<unsigned char> large(REDIS_WORLD_GENERATION_CHUNK_BYTES * 2 + 17);
    for (size_t index = 0; index < large.size(); ++index)
        large[index] = static_cast<unsigned char>(index);
    assert(redis_world_store_publish(&config, writer_b, lease, large.data(), large.size(),
                                     4, time(nullptr), 55));
    assert(redis_world_store_read_generation(&config, 4, &loaded));
    assert(loaded == large);
    redis_world_store_config wrong_key = config;
    wrong_key.authentication_secret = wrong_secret;
    assert(!redis_world_store_read_generation(&wrong_key, 4, &loaded));
    assert(loaded.empty());
    redis_world_store_config rotated = config;
    rotated.authentication_secret = rotated_secret;
    rotated.previous_authentication_secret = recovery_secret;
    assert(redis_world_store_read_generation(&rotated, 4, &loaded));
    assert(loaded == large);
    std::vector<unsigned char> forged_chunk(REDIS_WORLD_GENERATION_CHUNK_BYTES);
    memcpy(forged_chunk.data(), large.data(), forged_chunk.size());
    forged_chunk[0] ^= 0xff;
    reply = (redisReply *)redisCommand(
        context,
        "SET mud:season:42:world_state:generation:4:upload:bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb:chunk:000 %b",
        forged_chunk.data(), forged_chunk.size());
    assert(reply && reply->type == REDIS_REPLY_STATUS);
    freeReplyObject(reply);
    assert(!redis_world_store_read_generation(&config, 4, &loaded));
    assert(loaded.empty());
    assert(redis_world_store_publish(&config, writer_b, lease, large.data(), large.size(),
                                     4, time(nullptr), 59));
    reply = run(context, "GET mud:season:42:world_state:generation:4");
    assert(reply->type == REDIS_REPLY_STRING &&
           reply->len == REDIS_WORLD_GENERATION_MANIFEST_BYTES);
    std::vector<unsigned char> forged_manifest(reply->str, reply->str + reply->len);
    freeReplyObject(reply);
    forged_manifest[88] ^= 0xff;
    reply = (redisReply *)redisCommand(
        context, "SET mud:season:42:world_state:generation:4 %b",
        forged_manifest.data(), forged_manifest.size());
    assert(reply && reply->type == REDIS_REPLY_STATUS);
    freeReplyObject(reply);
    assert(!redis_world_store_read_generation(&config, 4, &loaded));
    assert(loaded.empty());
    reply = run(context, "TTL mud:season:42:world_state:generation:4:upload:bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb:chunk:000");
    assert(reply->type == REDIS_REPLY_INTEGER && reply->integer > 3500 && reply->integer <= 3600);
    freeReplyObject(reply);
    freeReplyObject(run(context, "DEL mud:season:42:world_state:generation:4:upload:bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb:chunk:001"));
    assert(!redis_world_store_read_generation(&config, 4, &loaded));
    assert(loaded.empty());
    assert(redis_world_store_publish(&config, writer_b, lease, large.data(), large.size(),
                                     4, time(nullptr), 56));
    std::vector<unsigned char> oversized(REDIS_WORLD_GENERATION_CHUNK_BYTES + 1, 1);
    reply = (redisReply *)redisCommand(
        context,
        "SET mud:season:42:world_state:generation:4:upload:bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb:chunk:001 %b",
        oversized.data(), oversized.size());
    assert(reply && reply->type == REDIS_REPLY_STATUS);
    freeReplyObject(reply);
    assert(!redis_world_store_read_generation(&config, 4, &loaded));
    assert(loaded.empty());
    assert(redis_world_store_publish(&config, writer_b, lease, large.data(), large.size(),
                                     4, time(nullptr), 57));
    freeReplyObject(run(context, "SET mud:season:42:world_state:generation:4 bad"));
    assert(!redis_world_store_read_generation(&config, 4, &loaded));
    assert(loaded.empty());
    assert(redis_world_store_publish(&config, writer_b, lease, large.data(), large.size(),
                                     4, time(nullptr), 58));
    assert(redis_world_store_read_generation(&config, 4, &loaded));
    assert(loaded == large);
    reply = run(context, "MGET mud:season:42:world_state:current mud:season:43:world_state:current");
    assert(reply->type == REDIS_REPLY_ARRAY && reply->elements == 2);
    assert(reply->element[0]->type == REDIS_REPLY_STRING && !strcmp(reply->element[0]->str, "4"));
    assert(reply->element[1]->type == REDIS_REPLY_STRING && !strcmp(reply->element[1]->str, "1"));
    freeReplyObject(reply);
    assert(redis_world_store_release_fence(&config, writer_b));
    assert(redis_world_store_release_fence(&next_season, writer_a));
    redisFree(context);
    redis_connection_settings_destroy(settings);
    return 0;
}
'''

    with tempfile.TemporaryDirectory(prefix="redis-world-store-") as temp_dir:
        temp = Path(temp_dir)
        port = free_port()
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
                "-I",
                str(ROOT / "src"),
                str(ROOT / "src" / "redis_connection.c"),
                str(ROOT / "src" / "redis_namespace.c"),
                str(ROOT / "src" / "redis_world_store.c"),
                str(source),
                "-lhiredis",
                "-lhiredis_ssl",
                "-lssl",
                "-lcrypto",
                "-o",
                str(binary),
            ],
            check=True,
        )
        server = subprocess.Popen(
            [
                "redis-server",
                "--bind",
                "127.0.0.1",
                "--port",
                str(port),
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
                    ["redis-cli", "-h", "127.0.0.1", "-p", str(port), "PING"],
                    capture_output=True,
                    text=True,
                    check=False,
                )
                if ready.returncode == 0 and ready.stdout.strip() == "PONG":
                    break
                time.sleep(0.02)
            else:
                raise AssertionError("isolated redis-server did not start")
            subprocess.run([str(binary), str(port)], check=True)
        finally:
            server.terminate()
            server.wait(timeout=5)

    print("live Redis world-store fencing and atomic floor handoff passed")


if __name__ == "__main__":
    main()
