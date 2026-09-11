#!/usr/bin/env python3
"""Pick up the library's hidden, area-authored money through the real server.

Uses only temporary world/player data and loopback listeners. --server accepts
an already built flat-file server; otherwise build with the shared test helper.
"""

import argparse
import os
from pathlib import Path
import re
import signal
import subprocess
import tempfile
import time

from test_flatfile_combat_journey import (
    ROOT, INSPECTOR, MudClient, available_ports, build_flatfile_server,
    create_character, generate_certificate, inspect_authority, make_fixture,
    require, runtime_logs,
)


def run(binary: Path, command: str) -> None:
    with tempfile.TemporaryDirectory(prefix="duris-area-coins-") as temporary:
        root = Path(temporary)
        state = root / "state"
        state.mkdir(mode=0o700)
        (state / "domains").mkdir(mode=0o700)
        subprocess.run([str(INSPECTOR), str(state), "seed-combat"], check=True)
        (root / "logs/log").mkdir(parents=True)
        (root / "logs/log/.gitignore").write_text("*\n!.gitignore\n")
        make_fixture(root)
        # Preserve the reported statue, ITEM_SECRET flag, money type, and ten
        # platinum; remap only vnums into the isolated regression zone.
        library = (ROOT / "areas/obj/library.obj").read_text()
        objects = root / "areas_mini/mini.obj"
        additions = []
        for old, new in ((402001, 22801), (402013, 22802)):
            match = re.search(rf"^#{old}\n.*?(?=^#|^\$)", library, re.M | re.S)
            require(match is not None, f"missing library prototype {old}")
            additions.append(match[0].replace(f"#{old}\n", f"#{new}\n", 1))
        objects.write_text(objects.read_text().replace("$~", "".join(additions) + "$~"))
        zone = root / "areas_mini/mini.zon"
        text = re.sub(r"^[MG] .*\n", "", zone.read_text(), flags=re.M)
        zone.write_text(text.replace(
            "\nS\n", "\nO 0 22801 1 22800 100 0 0 0\nP 1 22802 1 22801 100 0 0 0\nS\n"))
        generate_certificate(root)
        for kind in ("players", "critical"):
            (root / "journals" / kind).mkdir(parents=True, mode=0o700)
        port, tls, websocket = available_ports()
        env = {
            "PATH": os.environ.get("PATH", "/usr/bin:/bin"),
            "ENVIRONMENT": "local", "PERSISTENCE_MODE": "flatfile-primary",
            "FLATFILE_STATE_DIR": str(state),
            "PLAYER_SAVE_JOURNAL_DIR": str(root / "journals/players"),
            "CRITICAL_COMMAND_JOURNAL_DIR": str(root / "journals/critical"),
            "LISTEN_ADDRESS": "127.0.0.1", "DURIS_TLS_PORT": str(tls),
            "DURIS_WEBSOCKET_LISTEN_ADDRESS": "127.0.0.1",
            "DURIS_WEBSOCKET_PORT": str(websocket),
            "REDIS": "FALSE", "CHAOS_MUD": "FALSE",
        }
        if "LD_LIBRARY_PATH" in os.environ:
            env["LD_LIBRARY_PATH"] = os.environ["LD_LIBRARY_PATH"]
        output_path = root / "server.out"
        with output_path.open("w") as output:
            process = subprocess.Popen(
                [str(binary), "--minimal", "-s", "-d", str(root), str(port)],
                cwd=root, env=env, stdout=output, stderr=subprocess.STDOUT,
            )
            client = None
            try:
                deadline = time.monotonic() + 120
                while time.monotonic() < deadline and process.poll() is None:
                    if "Entering game loop." in output_path.read_text(errors="replace"):
                        break
                    time.sleep(0.1)
                require(process.poll() is None, "area coin server failed to boot")
                client = MudClient(port)
                create_character(client)
                # The unrelated carry-count gate also applies to money, so make
                # room explicitly before exercising the custody regression.
                client.send("drop all")
                client.expect("You drop", timeout=30)
                client.expect("Pos: standing >", timeout=30)
                # Search is a skill roll; retry normal misses without weakening
                # the pickup assertion or removing the authored secret flag.
                for _ in range(30):
                    client.send("search statue")
                    found, _ = client.expect_any((
                        "You find a small pile of hidden coins!",
                        "You don't find anything you didn't see before.",
                    ), timeout=20)
                    if found.startswith("You find"):
                        break
                else:
                    raise AssertionError("could not discover the hidden fixture coins")
                client.send("look in statue")
                client.expect("a small pile of hidden coins", timeout=20)
                require(inspect_authority(state)["wallet"] == [0, 0, 0, 0],
                        "fixture started with money")
                client.send(command)
                client.expect("You get", timeout=30)
                deadline = time.monotonic() + 15
                while time.monotonic() < deadline:
                    if inspect_authority(state)["wallet"] == [0, 0, 0, 10]:
                        break
                    time.sleep(0.1)
                authority = inspect_authority(state)
                require(authority["wallet"] == [0, 0, 0, 10],
                        "area coin pickup did not durably credit ten platinum")
                require(not any(item["vnum"] == 22802 for item in authority["room_items"]),
                        "consumed area coins still have active room custody")
                client.send("look in statue")
                client.expect("Nothing.", timeout=20)
                # Repeating pickup must not mint a second wallet credit.
                client.send("get coins statue")
                client.expect("does not contain the coins.", timeout=20)
                require(inspect_authority(state)["wallet"] == [0, 0, 0, 10],
                        "repeated pickup duplicated area money")
                process.send_signal(signal.SIGTERM)
                process.wait(timeout=30)
                require(process.returncode == 0, "area coin server shutdown failed")
                print(f"area P-reset pickup passed: {command}; wallet=10 platinum", flush=True)
            except Exception as error:
                raise AssertionError(
                    f"{error}\n{output_path.read_text(errors='replace')[-8000:]}\n"
                    f"{runtime_logs(root)}"
                ) from error
            finally:
                if client:
                    client.close()
                if process.poll() is None:
                    process.terminate()
                    try:
                        process.wait(timeout=10)
                    except subprocess.TimeoutExpired:
                        process.kill()
                        process.wait(timeout=10)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--server", type=Path)
    args = parser.parse_args()
    subprocess.run(["python3", "tests/async/test_flatfile_player_repository.py",
                    "--build-inspector", str(INSPECTOR)], cwd=ROOT, check=True, timeout=180)
    with tempfile.TemporaryDirectory(prefix="area-coin-build-", dir=ROOT / "bin/tests") as build:
        binary = args.server.resolve() if args.server else build_flatfile_server(Path(build))
        for command in ("get coins statue", "get all.coins statue", "take all statue"):
            run(binary, command)
