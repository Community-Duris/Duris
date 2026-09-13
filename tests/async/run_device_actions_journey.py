#!/usr/bin/env python3
"""Real Telnet use/recite, warning window, typeahead, abort and spent resources.

Explicit disposable flatfile journey. Synthetic devices select magic missile via
normal item values; account creation, parsing, commands and effects are native.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import signal
import subprocess
import tempfile
import time

import test_flatfile_combat_journey as journey
import run_weapon_actions_journey as weapon

HIT = "magic missile hits"


def configure(root: Path, enabled: bool, kind: str) -> None:
    weapon.configure(root, False)
    props = root / "lib/duris.properties"
    text = props.read_text()
    for key, value in {"itemActions.enabled": int(enabled), "itemActions.wands.enabled": 1,
                       "itemActions.staves.enabled": 1, "itemActions.scrolls.enabled": 1}.items():
        text, count = re.subn(rf"(?m)^{re.escape(key)}=.*$", f"{key}={value}", text)
        assert count == 1
    props.write_text(text)
    mini = root / "areas_mini"
    types = {"wand": 3, "staff": 4, "scroll": 2}
    values = "6 32 0 0 0 0 0 0" if kind == "scroll" else "6 1 1 32 0 0 0 0"
    obj = f"""#22802
regression {kind}~
the regression {kind}~
A regression {kind} lies here.~
~
{types[kind]} 6 3 8 7 0 0 16385 0 0 0
{values}
2 0 100
"""
    objects = mini / "mini.obj"
    objects.write_text(objects.read_text().replace("$~", obj + "$~"))
    zone = mini / "mini.zon"
    text = re.sub(r"^E .*\n", "", zone.read_text(), flags=re.M)
    zone.write_text(text.replace("\nS\n", "\nO 0 22802 1 22800 100 0 0 0 * device\nS\n"))


def run_case(binary: Path, inspector: Path, kind: str, enabled: bool, abort: bool) -> dict:
    case = f"{kind}-{'on' if enabled else 'off'}-{'abort' if abort else 'complete'}"
    print(f"device journey: {case}", flush=True)
    with tempfile.TemporaryDirectory(prefix="duris-device-state-") as state_tmp, \
         tempfile.TemporaryDirectory(prefix="duris-device-run-") as run_tmp:
        state = Path(state_tmp); state.chmod(0o700); (state / "domains").mkdir(mode=0o700)
        root = Path(run_tmp)
        subprocess.run([str(inspector), str(state), "seed-combat"], check=True)
        configure(root, enabled, kind)
        journal = root / "journals"
        (journal / "players").mkdir(parents=True, mode=0o700)
        (journal / "critical").mkdir(mode=0o700)
        plain, tls, websocket = journey.available_ports()
        env = {"PATH": os.environ.get("PATH", "/usr/bin:/bin"), "ENVIRONMENT": "local",
               "PERSISTENCE_MODE": "flatfile-primary", "FLATFILE_STATE_DIR": str(state),
               "PLAYER_SAVE_JOURNAL_DIR": str(journal / "players"),
               "CRITICAL_COMMAND_JOURNAL_DIR": str(journal / "critical"),
               "LISTEN_ADDRESS": "127.0.0.1", "DURIS_TLS_PORT": str(tls),
               "DURIS_WEBSOCKET_LISTEN_ADDRESS": "127.0.0.1", "DURIS_WEBSOCKET_PORT": str(websocket),
               "REDIS": "FALSE", "CHAOS_MUD": "FALSE"}
        if "LD_LIBRARY_PATH" in os.environ: env["LD_LIBRARY_PATH"] = os.environ["LD_LIBRARY_PATH"]
        output_path = root / "server.out"
        with output_path.open("w") as output:
            process = subprocess.Popen([str(binary), "--minimal", "-s", "-d", str(root), str(plain)],
                                       cwd=root, env=env, stdout=output, stderr=subprocess.STDOUT)
            client = None
            try:
                deadline = time.monotonic() + 60
                while "Entering game loop." not in output_path.read_text(errors="replace"):
                    assert process.poll() is None and time.monotonic() < deadline, output_path.read_text()[-6000:]
                    time.sleep(.1)
                client = journey.MudClient(plain)
                journey.create_character(client)
                client.send("toggle boon"); client.expect("You will no longer be affected by boons.")
                client.send("drop all"); client.expect("You drop a steel long sword.", timeout=20)
                client.send(f"get {kind}"); client.expect(f"You get the regression {kind}")
                if kind != "scroll":
                    client.send(f"hold {kind}"); client.expect(f"You hold the regression {kind}")
                weapon.drain(client, .3)
                command = f"{'recite' if kind == 'scroll' else 'use'} {kind} sentinel"
                started = time.monotonic(); client.send(command)
                begin = "You begin reciting" if kind == "scroll" else "You begin channeling"
                first, text = client.expect_any((begin, HIT), timeout=8)
                if not enabled:
                    assert first == HIT and begin not in text
                    result = {"case": case, "command_to_damage_s": round(time.monotonic()-started, 3)}
                else:
                    assert first == begin and HIT not in text
                    warned = time.monotonic()
                    # Ordinary typeahead must wait. Abort remains selectable past it.
                    client.send("say channelqueueprobe")
                    if abort:
                        client.send("abort")
                        stopped = client.expect("You stop using the item.", timeout=3)
                        elapsed = time.monotonic()-warned
                        trailing = weapon.drain(client, 2.5)
                        assert HIT not in stopped + trailing
                        assert "fades" in stopped + trailing and "channelqueueprobe" in trailing
                        result = {"case": case, "warning_to_abort_s": round(elapsed, 3), "damage": False}
                    else:
                        complete = client.expect(HIT, timeout=8)
                        elapsed = time.monotonic()-warned
                        assert elapsed >= 1.65 and "channelqueueprobe" not in complete, complete
                        assert ("final words" if kind == "scroll" else "intensifies") in complete
                        client.expect("channelqueueprobe", timeout=4)
                        result = {"case": case, "warning_to_damage_s": round(elapsed, 3)}
                # The last charge/scroll is gone even after an abort. Send through
                # real command input after any unchanged legacy recovery wait.
                weapon.drain(client, 2.5)
                # Recite is prohibited by the ordinary command table in combat;
                # inspect real inventory to verify scroll destruction instead.
                client.send("inventory" if kind == "scroll" else command)
                expected = ("You are carrying: (0/" if kind == "scroll" else
                            "There are no charges available to channel." if enabled else
                            "There are no more charges in the wand!" if kind == "wand" else
                            "The staff is completely burnt out!")
                again = client.expect(expected, timeout=10)
                assert begin not in again and HIT not in again
                result["resource_stays_consumed"] = True
                print(json.dumps(result), flush=True)
                return result
            except Exception:
                if client: print(bytes(client.transcript).decode(errors="replace")[-10000:], flush=True)
                print(journey.runtime_logs(root), flush=True)
                raise
            finally:
                if client: client.close()
                if process.poll() is None:
                    process.send_signal(signal.SIGTERM)
                    try: process.wait(timeout=30)
                    except subprocess.TimeoutExpired: process.kill(); process.wait(timeout=5)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--inspector", type=Path, default=journey.INSPECTOR)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    results = {"binary_sha256": hashlib.sha256(args.binary.read_bytes()).hexdigest(),
               "cases": [run_case(args.binary.resolve(), args.inspector.resolve(), kind, enabled, abort)
                         for kind in ("wand", "staff", "scroll")
                         for enabled, abort in ((False, False), (True, False), (True, True))]}
    if args.output: args.output.write_text(json.dumps(results, indent=2)+"\n")
    print(json.dumps(results, indent=2))
