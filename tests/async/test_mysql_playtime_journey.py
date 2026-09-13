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
import shutil
import subprocess
import tempfile
import time
import uuid

import test_flatfile_combat_journey as journey

ROOT = Path(__file__).resolve().parents[2]


def reconnect_resident(port):
    """A linkdead resident reconnects immediately, without the Play-as prompt."""
    client = journey.MudClient(port)
    try:
        entry, _ = client.expect_any(("term type", "account name"))
        if entry == "term type":
            client.send("9")
            client.expect("account name")
        client.send(journey.ACCOUNT)
        client.expect("enter your password")
        client.send(journey.PASSWORD)
        client.expect("PRESS RETURN")
        client.send("")
        client.expect("Please select an option")
        client.send("1")
        client.expect(journey.CHARACTER)
        client.send("1")
        client.expect("Reconnecting...", timeout=30)
        return client
    except Exception:
        client.close()
        raise


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
        subprocess.run(["python3", "tests/async/test_playtime_mysql_repository.py"],
                       cwd=ROOT, env=environment, check=True)
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
                    # Send no gameplay commands while the existing 30-second
                    # periodic owner checkpoints this otherwise quiet session.
                    deadline = time.monotonic() + 45
                    while persisted() < again + 15:
                        assert time.monotonic() < deadline, "quiet session never checkpointed"
                        time.sleep(.25)
                    quiet = persisted()
                    rows.append({"phase": "quiet-periodic-checkpoint", "played_seconds": quiet})
                    client.close()
                    client = reconnect_resident(plain)
                    client.send("look")
                    client.expect("The Regression Arena", timeout=30)
                    assert save("link-loss-reconnect") >= quiet
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
                    # Exercise the actual death/terminal-save route, checking
                    # only playtime rather than unrelated combat or item outcomes.
                    journey.attack_until_death(client)
                    client.expect("ACCOUNT MENU", timeout=45)
                    client.send("0")
                    client.close()
                    client = None
                    death_total = persisted()
                    assert death_total >= continued
                    rows.append({"phase": "death-save", "played_seconds": death_total})
                    client = journey.reconnect_character(plain)
                    acknowledged = save("death-reload")
                    assert acknowledged >= death_total
                    # An unclean process death must retain the acknowledged total;
                    # replay must not add the old session a second time.
                    process.kill()
                    process.wait(timeout=15)
                    assert process.returncode == -signal.SIGKILL
                    process = None
                    client.close()
                    client = None
                    durable = persisted()
                    assert durable >= acknowledged
                    time.sleep(3)
                    process = boot()
                    restart_begin = time.monotonic()
                    client = journey.reconnect_character(plain)
                    recovered = save("crash-recovery")
                    assert durable <= recovered <= durable + time.monotonic() - restart_begin + 2
                    quit_player()
                    client = None
                    stop()
                    process = None
                    # Promote only this synthetic, offline fixture character so
                    # the real staff command can exercise copyover. No live account
                    # or shared server is used. The binary belongs to this fixture.
                    sql("UPDATE player_data SET level=62,highest_level=62 WHERE name='" + journey.CHARACTER + "'")
                    runtime_binary = runtime / "bin/server/dms"
                    runtime_binary.parent.mkdir(parents=True)
                    shutil.copy2(server, runtime_binary)
                    process = boot()
                    client = journey.reconnect_character(plain)
                    before_copyover = save("pre-copyover")
                    copyover_begin = time.monotonic()
                    client.send("shutdown copyover")
                    client.expect("*** Copyover complete! ***", timeout=60)
                    assert process.poll() is None
                    after_copyover = save("post-copyover")
                    assert before_copyover <= after_copyover <= before_copyover + time.monotonic() - copyover_begin + 2
                    # Copyover playtime is verified on the preserved connection.
                    # Account-menu restoration is a separate contract; cleanup
                    # must not assume this minimal fixture retained its account UI.
                    client.close()
                    client = None
                    stop()
                    process = None
                    print(json.dumps(rows, indent=2))
                    print("[PASS] MariaDB elapsed/quiet/repeated saves, link loss, quit/restart, death/reload, crash recovery and live copyover")
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
