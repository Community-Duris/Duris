#!/usr/bin/env python3
"""Disposable real-server A/B for an offensive weapon and a reacting player.

The only forced selection is a synthetic packed weapon's normal value[7]=1.
Account creation, NPC attacks, spell damage, warning output, pulses and flee all
use production code. No gameplay endpoints are replaced or debug commands added.
Run explicitly with a freshly built flatfile server and repository inspector.
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

ROOT = Path(__file__).resolve().parents[2]
START = "begins gathering magic toward YOU!"
HIT = "magic missile from"


def configure(run_root: Path, enabled: bool) -> None:
    (run_root / "logs/log").mkdir(parents=True)
    journey.make_fixture(run_root)
    props = run_root / "lib/duris.properties"
    text = props.read_text()
    for key, value in {"itemActions.enabled": int(enabled), "itemActions.weapons.enabled": 1,
                       "itemActions.weapons.windupPulses": 8,
                       "hitpoints.mob.NpcPcRatio": 100, "hitpoints.class.Warrior": 12}.items():
        text, count = re.subn(rf"(?m)^{re.escape(key)}=.*$", f"{key}={value}", text)
        assert count == 1, key
    props.write_text(text)
    mini = run_root / "areas_mini"
    objects = (mini / "mini.obj").read_text()
    # Undo the borrowed death-journey's 100-damage starter mace. Mob conversion
    # derives HP from properties, not the raw dice below; keep both combatants
    # alive long enough to observe a proc even after misses/fumbles.
    objects = objects.replace("6 100 1 7 0 0 0 0", "6 1 6 7 0 0 0 0")
    weapon = """#22801
regression wandblade~
the regression wandblade~
A harmless-looking regression wandblade lies here.~
~
5 6 3 8 7 0 0 8193 0 0 0
6 1 1 7 0 32 1 1
2 0 100
"""
    (mini / "mini.obj").write_text(objects.replace("$~", weapon + "$~"))
    sentinel = """#22801
regression sentinel~
the regression sentinel~
The regression sentinel waits here with a wandblade.
~
~
10 0 0 0 0 0 0 0 S
PH 0 0 -1
20 -100 100 1d1+50000 1d1+0
0.0.0.0 0
8 8 0
"""
    mobiles = (mini / "mini.mob").read_text()
    (mini / "mini.mob").write_text(mobiles.replace("$~", sentinel + "$~"))
    world = (mini / "mini.wld").read_text()
    world = world.replace("1 0 0\nS\n$~", "1 0 0\nD0\n~\n~\n0 0 22801\nS\n$~")
    assert "0 0 22801" in world
    refuge = """#22801
