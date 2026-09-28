#!/usr/bin/env python3
"""Reproduce and verify NPC container claims on a disposable flat-file game server."""
import os
import json
from pathlib import Path
import re
import signal
import subprocess
import tempfile
import time
import test_flatfile_combat_journey as journey


def inspect_items(state: Path) -> dict:
    return json.loads(subprocess.check_output(
        [str(journey.INSPECTOR), str(state), "inspect-items", "1"], text=True, timeout=15))


def inspect_item(state: Path, uid: int) -> dict:
    return json.loads(subprocess.check_output(
        [str(journey.INSPECTOR), str(state), "inspect-item", str(uid)],
        text=True, timeout=15))


def run(binary: Path, expect_abort: bool = False, restart_only: bool = False,
        key_break: bool = False) -> None:
    with tempfile.TemporaryDirectory(prefix="npc-claim-state-") as state_dir:
        with tempfile.TemporaryDirectory(prefix="npc-claim-game-") as game_dir:
            state, game = Path(state_dir), Path(game_dir)
            state.chmod(0o700)
            (state / "domains").mkdir(mode=0o700)
            subprocess.run([str(journey.INSPECTOR), str(state), "seed-combat"], check=True)
            (game / "logs/log").mkdir(parents=True)
            journey.make_fixture(game)
            object_path = game / "areas_mini/mini.obj"
            objects = object_path.read_text()
            assert objects.count("12 28 0 0 7 0 0 16385 0 0 0") == 1
            objects = objects.replace("12 28 0 0 7 0 0 16385 0 0 0",
                                      "12 28 0 0 7 0 0 1 0 0 0")
            if key_break:
                objects = objects.replace("$~", """#22802
regression brass key~
a regression brass key~
A regression brass key lies here.~
~
18 0 0 0 7 0 0 1 0 0 0
0 100 0 0 0 0 0 0
1 0 100
#22803
regression chest~
a regression chest~
A regression chest stands here.~
~
15 0 0 0 7 0 0 0 0 0 0
100 12 22802 100 0 0 0 0
5 0 100
$~""")
            object_path.write_text(objects)
            journey.generate_certificate(game)
            mobile_path = game / "areas_mini/mini.mob"
            mobile_path.write_text(mobile_path.read_text().replace("$~", """#22801
claim scavenger~
a claim scavenger~
A claim scavenger waits here.~
~
4 0 0 0 0 0 0 0 S
PH 0 0 -1
10 0 0 20d1+400 2d1+1
0.0.0.0 0
8 8 0
$~"""))
            zone_path = game / "areas_mini/mini.zon"
            zone = "\n".join(line for line in zone_path.read_text().splitlines()
                             if not line.startswith(("M 0 11 ", "G 1 15 ", "G 1 3 ",
                                                     "M 0 22800 "))) + "\n"
            reset = ("\nO 0 22802 1 22800 100 0 0 0 * breaking key\n"
                     "O 0 22803 1 22800 100 0 0 0 * locked chest\n"
                     if key_break else
                     "\nO 0 48 1 22800 100 0 0 0 * takeable container\n"
                     "P 1 11 1 48 100 0 0 0 * nested ordinary item\n")
            zone_path.write_text(zone.replace("\nS\n", reset + "S\n"))
            for name in ("players", "critical"):
                (game / "journals" / name).mkdir(parents=True, mode=0o700)
            port, tls_port, ws_port = journey.available_ports()
            env = dict(PATH=os.environ.get("PATH", "/usr/bin:/bin"),
                       ENVIRONMENT="local", PERSISTENCE_MODE="flatfile-primary",
                       FLATFILE_STATE_DIR=str(state),
                       PLAYER_SAVE_JOURNAL_DIR=str(game / "journals/players"),
                       CRITICAL_COMMAND_JOURNAL_DIR=str(game / "journals/critical"),
                       LISTEN_ADDRESS="127.0.0.1", DURIS_TLS_PORT=str(tls_port),
                       DURIS_WEBSOCKET_LISTEN_ADDRESS="127.0.0.1",
                       DURIS_WEBSOCKET_PORT=str(ws_port), REDIS="FALSE",
                       CHAOS_MUD="FALSE")
            if os.environ.get("LD_LIBRARY_PATH"):
                env["LD_LIBRARY_PATH"] = os.environ["LD_LIBRARY_PATH"]
            output_path = game / "server.out"
            process = None
            client = None

            def boot() -> None:
                nonlocal process
                output = output_path.open("a")
                process = subprocess.Popen([str(binary), "--minimal", "-s", "-d",
                                            str(game), str(port)], cwd=game, env=env,
                                           stdout=output, stderr=subprocess.STDOUT)
                output.close()
                deadline = time.monotonic() + 120
                while "Entering game loop." not in output_path.read_text(errors="replace"):
                    assert process.poll() is None and time.monotonic() < deadline, output_path.read_text(errors="replace")[-4000:]
                    time.sleep(.1)

            def stop() -> None:
                nonlocal process, client
                if client:
                    client.close()
                    client = None
                if process and process.poll() is None:
                    process.terminate()
                    try:
                        process.wait(timeout=15)
                    except subprocess.TimeoutExpired:
                        process.kill()
                        process.wait()
                process = None

            try:
                boot()
                client = journey.MudClient(port)
                journey.create_character(client)
                if key_break:
                    client.pending.clear()
                    client.send("drop all")
                    client.expect("Pos: standing >", timeout=20)
                    client.send("get brass")
                    client.expect("get a regression brass key", timeout=20)
                    before = inspect_items(state)
                    keys = [item for item in before["player_items"] if item["vnum"] == 22802]
                    assert len(keys) == 1, before["player_items"]
                    key_uid = keys[0]["uid"]
                    active_key = inspect_item(state, key_uid)
                    assert active_key["state"] == 1 and active_key["owner_type"] == 1
                    client.pending.clear()
                    client.send("unlock chest")
                    client.expect("Damn!  You broke your key!", timeout=30)
                    deadline = time.monotonic() + 20
                    while time.monotonic() < deadline:
                        retired = inspect_item(state, key_uid)
                        if retired["state"] == 2:
                            break
                        time.sleep(.1)
                    assert retired["state"] == 2 and retired["owner_type"] == 8, retired
                    assert retired["root"] == key_uid and retired["parent"] == 0, retired
                    assert retired["revision"] > active_key["revision"], retired
                    assert not any(item["uid"] == key_uid for item in
                                   inspect_items(state)["player_items"])
                    client.send("save")
                    client.expect("Save complete for Taverek.", timeout=20)
                    client.send("quit")
                    client.expect("ACCOUNT MENU", timeout=20)
                    stop()
                    zone_path.write_text("\n".join(
                        line for line in zone_path.read_text().splitlines()
                        if not line.startswith(("O 0 22802 ", "O 0 22803 "))) + "\n")
                    output_path.write_text("")
                    boot()
                    client = journey.reconnect_character(port)
                    assert inspect_item(state, key_uid) == retired
                    assert not any(item["uid"] == key_uid for item in
                                   inspect_items(state)["player_items"])
                    client.send("inventory")
                    assert "regression brass key" not in client.expect(
                        "Pos: standing >", timeout=20)
                    print("flatfile: key retired with retained UID and topology through restart",
                          flush=True)
                    return
                if restart_only:
                    client.pending.clear()
                    client.send("drop all")
                    client.expect("Pos: standing >", timeout=20)
                    client.send("get miles backpack")
                    client.expect("get a 1000 frequent flier miles", timeout=20)
                    client.send("get backpack")
                    client.expect("get a large leather backpack", timeout=20)
                    client.send("put miles backpack")
                    client.expect("Ok.", timeout=20)
                    before = inspect_items(state)
                    bags = [item for item in before["player_items"] if item["vnum"] == 48]
                    contents = [item for item in before["player_items"] if item["vnum"] == 11]
                    assert len(bags) == len(contents) == 1, before["player_items"]
                    assert contents[0]["parent"] == bags[0]["uid"]
                    client.send("save")
                    client.expect("Save complete for Taverek.", timeout=20)
                    client.send("quit")
                    client.expect("ACCOUNT MENU", timeout=20)
                    stop()
                    # The reset supplied the claimed items; avoid another copy.
                    zone_path.write_text("\n".join(
                        line for line in zone_path.read_text().splitlines()
                        if not line.startswith(("O 0 48 ", "P 1 11 "))) + "\n")
                    output_path.write_text("")
                    boot()
                    client = journey.reconnect_character(port)
                    after = inspect_items(state)
                    after_bags = [item for item in after["player_items"] if item["vnum"] == 48]
                    after_contents = [item for item in after["player_items"] if item["vnum"] == 11]
                    assert len(after_bags) == len(after_contents) == 1, after["player_items"]
                    assert after_bags[0]["uid"] == bags[0]["uid"]
                    assert after_contents[0]["uid"] == contents[0]["uid"]
                    assert after_contents[0]["parent"] == after_bags[0]["uid"]
                    assert after["player_owner_revision"] == before["player_owner_revision"]
                    assert not any(item["uid"] in (bags[0]["uid"], contents[0]["uid"])
                                   for item in after["room_items"])
                    client.send("look in backpack")
                    client.expect("a 1000 frequent flier miles")
                    print("flatfile: nested player UIDs and custody survived save/restart", flush=True)
                    return
                client.send("save")
                client.expect("Save complete for Taverek.", timeout=20)
                client.send("quit")
                client.expect("ACCOUNT MENU", timeout=20)
                stop()
                subprocess.run([str(journey.INSPECTOR), str(state),
                                "seed-item-operator"], check=True)
                # The new boot resets the test container. A staff player makes the
                # nested item authoritative, then loads the scavenger in that room.
                output_path.write_text("")
                boot()
                client = journey.reconnect_character(port)
                client.send("get miles backpack")
                client.expect("get a 1000 frequent flier miles", timeout=20)
                client.send("get backpack")
                client.expect("get a large leather backpack", timeout=20)
                client.send("put miles backpack")
                client.expect("Ok.", timeout=20)
                client.send("drop backpack")
                client.expect("drop a large leather backpack", timeout=20)
                print("player: get miles backpack -> get backpack -> put miles backpack -> drop backpack (committed)", flush=True)
                before = inspect_items(state)
                bags = [item for item in before["room_items"] if item["vnum"] == 48]
                contents = [item for item in before["room_items"] if item["vnum"] == 11]
                assert len(bags) == len(contents) == 1, before["room_items"]
                assert contents[0]["parent"] == bags[0]["uid"], before["room_items"]
                assert not any(item["uid"] in (bags[0]["uid"], contents[0]["uid"])
                               for item in before["player_items"])
                client.send("look in backpack")
                client.expect("a 1000 frequent flier miles")
                client.send("load char 22801")
                client.expect("claim scavenger", timeout=10)
                client.send("setattr scavenger int 103")
                client.expect("OK.", timeout=10)
                print("player: load char 22801; setattr scavenger int 103 -> real NPC spawned", flush=True)
                deadline = time.monotonic() + 90
                moved = False
                while time.monotonic() < deadline and process.poll() is None:
                    if expect_abort:
                        time.sleep(.2)
                        continue
                    client.pending.clear()
                    client.send("look in backpack")
                    response = client.expect_any(("a 1000 frequent flier miles", "Nothing.", "You do not see"),
                                                 timeout=3)[0]
                    if response != "a 1000 frequent flier miles":
                        moved = True
                        break
                    time.sleep(.5)
                logs = journey.runtime_logs(game)
                if expect_abort:
                    assert process.poll() == -signal.SIGABRT, (process.poll(), output_path.read_text(errors="replace")[-2000:])
                    assert "GET_PID called on NPC" in logs, logs[-3000:]
                    print("baseline: SIGABRT; GET_PID called on NPC", flush=True)
                else:
                    assert process.poll() is None, output_path.read_text(errors="replace")[-3000:]
                    assert moved, "NPC did not move the nested item"
                    after = inspect_items(state)
                    assert after["room_owner_revision"] == before["room_owner_revision"] + 1, (
                        before["room_owner_revision"], after["room_owner_revision"])
                    after_bags = [item for item in after["room_items"] if item["vnum"] == 48]
                    after_contents = [item for item in after["room_items"] if item["vnum"] == 11]
                    assert len(after_bags) == len(after_contents) == 1, after["room_items"]
                    assert after_bags[0]["uid"] == bags[0]["uid"]
                    assert after_contents[0]["uid"] == contents[0]["uid"]
                    assert after_contents[0]["root"] == contents[0]["uid"]
                    assert after_contents[0]["parent"] == 0
                    assert not any(item["uid"] in (bags[0]["uid"], contents[0]["uid"])
                                   for item in after["player_items"])
                    client.pending.clear()
                    client.send("stat mob scavenger")
                    stat = client.expect("[Return to continue", timeout=10)
                    assert (re.search(r"Carried Items:\s*1\b", stat) or
                            re.search(r"Equipped Items:\s*1\b", stat)), stat
                    client.send("")
                    stat_page_two = client.expect("Pos: standing >", timeout=10)
                    assert "SCAVENGER ISNPC" in stat_page_two, stat_page_two
                    client.pending.clear()
                    client.send("stat obj miles")
                    object_stat = client.expect("Pos: standing >", timeout=10)
                    assert ("Location:" in object_stat and
                            ("equipped by claim scavenger" in object_stat or
                             "carried by claim scavenger" in object_stat)), object_stat
                    client.send("look")
                    room = client.expect("Pos: standing >", timeout=10)
                    assert "backpack lies here" in room and "frequent flier miles" not in room, room
                    print("fixed: one committed room revision; UID graph unique; NPC carries item; container empty; server alive", flush=True)
            except Exception:
                print("server tail:", output_path.read_text(errors="replace")[-2000:], flush=True)
                print("logs tail:", journey.runtime_logs(game)[-2000:], flush=True)
                if client:
                    print("game tail:", client.transcript.decode(errors="replace")[-2000:], flush=True)
                raise
            finally:
                stop()


if __name__ == "__main__":
    import sys
    run(Path(sys.argv[1]).resolve(), "--expect-abort" in sys.argv[2:],
        "--restart-only" in sys.argv[2:], "--key-break" in sys.argv[2:])
