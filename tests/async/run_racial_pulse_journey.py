#!/usr/bin/env python3
"""Drive the 'pulse' command through a real flat-file server.

A disposable character is created in the minimal world and raised to OVERLORD while the
server is down (creation refuses the god_list names that once did this), which is above
the Forger level the command needs for 'adjust' and 'save'. The journey lists the rates,
refuses badly formed and out-of-range values, adjusts one casting and one melee rate,
checks the change reached the property table, saves, and checks the saved file. Run it
explicitly with a fresh flat-file binary:

    python3 tests/async/run_racial_pulse_journey.py --server bin/server/dms_new
"""

from __future__ import annotations

import argparse
import os
import pathlib
import re
import signal
import subprocess
import tempfile
import time

import test_flatfile_combat_journey as journey


ROOT = pathlib.Path(__file__).resolve().parents[2]
ACCOUNT = "Pulseacct"
CHARACTER = "Pulsewarden"
EMAIL = "pulse@example.invalid"
PROMPTS = ("Pos: standing >", "<>")


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def create_character(client: journey.MudClient) -> None:
    entry, _ = client.expect_any(("term type", "account name"))
    if entry == "term type":
        client.send("9")
        client.expect("account name")
    client.send(ACCOUNT)
    client.expect("is this correct?")
    client.send("y")
    client.expect("email address")
    client.send(EMAIL)
    client.expect("is this correct?")
    client.send("y")
    client.expect("enter your password")
    client.send(journey.PASSWORD)
    client.expect("re-enter the same password to confirm")
    client.send(journey.PASSWORD)
    client.expect("information correct?")
    client.send("y")
    client.expect("PRESS RETURN")
    client.send("")
    client.expect("Please select an option")
    client.send("2")
    client.expect("Enter your new name")
    client.send(CHARACTER)
    client.expect("Is this correct?")
    client.send("y")
    client.expect("meet these criteria?")
    client.send("y")
    client.expect("Your selection")
    client.send("h")
    client.expect("Male or Female")
    client.send("m")
    client.expect("Hardcore")
    client.send("n")
    client.expect("Class Selection")
    client.send("w")
    client.expect("Alignment only affects")
    client.send("g")
    client.expect("Your selection")
    client.send("p")
    client.expect("Press return to continue")
    client.send("")
    for label in ("first bonus", "second bonus", "third bonus", "fourth bonus"):
        client.expect(label)
        client.send("s")
    client.expect("swap stats")
    client.send("n")
    client.expect("keep this character")
    client.send("y")
    client.expect("PRESS RETURN")
    client.send("")
    client.expect_any(PROMPTS, timeout=30)


def command(client: journey.MudClient, line: str, until: str, timeout: float = 15) -> str:
    """Send one command and return everything up to and including the prompt after 'until'."""
    start = len(client.transcript)
    client.send(line)
    client.expect(until, timeout=timeout)
    client.expect_any(PROMPTS, timeout=timeout)
    return bytes(client.transcript[start:]).decode("utf-8", errors="replace")


def paged(client: journey.MudClient, line: str, until: str, timeout: float = 30) -> str:
    """Like command(), pressing Return through the pager until 'until' arrives."""
    start = len(client.transcript)
    client.send(line)
    deadline = time.monotonic() + timeout
    while True:
        # A page can carry the closing text and the pager prompt together, so page on every
        # pager prompt and stop at the first gameplay prompt once the closing text is in.
        seen, _ = client.expect_any(("Return to continue",) + PROMPTS,
                                    timeout=max(1.0, deadline - time.monotonic()))
        if seen == "Return to continue":
            client.send("")
            continue
        text = bytes(client.transcript[start:]).decode("utf-8", errors="replace")
        if until in text:
            return text