The Regression Refuge~
A quiet refuge beyond the arena's northern exit.
~
1 0 0
D2
~
~
0 0 22800
S
"""
    (mini / "mini.wld").write_text(world.replace("$~", refuge + "$~"))
    zone = (mini / "mini.zon").read_text()
    zone = re.sub(r"^[MGE] .*\n", "", zone, flags=re.M)
    zone = zone.replace("\nS\n", "\nM 0 22801 1 22800 100 0 0 0 * sentinel\n"
                        "E 1 22801 1 16 100 0 0 0 * wandblade\nS\n")
    (mini / "mini.zon").write_text(zone)
    journey.generate_certificate(run_root)


def drain(client: journey.MudClient, seconds: float) -> str:
    deadline = time.monotonic() + seconds
    while time.monotonic() < deadline:
        client._receive()
    result = bytes(client.pending).decode(errors="replace")
    client.pending.clear()
    return result


def run_case(binary: Path, inspector: Path, enabled: bool, react: bool) -> dict:
    case = f"{'on' if enabled else 'off'}-{'flee' if react else 'hold'}"
    print(f"weapon journey: {case}", flush=True)
    with tempfile.TemporaryDirectory(prefix="duris-weapon-state-") as state_tmp, \
         tempfile.TemporaryDirectory(prefix="duris-weapon-run-") as run_tmp:
        state = Path(state_tmp); state.chmod(0o700)
        run_root = Path(run_tmp)
        (state / "domains").mkdir(mode=0o700)
        subprocess.run([str(inspector), str(state), "seed-combat"], check=True)
        configure(run_root, enabled)
        journal = run_root / "journals"
        (journal / "players").mkdir(parents=True, mode=0o700)
        (journal / "critical").mkdir(mode=0o700)
        plain, tls, websocket = journey.available_ports()
        env = {
            "PATH": os.environ.get("PATH", "/usr/bin:/bin"), "ENVIRONMENT": "local",
            "PERSISTENCE_MODE": "flatfile-primary", "FLATFILE_STATE_DIR": str(state),
            "PLAYER_SAVE_JOURNAL_DIR": str(journal / "players"),
            "CRITICAL_COMMAND_JOURNAL_DIR": str(journal / "critical"),
            "LISTEN_ADDRESS": "127.0.0.1", "DURIS_TLS_PORT": str(tls),
            "DURIS_WEBSOCKET_LISTEN_ADDRESS": "127.0.0.1", "DURIS_WEBSOCKET_PORT": str(websocket),
            "REDIS": "FALSE", "CHAOS_MUD": "FALSE", "DURIS_NEVENT_TRACE_PLAYER": "1",
        }
        if "LD_LIBRARY_PATH" in os.environ:
            env["LD_LIBRARY_PATH"] = os.environ["LD_LIBRARY_PATH"]
        output_path = run_root / "server.out"
        with output_path.open("w") as output:
            process = subprocess.Popen([str(binary), "--minimal", "-s", "-d", str(run_root), str(plain)],
                                       cwd=run_root, env=env, stdout=output, stderr=subprocess.STDOUT)
            client = None
            try:
                deadline = time.monotonic() + 60
                while "Entering game loop." not in output_path.read_text(errors="replace"):
                    assert process.poll() is None and time.monotonic() < deadline, output_path.read_text()[-6000:]
                    time.sleep(.1)
                client = journey.MudClient(plain)
                journey.create_character(client)
                client.send("toggle boon")
                client.expect("You will no longer be affected by boons.")
                client.send("wield mace")
                client.expect("You wield")
                drain(client, .3)
                attacked = time.monotonic()
                client.send("kill sentinel")
                first, output_text = client.expect_any((START, HIT, "is dead! R.I.P."), timeout=45)
                assert first != "is dead! R.I.P.", output_text
                if not enabled:
                    assert first == HIT and START not in output_text
                    result = {"case": case, "legacy_damage_after_attack_s": round(time.monotonic() - attacked, 3)}
                else:
                    assert first == START and HIT not in output_text
                    warned = time.monotonic()
                    if react:
                        client.send("flee")
                        for attempt in range(8):
                            outcome, escaped = client.expect_any(("The Regression Refuge", "PANIC!", HIT), timeout=8)
                            assert outcome != HIT, escaped
                            if outcome == "The Regression Refuge":
                                break
                            client.send("flee")
                        assert outcome == "The Regression Refuge", escaped
                        escaped_at = time.monotonic()
                        trailing = drain(client, 3)
                        assert HIT not in escaped + trailing, escaped + trailing
                        assert "fades" in escaped + trailing, escaped + trailing
                        result = {"case": case, "reaction_to_room_exit_s": round(escaped_at - warned, 3),
                                  "damage_after_warning": False, "fizzle_observed": True}
                    else:
                        complete = client.expect(HIT, timeout=8)
                        elapsed = time.monotonic() - warned
                        assert elapsed >= 1.65, (elapsed, complete)
                        assert "intensifies" in complete and "releases its gathered magic" in complete
                        result = {"case": case, "warning_to_damage_s": round(elapsed, 3)}
                print(json.dumps(result), flush=True)
                return result
            except Exception:
                if client:
                    print(bytes(client.transcript).decode(errors="replace")[-12000:], flush=True)
                print(journey.runtime_logs(run_root), flush=True)
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
               "cases": [run_case(args.binary.resolve(), args.inspector.resolve(), enabled, react)
                         for enabled, react in ((False, False), (True, False), (True, True))]}
    if args.output:
        args.output.write_text(json.dumps(results, indent=2) + "\n")
    print(json.dumps(results, indent=2))
