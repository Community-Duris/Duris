#!/usr/bin/env python3
"""Exercise bounded asynchronous presence writes and outage healing against Redis."""

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
#include "redis_presence_worker.h"

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

int main(int argc, char **argv)
{
    assert(argc == 3);
    const int live_port = atoi(argv[1]);
    const int unavailable_port = atoi(argv[2]);
    redis_presence_worker_config config = {"127.0.0.1", live_port, 100, 100, 6, 250};
    assert(redis_presence_worker_init(&config));

    // These submissions occur before the isolated server starts. They must stay local,
    // return immediately, retain order, and heal on the worker thread.
    assert(redis_presence_worker_submit_clear());
    assert(redis_presence_worker_submit_online(101, "{\"name\":\"Async\"}", true));
    assert(redis_presence_worker_drain(5000));

    redis_presence_worker_health health = redis_presence_worker_health_copy();
    assert(health.submitted == 2 && health.completed == 2);
    assert(health.reconnects >= 1 && health.queued == 0 && !health.busy);

    redisContext *context = redisConnect("127.0.0.1", live_port);
    assert(context && !context->err);
    redisReply *reply = run(context, "GET mud:presence:current");
    assert(reply->type == REDIS_REPLY_STRING);
    std::string instance(reply->str, reply->len);
    freeReplyObject(reply);
    const std::string session_key = "mud:presence:session:" + instance + ":101";
    reply = (redisReply *)redisCommand(context, "GET %s", session_key.c_str());
    assert(reply && reply->type == REDIS_REPLY_STRING);
    assert(!strcmp(reply->str, "{\"name\":\"Async\"}"));
    freeReplyObject(reply);
    std::this_thread::sleep_for(std::chrono::milliseconds(1200));
    reply = (redisReply *)redisCommand(context, "TTL %s", session_key.c_str());
    assert(reply && reply->type == REDIS_REPLY_INTEGER && reply->integer >= 4);
    freeReplyObject(reply);
    health = redis_presence_worker_health_copy();
    assert(health.active_sessions == 1 && health.lease_refreshes >= 1 &&
           health.lease_failures == 0);

    assert(redis_presence_worker_submit_offline(101, true));
    assert(redis_presence_worker_drain(1000));
    reply = (redisReply *)redisCommand(context, "GET %s", session_key.c_str());
    assert(reply->type == REDIS_REPLY_NIL);
    freeReplyObject(reply);

    // A permanent WRONGTYPE error must not write the retry marker or wedge the queue.
    freeReplyObject(run(context, "DEL mud:presence:current"));
    freeReplyObject(run(context, "LPUSH mud:presence:current wrong-type"));
    health = redis_presence_worker_health_copy();
    const uint64_t completed_before_error = health.completed;
    const uint64_t dropped_before_error = health.dropped;
    assert(redis_presence_worker_submit_online(303, "{}", false));
    assert(redis_presence_worker_drain(3000));
    health = redis_presence_worker_health_copy();
    assert(health.completed == completed_before_error);
    assert(health.dropped == dropped_before_error + 1);
    assert(health.command_failures >= REDIS_PRESENCE_MAX_COMMAND_ATTEMPTS);
    reply = run(context, "TYPE mud:presence:current");
    assert(reply->type == REDIS_REPLY_STATUS && !strcmp(reply->str, "list"));
    freeReplyObject(reply);
    assert(redis_presence_worker_submit_clear());
    assert(redis_presence_worker_drain(1000));
    reply = run(context, "TYPE mud:presence:current");
    assert(reply->type == REDIS_REPLY_STATUS && !strcmp(reply->str, "string"));
    freeReplyObject(reply);

    assert(redis_presence_worker_submit_online(404, "{\"name\":\"Lease\"}", false));
    assert(redis_presence_worker_drain(1000));
    reply = run(context, "GET mud:presence:current");
    assert(reply->type == REDIS_REPLY_STRING);
    const std::string expiring_key = "mud:presence:session:" +
                                     std::string(reply->str, reply->len) + ":404";
    freeReplyObject(reply);
    freeReplyObject(run(context, "SET mud:presence:current newer-instance EX 3"));
    std::this_thread::sleep_for(std::chrono::milliseconds(600));
    health = redis_presence_worker_health_copy();
    assert(health.active_sessions == 0 && health.lease_failures == 0);
    reply = run(context, "GET mud:presence:current");
    assert(reply->type == REDIS_REPLY_STRING && !strcmp(reply->str, "newer-instance"));
    freeReplyObject(reply);
    assert(redis_presence_worker_shutdown(1000));
    std::this_thread::sleep_for(std::chrono::milliseconds(6200));
    reply = (redisReply *)redisCommand(context, "EXISTS %s", expiring_key.c_str());
    assert(reply && reply->type == REDIS_REPLY_INTEGER && reply->integer == 0);
    freeReplyObject(reply);
    redisFree(context);

    // Cancellation may race a successful front job. Join before discarding the deque.
    redis_presence_worker_reset_for_tests();
    config.port = live_port;
    assert(redis_presence_worker_init(&config));
    for (int pid = 1; pid <= 512; ++pid)
        assert(redis_presence_worker_submit_online(pid, "{}", false));
    redis_presence_worker_cancel();
    health = redis_presence_worker_health_copy();
    assert(!health.initialized);
    assert(health.completed + health.dropped == health.submitted);

    // With Redis unavailable, the fixed queue must reject excess work without blocking.
    redis_presence_worker_reset_for_tests();
    config.port = unavailable_port;
    assert(redis_presence_worker_init(&config));
    std::string oversized(REDIS_PRESENCE_MAX_PAYLOAD_BYTES + 1, 'x');
    assert(!redis_presence_worker_submit_online(1, oversized.c_str(), false));
    health = redis_presence_worker_health_copy();
    const uint64_t dropped_before_saturation = health.dropped;
    size_t rejected = 0;
    for (size_t index = 0; index < REDIS_PRESENCE_QUEUE_CAPACITY * 2; ++index)
        if (!redis_presence_worker_submit_online(static_cast<int>(index + 1), "{}", false))
            ++rejected;
    health = redis_presence_worker_health_copy();
    assert(rejected > 0 && health.dropped == dropped_before_saturation + rejected);
    assert(health.queued <= REDIS_PRESENCE_QUEUE_CAPACITY);
    const size_t active_before_rejected_offline = health.active_sessions;
    assert(!redis_presence_worker_submit_offline(1, false));
    health = redis_presence_worker_health_copy();
    assert(active_before_rejected_offline > 0 &&
           health.active_sessions == active_before_rejected_offline - 1);
    redis_presence_worker_cancel();
    redis_presence_worker_reset_for_tests();
    return 0;
}
'''

    with tempfile.TemporaryDirectory(prefix="redis-presence-worker-") as temp_dir:
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
                str(ROOT / "src"),
                str(ROOT / "src" / "redis_presence_worker.c"),
                str(source),
                "-lhiredis",
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
            assert harness_process.wait(timeout=20) == 0
        finally:
            if harness_process.poll() is None:
                harness_process.terminate()
                harness_process.wait(timeout=5)
            server.terminate()
            server.wait(timeout=5)

    print("live Redis presence worker outage healing and queue bounds passed")


if __name__ == "__main__":
    main()
