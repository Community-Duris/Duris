#!/usr/bin/env python3
"""Exercise asynchronous floor publication and snapshot barriers."""

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
#include "redis_floor_store.h"
#include <hiredis/hiredis.h>
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <thread>

int main(int argc, char **argv)
{
    assert(argc == 2);
    redis_floor_store_config config = {"127.0.0.1", atoi(argv[1]), 100, 100};
    assert(redis_floor_store_init(&config));
    redis_floor_mutation first[] = {{100, "one", false}, {200, "two", false}};
    assert(redis_floor_store_submit("mud:season:1:floor_drops", first, 2));
    assert(redis_floor_store_request_barrier());
    redis_floor_mutation after[] = {{100, nullptr, true}, {300, "three", false}};
    assert(redis_floor_store_submit("mud:season:1:floor_drops", after, 2));

    bool succeeded = false;
    for (int count = 0; count < 100 && !redis_floor_store_take_barrier(&succeeded); ++count)
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    assert(succeeded);
    redis_floor_store_health health = redis_floor_store_health_copy();
    assert(health.paused && health.queued_batches == 1);

    redisContext *context = redisConnect("127.0.0.1", config.port);
    assert(context && !context->err);
    redisReply *reply = (redisReply *)redisCommand(context, "HMGET mud:season:1:floor_drops 100 200 300");
    assert(reply && reply->type == REDIS_REPLY_ARRAY && reply->elements == 3);
    assert(reply->element[0]->type == REDIS_REPLY_STRING && !strcmp(reply->element[0]->str, "one"));
    assert(reply->element[1]->type == REDIS_REPLY_STRING && !strcmp(reply->element[1]->str, "two"));
    assert(reply->element[2]->type == REDIS_REPLY_NIL);
    freeReplyObject(reply);

    redis_floor_store_resume();
    assert(redis_floor_store_drain(2000));
    reply = (redisReply *)redisCommand(context, "HMGET mud:season:1:floor_drops 100 200 300");
    assert(reply && reply->type == REDIS_REPLY_ARRAY && reply->elements == 3);
    assert(reply->element[0]->type == REDIS_REPLY_NIL);
    assert(reply->element[1]->type == REDIS_REPLY_STRING && !strcmp(reply->element[1]->str, "two"));
    assert(reply->element[2]->type == REDIS_REPLY_STRING && !strcmp(reply->element[2]->str, "three"));
    freeReplyObject(reply);
    redis_floor_mutation shutdown_before[] = {{400, "four", false}};
    redis_floor_mutation shutdown_after[] = {{500, "five", false}};
    assert(redis_floor_store_submit("mud:season:1:floor_drops", shutdown_before, 1));
    assert(redis_floor_store_request_barrier());
    assert(redis_floor_store_submit("mud:season:1:floor_drops", shutdown_after, 1));
    assert(redis_floor_store_shutdown(2000));
    reply = (redisReply *)redisCommand(context, "HLEN mud:season:1:floor_drops");
    assert(reply && reply->type == REDIS_REPLY_INTEGER && reply->integer == 4);
    freeReplyObject(reply);
    redisFree(context);
    redis_floor_store_reset_for_tests();
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
                "-I", str(ROOT / "src"), str(ROOT / "src" / "redis_floor_store.c"),
                str(source), "-lhiredis", "-pthread", "-o", str(binary),
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
