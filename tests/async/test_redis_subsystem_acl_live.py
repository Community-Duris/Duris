#!/usr/bin/env python3
"""Prove Redis ACL users cannot cross subsystem key or channel boundaries."""

from __future__ import annotations

import shutil
import socket
import subprocess
import tempfile
import time


PASSWORDS = {
    "world": "world-test-secret",
    "presence": "presence-test-secret",
    "cache": "cache-test-secret",
    "donation": "donation-test-secret",
    "maintenance": "maintenance-test-secret",
}
PREFIX = "duris:local:test:season:42"


def free_port() -> int:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as listener:
        listener.bind(("127.0.0.1", 0))
        return int(listener.getsockname()[1])


def run(port: int, *arguments: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        ["redis-cli", "--raw", "-h", "127.0.0.1", "-p", str(port), *arguments],
        capture_output=True,
        text=True,
        check=False,
    )


def as_user(port: int, user: str, *arguments: str) -> subprocess.CompletedProcess[str]:
    return run(
        port,
        "--no-auth-warning",
        "--user",
        user,
        "-a",
        PASSWORDS[user],
        *arguments,
    )


def allowed(port: int, user: str, *arguments: str) -> None:
    result = as_user(port, user, *arguments)
    assert result.returncode == 0 and "NOPERM" not in result.stdout, result


def denied(port: int, user: str, *arguments: str) -> None:
    result = as_user(port, user, *arguments)
    assert "NOPERM" in result.stdout, result


def subscription_reply(port: int, user: str, channel: str) -> bytes:
    def command(*parts: str) -> bytes:
        payload = f"*{len(parts)}\r\n".encode()
        for part in parts:
            encoded = part.encode()
            payload += f"${len(encoded)}\r\n".encode() + encoded + b"\r\n"
        return payload

    with socket.create_connection(("127.0.0.1", port), timeout=1) as client:
        client.settimeout(0.2)
        client.sendall(
            command("AUTH", user, PASSWORDS[user]) + command("SUBSCRIBE", channel)
        )
        reply = b""
        while True:
            try:
                reply += client.recv(4096)
            except TimeoutError:
                return reply


def main() -> None:
    for executable in ("redis-server", "redis-cli"):
        if not shutil.which(executable):
            raise SystemExit(f"{executable} is required")

    with tempfile.TemporaryDirectory(prefix="redis-subsystem-acl-") as temp_dir:
        port = free_port()
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
                temp_dir,
            ],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        try:
            for _ in range(100):
                if run(port, "PING").stdout.strip() == "PONG":
                    break
                time.sleep(0.02)
            else:
                raise AssertionError("isolated redis-server did not start")

            rules = {
                "world": [f"~{PREFIX}:world_state:*", f"~{PREFIX}:floor_*", "+ping", "+select", "+get", "+set", "+del"],
                "presence": [f"~{PREFIX}:presence:*", f"&{PREFIX}:player", "+ping", "+select", "+get", "+set", "+del", "+publish"],
                "cache": [f"~{PREFIX}:cache:*", "+ping", "+select", "+get", "+set", "+del"],
                "donation": [f"&{PREFIX}:nchat", "+ping", "+select", "+subscribe"],
                "maintenance": ["~duris:local:test:*", "+ping", "+select", "+get", "+set", "+del"],
            }
            for user, permissions in rules.items():
                result = run(
                    port,
                    "ACL",
                    "SETUSER",
                    user,
                    "reset",
                    "on",
                    f">{PASSWORDS[user]}",
                    *permissions,
                )
                assert result.stdout.strip() == "OK", result

            world_key = f"{PREFIX}:world_state:current"
            presence_key = f"{PREFIX}:presence:current"
            cache_key = f"{PREFIX}:cache:named"
            allowed(port, "world", "SET", world_key, "1")
            denied(port, "world", "SET", presence_key, "1")
            allowed(port, "presence", "SET", presence_key, "1")
            allowed(port, "presence", "PUBLISH", f"{PREFIX}:player", "{}")
            denied(port, "presence", "SET", cache_key, "1")
            denied(port, "presence", "PUBLISH", f"{PREFIX}:nchat", "{}")
            allowed(port, "cache", "SET", cache_key, "1")
            denied(port, "cache", "GET", world_key)
            assert b"subscribe" in subscription_reply(
                port, "donation", f"{PREFIX}:nchat"
            )
            assert b"NOPERM" in subscription_reply(
                port, "donation", f"{PREFIX}:player"
            )
            denied(port, "donation", "GET", cache_key)
            allowed(port, "maintenance", "DEL", world_key, presence_key, cache_key)
            denied(port, "maintenance", "SET", "other-application:key", "1")
        finally:
            server.terminate()
            server.wait(timeout=5)

    print("Redis subsystem ACL isolation passed against isolated Redis")


if __name__ == "__main__":
    main()
