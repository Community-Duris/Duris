#!/usr/bin/env python3
"""Matched real-server sparse-world capture using disposable flatfile state.

Actual movement and NPC decisions run normally. Report measured CPU, command
round-trip quantiles and the existing loop/callback windows, without inferring
whole-world savings or loop quantiles from aggregate mean/max summaries.
"""
from pathlib import Path
from datetime import datetime
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time

import test_flatfile_combat_journey as journey
from run_generated_npc_journey import drain


def cpu(path):
    fields = path.read_text().rsplit(") ", 1)[1].split()
    return (int(fields[11]) + int(fields[12])) / os.sysconf("SC_CLK_TCK")


def run(binary, output, population=1000, seconds=180, smoke=False):
    with tempfile.TemporaryDirectory(prefix="activity-server-journey-") as temporary:
        root = Path(temporary); state = root / "state"; runtime = root / "runtime"
        state.mkdir(mode=0o700); (state / "domains").mkdir(mode=0o700); runtime.mkdir()
        journey.make_fixture(runtime); journey.generate_certificate(runtime)
        for name in ("players", "critical"):
            (runtime / "journals" / name).mkdir(parents=True, mode=0o700)
        (runtime / "logs/log").mkdir(parents=True)
        (runtime / "bin/server").mkdir(parents=True)
        for name in ("dms", "dms_new"): shutil.copy2(binary, runtime / "bin/server" / name)
        subprocess.run([str(journey.INSPECTOR), str(state), "seed-combat"], check=True)
        path = runtime / "areas_mini/mini.mob"
        mob = "#22801\nactivitypredator _ignore_~\nan activity predator~\nAn activity predator explores here.\n~\n~\n8 0 0 0 0 0 0 0 S\nPH 0 0 -1\n20 0 100 1d1+20000 1d1+0\n0.0.0.0 0\n8 8 0\n"
        path.write_text(path.read_text().replace("$~", mob + "$~"))
        path = runtime / "areas_mini/mini.wld"
        world = path.read_text().replace("1 0 0\nS\n$~", "1 0 0\nD0\n~\n~\n0 0 22801\nS\n$~")
        rooms = ""
        for vnum in range(22801, 23061):
            rooms += f"#{vnum}\nThe Activity Corridor {vnum}~\nA connected activity test corridor.\n~\n1 0 0\n"
            if vnum < 23060: rooms += f"D0\n~\n~\n0 0 {vnum+1}\n"
            rooms += f"D2\n~\n~\n0 0 {vnum-1}\nS\n"
        path.write_text(world.replace("$~", rooms + "$~"))
        path = runtime / "areas_mini/mini.zon"
        resets = (f"M 0 22801 {population} 23000 100 0 0 0\n" * population)
        path.write_text(path.read_text().replace("\nS\n", "\n" + resets + "S\n"))
        properties = runtime / "lib/duris.properties"
        properties.write_text(properties.read_text()
                             .replace("world.activity.diagnostics=0.000", "world.activity.diagnostics=1.000")
                             .replace("world.activity.enabled=0.000", "world.activity.enabled=1.000"))
        port, tls, ws = journey.available_ports()
        env = dict(PATH=os.environ.get("PATH", "/usr/bin:/bin"), ENVIRONMENT="local",
                   PERSISTENCE_MODE="flatfile-primary", FLATFILE_STATE_DIR=str(state),
                   PLAYER_SAVE_JOURNAL_DIR=str(runtime / "journals/players"),
                   CRITICAL_COMMAND_JOURNAL_DIR=str(runtime / "journals/critical"), REDIS="FALSE", CHAOS_MUD="FALSE",
                   LISTEN_ADDRESS="127.0.0.1", DURIS_TLS_PORT=str(tls), DURIS_WEBSOCKET_PORT=str(ws),
                   DURIS_WEBSOCKET_LISTEN_ADDRESS="127.0.0.1", DURIS_NEVENT_ANALYTICS="1")
        process = stream = client = None

        def stop():
            nonlocal client, stream, process
            if client: client.close(); client = None
            if process and process.poll() is None: process.terminate(); process.wait(timeout=30)
            if stream: stream.close()

        def boot():
            nonlocal process, stream
            stream = (runtime / "server.out").open("w")
            process = subprocess.Popen([str(runtime / "bin/server/dms"), "--minimal", "-s", str(port)],
                                       cwd=runtime, env=env, stdout=stream, stderr=subprocess.STDOUT)
            deadline = time.monotonic() + 90
            while "Entering game loop." not in (runtime / "server.out").read_text(errors="replace"):
                assert process.poll() is None and time.monotonic() < deadline, "boot failed"
                time.sleep(.1)

        results = []
        try:
            boot(); client = journey.MudClient(port); journey.create_character(client)
            client.send("save"); client.expect("Save complete for Taverek.")
            client.send("quit"); client.expect("ACCOUNT MENU", timeout=30); stop()
            journey.make_overlord(state, "Taverek")
            boot(); client = journey.reconnect_character(port); drain(client)
            status = journey.runtime_logs(runtime)
            indexed = re.findall(r"WORLD ACTIVITY: enabled=1 ready=1 indexed_npcs=(\d+)", status)
            assert indexed and int(indexed[-1]) >= population, indexed
            for enabled in (0, 1):
                client.send(f"properties set world.activity.enabled {enabled}"); drain(client, 1)
                client.send("properties show world.activity.enabled")
                text = client.expect("world.activity.enabled") + drain(client, .5)
                assert f"{enabled}.000" in text, text
                if smoke:
                    drain(client, 24)
                    results.append(dict(enabled=enabled, population=population, mode="smoke"))
                    Path(output).write_text(json.dumps(results, indent=2) + "\n")
                    continue
                # Let the previous cadence and warm-up analytics window drain.
                deadline = time.monotonic() + 80
                while time.monotonic() < deadline: drain(client, 1)
                before = cpu(Path(f"/proc/{process.pid}/stat"))
                thread_before = cpu(Path(f"/proc/{process.pid}/task/{process.pid}/stat"))
                started_us = time.monotonic_ns() // 1000
                started_wall = time.time()
                started = time.monotonic(); samples = []
                while time.monotonic() - started < seconds:
                    drain(client, .05)
                    sent = time.monotonic(); client.send("look"); client.expect("The Regression Arena")
                    samples.append((time.monotonic() - sent) * 1000)
                    drain(client, .7)
                finished_us = time.monotonic_ns() // 1000
                finished_wall = time.time()
                samples.sort()
                result = dict(enabled=enabled, population=population, measured_seconds=time.monotonic()-started,
                              process_cpu_seconds=cpu(Path(f"/proc/{process.pid}/stat"))-before,
                              game_thread_cpu_seconds=cpu(Path(f"/proc/{process.pid}/task/{process.pid}/stat"))-thread_before,
                              command_samples=len(samples), command_p95_ms=samples[int(len(samples)*.95)],
                              command_p99_ms=samples[int(len(samples)*.99)])
                latency = (runtime / "logs/latency_trace.log").read_text()
                rows = []
                for block in latency.split("===== LATENCY TRACE SUMMARY =====")[1:]:
                    bounds = re.search(r"window_start_mono_us=(\d+) window_end_mono_us=(\d+)", block)
                    if bounds and started_us <= int(bounds[1]) and int(bounds[2]) <= finished_us:
                        row = re.search(r"(?m)^total_tick\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)", block)
                        if row: rows.append(tuple(map(int, row.groups())))
                assert rows, "no complete steady-state loop window"
                result.update(loop_windows=len(rows), loop_mean_us=sum(r[2]*r[3] for r in rows)/sum(r[3] for r in rows),
                              loop_max_us=max(r[1] for r in rows))
                status = (runtime / "logs/log/status").read_text(errors="replace")
                callbacks = {int(m[1]): tuple(map(int, m.groups()[1:])) for m in re.finditer(
                    r"NEVENT ANALYTICS CALLBACK: window_start_tick=(\d+).*name=event_mob_mundane calls=(\d+) total_us=(\d+).*deferred=(\d+)", status)}
                matched = []
                for line in status.splitlines():
                    window = re.search(r"NEVENT ANALYTICS WINDOW: start_tick=(\d+) end_tick=(\d+)", line)
                    if not window: continue
                    end = datetime.strptime(line.split("::", 1)[0], "%a %b %d %H:%M:%S %Y").timestamp()
                    duration = (int(window[2]) - int(window[1])) / 4
                    if started_wall + 1 <= end - duration and end <= finished_wall - 1:
                        matched.append((duration, callbacks.get(int(window[1]), (0, 0, 0))))
                assert matched and sum(item[1][0] for item in matched) >= population, "no repeated mundane callbacks in steady state"
                result.update(callback_windows=len(matched), callback_seconds=sum(item[0] for item in matched),
                              mundane_calls=sum(item[1][0] for item in matched),
                              mundane_total_us=sum(item[1][1] for item in matched),
                              mundane_deferred=sum(item[1][2] for item in matched))
                result["mundane_calls_per_second"] = result["mundane_calls"] / result["callback_seconds"]
                print(json.dumps(result), flush=True); results.append(result)
                Path(output).write_text(json.dumps(results, indent=2) + "\n")
            # Real recovery rebuild and repeated live reload retain the indexed population.
            client.send("save"); client.expect("Save complete for Taverek.")
            client.send("shutdown copyover"); client.expect("Copyover complete!", timeout=90)
            drain(client, 2); client.send("properties set world.activity.enabled 0"); drain(client, 1)
            client.send("properties set world.activity.enabled 1"); drain(client, 1)
            status = journey.runtime_logs(runtime)
            matches = re.findall(r"WORLD ACTIVITY: enabled=1 ready=1 indexed_npcs=(\d+) players=(\d+) corpses=(\d+)", status)
            assert matches and int(matches[-1][0]) >= population and int(matches[-1][1]) == 1, matches
            mode = "population/reload smoke" if smoke else "matched sparse capture"
            print(f"real world activity server: {mode}, normal wandering callbacks, live disable/enable and copyover rebuild passed", flush=True)
        except Exception:
            print((runtime / "server.out").read_text(errors="replace")[-5000:])
            print(journey.runtime_logs(runtime)); raise
        finally: stop()


if __name__ == "__main__":
    run(Path(sys.argv[1]).resolve(), sys.argv[2], smoke="--smoke" in sys.argv[3:])
