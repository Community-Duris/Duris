#!/usr/bin/env python3
"""Issue #259: real login/save/quit/restart playtime on a disposable MariaDB.

Requires TEST_DB_HOST=127.0.0.1, TEST_DB_USER, TEST_DB_PASSWORD and --server.
Never reads checkout .env or touches an existing schema. Only the fresh schema
created by this invocation is dropped. Uses the existing small journey world;
no combat or unrelated gameplay checks are run.
"""
from pathlib import Path
import argparse
import json
import os
import signal
import subprocess
import tempfile
import time
import uuid

import test_flatfile_combat_journey as journey

ROOT = Path(__file__).resolve().parents[2]


def run(server):
    host = os.environ["TEST_DB_HOST"]
    assert host == "127.0.0.1", "requires a disposable loopback database"
    database = "playtime_test_" + uuid.uuid4().hex[:12]
    environment = {
        "PATH": os.environ.get("PATH", "/usr/bin:/bin"),
        "ENVIRONMENT": "local", "DB_HOST": host, "DB_PORT": "3306",
        "DB_NAME": database, "DB_USER": os.environ["TEST_DB_USER"],
        "DB_PASSWD": os.environ["TEST_DB_PASSWORD"],
        "DB_ALLOWED_TARGETS": host + "/" + database,
        "MYSQL_PWD": os.environ["TEST_DB_PASSWORD"],
        "PERSISTENCE_MODE": "mariadb-primary", "DB_TLS": "FALSE",
        "REDIS": "FALSE", "CHAOS_MUD": "FALSE", "DURIS_NEVENT_TRACE_PLAYER": "1",
        "LISTEN_ADDRESS": host, "DURIS_WEBSOCKET_LISTEN_ADDRESS": host,
    }
    if "LD_LIBRARY_PATH" in os.environ:
        environment["LD_LIBRARY_PATH"] = os.environ["LD_LIBRARY_PATH"]
    mysql = ["mysql", "--protocol=tcp", "-h", host, "-u", environment["DB_USER"], "-N", "-B"]

    def sql(text, selected=True):
        return subprocess.check_output(mysql + ([database] if selected else []),
                                       input=text, text=True, env=environment).strip()

    sql("CREATE DATABASE " + database + " CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci", False)
    try:
        sql((ROOT / "migrations/bootstrap_multithread_safe.sql").read_text())
        for args in (["adopt", "--kind", "fresh_bootstrap"], ["run"]):
            result = subprocess.run(["python3", "scripts/migration_runner.py", *args],
                                    cwd=ROOT, env=environment, capture_output=True, text=True)
            assert result.returncode == 0, result.stdout + result.stderr
        (ROOT / "bin/tests").mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="mysql-playtime-", dir=ROOT / "bin/tests") as temporary:
            runtime = Path(temporary)
            journey.make_fixture(runtime)
            journey.generate_certificate(runtime)
            (runtime / "logs/log").mkdir(parents=True)
            for name in ("players", "critical"):
                (runtime / "journals" / name).mkdir(parents=True, mode=0o700)
            plain, tls, websocket = journey.available_ports()
            environment.update(PLAYER_SAVE_JOURNAL_DIR=str(runtime / "journals/players"),
                               CRITICAL_COMMAND_JOURNAL_DIR=str(runtime / "journals/critical"),
                               DURIS_TLS_PORT=str(tls), DURIS_WEBSOCKET_PORT=str(websocket))
            process = client = None
            output_path = runtime / "server.out"
            with output_path.open("w") as output:
                def boot():
                    offset = output_path.stat().st_size
                    proc = subprocess.Popen([str(server), "--minimal", "-s", "-d", str(runtime), str(plain)],
                                            cwd=runtime, env=environment, stdout=output, stderr=subprocess.STDOUT)
                    deadline = time.monotonic() + 120
                    while b"Entering game loop." not in output_path.read_bytes()[offset:]:
                        if proc.poll() is not None or time.monotonic() > deadline:
                            if proc.poll() is None:
                                proc.terminate()
                            proc.wait(timeout=10)
                            raise AssertionError("playtime fixture failed to boot: " + journey.runtime_logs(runtime))
                        time.sleep(.1)
                    return proc

                def persisted():
                    return int(sql("SELECT played_time FROM player_data WHERE name='" + journey.CHARACTER + "'"))

                rows = []

                def save(label):
                    assert client is not None
                    client.send("save")
                    client.expect("Save complete for " + journey.CHARACTER + ".", timeout=30)
                    value = persisted()
                    rows.append({"phase": label, "played_seconds": value, "monotonic": time.monotonic()})
                    return value

                def quit_player():
                    assert client is not None
                    client.send("quit")
                    client.expect("ACCOUNT MENU", timeout=30)
                    client.send("0")
                    client.close()

                def stop():
                    assert process is not None
                    process.send_signal(signal.SIGTERM)
                    process.wait(timeout=30)
                    assert process.returncode == 0

                try:
                    process = boot()
                    client = journey.MudClient(plain)
                    journey.create_character(client)
                    before = save("initial")
                    start = time.monotonic()
                    time.sleep(3)  # Exercise real elapsed playtime, not a guessed fixture value.
                    after = save("elapsed-save")
                    assert 2 <= after - before <= time.monotonic() - start + 2, rows
                    start = time.monotonic()
                    again = save("repeated-save")
                    assert 0 <= again - after <= time.monotonic() - start + 2, rows
                    quit_player()
                    client = None
                    terminal = persisted()
                    assert terminal >= again, rows
                    rows.append({"phase": "quit", "played_seconds": terminal})
                    stop()
                    process = None
                    time.sleep(3)  # Offline time must not be credited on the next login.
                    assert persisted() == terminal
                    process = boot()
                    start = time.monotonic()
                    client = journey.reconnect_character(plain)
                    reloaded = save("restart-login-save")
                    assert 0 <= reloaded - terminal <= time.monotonic() - start + 2, rows
                    start = time.monotonic()
                    time.sleep(3)
                    continued = save("second-session-save")
                    assert 2 <= continued - reloaded <= time.monotonic() - start + 2, rows
                    quit_player()
                    client = None
                    stop()
                    process = None
                    print(json.dumps(rows, indent=2))
                    print("[PASS] real MariaDB login, elapsed save, repeated save, terminal save, restart/reload and continued session")
                except Exception:
                    print(output_path.read_text()[-6000:])
                    print(journey.runtime_logs(runtime))
                    raise
                finally:
                    if client is not None:
                        client.close()
                    if process is not None and process.poll() is None:
                        process.terminate()
                        process.wait(timeout=30)
    finally:
        sql("DROP DATABASE " + database, False)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--server", required=True, type=Path)
    args = parser.parse_args()
    run(args.server.resolve(strict=True))
