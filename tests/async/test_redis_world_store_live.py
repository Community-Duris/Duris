#!/usr/bin/env python3
"""Exercise fenced world publication against an isolated local Redis server."""

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
#include "redis/redis_world_store.h"
#include "redis/redis_connection.h"
#include "world/world_recovery_pipeline.h"
#include "world/world_recovery_codec.h"
#include <hiredis/hiredis.h>
#include <array>
#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <vector>
#include <zlib.h>

static redisReply *run(redisContext *context, const char *command)
{
    redisReply *reply = (redisReply *)redisCommand(context, command);
    assert(reply);
    return reply;
}

static std::vector<unsigned char> large_valid_generation(world_recovery_header *header)
{
    assert(header);
    constexpr size_t tree_count = 40;
    constexpr size_t items_per_tree = WORLD_RECOVERY_MAX_ITEM_TREE;
    constexpr size_t record_bytes = WORLD_RECOVERY_WIRE_RECORD_HEADER_BYTES +
        WORLD_RECOVERY_WIRE_OBJECT_HEADER_BYTES + items_per_tree * WORLD_RECOVERY_WIRE_ITEM_BYTES;
    std::vector<unsigned char> blob(WORLD_RECOVERY_WIRE_HEADER_BYTES);
    blob.reserve(WORLD_RECOVERY_WIRE_HEADER_BYTES + tree_count * record_bytes);
    std::array<unsigned char, WORLD_RECOVERY_MAX_RECORD_BYTES> payload = {};
    for (size_t tree = 0; tree < tree_count; ++tree) {
        world_recovery_object_record object = {100, static_cast<uint32_t>(items_per_tree)};
        std::vector<unsigned char> native(sizeof(object) + items_per_tree *
                                          sizeof(world_recovery_item_snapshot));
        memcpy(native.data(), &object, sizeof(object));
        const uint64_t root_uid = 1000000 + tree * items_per_tree;
        for (size_t index = 0; index < items_per_tree; ++index) {
            world_recovery_item_snapshot item = {};
            item.item_uid = root_uid + index;
            item.root_item_uid = root_uid;
            item.parent_item_uid = index ? root_uid : 0;
            item.vnum = 1000;
            item.type = ITEM_CONTAINER;
            memcpy(native.data() + sizeof(object) + index * sizeof(item), &item, sizeof(item));
        }
        size_t payload_size = 0;
        assert(world_recovery_encode_record(world_recovery_record_type::object,
                                            native.data(), native.size(), payload.data(),
                                            payload.size(), &payload_size));
        const size_t offset = blob.size();
        blob.resize(offset + WORLD_RECOVERY_WIRE_RECORD_HEADER_BYTES + payload_size);
        assert(world_recovery_encode_record_header(
            world_recovery_record_type::object, static_cast<uint32_t>(payload_size),
            blob.data() + offset, blob.size() - offset));
        memcpy(blob.data() + offset + WORLD_RECOVERY_WIRE_RECORD_HEADER_BYTES,
               payload.data(), payload_size);
    }
    memcpy(header->magic, "WR12", 4);
    header->schema_version = WORLD_RECOVERY_SCHEMA_VERSION;
    header->header_size = WORLD_RECOVERY_WIRE_HEADER_BYTES;
    header->sequence = 5;
    header->timestamp = time(nullptr);
    header->payload_size = blob.size() - WORLD_RECOVERY_WIRE_HEADER_BYTES;
    header->checksum = crc32(0, blob.data() + WORLD_RECOVERY_WIRE_HEADER_BYTES,
                             header->payload_size);
    header->object_count = tree_count;
    header->complete = 1;
    assert(world_recovery_encode_header(header, blob.data(), blob.size()));
    return blob;
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

    redis_shared_command_outcome publish_outcome = REDIS_SHARED_OUTCOME_SUCCESS;
    assert(!redis_world_store_publish_observed(
        &config, writer_b, lease, first, sizeof(first) - 1, 1, time(nullptr), 11,
        &publish_outcome));
    assert(publish_outcome == REDIS_SHARED_OUTCOME_ERROR_REPLY);
    redisReply *reply = run(context, "EXISTS mud:season:42:world_state:current");
    assert(reply->type == REDIS_REPLY_INTEGER && reply->integer == 0);
    freeReplyObject(reply);
    reply = run(context, "HLEN mud:season:42:floor_drops");
    assert(reply->type == REDIS_REPLY_INTEGER && reply->integer == 1);
    freeReplyObject(reply);
    reply = run(context, "ZCARD mud:season:42:floor_drop_index");
    assert(reply->type == REDIS_REPLY_INTEGER && reply->integer == 1);
    freeReplyObject(reply);

    assert(redis_world_store_publish_observed(
        &config, writer_a, lease, first, sizeof(first) - 1, 1, time(nullptr), 11,
        &publish_outcome));
    assert(publish_outcome == REDIS_SHARED_OUTCOME_SUCCESS);
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

    world_recovery_header large_header = {};
    const auto large_generation = large_valid_generation(&large_header);
    assert(large_generation.size() > 64 * 1024 * 1024);
    assert(large_generation.size() <= WORLD_RECOVERY_MAX_BYTES);
    assert(world_recovery_validate(large_generation.data(), large_generation.size(), 300, 5,
                                   nullptr));
    assert(redis_world_store_publish(&config, writer_b, lease, large_generation.data(),
                                     large_generation.size(), 5, large_header.timestamp,
                                     large_header.checksum));
    assert(redis_world_store_read_generation(&config, 5, &loaded));
    assert(loaded == large_generation);
    assert(world_recovery_validate(loaded.data(), loaded.size(), 300, 5, nullptr));
    std::printf("[PASS] isolated Redis publish/readback bytes=%zu chunks=%zu (>64 MiB)\n",
                large_generation.size(),
                (large_generation.size() + REDIS_WORLD_GENERATION_CHUNK_BYTES - 1) /
                    REDIS_WORLD_GENERATION_CHUNK_BYTES);
    reply = run(context, "GET mud:season:42:world_state:generation:5");
    assert(reply->type == REDIS_REPLY_STRING &&
           reply->len == REDIS_WORLD_GENERATION_MANIFEST_BYTES);
    std::vector<unsigned char> large_manifest(reply->str, reply->str + reply->len);
    freeReplyObject(reply);
    assert(redis_world_store_read_generation(&config, 5, &loaded));
    assert(loaded == large_generation);

    std::vector<unsigned char> over_limit(WORLD_RECOVERY_MAX_BYTES + 1);
    assert(!redis_world_store_publish(&config, writer_b, lease, over_limit.data(),
                                      over_limit.size(), 6, time(nullptr), 61));
    assert(redis_world_store_read_generation(&config, 5, &loaded));
    assert(loaded == large_generation);
    reply = run(context, "GET mud:season:42:world_state:current");
    assert(reply->type == REDIS_REPLY_STRING && !strcmp(reply->str, "5"));
    freeReplyObject(reply);

    redis_world_store_config wrong_large_key = config;
    wrong_large_key.authentication_secret = wrong_secret;
    assert(!redis_world_store_read_generation(&wrong_large_key, 5, &loaded));
    assert(loaded.empty());
    reply = (redisReply *)redisCommand(
        context, "SET mud:season:42:world_state:generation:5 %b", large_manifest.data(),
        large_manifest.size());
    assert(reply && reply->type == REDIS_REPLY_STATUS);
    freeReplyObject(reply);
    std::vector<unsigned char> forged_large_manifest = large_manifest;
    forged_large_manifest[88] ^= 0xff;
    reply = (redisReply *)redisCommand(
        context, "SET mud:season:42:world_state:generation:5 %b",
        forged_large_manifest.data(), forged_large_manifest.size());
    assert(reply && reply->type == REDIS_REPLY_STATUS);
    freeReplyObject(reply);
    assert(!redis_world_store_read_generation(&config, 5, &loaded));
    assert(loaded.empty());
    reply = (redisReply *)redisCommand(
        context, "SET mud:season:42:world_state:generation:5 %b", large_manifest.data(),
        large_manifest.size());
    assert(reply && reply->type == REDIS_REPLY_STATUS);
    freeReplyObject(reply);
    const size_t last_chunk_offset = 64 * REDIS_WORLD_GENERATION_CHUNK_BYTES;
    const size_t last_chunk_size = large_generation.size() - last_chunk_offset;
    std::vector<unsigned char> forged_large_chunk(
        large_generation.begin() + last_chunk_offset,
        large_generation.begin() + last_chunk_offset + last_chunk_size);
    forged_large_chunk[0] ^= 0xff;
    reply = (redisReply *)redisCommand(
        context,
        "SET mud:season:42:world_state:generation:5:upload:bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb:chunk:064 %b",
        forged_large_chunk.data(), forged_large_chunk.size());
    assert(reply && reply->type == REDIS_REPLY_STATUS);
    freeReplyObject(reply);
    assert(!redis_world_store_read_generation(&config, 5, &loaded));
    assert(loaded.empty());
    reply = run(context, "GET mud:season:42:world_state:current");
    assert(reply->type == REDIS_REPLY_STRING && !strcmp(reply->str, "5"));
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
                "-ffunction-sections",
                "-fdata-sections",
                "-I",
                str(SRC),
                str(SRC / "redis_connection.c"),
                str(SRC / "redis_command_observability.c"),
                str(SRC / "redis_namespace.c"),
                str(SRC / "redis_world_store.c"),
                str(SRC / "world/world_recovery_pipeline.c"),
                str(SRC / "world/world_recovery_codec.c"),
                str(SRC / "world/generated_npc_state.c"),
                str(SRC / "world/generated_npc_runtime.c"),
                str(SRC / "player/pet_restore_state.c"),
                str(source),
                "-lhiredis",
                "-lhiredis_ssl",
                "-lssl",
                "-lcrypto",
                "-lz",
                "-pthread",
                "-Wl,--gc-sections",
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
