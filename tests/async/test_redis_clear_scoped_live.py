#!/usr/bin/env python3
"""Verify destructive Redis maintenance deletes only Duris-owned key prefixes."""

from __future__ import annotations

import os
import shutil
import socket
import subprocess
import tempfile
import time
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
HELPER = ROOT / "scripts" / "clear-duris-redis-keys.sh"


def free_port() -> int:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as listener:
        listener.bind(("127.0.0.1", 0))
        return int(listener.getsockname()[1])


def run_helper(port: int, **updates: str) -> subprocess.CompletedProcess[str]:
    target = f"127.0.0.1:{port}/0"
    environment = os.environ.copy()
    environment.update(
        {
            "ENVIRONMENT": "local",
            "REDIS_HOST": "127.0.0.1",
            "REDIS_PORT": str(port),
            "REDIS_SOCKET": "",
            "REDIS_DB": "0",
            "REDIS_NAMESPACE": "duris:local:test",
            "REDIS_TLS": "FALSE",
            "REDIS_ALLOWED_TARGETS": target,
            "REDIS_DESTRUCTIVE_CONFIRM": target,
        }
    )
    environment.update(updates)
    return subprocess.run(
        [str(HELPER)],
        cwd=ROOT,
        env=environment,
        text=True,
        capture_output=True,
        check=False,
    )


def main() -> None:
    if not shutil.which("redis-server") or not shutil.which("redis-cli"):
        raise SystemExit("redis-server and redis-cli are required")

    source = HELPER.read_text(encoding="ascii")
    migration = (ROOT / "migrations" / "run_migration.sh").read_text(encoding="ascii")
    wrapper = (ROOT / "scripts" / "clear-redis.sh").read_text(encoding="ascii")
    for text in (source, migration, wrapper):
        assert "FLUSHDB" not in text and "FLUSHALL" not in text
    assert 'PATTERNS=("$REDIS_NAMESPACE:*" \'mud:*\' \'ship:snapshot:*\')' in source
    assert "REDIS_ALLOWED_TARGETS" in source
    assert "REDIS_DESTRUCTIVE_CONFIRM" in source
    assert "ENVIRONMENT must be local" in source
    assert "redis-cli is required" in source
    assert "postflight" in source
    assert "clear-duris-redis-keys.sh" in migration
    assert '[[ "${REDIS:-FALSE}" != "TRUE" ]]' in migration

    with tempfile.TemporaryDirectory(prefix="redis-scoped-clear-") as temp_dir:
        port = free_port()
        socket_path = Path(temp_dir) / "redis.sock"
        server = subprocess.Popen(
            [
                "redis-server",
                "--bind",
                "127.0.0.1",
                "--port",
                str(port),
                "--unixsocket",
                str(socket_path),
                "--unixsocketperm",
                "700",
                "--save",
                "",
                "--appendonly",
                "no",
                "--dir",
                temp_dir,
                "--dbfilename",
                "isolated.rdb",
            ],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        cli = ["redis-cli", "-h", "127.0.0.1", "-p", str(port)]
        try:
            for _ in range(50):
                ready = subprocess.run(
                    [*cli, "PING"], capture_output=True, text=True, check=False
                )
                if ready.returncode == 0 and ready.stdout.strip() == "PONG":
                    break
                time.sleep(0.02)
            else:
                raise AssertionError("isolated redis-server did not start")

            for key in (
                "duris:local:test:season:42:world_state:current",
                "mud:cache:named",
                "mud:season:42:world_state:current",
                "ship:snapshot:Tester",
                "other-application:key",
            ):
                subprocess.run([*cli, "SET", key, "value"], check=True, capture_output=True)

            wrong_environment = run_helper(port, ENVIRONMENT="production")
            assert wrong_environment.returncode == 2
            wrong_confirmation = run_helper(port, REDIS_DESTRUCTIVE_CONFIRM="wrong")
            assert wrong_confirmation.returncode == 2
            wrong_allowlist = run_helper(port, REDIS_ALLOWED_TARGETS="127.0.0.1:1/0")
            assert wrong_allowlist.returncode == 2
            wrong_namespace = run_helper(port, REDIS_NAMESPACE="duris:production:test")
            assert wrong_namespace.returncode == 2

            cleared = run_helper(port)
            assert cleared.returncode == 0, cleared
            assert "deleted 4 Duris Redis keys" in cleared.stdout
            assert "postflight clean" in cleared.stdout
            for key in (
                "duris:local:test:season:42:world_state:current",
                "mud:cache:named",
                "mud:season:42:world_state:current",
                "ship:snapshot:Tester",
            ):
                exists = subprocess.run(
                    [*cli, "EXISTS", key], capture_output=True, text=True, check=True
                )
                assert exists.stdout.strip() == "0"
            unrelated = subprocess.run(
                [*cli, "GET", "other-application:key"],
                capture_output=True,
                text=True,
                check=True,
            )
            assert unrelated.stdout.strip() == "value"

            subprocess.run(
                [*cli, "SET", "duris:local:test:season:43:cache:named", "value"],
                check=True,
                capture_output=True,
            )
            socket_target = f"unix:{socket_path}/0"
            socket_cleared = run_helper(
                port,
                REDIS_HOST="",
                REDIS_PORT="",
                REDIS_SOCKET=str(socket_path),
                REDIS_ALLOWED_TARGETS=socket_target,
                REDIS_DESTRUCTIVE_CONFIRM=socket_target,
            )
            assert socket_cleared.returncode == 0, socket_cleared
            assert "deleted 1 Duris Redis keys" in socket_cleared.stdout
            exists = subprocess.run(
                [*cli, "EXISTS", "duris:local:test:season:43:cache:named"],
                capture_output=True,
                text=True,
                check=True,
            )
            assert exists.stdout.strip() == "0"

            maintenance_password = "maintenance-clear-test-secret"
            subprocess.run(
                [
                    *cli,
                    "ACL",
                    "SETUSER",
                    "maintenance",
                    "reset",
                    "on",
                    f">{maintenance_password}",
                    "~duris:local:test:*",
                    "~mud:*",
                    "~ship:snapshot:*",
                    "+ping",
                    "+select",
                    "+eval",
                    "+scan",
                    "+del",
                ],
                check=True,
                capture_output=True,
            )
            subprocess.run(
                [*cli, "SET", "duris:local:test:season:44:cache:named", "value"],
                check=True,
                capture_output=True,
            )
            subprocess.run(
                [*cli, "ACL", "SETUSER", "default", "off"],
                check=True,
                capture_output=True,
            )
            authenticated_clear = run_helper(
                port,
                REDIS_MAINTENANCE_USERNAME="maintenance",
                REDIS_MAINTENANCE_PASSWORD=maintenance_password,
            )
            assert authenticated_clear.returncode == 0, authenticated_clear
            assert "deleted 1 Duris Redis keys" in authenticated_clear.stdout

            partial_credentials = run_helper(
                port,
                REDIS_MAINTENANCE_USERNAME="maintenance",
                REDIS_MAINTENANCE_PASSWORD="",
            )
            assert partial_credentials.returncode == 2
            assert "must be configured together" in partial_credentials.stderr
        finally:
            server.terminate()
            server.wait(timeout=5)

    print("scoped Redis maintenance deletion and safety gates passed")


if __name__ == "__main__":
    main()
