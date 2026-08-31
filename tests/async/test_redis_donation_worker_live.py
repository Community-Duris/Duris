#!/usr/bin/env python3
"""Exercise background donation subscribe, validation, replay, and outage healing."""

from __future__ import annotations

from _paths import SRC
import hashlib
import hmac
import json
import shutil
import socket
import subprocess
import tempfile
import time
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SECRET = "donation-worker-live-test-secret-32-bytes"


def free_port() -> int:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as listener:
        listener.bind(("127.0.0.1", 0))
        return int(listener.getsockname()[1])


def payload(event_id: str = "worker-live-event-0001") -> str:
    issued_at = int(time.time())
    amount = 1234
    currency = "USD"
    character = "Async"
    message = "background delivery"
    canonical = (
        f"v1\n{event_id}\n{issued_at}\n{amount}\n{currency}\n1\n"
        f"{character}\n{message}"
    )
    signature = hmac.new(
        SECRET.encode("ascii"), canonical.encode("ascii"), hashlib.sha256
    ).hexdigest()
    return json.dumps(
        {
            "schema_version": 1,
            "event_id": event_id,
            "issued_at": issued_at,
            "amount_cents": amount,
            "currency": currency,
            "is_public": True,
            "character_name": character,
            "message": message,
            "signature": signature,
        },
        separators=(",", ":"),
    )


def main() -> None:
    if not shutil.which("redis-server") or not shutil.which("redis-cli"):
        raise SystemExit("redis-server and redis-cli are required")

    harness = r'''
#include "redis/redis_donation_worker.h"
#include "redis/redis_connection.h"

#include <cassert>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>

int main(int argc, char **argv)
{
    assert(argc == 3);
    redis_connection_options options = {
        "127.0.0.1", atoi(argv[1]), 100, 100, 0, nullptr, nullptr, false, nullptr, nullptr, false, nullptr};
    redis_connection_settings *settings = redis_connection_settings_create(&options);
    assert(settings);
    redis_donation_worker_config config = {settings, argv[2], "mud:season:7:nchat"};
    std::string oversized_channel(161, 'x');
    config.channel = oversized_channel.c_str();
    assert(!redis_donation_worker_init(&config));
    config.channel = "mud:season:7:nchat";
    assert(redis_donation_worker_init(&config));

    redis_donation_worker_health health = {};
    for (int attempt = 0; attempt < 100; ++attempt)
    {
        health = redis_donation_worker_health_copy();
        if (health.connected)
            break;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    assert(health.connected && health.connection_failures >= 1);
    assert(health.operations.calls >= 2);
    assert(health.operations.successes >= 1);
    assert(health.operations.failures >= 1);
    assert(health.operations.consecutive_failures == 0);
    assert(health.operations.last_success_available);
    assert(health.operations.last_success_age_msec < 1000);

    donation_event event = {};
    for (int attempt = 0; attempt < 100; ++attempt)
    {
        if (redis_donation_worker_take(&event))
            break;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    assert(!strcmp(event.event_id, "worker-live-event-0001"));
    assert(event.amount_cents == 1234 && !strcmp(event.currency, "USD"));
    assert(event.is_public && !strcmp(event.character_name, "Async"));
    assert(!strcmp(event.message, "background delivery"));

    for (int attempt = 0; attempt < 100; ++attempt)
    {
        health = redis_donation_worker_health_copy();
        if (health.received >= 84)
            break;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    assert(health.received >= 84 && health.validated == 81 && health.replayed >= 3);
    assert(health.queued == REDIS_DONATION_QUEUE_CAPACITY && health.dropped >= 16);
    redis_donation_worker_shutdown();
    health = redis_donation_worker_health_copy();
    assert(!health.initialized && !health.connected);
    redis_donation_worker_reset_for_tests();
    redis_connection_settings_destroy(settings);
    return 0;
}
'''

    with tempfile.TemporaryDirectory(prefix="redis-donation-worker-") as temp_dir:
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
                "-fsanitize=address,undefined",
                "-fno-omit-frame-pointer",
                "-I",
                str(SRC),
                str(SRC / "redis_connection.c"),
                str(SRC / "redis_donation_worker.c"),
                str(SRC / "redis_command_observability.c"),
                str(SRC / "donation_event.c"),
                str(source),
                "-lhiredis",
                "-lhiredis_ssl",
                "-lssl",
                "-lcjson",
                "-lcrypto",
                "-pthread",
                "-o",
                str(binary),
            ],
            check=True,
        )
        harness_process = subprocess.Popen([str(binary), str(port), SECRET])
        time.sleep(0.25)
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

            encoded = payload()
            old_season = subprocess.run(
                [
                    "redis-cli", "-h", "127.0.0.1", "-p", str(port),
                    "PUBLISH", "mud:season:6:nchat", encoded,
                ],
                capture_output=True,
                text=True,
                check=True,
            )
            assert int(old_season.stdout.strip()) == 0
            subscribers = 0
            for _ in range(80):
                published = subprocess.run(
                    [
                        "redis-cli",
                        "-h",
                        "127.0.0.1",
                        "-p",
                        str(port),
                        "PUBLISH",
                        "mud:season:7:nchat",
                        encoded,
                    ],
                    capture_output=True,
                    text=True,
                    check=True,
                )
                subscribers = int(published.stdout.strip())
                if subscribers:
                    break
                time.sleep(0.05)
            assert subscribers == 1
            for _ in range(3):
                subprocess.run(
                    [
                        "redis-cli",
                        "-h",
                        "127.0.0.1",
                        "-p",
                        str(port),
                        "PUBLISH",
                        "mud:season:7:nchat",
                        encoded,
                    ],
                    check=True,
                    stdout=subprocess.DEVNULL,
                )
                time.sleep(0.05)
            for index in range(80):
                subprocess.run(
                    [
                        "redis-cli",
                        "-h",
                        "127.0.0.1",
                        "-p",
                        str(port),
                        "PUBLISH",
                        "mud:season:7:nchat",
                        payload(f"worker-flood-event-{index:04d}"),
                    ],
                    check=True,
                    stdout=subprocess.DEVNULL,
                )
            assert harness_process.wait(timeout=15) == 0
        finally:
            if harness_process.poll() is None:
                harness_process.terminate()
                harness_process.wait(timeout=5)
            server.terminate()
            server.wait(timeout=5)

    print("live Redis donation worker outage healing and bounded delivery passed")


if __name__ == "__main__":
    main()
