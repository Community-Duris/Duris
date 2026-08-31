#!/usr/bin/env python3
"""Exercise Redis timeout and committed-but-lost world publish recovery."""

from __future__ import annotations

from _paths import SRC
import select
import shutil
import socket
import subprocess
import tempfile
import threading
import time
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


def free_port() -> int:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as listener:
        listener.bind(("127.0.0.1", 0))
        return int(listener.getsockname()[1])


class StallServer:
    def __init__(self) -> None:
        self.port = free_port()
        self._listener = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self._listener.bind(("127.0.0.1", self.port))
        self._listener.listen(1)
        self._thread = threading.Thread(target=self._run, daemon=True)

    def __enter__(self) -> "StallServer":
        self._thread.start()
        return self

    def __exit__(self, *unused: object) -> None:
        self._listener.close()
        self._thread.join(timeout=2)

    def _run(self) -> None:
        try:
            client, _ = self._listener.accept()
        except OSError:
            return
        with client:
            client.settimeout(2)
            try:
                while client.recv(4096):
                    pass
            except (OSError, TimeoutError):
                pass


class DropReplyProxy:
    def __init__(self, backend_port: int, marker: bytes) -> None:
        self.port = free_port()
        self.backend_port = backend_port
        self.marker = marker
        self.fault_fired = threading.Event()
        self._stop = threading.Event()
        self._listener = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self._listener.bind(("127.0.0.1", self.port))
        self._listener.listen(1)
        self._listener.settimeout(0.2)
        self._thread = threading.Thread(target=self._run, daemon=True)

    def __enter__(self) -> "DropReplyProxy":
        self._thread.start()
        return self

    def __exit__(self, *unused: object) -> None:
        self._stop.set()
        self._listener.close()
        self._thread.join(timeout=2)

    def _run(self) -> None:
        while not self._stop.is_set():
            try:
                client, _ = self._listener.accept()
            except socket.timeout:
                continue
            except OSError:
                return
            self._relay(client, not self.fault_fired.is_set())

    def _relay(self, client: socket.socket, drop_enabled: bool) -> None:
        with client, socket.create_connection(
            ("127.0.0.1", self.backend_port), timeout=2
        ) as backend:
            request_tail = b""
            drop_reply = False
            while True:
                readable, _, _ = select.select([client, backend], [], [], 2)
                if not readable:
                    return
                if client in readable:
                    request = client.recv(65536)
                    if request == b"":
                        return
                    if request:
                        combined = request_tail + request
                        if drop_enabled and self.marker in combined:
                            drop_reply = True
                        request_tail = combined[-len(self.marker) :]
                        backend.sendall(request)
                if backend in readable:
                    response = backend.recv(65536)
                    if response == b"":
                        return
                    if response:
                        if drop_reply:
                            self.fault_fired.set()
                            return
                        client.sendall(response)


HARNESS = r'''
#include "redis/redis_connection.h"
#include "redis/redis_world_store.h"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

int main(int argc, char **argv)
{
    if (argc != 3)
        return 90;
    redis_connection_options options = {
        "127.0.0.1", atoi(argv[1]), 100, 100, 0, nullptr, nullptr, false,
        nullptr, nullptr, false, nullptr};
    redis_connection_settings *settings = redis_connection_settings_create(&options);
    if (!settings)
        return 91;
    redis_world_store_config config = {
        settings, "duris:local:fault", "world-fault-live-authentication-secret", nullptr,
        77, 3600};
    constexpr const char *writer = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    constexpr unsigned char generation[] = "fault-recovery-generation";
    int result = 92;
    if (!strcmp(argv[2], "claim"))
    {
        result = redis_world_store_claim_fence(&config, writer, 600000) ? 0 : 2;
    }
    else
    {
        redis_shared_command_outcome outcome = REDIS_SHARED_OUTCOME_SUCCESS;
        const bool published = redis_world_store_publish_observed(
            &config, writer, 600000, generation, sizeof(generation) - 1, 1,
            time(nullptr), 1234, &outcome);
        fprintf(stderr, "published=%d outcome=%d errno=%d\n", published, outcome, errno);
        if (!strcmp(argv[2], "expect-timeout"))
            result = !published && outcome == REDIS_SHARED_OUTCOME_TIMEOUT ? 0 : 3;
        else if (!strcmp(argv[2], "expect-transport"))
            result = !published && outcome == REDIS_SHARED_OUTCOME_TRANSPORT ? 0 : 4;
        else if (!strcmp(argv[2], "expect-success"))
            result = published && outcome == REDIS_SHARED_OUTCOME_SUCCESS ? 0 : 5;
    }
    redis_connection_settings_destroy(settings);
    return result;
}
'''


