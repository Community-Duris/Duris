#!/usr/bin/env python3
"""Consume a real NPC quest offering, then save and cold-load its item reward."""

from pathlib import Path
import os
import hashlib
import struct
import re
import signal
import subprocess
import tempfile
import time

import server_build_artifacts
import test_flatfile_combat_journey as journey


ROOT = Path(__file__).resolve().parents[2]
REWARD_VNUM = 22805


def quest_fixture(run_root: Path, xp_reward: int = 0, *, daily: bool = False) -> None:
    journey.make_fixture(run_root)
    mini = run_root / "areas_mini"
    mobiles = mini / "mini.mob"
    mob = """#22801
lapney~
Lapney the quest keeper~
Lapney the quest keeper waits here.
~
~
10 0 0 0 0 0 0 0 S
PH 0 0 -1
1 0 0 1d1+1 1d1+1
0.0.0.0 0
8 8 0
"""
    content = mobiles.read_text()
    assert content.count("$~") == 1
    mobiles.write_text(content.replace("$~", mob + "$~"))

    objects = mini / "mini.obj"
    content = objects.read_text()
    banana = content[content.index("#15\n"):content.index("#16\n")]
    mace = content[content.index("#677\n"):content.index("#678\n")]
    offerings = ""
    for vnum, name in ((22802, "acorn"), (22803, "branch"), (22804, "feather")):
        offerings += (banana.replace("#15\n", f"#{vnum}\n", 1)
                      .replace("banana", name).replace("Banana", name.capitalize()))
    if daily:
        flower = banana.replace("#15\n", "#22806\n", 1).replace("banana", "flower").replace("Banana", "Flower")
    reward = (mace.replace("#677\n", "#22805\n", 1)
              .replace("mace wooden gnoby wood", "blade reward quest")
              .replace("a small wooden mace", "a quest reward blade")
              .replace("A gnoby piece of wood, perhaps a small mace, lies here.",
                       "A quest reward blade lies here."))
    assert content.count("$~") == 1
    objects.write_text(content.replace("$~", offerings + reward + (flower if daily else "") + "$~"))

    quests = mini / "mini.qst"
    content = quests.read_text()
    quest = """#22801
Q
Lapney accepts your offering and blesses your journey.
~
R I 22805
R C 1000
G I 22802
G I 22803
G I 22804
S
"""
    if daily:
        quest = quest.removesuffix("S\n") + "Q\nLapney accepts the flower.\n~\nR C 500\nG I 22806\nS\n"
    if xp_reward:
        assert 0 < xp_reward <= 1000
        quest = quest.replace("R C 1000\n", f"R C 1000\nR E {xp_reward}\n", 1)
    assert content.count("$~") == 1
    quests.write_text(content.replace("$~", quest + "$~"))

    zone = mini / "mini.zon"
    content = re.sub(r"^[MG] .*\n", "", zone.read_text(), flags=re.M)
    reset = (
        "M 0 22801 1 22800 100 0 0 0 * quest keeper\n"
        "O 0 22802 1 22800 100 0 0 0 * acorn\n"
        "O 0 22803 1 22800 100 0 0 0 * branch\n"
        "O 0 22804 1 22800 100 0 0 0 * feather\n"
    )
    if daily:
        reset += "O 0 22806 1 22800 100 0 0 0 * flower\n"
    assert content.count("\nS\n") == 1
    zone.write_text(content.replace("\nS\n", "\n" + reset + "S\n"))


