#!/usr/bin/env python3
"""Exercise asynchronous floor publication and snapshot barriers."""

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
#include "redis_floor_store.h"
#include "redis_connection.h"
#include "world_recovery_codec.h"
#include <hiredis/hiredis.h>
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <vector>

int main(int argc, char **argv)
{
    assert(argc == 2);
    const int live_port = atoi(argv[1]);
    redis_connection_options options = {
        "127.0.0.1", live_port, 100, 100, 0, nullptr, nullptr, false, nullptr, nullptr, false, nullptr};
    redis_connection_settings *settings = redis_connection_settings_create(&options);
    assert(settings);
    redis_floor_store_config config = {settings};
    assert(redis_floor_store_init(&config));
    const unsigned char binary_value[] = {'o', 0, 'e'};
    redis_floor_mutation first[] = {
        {100, binary_value, sizeof(binary_value), false},
        {200, reinterpret_cast<const unsigned char *>("two"), 3, false}};
    assert(redis_floor_store_submit("mud:season:1:floor_drops",
                                    "mud:season:1:floor_drop_index", first, 2));
    assert(redis_floor_store_request_barrier());
    redis_floor_mutation after[] = {
        {100, nullptr, 0, true},
        {300, reinterpret_cast<const unsigned char *>("three"), 5, false}};
    assert(redis_floor_store_submit("mud:season:1:floor_drops",
                                    "mud:season:1:floor_drop_index", after, 2));

    bool succeeded = false;
    for (int count = 0; count < 100 && !redis_floor_store_take_barrier(&succeeded); ++count)
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    assert(succeeded);
    redis_floor_store_health health = redis_floor_store_health_copy();
    assert(health.paused && health.queued_batches == 1);

    redisContext *context = redisConnect("127.0.0.1", live_port);
    assert(context && !context->err);
    redisReply *reply = (redisReply *)redisCommand(context, "HMGET mud:season:1:floor_drops 100 200 300");
    assert(reply && reply->type == REDIS_REPLY_ARRAY && reply->elements == 3);
    assert(reply->element[0]->type == REDIS_REPLY_STRING && reply->element[0]->len == 3 &&
           !memcmp(reply->element[0]->str, binary_value, sizeof(binary_value)));
    assert(reply->element[1]->type == REDIS_REPLY_STRING && !strcmp(reply->element[1]->str, "two"));
    assert(reply->element[2]->type == REDIS_REPLY_NIL);
    freeReplyObject(reply);
    reply = (redisReply *)redisCommand(context, "ZRANGE mud:season:1:floor_drop_index 0 -1");
    assert(reply && reply->type == REDIS_REPLY_ARRAY && reply->elements == 2);
    assert(!strcmp(reply->element[0]->str, "100") && !strcmp(reply->element[1]->str, "200"));
    freeReplyObject(reply);
    const long long first_page = 0;
    const long long last_page = 1;
    reply = (redisReply *)redisCommand(context,
                                       "ZRANGE %s %lld %lld",
                                       "mud:season:1:floor_drop_index",
                                       first_page, last_page);
    assert(reply && reply->type == REDIS_REPLY_ARRAY && reply->elements == 2);
    freeReplyObject(reply);

    redis_floor_store_resume();
    assert(redis_floor_store_drain(2000));
    reply = (redisReply *)redisCommand(context, "HMGET mud:season:1:floor_drops 100 200 300");
    assert(reply && reply->type == REDIS_REPLY_ARRAY && reply->elements == 3);
    assert(reply->element[0]->type == REDIS_REPLY_NIL);
    assert(reply->element[1]->type == REDIS_REPLY_STRING && !strcmp(reply->element[1]->str, "two"));
    assert(reply->element[2]->type == REDIS_REPLY_STRING && !strcmp(reply->element[2]->str, "three"));
    freeReplyObject(reply);
    reply = (redisReply *)redisCommand(context, "ZRANGE mud:season:1:floor_drop_index 0 -1");
    assert(reply && reply->type == REDIS_REPLY_ARRAY && reply->elements == 2);
    assert(!strcmp(reply->element[0]->str, "200") && !strcmp(reply->element[1]->str, "300"));
    freeReplyObject(reply);
    world_recovery_object_record object = {321, 1};
    world_recovery_item_snapshot item = {};
    item.item_uid = 600;
    item.root_item_uid = 600;
    item.vnum = 456;
    item.type = 7;
    std::vector<unsigned char> native_object(sizeof(object) + sizeof(item));
    memcpy(native_object.data(), &object, sizeof(object));
    memcpy(native_object.data() + sizeof(object), &item, sizeof(item));
    redis_floor_mutation encoded[] = {
        {600, native_object.data(), native_object.size(), false, true}};
    assert(redis_floor_store_submit("mud:season:1:floor_drops",
                                    "mud:season:1:floor_drop_index", encoded, 1));
    assert(redis_floor_store_drain(2000));
    reply = (redisReply *)redisCommand(context, "HGET mud:season:1:floor_drops 600");
    assert(reply && reply->type == REDIS_REPLY_STRING && reply->len > 5 &&
           !memcmp(reply->str, "WRF4:", 5));
    uint64_t root_uid = 0;
    assert(world_recovery_floor_object_root_uid(
        reinterpret_cast<const unsigned char *>(reply->str), reply->len, &root_uid));
    assert(root_uid == 600);
    freeReplyObject(reply);
    health = redis_floor_store_health_copy();
    assert(health.operations.calls >= 3);
    assert(health.operations.successes == health.operations.calls);
    assert(health.operations.failures == 0);
    assert(health.operations.consecutive_failures == 0);
    assert(health.operations.last_success_available);
    assert(health.operations.last_success_age_msec < 1000);
    redis_floor_mutation shutdown_before[] = {
        {400, reinterpret_cast<const unsigned char *>("four"), 4, false}};
    redis_floor_mutation shutdown_after[] = {
        {500, reinterpret_cast<const unsigned char *>("five"), 4, false}};
    assert(redis_floor_store_submit("mud:season:1:floor_drops",
                                    "mud:season:1:floor_drop_index", shutdown_before, 1));
    assert(redis_floor_store_request_barrier());
    assert(redis_floor_store_submit("mud:season:1:floor_drops",
                                    "mud:season:1:floor_drop_index", shutdown_after, 1));
    assert(redis_floor_store_shutdown(2000));
    reply = (redisReply *)redisCommand(context, "HLEN mud:season:1:floor_drops");
    assert(reply && reply->type == REDIS_REPLY_INTEGER && reply->integer == 5);
    freeReplyObject(reply);
    reply = (redisReply *)redisCommand(context, "ZCARD mud:season:1:floor_drop_index");
    assert(reply && reply->type == REDIS_REPLY_INTEGER && reply->integer == 5);
    freeReplyObject(reply);
    redisFree(context);
    redis_floor_store_reset_for_tests();
    redis_connection_settings_destroy(settings);
    return 0;
}
'''

    with tempfile.TemporaryDirectory(prefix="redis-floor-store-") as temp_dir:
        temp = Path(temp_dir)
        port = free_port()
        source = temp / "harness.cpp"
        binary = temp / "harness"
        source.write_text(harness, encoding="ascii")
        subprocess.run(
            [
                "g++", "-std=c++20", "-Wall", "-Wextra", "-Werror",
                "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
                "-I", str(SRC), str(SRC / "redis_connection.c"),
                str(SRC / "redis_floor_store.c"),
                str(SRC / "redis_command_observability.c"),
                str(SRC / "world_recovery_codec.c"), str(source), "-lhiredis",
                "-lhiredis_ssl", "-lssl", "-lcrypto", "-pthread", "-o", str(binary),
            ],
            check=True,
        )
        harness_process = subprocess.Popen([str(binary), str(port)])
        time.sleep(0.25)
        server = subprocess.Popen(
            ["redis-server", "--bind", "127.0.0.1", "--port", str(port),
             "--save", "", "--appendonly", "no", "--dir", str(temp)],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        try:
            for _ in range(50):
                ready = subprocess.run(
                    ["redis-cli", "-h", "127.0.0.1", "-p", str(port), "PING"],
                    capture_output=True, text=True, check=False,
                )
                if ready.returncode == 0 and ready.stdout.strip() == "PONG":
                    break
                time.sleep(0.02)
            else:
                raise AssertionError("isolated redis-server did not start")
            assert harness_process.wait(timeout=10) == 0
        finally:
            if harness_process.poll() is None:
                harness_process.terminate()
                harness_process.wait(timeout=5)
            server.terminate()
            server.wait(timeout=5)

    print("live asynchronous Redis floor publication barrier passed")


if __name__ == "__main__":
    main()
