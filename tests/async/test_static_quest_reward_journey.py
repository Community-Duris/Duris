#!/usr/bin/env python3
"""Consume a real NPC quest offering, then save and cold-load its item reward."""

from pathlib import Path
import os
import re
import signal
import subprocess
import tempfile
import time

import server_build_artifacts
import test_flatfile_combat_journey as journey


ROOT = Path(__file__).resolve().parents[2]
REWARD_VNUM = 22805


def quest_fixture(run_root: Path, xp_reward: int = 0) -> None:
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
    reward = (mace.replace("#677\n", "#22805\n", 1)
              .replace("mace wooden gnoby wood", "blade reward quest")
              .replace("a small wooden mace", "a quest reward blade")
              .replace("A gnoby piece of wood, perhaps a small mace, lies here.",
                       "A quest reward blade lies here."))
    assert content.count("$~") == 1
    objects.write_text(content.replace("$~", offerings + reward + "$~"))

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
    assert content.count("\nS\n") == 1
    zone.write_text(content.replace("\nS\n", "\n" + reset + "S\n"))


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


def run(binary: Path) -> None:
    with tempfile.TemporaryDirectory(prefix="duris-quest-state-") as state_tmp, \
         tempfile.TemporaryDirectory(prefix="duris-quest-run-") as run_tmp:
        state_root, run_root = Path(state_tmp), Path(run_tmp)
        state_root.chmod(0o700)
        (state_root / "domains").mkdir(mode=0o700)
        subprocess.run([str(journey.INSPECTOR), str(state_root), "seed-combat"], check=True)
        (run_root / "logs/log").mkdir(parents=True)
        (run_root / "logs/log/.gitignore").write_text("*\n!.gitignore\n")
        quest_fixture(run_root)
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
                    client.send("drop all")
                    client.expect("You drop", timeout=20)
                    for name in ("acorn", "branch", "feather"):
                        client.send(f"get {name}")
                        client.expect("You get", timeout=15)
                    client.send("give acorn lapney")
                    client.expect("Your quest offering is being accepted.", timeout=15)
                    client.expect("Your committed quest reward is being recovered.", timeout=30)
                else:
                    client = journey.reconnect_character(port)
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
    subprocess.run(["python3", "tests/async/test_flatfile_player_repository.py",
                    "--build-inspector", str(journey.INSPECTOR)],
                   cwd=ROOT, check=True, timeout=180)
    with tempfile.TemporaryDirectory(prefix="duris-quest-build-") as build_tmp:
        os.environ.setdefault("DURIS_REGRESSION_BUILD_CACHE",
                              str(ROOT / "bin/regression-artifacts"))
        binary = server_build_artifacts.build_flatfile_server(Path(build_tmp))
        run(binary)
    print("static quest reward grant, save and cold reconnect passed")