def row(listing: str, race: str) -> tuple[float, float, float]:
    match = re.search(rf"^\s*{re.escape(race)}\s+\S*\s+(\d+\.\d{{3}})\s+(\d+\.\d{{3}})\s+(\d+\.\d{{3}})",
                      listing, re.M)
    require(match is not None, f"no {race} row in:\n{listing}")
    return tuple(float(match.group(i)) for i in (1, 2, 3))


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--server", type=pathlib.Path, required=True)
    binary = parser.parse_args().server.resolve()
    require(binary.is_file() and os.access(binary, os.X_OK), f"not executable: {binary}")
    ROOT.joinpath("bin/tests").mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="duris-pulse-inspector-", dir=ROOT / "bin/tests") as tools, \
            tempfile.TemporaryDirectory(prefix="duris-pulse-state-") as state_tmp, \
            tempfile.TemporaryDirectory(prefix="duris-pulse-run-") as run_tmp:
        inspector = pathlib.Path(tools) / "inspector"
        subprocess.run(["python3", "tests/async/test_flatfile_player_repository.py",
                        "--build-inspector", str(inspector)], cwd=ROOT, check=True, timeout=180)
        state_root, run_root = pathlib.Path(state_tmp), pathlib.Path(run_tmp)
        state_root.chmod(0o700)
        (state_root / "domains").mkdir(mode=0o700)
        subprocess.run([str(inspector), str(state_root), "seed-combat"], check=True, timeout=30)
        (run_root / "logs/log").mkdir(parents=True)
        (run_root / "logs/log/.gitignore").write_text("*\n!.gitignore\n")
        journey.make_fixture(run_root)
        journey.generate_certificate(run_root)
        (run_root / "journals/players").mkdir(parents=True, mode=0o700)
        (run_root / "journals/critical").mkdir(mode=0o700)
        plain_port, tls_port, websocket_port = journey.available_ports()
        environment = {
            "PATH": os.environ.get("PATH", "/usr/bin:/bin"), "ENVIRONMENT": "local",
            "PERSISTENCE_MODE": "flatfile-primary", "FLATFILE_STATE_DIR": str(state_root),
            "PLAYER_SAVE_JOURNAL_DIR": str(run_root / "journals/players"),
            "CRITICAL_COMMAND_JOURNAL_DIR": str(run_root / "journals/critical"),
            "LISTEN_ADDRESS": "127.0.0.1", "DURIS_TLS_PORT": str(tls_port),
            "DURIS_WEBSOCKET_LISTEN_ADDRESS": "127.0.0.1", "DURIS_WEBSOCKET_PORT": str(websocket_port),
            "REDIS": "FALSE", "CHAOS_MUD": "FALSE", "CREATION_ALL_CLASSES": "TRUE",
        }
        if os.environ.get("LD_LIBRARY_PATH"):
            environment["LD_LIBRARY_PATH"] = os.environ["LD_LIBRARY_PATH"]
        output_path = run_root / "server.out"
        with output_path.open("w", encoding="utf-8") as output:
            def start() -> subprocess.Popen:
                return subprocess.Popen([str(binary), "--minimal", "-s", "-d", str(run_root), str(plain_port)],
                                        cwd=run_root, env=environment, text=True,
                                        stdout=output, stderr=subprocess.STDOUT)

            def wait_for_boot(boots: int) -> None:
                deadline = time.monotonic() + 120
                while time.monotonic() < deadline and output_path.read_text(errors="replace").count("Entering game loop.") < boots:
                    require(process.poll() is None, "server exited during boot:\n" + output_path.read_text(errors="replace")[-6000:])
                    time.sleep(0.1)

            process = start()
            client = None
            try:
                wait_for_boot(1)
                client = journey.MudClient(plain_port)
                create_character(client)
                # Creation refuses god_list names, so the OVERLORD that 'pulse adjust' and
                # 'pulse save' need is an ordinary character raised while the server is down.
                command(client, "save", f"Save complete for {CHARACTER}.")
                client.send("quit")
                client.expect("ACCOUNT MENU", timeout=30)
                client.close()
                client = None
                process.send_signal(signal.SIGTERM)
                process.wait(timeout=30)
                journey.make_overlord(state_root, CHARACTER)
                process = start()
                wait_for_boot(2)
                client = journey.reconnect_character(plain_port, expected_room=None,
                                                     account=ACCOUNT, character=CHARACTER)
                client.expect_any(PROMPTS, timeout=30)

                listing = command(client, "pulse", "pulse save")
                require("Racial pulse" in listing, listing)
                require(row(listing, "Gnome") == (12.0, 14.0, 0.895), f"gnome row: {row(listing, 'Gnome')}")
                require(row(listing, "Ogre") == (16.25, 18.25, 1.055), f"ogre row: {row(listing, 'Ogre')}")
                require(row(listing, "Kobold") == (12.0, 14.0, 0.895), f"kobold row: {row(listing, 'Kobold')}")
                print("list: gnome", row(listing, "Gnome"), "ogre", row(listing, "Ogre"), flush=True)

                for bad in ("0.9", "0.90", "0.9000", "-0.900", "0.900 extra"):
                    reply = command(client, f"pulse adjust cast gnome {bad}", "exactly three decimals")
                    require("->" not in reply, f"'{bad}' was accepted:\n{reply}")
                reply = command(client, "pulse adjust melee ogre 41.000", "must be from")
                require("1.000 to 40.000" in reply, reply)
                reply = command(client, "pulse adjust cast nosuchrace 0.900", "No such race")
                reply = command(client, "pulse adjust fast gnome 0.900", "Usage: pulse adjust")

                reply = command(client, "pulse adjust cast gnome 0.900", "pulse:")
                require("Gnome cast pulse: 0.895 -> 0.900" in reply, reply)
                reply = command(client, "pulse adjust melee ogre 16.500", "pulse:")
                require("Ogre melee pulse: 16.250 -> 16.500" in reply, reply)
                shown = command(client, "properties show spellcast.pulse.racial.Gnome", "spellcast.pulse.racial.Gnome")
                require("0.900(0.895)" in shown, f"property table did not change:\n{shown}")
                listing = command(client, "pulse", "pulse save")
                require(row(listing, "Gnome")[2] == 0.900 and row(listing, "Ogre")[0] == 16.5,
                        f"list after adjust: gnome {row(listing, 'Gnome')} ogre {row(listing, 'Ogre')}")
                every = paged(client, "pulse list all", "pulse save")
                require(re.search(r"^\s*Illithid\s", every, re.M), "list all omitted a restricted race")
                print("adjust: gnome cast 0.895 -> 0.900, ogre melee 16.250 -> 16.500; list all has",
                      len(re.findall(r"^\s+\S.*\d+\.\d{3}\s*$", every, re.M)), "rows", flush=True)

                client.send("pulse save")
                deadline = time.monotonic() + 15
                saved = run_root / "lib/duris.properties"
                while time.monotonic() < deadline and "spellcast.pulse.racial.Gnome=0.900" not in saved.read_text():
                    time.sleep(0.2)
                text = saved.read_text()
                require("spellcast.pulse.racial.Gnome=0.900" in text and "damage.pulse.racial.Ogre=16.500" in text,
                        "pulse save did not write the new values")
                print("save: lib/duris.properties has Gnome cast 0.900 and Ogre melee 16.500", flush=True)

                client.send("quit")
                client.expect_any(("ACCOUNT MENU", "Goodbye", "account menu"), timeout=30)
                client.close()
                client = None
                process.send_signal(signal.SIGTERM)
                process.wait(timeout=30)
                log = output_path.read_text(errors="replace")
                require("FATAL:" not in log and "assert:" not in log, "server logged a fatal or assertion")
                print("[PASS] racial pulse journey: list, refusals, adjust cast and melee, properties, save", flush=True)
            except Exception as error:
                raise AssertionError(f"{error}\n--- server output ---\n"
                                     + output_path.read_text(errors="replace")[-10000:]) from error
            finally:
                if client is not None:
                    client.close()
                if process.poll() is None:
                    process.terminate()
                    try:
                        process.wait(timeout=10)
                    except subprocess.TimeoutExpired:
                        process.kill()


if __name__ == "__main__":
    main()
