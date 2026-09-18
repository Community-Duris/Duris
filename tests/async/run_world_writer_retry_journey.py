#!/usr/bin/env python3
"""Exercise a boot-time writer lease on disposable MariaDB, Redis and game state."""
import os
from pathlib import Path
import re
import socket
import subprocess
import sys
import tempfile
import time
import uuid

import test_flatfile_combat_journey as journey

ROOT = Path(__file__).resolve().parents[2]
SECRET = "local-development-only-world-state-hmac-change-before-shared-use"


def free_port() -> int:
    with socket.socket() as listener:
        listener.bind(("127.0.0.1", 0))
        return listener.getsockname()[1]


def run(binary: Path, expect_stuck: bool, outage: bool, cancel: bool,
        auth: bool, repeat: bool) -> None:
    sql_host = os.environ.get("TEST_DB_HOST", "127.0.0.1")
    sql_port = os.environ.get("TEST_DB_PORT", "3306")
    assert sql_host in ("127.0.0.1", "localhost", "::1")
    database = "writer_retry_" + uuid.uuid4().hex[:12]
    namespace = "duris:local:retry_" + uuid.uuid4().hex[:8]
    redis_port = free_port()
    env = dict(os.environ, ENVIRONMENT="local", DB_HOST=sql_host, DB_PORT=sql_port,
               DB_NAME=database, DB_USER=os.environ["TEST_DB_USER"],
               DB_PASSWD=os.environ["TEST_DB_PASSWORD"],
               MYSQL_PWD=os.environ["TEST_DB_PASSWORD"],
               DB_ALLOWED_TARGETS=sql_host + "/" + database,
               PERSISTENCE_MODE="mariadb-primary", DB_TLS="FALSE",
               REDIS="TRUE", REDIS_HOST="127.0.0.1", REDIS_PORT=str(redis_port),
               REDIS_DB="0", REDIS_NAMESPACE=namespace, REDIS_TLS="FALSE",
               REDIS_ALLOWED_TARGETS=f"127.0.0.1:{redis_port}/0",
               REDIS_WORLD_STATE="TRUE", REDIS_WORLD_STATE_INTERVAL="5",
               REDIS_WORLD_STATE_SECRET=SECRET,
               REDIS_DONATION_SUBSCRIBER="FALSE",
               CHAOS_MUD="FALSE", LISTEN_ADDRESS="127.0.0.1",
               DURIS_WEBSOCKET_LISTEN_ADDRESS="127.0.0.1")
    if auth:
        env.update(REDIS_USERNAME="default", REDIS_PASSWORD="fixture-wrong",
                   REDISCLI_AUTH="fixture-correct")
    mysql = ["mysql", "--protocol=tcp", "-h", sql_host, "-P", sql_port,
             "-u", env["DB_USER"], "-N", "-B", "--unbuffered"]

    def sql(statement: str, selected: bool = True) -> str:
        return subprocess.check_output(mysql + ([database] if selected else []),
                                       input=statement, text=True, env=env,
                                       stderr=subprocess.DEVNULL).strip()

    with tempfile.TemporaryDirectory(prefix="writer-retry-") as temporary:
        root = Path(temporary)
        redis_dir = root / "redis"
        redis_dir.mkdir()
        def start_redis() -> subprocess.Popen:
            arguments = ["redis-server", "--bind", "127.0.0.1",
                         "--port", str(redis_port), "--save", "",
                         "--appendonly", "no", "--dir", str(redis_dir)]
            if auth:
                arguments += ["--requirepass", "fixture-correct"]
            return subprocess.Popen(
                arguments,
                env=env, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

        redis = start_redis()
        game_process = None
        client = None

        def command(*arguments: str) -> str:
            return subprocess.check_output(
                ["redis-cli", "--raw", "-h", "127.0.0.1", "-p", str(redis_port),
                 *arguments], env=env, text=True, stderr=subprocess.DEVNULL).strip()

        def wait_redis() -> None:
            deadline = time.monotonic() + 10
            while time.monotonic() < deadline:
                if redis.poll() is not None:
                    raise AssertionError("disposable Redis exited")
                try:
                    if command("PING") == "PONG":
                        return
                except subprocess.CalledProcessError:
                    pass
                time.sleep(.1)
            raise AssertionError("disposable Redis did not start")

        try:
            wait_redis()

            sql("CREATE DATABASE " + database, False)
            try:
                sql((ROOT / "migrations/bootstrap_multithread_safe.sql").read_text())
                for args in (("adopt", "--kind", "fresh_bootstrap"), ("run",)):
                    subprocess.run(["python3", "scripts/migration_runner.py", *args],
                                   cwd=ROOT, env=env, check=True, capture_output=True)
                assert sql("SELECT season_epoch FROM season_reset_state WHERE state_id=1") == "1"
                fence = namespace + ":season:1:world_state:writer_fence"
                current = namespace + ":season:1:world_state:current"
                old_token = "a" * 32
                assert command("SET", fence, old_token, "NX", "PX", "30000") == "OK"
                if outage:
                    redis.terminate()
                    redis.wait(timeout=10)
                game = root / "game"
                game.mkdir()
                (game / "logs/log").mkdir(parents=True)
                journey.make_fixture(game)
                journey.generate_certificate(game)
                for kind in ("players", "critical"):
                    (game / "journals" / kind).mkdir(parents=True, mode=0o700)
                port, tls, websocket = journey.available_ports()
                env.update(DURIS_TLS_PORT=str(tls),
                           DURIS_WEBSOCKET_PORT=str(websocket),
                           PLAYER_SAVE_JOURNAL_DIR=str(game / "journals/players"),
                           CRITICAL_COMMAND_JOURNAL_DIR=str(game / "journals/critical"))
                output_path = game / "server.out"
                with output_path.open("w") as output:
                    game_process = subprocess.Popen(
                        [str(binary), "--minimal", "-s", "-d", str(game), str(port)],
                        cwd=game, env=env, stdout=output, stderr=subprocess.STDOUT)
                deadline = time.monotonic() + 60
                while time.monotonic() < deadline and game_process.poll() is None:
                    if "Entering game loop." in output_path.read_text(errors="replace"):
                        break
                    time.sleep(.1)
                assert game_process.poll() is None, output_path.read_text(errors="replace")[-1500:]
                client = journey.MudClient(port)
                journey.create_character(client)
                def logs() -> str:
                    return output_path.read_text(errors="replace") + journey.runtime_logs(game)
                if outage:
                    assert "reason=redis_unavailable_at_boot" in logs(), logs()[-2000:]
                    redis = start_redis()
                    wait_redis()
                    print("outage: gameplay live while Redis was unavailable; service restored", flush=True)
                elif auth:
                    assert "reason=redis_unavailable_at_boot" in logs(), logs()[-2000:]
                    assert command("GET", fence) == old_token
                    assert command("CONFIG", "SET", "requirepass", "fixture-wrong") == "OK"
                    env["REDISCLI_AUTH"] = "fixture-wrong"
                    print("authentication: gameplay live during rejected credentials; Redis accepted configured credentials",
                          flush=True)
                else:
                    assert "reason=writer_lease_unavailable_at_boot" in logs(), logs()[-2000:]
                    assert command("GET", fence) == old_token, "boot stole the active lease"
                    print("boot: gameplay live; old writer lease retained; publisher degraded", flush=True)
                if not expect_stuck and not outage:
                    time.sleep(5)
                    assert command("GET", fence) == old_token, "active writer lease was displaced"
                    assert "world recovery generation and floor handoff acknowledged" not in logs()
                    print("contention: active writer retained fence; no competing publication", flush=True)
                if cancel:
                    game_process.terminate()
                    game_process.wait(timeout=20)
                    assert command("GET", fence) == old_token
                    assert command("GET", current) == ""
                    print("cancellation: shutdown left the competing writer fence and no publication",
                          flush=True)
                    return
                deadline = time.monotonic() + 75
                ack = None
                while time.monotonic() < deadline and game_process.poll() is None:
                    matches = re.findall(
                        r"world recovery generation and floor handoff acknowledged sequence=(\d+)",
                        logs())
                    if matches:
                        ack = int(matches[-1])
                        break
                    time.sleep(.5)
                if expect_stuck:
                    assert ack is None, "baseline unexpectedly recovered"
                    assert command("EXISTS", fence) == "0", "expired lease still exists"
                    assert "world recovery worker unavailable" in logs(), logs()[-2000:]
                    print("baseline: lease expired; no generation/floor acknowledgment; worker stayed quiesced",
                          flush=True)
                else:
                    assert ack is not None, logs()[-2500:]
                    assert int(command("GET", current)) == ack
                    assert command("GET", fence) != old_token
                    print("fixed: writer acquired lease; world generation and floor handoff acknowledged",
                          flush=True)
                    if repeat:
                        client.close()
                        client = None
                        game_process.terminate()
                        game_process.wait(timeout=30)
                        assert game_process.returncode == 0
                        assert command("EXISTS", fence) == "0", "clean shutdown retained lease"
                        port, tls, websocket = journey.available_ports()
                        env.update(DURIS_TLS_PORT=str(tls),
                                   DURIS_WEBSOCKET_PORT=str(websocket))
                        with output_path.open("w") as output:
                            game_process = subprocess.Popen(
                                [str(binary), "--minimal", "-s", "-d",
                                 str(game), str(port)], cwd=game, env=env,
                                stdout=output, stderr=subprocess.STDOUT)
                        deadline = time.monotonic() + 60
                        while time.monotonic() < deadline and game_process.poll() is None:
                            if "Entering game loop." in output_path.read_text(errors="replace"):
                                break
                            time.sleep(.1)
                        assert game_process.poll() is None, logs()[-2000:]
                        deadline = time.monotonic() + 45
                        later = None
                        while time.monotonic() < deadline and game_process.poll() is None:
                            matches = re.findall(
                                r"world recovery generation and floor handoff acknowledged sequence=(\d+)",
                                logs())
                            if matches and int(matches[-1]) > ack:
                                later = int(matches[-1])
                                break
                            time.sleep(.5)
                        assert later is not None and later > ack, logs()[-2500:]
                        assert int(command("GET", current)) == later
                        print("reboot: clean lease release and fresh generation/floor acknowledgment",
                              flush=True)
            finally:
                if client:
                    client.close()
                if game_process and game_process.poll() is None:
                    game_process.terminate()
                    game_process.wait(timeout=20)
                sql("DROP DATABASE " + database, False)
        finally:
            if redis.poll() is None:
                redis.terminate()
                redis.wait(timeout=10)


if __name__ == "__main__":
    run(Path(sys.argv[1]).resolve(), "--expect-stuck" in sys.argv[2:],
        "--outage" in sys.argv[2:], "--cancel" in sys.argv[2:],
        "--auth" in sys.argv[2:], "--repeat" in sys.argv[2:])