FLOOR_HARNESS = r'''
#include "redis/redis_connection.h"
#include "redis/redis_floor_store.h"

#include <cstdlib>

int main(int argc, char **argv)
{
    if (argc != 2)
        return 90;
    redis_connection_options options = {
        "127.0.0.1", atoi(argv[1]), 100, 100, 0, nullptr, nullptr, false,
        nullptr, nullptr, false, nullptr};
    redis_connection_settings *settings = redis_connection_settings_create(&options);
    if (!settings)
        return 91;
    redis_floor_store_config config = {settings};
    if (!redis_floor_store_init(&config))
        return 92;
    const unsigned char value[] = "uncertain-exec";
    redis_floor_mutation mutation = {100, value, sizeof(value) - 1, false};
    if (!redis_floor_store_submit(
            "duris:local:fault:season:77:floor_drops",
            "duris:local:fault:season:77:floor_drop_index", &mutation, 1))
        return 93;
    if (!redis_floor_store_drain(5000))
        return 94;
    const redis_floor_store_health health = redis_floor_store_health_copy();
    const bool recovered = health.completed_batches == 1 && health.command_failures >= 1 &&
                           health.operations.failures >= 1 &&
                           health.operations.successes >= 1 &&
                           health.operations.consecutive_failures == 0;
    const bool stopped = redis_floor_store_shutdown(1000);
    redis_floor_store_reset_for_tests();
    redis_connection_settings_destroy(settings);
    return recovered && stopped ? 0 : 95;
}
'''


def run(binary: Path, port: int, mode: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [str(binary), str(port), mode],
        capture_output=True,
        text=True,
        check=False,
        timeout=8,
    )


def redis_cli(port: int, *arguments: str) -> str:
    result = subprocess.run(
        ["redis-cli", "-h", "127.0.0.1", "-p", str(port), *arguments],
        capture_output=True,
        text=True,
        check=True,
    )
    return result.stdout.strip()


def main() -> None:
    for executable in ("g++", "redis-cli", "redis-server"):
        if not shutil.which(executable):
            raise SystemExit(f"{executable} is required")

    with tempfile.TemporaryDirectory(prefix="redis-fault-recovery-") as temp_dir:
        temp = Path(temp_dir)
        source = temp / "harness.cpp"
        binary = temp / "harness"
        source.write_text(HARNESS, encoding="ascii")
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
                str(SRC / "redis_command_observability.c"),
                str(SRC / "redis_namespace.c"),
                str(SRC / "redis_world_store.c"),
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
        floor_source = temp / "floor_harness.cpp"
        floor_binary = temp / "floor_harness"
        floor_source.write_text(FLOOR_HARNESS, encoding="ascii")
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
                str(SRC / "redis_command_observability.c"),
                str(SRC / "redis_floor_store.c"),
                str(SRC / "world_recovery_codec.c"),
                str(floor_source),
                "-lhiredis",
                "-lhiredis_ssl",
                "-lssl",
                "-lcrypto",
                "-pthread",
                "-o",
                str(floor_binary),
            ],
            check=True,
        )

        with StallServer() as stall:
            timed_out = run(binary, stall.port, "expect-timeout")
            assert timed_out.returncode == 0, timed_out

        redis_port = free_port()
        server = subprocess.Popen(
            [
                "redis-server",
                "--bind",
                "127.0.0.1",
                "--port",
                str(redis_port),
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
                    [
                        "redis-cli",
                        "-h",
                        "127.0.0.1",
                        "-p",
                        str(redis_port),
                        "PING",
                    ],
                    capture_output=True,
                    text=True,
                    check=False,
                )
                if ready.returncode == 0 and ready.stdout.strip() == "PONG":
                    break
                time.sleep(0.02)
            else:
                raise AssertionError("isolated redis-server did not start")

            claimed = run(binary, redis_port, "claim")
            assert claimed.returncode == 0, claimed
            redis_cli(
                redis_port,
                "HSET",
                "duris:local:fault:season:77:floor_drops",
                "100",
                "delta",
            )
            redis_cli(
                redis_port,
                "ZADD",
                "duris:local:fault:season:77:floor_drop_index",
                "0",
                "100",
            )

            with DropReplyProxy(redis_port, b"\r\n$4\r\nEVAL\r\n") as proxy:
                ambiguous = run(binary, proxy.port, "expect-transport")
                assert ambiguous.returncode == 0, ambiguous
                assert proxy.fault_fired.wait(timeout=2)

            prefix = "duris:local:fault:season:77"
            assert redis_cli(redis_port, "GET", f"{prefix}:world_state:current") == "1"
            assert redis_cli(redis_port, "EXISTS", f"{prefix}:floor_drops") == "0"
            assert redis_cli(redis_port, "EXISTS", f"{prefix}:floor_drop_index") == "0"
            assert redis_cli(
                redis_port, "EXISTS", f"{prefix}:world_state:generation:1"
            ) == "1"

            retried = run(binary, redis_port, "expect-success")
            assert retried.returncode == 0, retried
            assert redis_cli(redis_port, "GET", f"{prefix}:world_state:current") == "1"

            redis_cli(redis_port, "DEL", f"{prefix}:floor_drops", f"{prefix}:floor_drop_index")
            with DropReplyProxy(redis_port, b"\r\n$4\r\nEXEC\r\n") as proxy:
                floor = subprocess.run(
                    [str(floor_binary), str(proxy.port)],
                    capture_output=True,
                    text=True,
                    check=False,
                    timeout=8,
                )
                assert floor.returncode == 0, floor
                assert proxy.fault_fired.wait(timeout=2)
            assert redis_cli(redis_port, "HGET", f"{prefix}:floor_drops", "100") == (
                "uncertain-exec"
            )
            assert redis_cli(redis_port, "ZSCORE", f"{prefix}:floor_drop_index", "100") == (
                "0"
            )
        finally:
            server.terminate()
            server.wait(timeout=5)

    print("live Redis timeout and uncertain publish/EXEC recovery passed")


if __name__ == "__main__":
    main()