def seed_daily_evidence(state_root: Path, xp_reward: int = 0) -> None:
    """Isolated fixture only: reviewed successful same-faction, solo evidence."""
    contracts = ["give=I:22802,I:22803,I:22804;receive=C:1000," + (f"E:{xp_reward}," if xp_reward else "") + "I:22805;disappear=0",
                 "give=I:22806;receive=C:500;disappear=0"]
    lines = ["ZSQF|2", "K|0"]
    for quest_index, contract in enumerate(contracts):
        definition = "zone-story:qst:22801:" + contract.encode().hex()
        for index in range(20):
            observation = f"pilot:{quest_index}:{index}"
            values = [observation.encode().hex(), definition.encode().hex(), "2",
                      str(int(time.time()) - 60), str(100 + index % 5), "1", "1", "3", "1", "1", "0", "success", "1"]
            lines.append("E|" + observation.encode().hex() + "|" + ":".join(values).encode().hex())
    payload = ("\n".join(lines) + "\n").encode()
    header = b"DURZQST1" + struct.pack("<IIQ", 2, 2, len(payload)) + hashlib.sha256(payload).digest()
    path = state_root / "domains/zone-story-quests.state"
    path.write_bytes(header + payload)
    path.chmod(0o600)


def boot(binary: Path, run_root: Path, environment: dict[str, str], port: int,
         output_path: Path):
    output = output_path.open("w", encoding="utf-8")
    process = subprocess.Popen(
        [str(binary), "--minimal", "-d", str(run_root), str(port)],
        cwd=run_root, env=environment, stdout=output, stderr=subprocess.STDOUT,
    )
    deadline = time.monotonic() + 120
    while time.monotonic() < deadline:
        output.flush()
        if "Entering game loop." in output_path.read_text(errors="replace"):
            return process, output
        if process.poll() is not None:
            break
        time.sleep(0.1)
    raise AssertionError("quest server did not boot:\n" + output_path.read_text(errors="replace")[-8000:])


def stop(process, output) -> None:
    process.send_signal(signal.SIGTERM)
    process.wait(timeout=30)
    output.close()
    journey.require(process.returncode == 0, f"quest server exited {process.returncode}")


def player_item_rows(state_root: Path) -> list[dict]:
    state = journey.inspect_authority(state_root)
    return state["player_items"]


