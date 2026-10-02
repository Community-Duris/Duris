#!/usr/bin/env python3
"""Real account-menu deletion and cold reload in an owned flatfile fixture."""

import argparse
import hashlib
import os
from pathlib import Path
import signal
import subprocess
import struct
import tempfile
import time

import test_flatfile_combat_journey as journey


def quest_aliases(encoded):
    """Validate the native envelope and decode retained name records."""
    assert encoded[:8] == b"DURZQST1" and len(encoded) >= 56
    version, revision, length = struct.unpack_from("<IIQ", encoded, 8)
    payload = encoded[56:]
    assert version == 1 and revision > 0 and length == len(payload)
    assert hashlib.sha256(payload).digest() == encoded[24:56]
    aliases = []
    for line in payload.decode().splitlines():
        if line.startswith("N|"):
            fields = line.split("|")
            assert len(fields) == 5
            aliases.append((int(fields[2]), bytes.fromhex(fields[3]).decode()))
    return aliases


def run(server, inspector):
    with tempfile.TemporaryDirectory(prefix="flatfile-deletion-") as temporary:
        root = Path(temporary)
        state, runtime = root / "state", root / "runtime"
        state.mkdir(mode=0o700)
        (state / "domains").mkdir(mode=0o700)
        runtime.mkdir()
        subprocess.run([str(inspector), str(state), "seed-empty-deletion"], check=True)
        journey.make_fixture(runtime)
        journey.generate_certificate(runtime)
        (runtime / "logs/log").mkdir(parents=True)
        for name in ("players", "critical"):
            (runtime / "journals" / name).mkdir(parents=True, mode=0o700)
        plain, tls, websocket = journey.available_ports()
        environment = {
            "PATH": os.environ.get("PATH", "/usr/bin:/bin"), "ENVIRONMENT": "local",
            "PERSISTENCE_MODE": "flatfile-primary", "FLATFILE_STATE_DIR": str(state),
            "PLAYER_SAVE_JOURNAL_DIR": str(runtime / "journals/players"),
            "CRITICAL_COMMAND_JOURNAL_DIR": str(runtime / "journals/critical"),
            "LISTEN_ADDRESS": "127.0.0.1", "DURIS_TLS_PORT": str(tls),
            "DURIS_WEBSOCKET_PORT": str(websocket),
            "DURIS_WEBSOCKET_LISTEN_ADDRESS": "127.0.0.1",
            "REDIS": "FALSE", "CHAOS_MUD": "FALSE",
        }
        if "LD_LIBRARY_PATH" in os.environ:
            environment["LD_LIBRARY_PATH"] = os.environ["LD_LIBRARY_PATH"]
        output_path = runtime / "server.out"
        process = client = None
        with output_path.open("w") as output:
            def boot():
                offset = output_path.stat().st_size
                proc = subprocess.Popen([str(server), "--minimal", "-s", "-d", str(runtime),
                                         str(plain)], cwd=runtime, env=environment,
                                        stdout=output, stderr=subprocess.STDOUT)
                deadline = time.monotonic() + 120
                while b"Entering game loop." not in output_path.read_bytes()[offset:]:
                    if proc.poll() is not None or time.monotonic() > deadline:
                        if proc.poll() is None:
                            proc.terminate()
                        proc.wait(timeout=10)
                        raise AssertionError("flatfile deletion fixture failed to boot")
                    time.sleep(.1)
                return proc

            def stop():
                process.send_signal(signal.SIGTERM)
                process.wait(timeout=30)
                assert process.returncode == 0

            try:
                process = boot()
                client = journey.MudClient(plain)
                journey.create_character(client)
                client.send("save")
                client.expect("Save complete for " + journey.CHARACTER + ".", timeout=30)
                snapshot = state / "players/1.snapshot"
                assert snapshot.exists(), "synthetic player snapshot missing before deletion"
                client.send("quit")
                client.expect("ACCOUNT MENU", timeout=30)

                def request_deletion():
                    client.send("3")
                    client.expect("Which character do you want to")
                    client.send("1")
                    client.expect("FINAL WARNING", timeout=30)
                    client.send("yes")

                retained = state / "domains/zone-story-quests.state"
                original_quest_state = retained.read_bytes()
                assert (1, journey.CHARACTER) in quest_aliases(original_quest_state), "quest alias was never retained"
                original_snapshot = snapshot.read_bytes()
                summon_catalog = state / "domains/account_reward_summon_catalog"
                held_catalog = root / "held-summon-catalog"
                summon_catalog.rename(held_catalog)
                request_deletion()
                client.expect("Character deletion did not complete.", timeout=30)
                client.expect("ACCOUNT MENU")
                assert snapshot.read_bytes() == original_snapshot, "missing authority changed the snapshot"
                assert retained.read_bytes() == original_quest_state, "refusal erased retained quest identity"
                assert b"Character deleted successfully." not in client.transcript
                held_catalog.rename(summon_catalog)
                client.send("0")
                client.close()
                client = journey.reconnect_character(plain)
                client.send("save")
                client.expect("Save complete for " + journey.CHARACTER + ".", timeout=30)
                client.send("quit")
                client.expect("ACCOUNT MENU", timeout=30)
                request_deletion()
                client.expect("Character deleted successfully.", timeout=30)
                client.expect("ACCOUNT MENU")
                assert client.transcript.count(b"Character deleted successfully.") == 1
                assert not snapshot.exists(), "deleted snapshot remains loadable"
                erased = retained.read_bytes()
                assert (1, journey.CHARACTER) not in quest_aliases(erased)
                client.send("3")
                client.expect("don't have any characters to delete", timeout=15)
                client.send("0")
                client.close()
                client = None
                stop()
                process = boot()
                client = journey.MudClient(plain)
                client.expect("Please enter your account name:")
                client.send(journey.ACCOUNT)
                client.expect("Please enter your password:")
                client.send(journey.PASSWORD)
                client.expect("PRESS RETURN")
                client.send("")
                client.expect("ACCOUNT MENU")
                client.send("3")
                client.expect("don't have any characters to delete")
                assert not snapshot.exists() and retained.read_bytes() == erased
                stop()
                print("[PASS] flatfile missing-authority refusal, unchanged snapshot/quest alias, playable retry, deletion once, alias erasure and usable account after cold restart")
            except Exception:
                # Keep diagnostics to the owned deletion boundary; do not dump
                # unrelated account/session captures into the regression output.
                diagnostics = journey.runtime_logs(runtime) + output_path.read_text(errors="replace")
                for line in diagnostics.splitlines():
                    if "deleteCharacter()" in line:
                        print(line)
                raise
            finally:
                if client is not None:
                    client.close()
                if process is not None and process.poll() is None:
                    process.terminate()
                    process.wait(timeout=30)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--server", type=Path, required=True)
    parser.add_argument("--inspector", type=Path, required=True)
    args = parser.parse_args()
    run(args.server.resolve(strict=True), args.inspector.resolve(strict=True))
