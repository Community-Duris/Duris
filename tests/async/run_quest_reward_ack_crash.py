#!/usr/bin/env python3
"""Crash after offering publication ack and inspect quest reward on restart."""

import argparse
import os
from pathlib import Path
import subprocess
import tempfile
import time

import test_flatfile_combat_journey as journey
import test_static_quest_reward_journey as quest


def fault_boot(binary: Path, run_root: Path, environment: dict[str, str],
               port: int, output_path: Path):
    output = output_path.open("w", encoding="utf-8")
    command = [
        "gdb", "-q", "-batch", "-ex", "set confirm off",
        "-ex", "set debuginfod enabled off",
        "-ex", "break complete_quest_offering", "-ex", "run",
        "-ex", "kill", "--args", str(binary), "--minimal",
        "-d", str(run_root), str(port),
    ]
    process = subprocess.Popen(command, cwd=run_root, env=environment,
                               stdout=output, stderr=subprocess.STDOUT)
    deadline = time.monotonic() + 120
    while time.monotonic() < deadline:
        output.flush()
        if "Entering game loop." in output_path.read_text(errors="replace"):
            return process, output
        if process.poll() is not None:
            break
        time.sleep(0.1)
    raise AssertionError("fault server did not boot:\n" +
                         output_path.read_text(errors="replace")[-8000:])


def run(binary: Path, expect_recovered: bool) -> None:
    with tempfile.TemporaryDirectory(prefix="duris-quest-crash-state-") as state_tmp, \
         tempfile.TemporaryDirectory(prefix="duris-quest-crash-run-") as run_tmp:
        state_root, run_root = Path(state_tmp), Path(run_tmp)
        state_root.chmod(0o700)
        (state_root / "domains").mkdir(mode=0o700)
        subprocess.run([str(journey.INSPECTOR), str(state_root), "seed-combat"],
                       check=True)
        (run_root / "logs/log").mkdir(parents=True)
        (run_root / "logs/log/.gitignore").write_text("*\n!.gitignore\n")
        quest.quest_fixture(run_root)
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

        crash_output = run_root / "ack-crash.out"
        process, output = fault_boot(binary, run_root, environment, port,
                                     crash_output)
        client = journey.MudClient(port)
        try:
            journey.create_character(client, expected_room=None, class_name="d",
                                     hometown="p")
            client.send("drop all")
            client.expect("You drop", timeout=20)
            for name in ("acorn", "branch", "feather"):
                client.send(f"get {name}")
                client.expect("You get", timeout=15)
            client.send("give acorn lapney")
            client.expect("Your quest offering is being accepted.", timeout=15)
            process.wait(timeout=30)
            output.flush()
            fault_text = crash_output.read_text(errors="replace")
            journey.require("Breakpoint 1, complete_quest_offering" in fault_text,
                            "fault did not reach post-ack quest completion callback")
        finally:
            client.close()
            if process.poll() is None:
                process.kill()
                process.wait(timeout=5)
            output.close()

        restart_output = run_root / "restart.out"
        process, output = quest.boot(binary, run_root, environment, port,
                                     restart_output)
        client = None
        try:
            client = journey.reconnect_character(port)
            client.send("save")
            client.expect(f"Save complete for {journey.CHARACTER}.", timeout=30)
            items = quest.player_item_rows(state_root)
            rewards = [item for item in items if item["vnum"] == quest.REWARD_VNUM]
            offerings = [item for item in items if item["vnum"] in (22802, 22803, 22804)]
            journey.require(not offerings, f"consumed offerings returned: {offerings}")
            expected = 1 if expect_recovered else 0
            journey.require(len(rewards) == expected,
                            f"expected {expected} reward after ack crash, found {rewards}")
            client.send("quit")
            client.expect("ACCOUNT MENU", timeout=30)
            client.send("0")
            client.close()
            client = None
            quest.stop(process, output)
        finally:
            if client is not None:
                client.close()
            if process.poll() is None:
                process.kill()
                process.wait(timeout=5)
            if not output.closed:
                output.close()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--server", type=Path, required=True)
    parser.add_argument("--confirm-loss", action="store_true",
                        help="record the known failing baseline before recovery is implemented")
    args = parser.parse_args()
    subprocess.run(["python3", "tests/async/test_flatfile_player_repository.py",
                    "--build-inspector", str(journey.INSPECTOR)],
                   cwd=quest.ROOT, check=True, timeout=180)
    run(args.server, not args.confirm_loss)
    print("post-ack quest reward crash journey passed")
