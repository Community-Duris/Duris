#!/usr/bin/env python3
"""Drive the 'artifeed' command through a real flat-file server.

A disposable character is created in the minimal world and raised to OVERLORD while the
server is down (creation refuses god_list names), above the Forger level the command needs
to change a rate.
The journey lists the proposed rates, refuses unknown settings and out-of-range values,
sets the zone rate and the non-PvP ceiling, checks both reached the live property table
and lib/duris.properties at once, then resets everything to the proposed rates. Run it
explicitly with a fresh flat-file binary:

    python3 tests/async/run_artifeed_journey.py --server bin/server/dms_new
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
ACCOUNT = "Feedacct"
CHARACTER = "Feedwarden"
EMAIL = "feed@example.invalid"
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


def rate(listing: str, setting: str) -> tuple[float, str, float, bool]:
    """(rate, per-epic time, proposed rate, marked as changed) for one row of the listing."""
    match = re.search(rf"^\s*{setting}\s+.+?\s(\d+\.\d{{3}})\s+(\S*(?: \S+)?)\s+(\d+\.\d{{3}})(\s+\*)?\s*$",
                      listing, re.M)
    require(match is not None, f"no {setting} row in:\n{listing}")
    return float(match.group(1)), match.group(2), float(match.group(3)), bool(match.group(4))


def saved_value(run_root: pathlib.Path, key: str) -> str | None:
    found = re.search(rf"^{re.escape(key)}=(\S+)$", (run_root / "lib/duris.properties").read_text(), re.M)
    return found.group(1) if found else None


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--server", type=pathlib.Path, required=True)
    binary = parser.parse_args().server.resolve()
    require(binary.is_file() and os.access(binary, os.X_OK), f"not executable: {binary}")
    ROOT.joinpath("bin/tests").mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="duris-artifeed-inspector-", dir=ROOT / "bin/tests") as tools, \
            tempfile.TemporaryDirectory(prefix="duris-artifeed-state-") as state_tmp, \
            tempfile.TemporaryDirectory(prefix="duris-artifeed-run-") as run_tmp:
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
                # Creation refuses god_list names, so the Forger-level character 'artifeed set'
                # needs is an ordinary character raised to OVERLORD while the server is down.
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

                listing = command(client, "artifeed", "artifeed reset")
                require("Artifact feeding" in listing and "Non-PvP feeds stop 72 hours ahead" in listing, listing)
                require(rate(listing, "zone") == (0.02, "24 s", 0.02, False), f"zone: {rate(listing, 'zone')}")
                require(rate(listing, "pvp") == (7.2, "2.4 h", 7.2, False), f"pvp: {rate(listing, 'pvp')}")
                require(rate(listing, "randommob")[0] == 0.01 and rate(listing, "strahd")[0] == 0.02, listing)
                print("list: zone", rate(listing, "zone"), "pvp", rate(listing, "pvp"), flush=True)

                reply = command(client, "artifeed set zone 99", "takes a number")
                require("0.000 to 50.000" in reply and "is now" not in reply, reply)
                reply = command(client, "artifeed set zone fast", "takes a number")
                require("is now" not in reply, reply)
                reply = command(client, "artifeed set ceiling 500", "takes a number")
                require("1.000 to 240.000" in reply, reply)
                command(client, "artifeed set nosuch 1", "no feeding setting called 'nosuch'")
                command(client, "artifeed bogus", "Usage: artifeed")
                require(saved_value(run_root, "artifact.feeding.epic.typeMod.zone") == "0.020",
                        "a refused value reached the properties file")

                reply = command(client, "artifeed set zone 0.05", "artifeed reset")
                require("artifact.feeding.epic.typeMod.zone is now 0.050, saved" in reply, reply)
                require(rate(reply, "zone") == (0.05, "60 s", 0.02, True), f"zone after set: {rate(reply, 'zone')}")
                reply = command(client, "artifeed set ceiling 96", "artifeed reset")
                require("Non-PvP feeds stop 96 hours ahead" in reply, reply)
                shown = command(client, "properties show artifact.feeding.*", "typeMod.zone")
                require(re.search(r"^artifact\.feeding\.epic\.typeMod\.zone: 0\.050\s*$", shown, re.M),
                        f"live property table did not change (or was left unsaved):\n{shown}")
                require(saved_value(run_root, "artifact.feeding.epic.typeMod.zone") == "0.050" and
                        saved_value(run_root, "artifact.feeding.nonPvp.ceilingHours") == "96.000",
                        "set did not save to lib/duris.properties")
                print("set: zone 0.020 -> 0.050 (60 s an epic), ceiling 72 -> 96 h, both saved", flush=True)

                reply = command(client, "artifeed reset all", "artifeed reset", timeout=30)
                rows = reply.split("Setting", 1)[1].split("* differs from")[0]
                require(rate(reply, "zone") == (0.02, "24 s", 0.02, False) and "*" not in rows, reply)
                require(saved_value(run_root, "artifact.feeding.epic.typeMod.zone") == "0.020" and
                        saved_value(run_root, "artifact.feeding.nonPvp.ceilingHours") == "72.000",
                        "reset did not save the proposed rates")
                print("reset: every rate back to the proposed value and saved", flush=True)

                client.send("quit")
                client.expect_any(("ACCOUNT MENU", "Goodbye", "account menu"), timeout=30)
                client.close()
                client = None
                process.send_signal(signal.SIGTERM)
                process.wait(timeout=30)
                log = output_path.read_text(errors="replace")
                require("FATAL:" not in log and "assert:" not in log, "server logged a fatal or assertion")
                print("[PASS] artifeed journey: list, refusals, set and save, live table, reset", flush=True)
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
