#!/usr/bin/env python3
"""Real-server recovery of generated pets and held equipment in a private fixture.

Usage: python3 tests/async/test_pet_restart_journey.py /absolute/flatfile/server
The server is built separately so this test never invokes a CI pipeline.
"""
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import time
import test_flatfile_combat_journey as journey

ROOT = journey.ROOT


def run(binary):
    with tempfile.TemporaryDirectory(prefix="duris-pet-restart-") as temporary:
        root = Path(temporary)
        state, runtime = root / "state", root / "runtime"
        state.mkdir(mode=0o700)
        (state / "domains").mkdir(mode=0o700)
        runtime.mkdir()
        (runtime / "logs/log").mkdir(parents=True)
        (runtime / "logs/log/.gitignore").write_text("*\n")
        journey.make_fixture(runtime)
        zone = runtime / "areas_mini/mini.zon"
        zone.write_text(re.sub(r"^[MG] .*\n", "", zone.read_text(), flags=re.M))
        mobile = runtime / "areas_mini/mini.mob"
        source = (ROOT / "areas/mob/heavens.mob").read_text()
        prototype = re.search(r"^#1201\n.*?(?=^#|^\$)", source, re.M | re.S).group()
        mobile.write_text(mobile.read_text().replace("$~", prototype + "$~"))
        journey.generate_certificate(runtime)
        for directory in ("players", "critical"):
            (runtime / "journals" / directory).mkdir(parents=True, mode=0o700)
        port, tls, websocket = journey.available_ports()
        env = dict(PATH=os.environ.get("PATH", "/usr/bin:/bin"), ENVIRONMENT="local",
                   PERSISTENCE_MODE="flatfile-primary", FLATFILE_STATE_DIR=str(state),
                   PLAYER_SAVE_JOURNAL_DIR=str(runtime / "journals/players"),
                   CRITICAL_COMMAND_JOURNAL_DIR=str(runtime / "journals/critical"),
                   LISTEN_ADDRESS="127.0.0.1", DURIS_TLS_PORT=str(tls),
                   DURIS_WEBSOCKET_LISTEN_ADDRESS="127.0.0.1", DURIS_WEBSOCKET_PORT=str(websocket),
                   REDIS="FALSE", CHAOS_MUD="FALSE")
        if os.environ.get("LD_LIBRARY_PATH"):
            env["LD_LIBRARY_PATH"] = os.environ["LD_LIBRARY_PATH"]
        subprocess.run([str(journey.INSPECTOR), str(state), "seed-combat"], check=True)
        fixture = root / "pet-fixture"
        subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-Isrc",
                        "tests/async/pet_restart_fixture.cpp", "src/player/pet_restore_state.c",
                        "src/player/player_snapshot_codec.c", "src/flatfile/flatfile_player_snapshot_file.c",
                        "src/flatfile/flatfile_store.c", "-lcrypto", "-o", str(fixture)],
                       cwd=ROOT, check=True, timeout=90)

        def rows(mode="inspect"):
            output = subprocess.check_output([str(fixture), str(state), mode], text=True)
            return [line.split("|") for line in output.splitlines()]

        process = client = output = None

        def stop():
            nonlocal process, client, output
            if client:
                client.close()
                client = None
            if process and process.poll() is None:
                process.terminate()
                try:
                    process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait(timeout=5)
            if output:
                output.close()

        def boot():
            nonlocal process, output
            output = (runtime / "server.out").open("w")
            command = [str(binary), "--minimal", "-s", "-d", str(runtime), str(port)]
            if os.environ.get("PET_RESTART_GDB"):
                command = ["gdb", "--batch", "-ex", "run", "-ex", "thread apply all bt", "--args"] + command
            process = subprocess.Popen(command,
                                       cwd=runtime, env=env, stdout=output, stderr=subprocess.STDOUT)
            deadline = time.monotonic() + 90
            while time.monotonic() < deadline:
                if "Entering game loop." in (runtime / "server.out").read_text(errors="replace"):
                    return
                assert process.poll() is None, "server failed to boot"
                time.sleep(0.1)
            raise AssertionError("boot deadline exceeded")

        try:
            boot()
            client = journey.MudClient(port)
            journey.create_character(client)
            client.send("save")
            client.expect(f"Save complete for {journey.CHARACTER}.", timeout=30)
            stop()
            seeded = rows("seed")
            expected_states = {row[1] for row in seeded if row[1]}
            expected_uids = {row[3] for row in seeded}
            assert len(expected_states) == 2 and len(expected_uids) == 3
            for cycle in range(2):
                boot()
                client = journey.reconnect_character(port)
                client.send("look")
                visible = client.expect("Pos: standing >", timeout=15)
                assert "skeleton of fixture0" in visible and "skeleton of fixture1" in visible, visible
                assert "undead corpse stands" not in visible, visible
                client.send("save")
                client.expect(f"Save complete for {journey.CHARACTER}.", timeout=30)
                current = rows()
                assert len(current) == 3, current
                assert {row[1] for row in current if row[1]} == expected_states, "generated state changed"
                assert {row[3] for row in current} == expected_uids, "equipment lost or duplicated"
                assert sum(row[0] == "1" for row in current) == 1, "legacy pet not held"
                # Gear contributes once to derived HP; the serialized base stays 1234.
                assert sorted(int(row[2]) for row in current) == [1234, 1234, 1253], current
                print(f"restart cycle {cycle + 1}: identity, base stats, gear, deadlines, held UID passed", flush=True)
                if cycle == 0:
                    stop()
            stop()
            rows("arm-expiry")
            boot()
            client = journey.reconnect_character(port)
            deadline = time.monotonic() + 30
            while True:
                client.pending.clear()
                client.send("look")
                visible = client.expect("Pos: standing >", timeout=15)
                if "The skeleton of fixture1 waits here" not in visible:
                    break
                assert time.monotonic() < deadline, "restored finite pet did not die at its deadline"
                time.sleep(1)
            client.send("save")
            client.expect(f"Save complete for {journey.CHARACTER}.", timeout=30)
            assert len(rows()) == 2, "expired pet remained in owner's checkpoint"
            assert any(row[1] == seeded[0][1] for row in rows()), "permanent pet expired"
            print("restored finite pet dies at its deadline; permanent pet remains", flush=True)
            client.send("quit")
            client.expect("ACCOUNT MENU", timeout=30)
            remaining = rows()
            assert len(remaining) == 1 and remaining[0][0] == "1", remaining
            assert remaining[0][3] == seeded[2][3], "quit discarded held equipment"
            stop()
            boot()
            client = journey.reconnect_character(port)
            client.send("save")
            client.expect(f"Save complete for {journey.CHARACTER}.", timeout=30)
            assert rows() == remaining, "held record changed across quit/restart"
            print("held pet equipment survives quit, restart and subsequent save", flush=True)
        except Exception:
            print((runtime / "server.out").read_text(errors="replace")[-12000:])
            print(journey.runtime_logs(runtime))
            if client:
                print(bytes(client.transcript).decode(errors="replace")[-8000:])
            raise
        finally:
            stop()


if __name__ == "__main__":
    assert len(sys.argv) == 2, __doc__
    run(Path(sys.argv[1]).resolve())
