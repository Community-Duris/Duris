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
#include <hiredis/hiredis.h>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <ctime>

static redisReply *run(redisContext *context, const char *command)
{
    redisReply *reply = (redisReply *)redisCommand(context, command);
    assert(reply);
    return reply;
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    redis_world_store_config config = {"127.0.0.1", atoi(argv[1]), 250, 100};
    constexpr const char *writer_a = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    constexpr const char *writer_b = "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";
    constexpr uint64_t lease = 600000;
    const unsigned char first[] = "generation-one";
    const unsigned char second[] = "generation-two";

    assert(redis_world_store_claim_fence(&config, writer_a, lease));
    assert(!redis_world_store_claim_fence(&config, writer_b, lease));
    assert(redis_world_store_renew_fence(&config, writer_a, lease));
    assert(!redis_world_store_renew_fence(&config, writer_b, lease));

    redisContext *context = redisConnect("127.0.0.1", config.port);
    assert(context && !context->err);
    freeReplyObject(run(context, "HSET mud:floor_drops 100 delta"));

    assert(!redis_world_store_publish(&config, writer_b, lease, first, sizeof(first) - 1,
                                      1, time(nullptr), 11));
    redisReply *reply = run(context, "EXISTS mud:world_state:current");
    assert(reply->type == REDIS_REPLY_INTEGER && reply->integer == 0);
    freeReplyObject(reply);
    reply = run(context, "HLEN mud:floor_drops");
    assert(reply->type == REDIS_REPLY_INTEGER && reply->integer == 1);
    freeReplyObject(reply);

    assert(redis_world_store_publish(&config, writer_a, lease, first, sizeof(first) - 1,
                                     1, time(nullptr), 11));
    reply = run(context, "GET mud:world_state:current");
    assert(reply->type == REDIS_REPLY_STRING && !strcmp(reply->str, "1"));
    freeReplyObject(reply);
    reply = run(context, "EXISTS mud:floor_drops");
    assert(reply->type == REDIS_REPLY_INTEGER && reply->integer == 0);
    freeReplyObject(reply);

    freeReplyObject(run(context, "HSET mud:floor_drops 200 newer-delta"));
    assert(!redis_world_store_publish(&config, writer_b, lease, second,
                                      sizeof(second) - 1, 2, time(nullptr), 22));
    reply = run(context, "HLEN mud:floor_drops");
    assert(reply->type == REDIS_REPLY_INTEGER && reply->integer == 1);
    freeReplyObject(reply);

    assert(redis_world_store_release_fence(&config, writer_a));
    assert(redis_world_store_claim_fence(&config, writer_b, lease));
    assert(!redis_world_store_renew_fence(&config, writer_a, lease));
    assert(redis_world_store_renew_fence(&config, writer_b, lease));
    assert(redis_world_store_publish(&config, writer_b, lease, second, sizeof(second) - 1,
                                     2, time(nullptr), 22));
    reply = run(context, "MGET mud:world_state:current mud:world_state:generation:1 mud:world_state:generation:2");
    assert(reply->type == REDIS_REPLY_ARRAY && reply->elements == 3);
    assert(reply->element[0]->type == REDIS_REPLY_STRING && !strcmp(reply->element[0]->str, "2"));
    assert(reply->element[1]->type == REDIS_REPLY_NIL);
    assert(reply->element[2]->type == REDIS_REPLY_STRING &&
           reply->element[2]->len == sizeof(second) - 1 &&
           !memcmp(reply->element[2]->str, second, sizeof(second) - 1));
    freeReplyObject(reply);
    reply = run(context, "EXISTS mud:floor_drops");
    assert(reply->type == REDIS_REPLY_INTEGER && reply->integer == 0);
    freeReplyObject(reply);
    assert(!redis_world_store_release_fence(&config, writer_a));
    assert(redis_world_store_release_fence(&config, writer_b));
    redisFree(context);
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
                str(ROOT / "src" / "redis_world_store.c"),
                str(source),
                "-lhiredis",
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