def run(binary: Path, *, daily: bool = False) -> None:
    with tempfile.TemporaryDirectory(prefix="duris-quest-state-") as state_tmp, \
         tempfile.TemporaryDirectory(prefix="duris-quest-run-") as run_tmp:
        state_root, run_root = Path(state_tmp), Path(run_tmp)
        state_root.chmod(0o700)
        (state_root / "domains").mkdir(mode=0o700)
        subprocess.run([str(journey.INSPECTOR), str(state_root), "seed-combat"], check=True)
        (run_root / "logs/log").mkdir(parents=True)
        (run_root / "logs/log/.gitignore").write_text("*\n!.gitignore\n")
        quest_fixture(run_root, daily=daily)
        if daily:
            seed_daily_evidence(state_root)
        journey.generate_certificate(run_root)
        journals = run_root / "journals"
        (journals / "players").mkdir(parents=True, mode=0o700)
        (journals / "critical").mkdir(mode=0o700)
        port, tls_port, websocket_port = journey.available_ports()
        environment = {
            "PATH": os.environ.get("PATH", "/usr/bin:/bin"),
            "ENVIRONMENT": "local", "PERSISTENCE_MODE": "flatfile-primary",
            "FLATFILE_STATE_DIR": str(state_root),
            "PLAYER_SAVE_JOURNAL_DIR": str(journals / "players"),
            "CRITICAL_COMMAND_JOURNAL_DIR": str(journals / "critical"),
            "LISTEN_ADDRESS": "127.0.0.1", "DURIS_TLS_PORT": str(tls_port),
            "DURIS_WEBSOCKET_LISTEN_ADDRESS": "127.0.0.1",
            "DURIS_WEBSOCKET_PORT": str(websocket_port), "REDIS": "FALSE",
            "CHAOS_MUD": "FALSE",
        }
        if daily:
            environment["ZONE_STORY_DAILY_ENABLED"] = "true"
        if runtime_library_path := os.environ.get("LD_LIBRARY_PATH"):
            environment["LD_LIBRARY_PATH"] = runtime_library_path

        for phase in ("initial", "restart"):
            output_path = run_root / f"{phase}.out"
            process, output = boot(binary, run_root, environment, port, output_path)
            client = None
            try:
                if phase == "initial":
                    client = journey.MudClient(port)
                    journey.create_character(client, expected_room=None, class_name="d",
                                             hometown="p")
                    client.send("look")
                    client.expect("The Regression Arena", timeout=15)
                    if daily:
                        client.send("quest zone Minimal World")
                        client.expect("Story incomplete", timeout=15)
                        client.expect("Return to continue", timeout=15)
                        client.send("q")
                        client.send("quest daily Minimal World")
                        client.expect("Available", timeout=15)
                        client.expect("Return to continue", timeout=15)
                        client.send("q")
                    client.send("drop all")
                    client.expect("You drop", timeout=20)
                    for name in ("acorn", "branch", "feather"):
                        client.send(f"get {name}")
                        client.expect("You get", timeout=15)
                    client.send("give acorn lapney")
                    client.expect("Your quest offering is being accepted.", timeout=15)
                    client.expect_any(("a quest reward blade", "Your committed quest reward is being recovered."), timeout=30)
                    client.send("save")
                    client.expect(f"Save complete for {journey.CHARACTER}.", timeout=30)
                else:
                    client = journey.reconnect_character(port)
                if daily:
                    client.send("score")
                    client.expect("renown 1", timeout=15)
                    client.send("quest daily Minimal World")
                    client.expect("Done today", timeout=15)
                    client.expect("Return to continue", timeout=15)
                    client.send("q")
                    if phase == "initial":
                        client.send("get flower")
                        client.expect("You get", timeout=15)
                        client.send("give flower lapney")
                        client.expect("Lapney accepts the flower", timeout=30)
                        client.send("quest daily")
                        client.expect("Completed today: 2; renown: 1", timeout=15)
                    else:
                        client.send("quest daily")
                        client.expect("Completed today: 2; renown: 1", timeout=15)
                client.send("inventory")
                client.expect("a quest reward blade", timeout=20)
                client.send("save")
                client.expect(f"Save complete for {journey.CHARACTER}.", timeout=30)
                items = player_item_rows(state_root)
                rows = [item for item in items if item["vnum"] == REWARD_VNUM]
                journey.require(len(rows) == 1, f"expected one durable reward, found {rows}")
                journey.require(not any(item["vnum"] in (22802, 22803, 22804)
                                        for item in items),
                                "consumed offerings reappeared in player custody")
                if phase == "initial":
                    reward_uid = rows[0]["uid"]
                else:
                    journey.require(rows[0]["uid"] == reward_uid,
                                    "cold load changed reward UID")
                client.send("quit")
                client.expect("ACCOUNT MENU", timeout=30)
                client.send("0")
                client.close()
                client = None
                stop(process, output)
            except Exception as error:
                output.flush()
                raise AssertionError(
                    f"{phase}: {error}\n--- server ---\n{output_path.read_text(errors='replace')[-10000:]}"
                    f"\n--- client ---\n{bytes(client.transcript).decode('utf-8', errors='replace')[-6000:] if client else ''}"
                    f"\n--- logs ---\n{journey.runtime_logs(run_root)}"
                ) from error
            finally:
                if client is not None:
                    client.close()
                if process.poll() is None:
                    process.terminate()
                    try:
                        process.wait(timeout=5)
                    except subprocess.TimeoutExpired:
                        process.kill()
                        process.wait(timeout=5)
                if not output.closed:
                    output.close()


if __name__ == "__main__":
    journey.build_inspector()
    with tempfile.TemporaryDirectory(prefix="duris-quest-build-") as build_tmp:
        os.environ.setdefault("DURIS_REGRESSION_BUILD_CACHE",
                              str(ROOT / "bin/regression-artifacts"))
        binary = server_build_artifacts.build_flatfile_server(Path(build_tmp))
        run(binary)
    print("static quest reward grant, save and cold reconnect passed")
