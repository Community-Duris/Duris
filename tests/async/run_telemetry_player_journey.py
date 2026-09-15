#!/usr/bin/env python3
"""Manual #265 real-player journey; requires a fresh task-owned SQL fixture.

Usage: run_telemetry_player_journey.py ABS_SQL_BINARY ENV_JSON
The runner creates only synthetic player data in duris_265_*test, never .env.
Use a network-isolated container and provision/bootstrap that database first.
"""
from pathlib import Path
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time

import run_copyover_runtime_journey as copyover
import test_flatfile_combat_journey as journey


def run(binary: Path, settings: Path) -> None:
    cfg = json.loads(settings.read_text())
    assert re.fullmatch(r"duris_265_[a-z0-9_]*test", cfg["DB_NAME"])
    assert cfg["DB_HOST"] == "127.0.0.1" and cfg["ENVIRONMENT"] == "local"
    env = {"PATH": os.environ.get("PATH", "/usr/bin:/bin"), **cfg}
    env["MYSQL_PWD"] = cfg["DB_PASSWD"]
    mysql = ["mysql", "--protocol=tcp", "-h127.0.0.1", "-P" + cfg["DB_PORT"],
             "-u" + cfg["DB_USER"], "-N", "-B", cfg["DB_NAME"]]

    def sql(statement):
        return subprocess.check_output(mysql + ["-e", statement], env=env, text=True,
                                       timeout=15).strip()

    def until(predicate, label, timeout=20):
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            if predicate():
                return
            time.sleep(0.1)
        raise AssertionError(label)

    def reconnect():
        connected = copyover.Client(port)
        try:
            entry, _ = connected.expect_any(("term type", "account name"))
            if entry == "term type":
                connected.send("9")
                connected.expect("account name")
            connected.send(journey.ACCOUNT)
            connected.expect("enter your password")
            connected.send(journey.PASSWORD)
            connected.expect("PRESS RETURN")
            connected.send("")
            connected.expect("Please select an option")
            connected.send("1")
            connected.expect(journey.CHARACTER)
            connected.send("1")
            branch, _ = connected.expect_any(("Play as", "Reconnecting..."))
            if branch == "Play as":
                connected.send("y")
            connected.send("look")
            connected.expect("The Regression Arena", timeout=30)
            return connected
        except Exception:
            connected.close()
            raise

    assert sql("SELECT COUNT(*) FROM player_data") == "0", "requires fresh synthetic DB"
    assert sql("SELECT COUNT(*) FROM telemetry_interval") == "0"
    with tempfile.TemporaryDirectory(prefix="telemetry265-player-") as temporary:
        root = Path(temporary)
        runtime = root / "runtime"
        runtime.mkdir()
        journey.make_fixture(runtime)
        journey.generate_certificate(runtime)
        # Reviewed registry-v1 fixture, not a catalog generated from arbitrary
        # live properties. Source registry entries 1..14 include hard-coded
        # policy values and the five effective properties pinned below; entries
        # 15..19 are maintained-but-unused and excluded from identity.
        properties = (runtime / "lib/duris.properties").read_text()
        for key, expected in {
            "exp.zoneTrophy.observe": 0.0,
            "epic.touch.maxPayoutFactor": 10.0,
            "epic.touch.PayoutFactor": 1.0,
            "epic.zone.alignmentMod": 0.2,
            "epic.alignment.minPercentage": 0.15,
        }.items():
            values = re.findall(r"^" + re.escape(key) + r"=([^\r\n]+)$", properties, re.M)
            assert len(values) == 1 and float(values[0]) == expected, f"reviewed fixture drift: {key}"
        catalog = root / "reviewed-properties.catalog"
        catalog.touch(mode=0o600)
        catalog.write_text("6f49b7e9b16055b6c7d48d83f4a9de789d89adeddabbb4bfc1f6d2c86f6b8792 265 265 1\n")
        env["TELEMETRY_PROPERTY_CATALOG_FILE"] = str(catalog)
        for name in ("logs/log", "journals/players", "journals/critical", "bin/server"):
            (runtime / name).mkdir(parents=True, exist_ok=True, mode=0o700)
        for name in ("dms", "dms_new"):
            shutil.copy2(binary, runtime / "bin/server" / name)
        state_file = root / "copyover-state/copyover.dat"
        port, tls, websocket = journey.available_ports()
        env.update(PERSISTENCE_MODE="mariadb-primary", PERSISTENCE_BACKEND="mariadb",
                   PLAYER_SAVE_JOURNAL_DIR=str(runtime / "journals/players"),
                   CRITICAL_COMMAND_JOURNAL_DIR=str(runtime / "journals/critical"),
                   COPYOVER_STATE_FILE=str(state_file), LISTEN_ADDRESS="127.0.0.1",
                   DURIS_TLS_PORT=str(tls), DURIS_WEBSOCKET_LISTEN_ADDRESS="127.0.0.1",
                   DURIS_WEBSOCKET_PORT=str(websocket), REDIS="FALSE", CHAOS_MUD="FALSE",
                   TELEMETRY_ENABLED="false", TELEMETRY_BACKEND="sql",
                   TELEMETRY_INTERVAL_USEC="1000000", TELEMETRY_CHECKPOINT_INTERVAL_USEC="1000000",
                   TELEMETRY_ACTIVE_WINDOW_USEC="3000000", TELEMETRY_CONTEXT_SEGMENTS_PER_MINUTE="64")
        process = output = client = None

        def stop():
            nonlocal process, output, client
            if client:
                client.close()
                client = None
            if process and process.poll() is None:
                process.terminate()
                try:
                    process.wait(timeout=30)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait(timeout=5)
                    raise AssertionError("fixture process did not shut down")
            if output:
                output.close()

        def boot():
            nonlocal process, output
            output = (runtime / "server.out").open("w")
            process = subprocess.Popen([str(runtime / "bin/server/dms"), "--minimal", "-s", str(port)],
                                       cwd=runtime, env=env, stdout=output, stderr=subprocess.STDOUT)
            until(lambda: process.poll() is not None or "Entering game loop." in
                  (runtime / "server.out").read_text(errors="replace"), "boot timed out", 90)
            assert process.poll() is None, "server exited during boot"
            assert hashlib.sha256(Path(f"/proc/{process.pid}/exe").read_bytes()).digest() == hashlib.sha256(binary.read_bytes()).digest()

        try:
            boot()
            client = copyover.Client(port)
            journey.create_character(client)
            client.send("save")
            client.expect(f"Save complete for {journey.CHARACTER}.")
            client.send("quit")
            client.expect("ACCOUNT MENU", timeout=30)
            stop()
            assert sql("SELECT COUNT(*) FROM telemetry_interval") == "0"
            pid = int(sql("SELECT pid FROM player_data"))
            sql(f"UPDATE player_data SET level=62 WHERE pid={pid}")
            assert sql(f"SELECT level FROM player_data WHERE pid={pid}") == "62"
            print("PASS disabled telemetry: create/login/save/quit; no telemetry facts", flush=True)
            env["TELEMETRY_ENABLED"] = "true"
            boot()
            client = reconnect()
            client.enable_compression()
            until(lambda: int(sql("SELECT COUNT(*) FROM telemetry_session")) == 1,
                  "enabled telemetry did not write session")
            identity = sql("SELECT CONCAT(session_boot_id,':',session_process_id,':',session_seq) FROM telemetry_session")
            until(lambda: int(sql("SELECT latest_revision FROM telemetry_session")) > 0,
                  "no initial checkpoint")
            client.send("shutdown copyover")
            client.expect("Copyover FAILED", timeout=60)
            client.send("look")
            client.expect("The Regression Arena")
            client.send("save")
            client.expect(f"Save complete for {journey.CHARACTER}.", timeout=30)
            before = int(sql("SELECT latest_revision FROM telemetry_session"))
            until(lambda: int(sql("SELECT latest_revision FROM telemetry_session")) > before,
                  "telemetry stopped after failed copyover")
            print("PASS enabled failed-copyover: original MCCP socket, save, advancing checkpoint", flush=True)
            state_file.parent.mkdir()
            client.send("shutdown copyover")
            client.expect("Copyover complete!", timeout=90)
            client.send("look")
            client.expect("The Regression Arena")
            client.send("save")
            client.expect(f"Save complete for {journey.CHARACTER}.", timeout=30)
            until(lambda: int(sql("SELECT COUNT(DISTINCT boot_id,process_id) FROM telemetry_interval WHERE session_seq IS NOT NULL")) >= 2,
                  "new producer did not emit resumed session facts")
            assert sql("SELECT COUNT(*) FROM telemetry_session") == "1", "copyover opened duplicate logical session"
            assert sql("SELECT CONCAT(session_boot_id,':',session_process_id,':',session_seq) FROM telemetry_session") == identity
            print("PASS real exec: producer changed; logical session retained", flush=True)
            client.close()
            client = None
            before = int(sql("SELECT latest_revision FROM telemetry_session"))
            until(lambda: int(sql("SELECT latest_revision FROM telemetry_session")) > before,
                  "detached checkpoint did not advance")
            client = reconnect()
            assert sql("SELECT COUNT(*) FROM telemetry_session") == "1"
            client.send("save")
            client.expect(f"Save complete for {journey.CHARACTER}.", timeout=30)
            client.send("quit")
            client.expect("ACCOUNT MENU", timeout=30)
            until(lambda: sql("SELECT exited FROM telemetry_session") == "1", "logical exit not durable")
            counters = list(map(int, sql("SELECT connected_usec,active_usec,idle_usec,unknown_usec,resident_usec,linkdead_usec FROM telemetry_session").split()))
            connected, active, idle, unknown, resident, linkdead = counters
            assert connected == active + idle + unknown and resident == connected + linkdead
            assert active > 0 and linkdead > 0
            print(json.dumps({"pass": "reconnect/exit conservation", "connected_usec": connected,
                              "active_usec": active, "idle_usec": idle, "unknown_usec": unknown,
                              "resident_usec": resident, "linkdead_usec": linkdead}), flush=True)
            stop()
            committed = sql("SELECT COUNT(*) FROM telemetry_interval")
            # Fail only the private telemetry connection; authoritative gameplay
            # SQL keeps its real fixture credentials and must remain usable.
            env["TELEMETRY_DB_USER"] = "telemetry265_unavailable"
            boot()
            client = reconnect()
            client.send("save")
            client.expect(f"Save complete for {journey.CHARACTER}.", timeout=30)
            client.send("quit")
            client.expect("ACCOUNT MENU", timeout=30)
            stop()
            assert sql("SELECT COUNT(*) FROM telemetry_interval") == committed
            print("PASS unavailable private telemetry SQL: login/look/save/quit; no new facts", flush=True)
        except Exception:
            # Server logs contain synthetic fixture data only; never print env or authentication inputs.
            diagnostics = (runtime / "server.out").read_text(errors="replace") + "\n" + journey.runtime_logs(runtime)
            evidence = journey.ROOT / "bin/tests/telemetry265-journey-failure.log"
            evidence.parent.mkdir(parents=True, exist_ok=True)
            evidence.write_text(diagnostics)
            print(diagnostics[-16000:])
            raise
        finally:
            stop()


if __name__ == "__main__":
    run(Path(sys.argv[1]).resolve(), Path(sys.argv[2]).resolve())
