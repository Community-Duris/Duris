#!/usr/bin/env python3
"""Real command, live-toggle and save/reload journey in disposable flatfile state.

The focused runtime test supplies exact XP and purchase comparisons; this journey
adds the actual login/score/command/persistence paths, including community newbsa.
"""
from pathlib import Path
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time

import test_flatfile_combat_journey as journey
from run_generated_npc_journey import drain

STAFF_GRANT_FLAG = 16  # AFFTYPE_CUSTOM1 == BIT_5 (bits are numbered from one).


INSPECT = r'''
#include "flatfile/flatfile_player_snapshot_file.h"
#include <cstdio>
int main(int argc, char **argv) {
    if (argc != 2) return 2;
    player_snapshot snapshot; std::string error;
    if (flatfile_player_snapshot_read(argv[1], 2, &snapshot, &error) != flatfile_player_load_result::ok)
        return 1;
    for (const auto &affect : snapshot.affects)
        if (affect.type == 2107 || affect.type == 2108)
            std::printf("%d %u %d\n", affect.type, affect.flags, affect.duration);
}
'''


def run(binary: Path):
    with tempfile.TemporaryDirectory(prefix="rested-server-journey-") as temporary:
        root = Path(temporary)
        runtime, state = root / "runtime", root / "state"
        runtime.mkdir(); state.mkdir(mode=0o700); (state / "domains").mkdir(mode=0o700)
        journey.make_fixture(runtime); journey.generate_certificate(runtime)
        properties = runtime / "lib/duris.properties"
        properties.write_text(properties.read_text().replace("exp.rested.enabled=1.000", "exp.rested.enabled=0.000"))
        text = properties.read_text().replace("exp.factor.global=0.500", "exp.factor.global=10.000")
        text = re.sub(r"(?m)^exp.required.\d+=.*$", lambda m: m.group().split("=")[0] + "=1000000.000", text)
        properties.write_text(text)
        # A stationary, harmless target keeps the melee XP input fixed across
        # toggles and grants. Large thresholds avoid an intervening level change.
        path = runtime / "areas_mini/mini.mob"
        dummy = "#11\nraoul _ignore_~\nRaoul the training dummy~\nRaoul stands patiently here.\n~\n~\n10 0 0 0 0 0 0 0 S\nPH 0 0 -1\n1 0 100 1d1+20000 1d1+0\n0.0.0.0 0\n8 8 0\n"
        path.write_text(re.sub(r"(?ms)^#11\n.*?(?=^#12\n)", lambda _: dummy, path.read_text()))
        path = runtime / "areas_mini/mini.zon"
        path.write_text(re.sub(r"(?m)^M 0 22800 .*\n", "", path.read_text()))
        for name in ("players", "critical"):
            (runtime / "journals" / name).mkdir(parents=True, mode=0o700)
        (runtime / "logs/log").mkdir(parents=True)
        (runtime / "bin/server").mkdir(parents=True)
        for name in ("dms", "dms_new"):
            shutil.copy2(binary, runtime / "bin/server" / name)
        subprocess.run([str(journey.INSPECTOR), str(state), "seed-combat"], check=True)
        cpp, inspector = root / "inspect.cpp", root / "inspect"
        cpp.write_text(INSPECT)
        subprocess.run(["g++", "-std=c++20", "-Isrc", str(cpp), "src/player/player_snapshot_codec.c",
                        "src/flatfile/flatfile_player_snapshot_file.c", "src/flatfile/flatfile_store.c",
                        "-lcrypto", "-o", str(inspector)], cwd=journey.ROOT, check=True)
        port, tls, ws = journey.available_ports()
        env = dict(PATH=os.environ.get("PATH", "/usr/bin:/bin"), ENVIRONMENT="local",
                   PERSISTENCE_MODE="flatfile-primary", FLATFILE_STATE_DIR=str(state),
                   PLAYER_SAVE_JOURNAL_DIR=str(runtime / "journals/players"),
                   CRITICAL_COMMAND_JOURNAL_DIR=str(runtime / "journals/critical"),
                   REDIS="FALSE", CHAOS_MUD="FALSE", LISTEN_ADDRESS="127.0.0.1",
                   DURIS_TLS_PORT=str(tls), DURIS_WEBSOCKET_PORT=str(ws),
                   DURIS_WEBSOCKET_LISTEN_ADDRESS="127.0.0.1")
        process = output = staff = player = None

        def stop():
            nonlocal process, output, staff, player
            for client in (staff, player):
                if client: client.close()
            staff = player = None
            if process and process.poll() is None:
                process.terminate(); process.wait(timeout=30)
            if output: output.close()

        def boot():
            nonlocal process, output
            output = (runtime / "server.out").open("w")
            process = subprocess.Popen([str(runtime / "bin/server/dms"), "--minimal", "-s", str(port)],
                                       cwd=runtime, env=env, stdout=output, stderr=subprocess.STDOUT)
            deadline = time.monotonic() + 90
            while "Entering game loop." not in (runtime / "server.out").read_text(errors="replace"):
                assert process.poll() is None and time.monotonic() < deadline, "boot failed"
                time.sleep(.1)

        def reconnect_player():
            return journey.reconnect_character(port, account="Restedacct", character="Restling")

        def score():
            drain(player); player.send("score"); text = drain(player, 1)
            if "[Return to continue" in text:
                player.send("q"); drain(player)
            return text

        def save():
            player.send("save"); player.expect("Save complete for Restling.")
            rows = subprocess.check_output([str(inspector), str(state)], text=True).splitlines()
            return [tuple(map(int, row.split())) for row in rows]

        def toggle(value):
            drain(staff); staff.send(f"properties set exp.rested.enabled {value}")
            drain(staff, 1)
            staff.send("properties show exp.rested.enabled")
            text = staff.expect("exp.rested.enabled") + drain(staff, .5)
            assert f"{value}.000" in text, text

        def melee_xp():
            drain(staff)
            deadline = time.monotonic() + 45
            text = ""
            while time.monotonic() < deadline:
                text += drain(staff, .3)
                match = re.search(r"Restling gained (\d+) \(7\) experience.*?curr_exp = (\d+)", text)
                if match:
                    return int(match[1])
            raise AssertionError("no real melee XP award: " + text)

        try:
            boot(); staff = journey.MudClient(port); journey.create_character(staff)
            staff.send("save"); staff.expect("Save complete for Taverek.")
            staff.send("quit"); staff.expect("ACCOUNT MENU", timeout=30); stop()
            journey.make_overlord(state, "Taverek")
            boot(); staff = journey.reconnect_character(port); drain(staff)
            player = journey.MudClient(port)
            journey.create_character(player, account="Restedacct", character="Restling")
            assert not save(), "disabled new-character login granted a bonus"
            assert "rested" not in score().lower()
            player.send("quit"); menu = player.expect("Please select an option", timeout=30)
            assert "Check rested bonus" not in menu
            player.send("8"); text = player.expect("Please select an option")
            assert "disabled" in text.lower(), text
            player.close(); player = reconnect_player(); drain(player)
            staff.send("toggle exp"); staff.expect("Experience Display is now ON")
            staff.send("force raoul hit Restling"); drain(staff)
            print("combat start: " + drain(player, 1), flush=True)
            baseline = melee_xp()
            print(f"baseline melee XP={baseline}", flush=True)
            # Both current staff routes must use persisted override provenance.
            staff.send("newbsu Restling"); staff.expect("Done."); drain(player)
            rows = save(); assert len(rows) == 1 and rows[0][0] == 2107 and rows[0][1] & STAFF_GRANT_FLAG, rows
            assert "rested" in score().lower()
            rested = melee_xp()
            print(f"rested melee XP={rested}", flush=True)
            assert baseline > 0 and rested > baseline and abs(rested - baseline * 1.5) <= 2, (baseline, rested)
            staff.send("newbsa"); text = drain(staff, 2); drain(player)
            assert "rest" in text.lower(), text
            wellrested = melee_xp()
            assert wellrested > rested and abs(wellrested - baseline * 2) <= 2, (baseline, rested, wellrested)
            print(f"real server fixed-input melee XP: ordinary off={baseline}, staff rested={rested}, staff well-rested={wellrested}", flush=True)
            staff.send("purge raoul"); drain(staff); drain(player)
            rows = save(); assert len(rows) == 1 and rows[0][0] == 2108 and rows[0][1] & STAFF_GRANT_FLAG, rows
            player.send("quit"); player.expect("ACCOUNT MENU", timeout=30); stop()
            boot(); staff = journey.reconnect_character(port); drain(staff)
            player = reconnect_player(); drain(player)
            assert "well" in score().lower()
            rows = save(); assert len(rows) == 1 and rows[0][0] == 2108 and rows[0][1] & STAFF_GRANT_FLAG, rows
            toggle(1); assert "rested" in score().lower()
            toggle(0); assert "rested" in score().lower(), "staff override became dormant"
            # Actual copyover also preserves the flag while the live gate is off.
            save(); staff.send("shutdown copyover"); staff.expect("Copyover complete!", timeout=90)
            drain(player, 2); assert "rested" in score().lower()
            rows = save(); assert rows[0][1] & STAFF_GRANT_FLAG, rows
            print("rested real server: disabled login/menu, newbsu/rested, community newbsa/well-rested, "
                  "live toggles, marked save, cold reload and copyover passed", flush=True)
        except Exception:
            print((runtime / "server.out").read_text(errors="replace")[-4000:])
            print(journey.runtime_logs(runtime))
            raise
        finally:
            stop()


if __name__ == "__main__":
    run(Path(sys.argv[1]).resolve())
