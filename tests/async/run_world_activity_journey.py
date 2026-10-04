#!/usr/bin/env python3
"""Matched real-server activity captures using disposable flatfile state.

Actual movement and NPC decisions run normally. Report measured CPU, command
round-trip quantiles and the existing loop/callback windows, without inferring
production savings. Complete-window loop quantiles come from actual samples.
"""
from pathlib import Path
from datetime import datetime
import argparse
import json
import os
import re
import shutil
import subprocess
import tempfile
import time

import test_flatfile_combat_journey as journey
from run_generated_npc_journey import drain


def cpu(path):
    fields = path.read_text().rsplit(") ", 1)[1].split()
    return (int(fields[11]) + int(fields[12])) / os.sysconf("SC_CLK_TCK")


def append_records(path, records):
    content = path.read_text()
    # Combined generators emit a numeric sentinel before $~. Placing records
    # after it silently omits them from the full-world readers.
    marker = "#9999999\n$~" if "#9999999\n$~" in content else "$~"
    assert content.count(marker) == 1, f"missing/ambiguous terminator in {path}"
    path.write_text(content.replace(marker, records + marker, 1))


def run(binary, output, population=1000, seconds=180, smoke=False, runtime_index=False,
        scenario="sparse", full_world=False):
    with tempfile.TemporaryDirectory(prefix="activity-server-journey-") as temporary:
        root = Path(temporary); state = root / "state"; runtime = root / "runtime"
        state.mkdir(mode=0o700); (state / "domains").mkdir(mode=0o700); runtime.mkdir()
        journey.make_fixture(runtime); journey.generate_certificate(runtime)
        for name in ("players", "critical"):
            (runtime / "journals" / name).mkdir(parents=True, mode=0o700)
        (runtime / "logs/log").mkdir(parents=True)
        (runtime / "bin/server").mkdir(parents=True)
        for name in ("dms", "dms_new"): shutil.copy2(binary, runtime / "bin/server" / name)
        if not full_world:
            subprocess.run([str(journey.INSPECTOR), str(state), "seed-combat"], check=True)
        home = 9900000 if full_world else 22800
        mob_vnum = 9900100 if full_world else 22801
        data = runtime / "areas_mini"
        stem, zone = "mini", 1
        if full_world:
            # Copy only combined data for this fixture; retain the tracked
            # world's rooms, populations, scripts, ferries and reset behavior.
            data = runtime / "areas"
            data.unlink(); data.mkdir()
            for source in (journey.ROOT / "areas").iterdir():
                if source.name.startswith("world.") and source.is_file():
                    shutil.copy2(source, data / source.name)
                else:
                    (data / source.name).symlink_to(source, target_is_directory=source.is_dir())
            stem, zone = "world", 99000
        path = data / f"{stem}.mob"
        mob = f"#{mob_vnum}\nactivitypredator _ignore_~\nan activity predator~\nAn activity predator explores here.\n~\n~\n8 0 0 0 0 0 0 0 S\nPH 0 0 -1\n20 0 100 1d1+20000 1d1+0\n0.0.0.0 0\n8 8 0\n"
        if scenario == "busy":
            for i in range(40):
                mob += f"#{mob_vnum+i+1}\nactivityfighter{i} _ignore_~\nan activity fighter~\nAn activity fighter stands here.\n~\n~\n10 0 0 0 0 0 0 0 S\nPH 0 0 -1\n20 0 100 1d1+20000 1d1+0\n0.0.0.0 0\n8 8 0\n"
        append_records(path, mob)
        path = data / f"{stem}.wld"
        world = path.read_text().replace("1 0 0\nS\n$~", "1 0 0\nD0\n~\n~\n0 0 22801\nS\n$~")
        rooms = f"#{home}\nThe Regression Arena~\nAn isolated activity workload arena.\n~\n{zone} 0 0\nD0\n~\n~\n0 0 {home+1}\nS\n" if full_world else ""
        for vnum in range(home+1, home+261):
            rooms += f"#{vnum}\nThe Activity Corridor {vnum}~\nA connected activity test corridor.\n~\n{zone} 0 0\n"
            if vnum < home+260: rooms += f"D0\n~\n~\n0 0 {vnum+1}\n"
            rooms += f"D2\n~\n~\n0 0 {vnum-1}\nS\n"
        path.write_text(world)
        append_records(path, rooms)
        path = data / f"{stem}.zon"
        spawn_room = home if scenario == "busy" else home+200
        resets = f"M 0 {mob_vnum} {population} {spawn_room} 100 0 0 0\n" * population
        if scenario == "busy":
            resets += "".join(f"M 0 {mob_vnum+i+1} 1 {home} 100 0 0 0\n" for i in range(40))
        if scenario == "corpse":
            # Explicitly synthetic PC-flagged corpse population for load cost;
            # real death/loot/publication is qualified by the combat journey.
            path_obj = data / f"{stem}.obj"
            # ITEM_TAKE permits repeated ordinary O resets in the same room;
            # a fixed object with the same prototype would be loaded only once.
            corpse = f"#{mob_vnum+100}\nactivitycorpse corpse~\na player corpse fixture~\nA player corpse fixture lies here.\n~\n~\n24 2 3 30 7 0 73728 1 0 0 0\n0 1 0 0 0 0 0 0\n200 0 100\n"
            append_records(path_obj, corpse)
            resets += f"O 0 {mob_vnum+100} 100 {spawn_room} 100 0 0 0\n" * 100
        if full_world:
            new_zone = f"#{zone}\nActivity Qualification~\nactivity qualification~\n{home+260} 0 0 6 11 1\n" + resets + "S\n"
            append_records(path, new_zone)
        else:
            path.write_text(path.read_text().replace("\nS\n", "\n" + resets + "S\n"))
        properties = runtime / "lib/duris.properties"
        properties.write_text(properties.read_text()
                             .replace("world.activity.diagnostics=0.000", "world.activity.diagnostics=1.000"))
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
            arguments = [] if full_world else ["--minimal", "-s"]
            process = subprocess.Popen([str(runtime / "bin/server/dms"), *arguments, str(port)],
                                       cwd=runtime, env=env, stdout=stream, stderr=subprocess.STDOUT)
            deadline = time.monotonic() + (600 if full_world else 90)
            while "Entering game loop." not in (runtime / "server.out").read_text(errors="replace"):
                assert process.poll() is None and time.monotonic() < deadline, "boot failed"
                time.sleep(.1)

        results = []
        def copyover():
            client.send("save"); client.expect("Save complete for Taverek.")
            if full_world:
                # The base branch cannot snapshot cosmetic blood in virtual
                # room zero (the existing recovery writer requires > 0).
                # Use the normal extraction command only in this private fixture.
                # Keep the production serializer and all custody fences intact.
                client.send("goto 0"); drain(client, 1)
                client.send("purge blood"); drain(client, 1)
                client.send(f"goto {home}"); client.expect("The Regression Arena"); drain(client, 1)
            boot_reports = (runtime / "server.out").read_text(errors="replace").count("Entering game loop.")
            began = time.monotonic(); client.send("shutdown copyover")
            outcome, text = client.expect_any(("Copyover complete!", "Copyover FAILED"),
                                              timeout=180 if full_world else 90)
            assert outcome == "Copyover complete!", text
            # Full-world descriptor restoration precedes final boot work. Wait
            # until commands can actually run before validating or measuring.
            deadline = time.monotonic() + (600 if full_world else 90)
            while (runtime / "server.out").read_text(errors="replace").count("Entering game loop.") <= boot_reports:
                assert process.poll() is None and time.monotonic() < deadline, "copyover did not enter the game loop"
                time.sleep(.1)
            return time.monotonic() - began

        def check_runtime_index(stage):
            if not runtime_index:
                return
            client.send("toggle debug"); drain(client, .5)
            client.send("world debug_events once")
            report = client.expect("check_nevents: errors=", timeout=15) + drain(client, .5)
            assert "check_nevents: errors=0 " in report, report
            assert "character runtime index disagrees" not in journey.runtime_logs(runtime)
            client.send("toggle debug"); drain(client, .5)
            print(f"runtime index/list and scheduler consistent after {stage}", flush=True)

        def check_combat(stage):
            if scenario != "busy":
                return
            for i in range(0, 40, 2):
                client.send(f"stat mob activityfighter{i}")
                report = client.expect("Fighting:") + drain(client, .3)
                assert f"activityfighter{i+1} " in report.split("Fighting:", 1)[1], report
            print(f"20 real NPC combat pairs remain engaged after {stage}", flush=True)

        def start_combat():
            if scenario == "busy":
                for i in range(0, 40, 2):
                    client.send(f"force activityfighter{i} kill activityfighter{i+1}"); drain(client, .3)

        def check_population(stage):
            client.send("stat mob activitypredator")
            report = client.expect("# in game:") + drain(client, .5)
            count = re.search(r"# in game:\s*(\d+)", report)
            assert count and int(count[1]) == population, report
            print(f"{population} workload NPC instances verified after {stage}", flush=True)

        try:
            boot(); client = journey.MudClient(port); journey.create_character(client, expected_room=None if full_world else "The Regression Arena")
            client.send("save"); client.expect("Save complete for Taverek.")
            client.send("quit"); client.expect("ACCOUNT MENU", timeout=30); stop()
            journey.make_overlord(state, "Taverek")
            boot(); client = journey.reconnect_character(port, expected_room="Pos: standing >" if full_world else "The Regression Arena"); drain(client)
            client.send("toggle paging"); client.expect("Paging mode off."); drain(client)
            if full_world:
                client.send(f"goto {home}"); client.expect("The Regression Arena"); drain(client)
            start_combat()
            check_population("fixture setup")
            status = (runtime / "logs/log/status").read_text(errors="replace")
            indexed = re.findall(r"WORLD ACTIVITY: enabled=1 ready=1 indexed_npcs=(\d+)", status)
            assert indexed and int(indexed[-1]) >= population, indexed
            if scenario == "corpse":
                counts = re.findall(r"WORLD ACTIVITY: .*corpses=(\d+)", status)
                assert counts and int(counts[-1]) >= 100, counts
            check_runtime_index("boot population and player login")
            check_combat("fixture setup")
            for enabled in (0, 1):
                # Runtime property commands do not persist across copyover.
                # Keep this private fixture's startup value in the measured mode.
                content, replacements = re.subn(r"(?m)^world\.activity\.enabled=.*$",
                                                f"world.activity.enabled={enabled}.000",
                                                properties.read_text())
                assert replacements == 1, "missing/duplicate activity startup property"
                properties.write_text(content)
                client.send(f"properties set world.activity.enabled {enabled}"); drain(client, 1)
                client.send("properties show world.activity.enabled")
                text = client.expect("world.activity.enabled") + drain(client, .5)
                assert f"{enabled}.000" in text, text
                copyover_seconds = None
                if scenario == "recovery":
                    copyover_seconds = copyover()
                    drain(client, 2)
                    client.send("properties show world.activity.enabled")
                    text = client.expect("world.activity.enabled") + drain(client, .5)
                    assert f"{enabled}.000" in text, text
                    check_runtime_index(f"enabled={enabled} pre-capture copyover")
                if scenario == "empty":
                    client.send("quit"); client.expect("ACCOUNT MENU", timeout=30)
                    client.close(); client = None
                if smoke:
                    if client: drain(client, 24)
                    else: time.sleep(24)
                    results.append(dict(enabled=enabled, population=population, scenario=scenario, full_world=full_world, mode="smoke"))
                    Path(output).write_text(json.dumps(results, indent=2) + "\n")
                    if scenario == "empty":
                        client = journey.reconnect_character(port, expected_room="Pos: standing >"); drain(client)
                    continue
                # Let the previous cadence and warm-up analytics window drain.
                print(f"CAPTURE PHASE: scenario={scenario} enabled={enabled} warmup=80 measured={seconds}", flush=True)
                deadline = time.monotonic() + 80
                while time.monotonic() < deadline:
                    if client: drain(client, 1)
                    else: time.sleep(1)
                before = cpu(Path(f"/proc/{process.pid}/stat"))
                thread_before = cpu(Path(f"/proc/{process.pid}/task/{process.pid}/stat"))
                started_us = time.monotonic_ns() // 1000
                started_wall = time.time()
                started = time.monotonic(); samples = []
                while time.monotonic() - started < seconds:
                    if scenario == "empty":
                        time.sleep(.25); continue
                    if scenario in ("travel", "mass-wake"):
                        epoch = len(samples) if scenario == "travel" else len(samples) // 24
                        position = home + (200 if epoch % 2 else 0)
                        if scenario == "travel" or len(samples) % 24 == 0:
                            client.send(f"goto {position}"); drain(client, .3)
                    drain(client, .05)
                    sent = time.monotonic(); marker = f"activity-sample-{len(samples)}"
                    client.send(f"echo {marker}"); client.expect(marker)
                    samples.append((time.monotonic() - sent) * 1000)
                    drain(client, .7)
                finished_us = time.monotonic_ns() // 1000
                finished_wall = time.time()
                samples.sort()
                result = dict(enabled=enabled, population=population, scenario=scenario, full_world=full_world,
                              copyover_seconds=copyover_seconds, measured_seconds=time.monotonic()-started,
                              process_cpu_seconds=cpu(Path(f"/proc/{process.pid}/stat"))-before,
                              game_thread_cpu_seconds=cpu(Path(f"/proc/{process.pid}/task/{process.pid}/stat"))-thread_before,
                              command_samples=len(samples), command_p95_ms=samples[int(len(samples)*.95)] if samples else None,
                              command_p99_ms=samples[int(len(samples)*.99)] if samples else None)
                latency = (runtime / "logs/latency_trace.log").read_text()
                rows, quantiles = [], []
                for block in latency.split("===== LATENCY TRACE SUMMARY =====")[1:]:
                    bounds = re.search(r"window_start_mono_us=(\d+) window_end_mono_us=(\d+)", block)
                    if bounds and started_us <= int(bounds[1]) and int(bounds[2]) <= finished_us:
                        row = re.search(r"(?m)^total_tick\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)", block)
                        if row: rows.append(tuple(map(int, row.groups())))
                        quantile = re.search(r"LOOP QUANTILES: samples=(\d+) p95_us=(\d+) p99_us=(\d+) dropped=0", block)
                        if quantile: quantiles.append(dict(zip(("samples", "p95_us", "p99_us"), map(int, quantile.groups()))))
                assert rows, "no complete steady-state loop window"
                result.update(loop_windows=len(rows), loop_mean_us=sum(r[2]*r[3] for r in rows)/sum(r[3] for r in rows),
                              loop_max_us=max(r[1] for r in rows), loop_quantile_windows=quantiles)
                assert len(quantiles) == len(rows), "missing or incomplete loop quantiles"
                status = (runtime / "logs/log/status").read_text(errors="replace")
                callbacks = {int(m[1]): tuple(map(int, m.groups()[1:])) for m in re.finditer(
                    r"NEVENT ANALYTICS CALLBACK: window_start_tick=(\d+).*name=event_mob_mundane calls=(\d+) total_us=(\d+).*deferred=(\d+)", status)}
                matched, scheduler = [], []
                for line in status.splitlines():
                    window = re.search(r"NEVENT ANALYTICS WINDOW: start_tick=(\d+) end_tick=(\d+)", line)
                    if not window: continue
                    end = datetime.strptime(line.split("::", 1)[0], "%a %b %d %H:%M:%S %Y").timestamp()
                    duration = (int(window[2]) - int(window[1])) / 4
                    if started_wall + 1 <= end - duration and end <= finished_wall - 1:
                        matched.append((duration, callbacks.get(int(window[1]), (0, 0, 0))))
                        scheduler.append({key: float(value) if "." in value else int(value)
                                          for key, value in re.findall(r"(\w+)=(\d+(?:\.\d+)?)", line.split("NEVENT ANALYTICS WINDOW: ", 1)[1])})
                assert matched and sum(item[1][0] for item in matched) >= population, "no repeated mundane callbacks in steady state"
                result.update(callback_windows=len(matched), callback_seconds=sum(item[0] for item in matched),
                              mundane_calls=sum(item[1][0] for item in matched),
                              mundane_total_us=sum(item[1][1] for item in matched),
                              mundane_deferred=sum(item[1][2] for item in matched),
                              scheduler_windows=scheduler)
                health = []
                for line in status.splitlines():
                    if "WORLD ACTIVITY: " not in line:
                        continue
                    recorded = datetime.strptime(line.split("::", 1)[0], "%a %b %d %H:%M:%S %Y").timestamp()
                    if started_wall <= recorded <= finished_wall:
                        health.append(dict((key, int(value)) for key, value in re.findall(
                            r"(\w+)=(\d+)", line.split("WORLD ACTIVITY: ", 1)[1])))
                result["activity_windows"] = health
                assert health and all(row["enabled"] == enabled for row in health), \
                    f"activity diagnostics disagree with measured mode {enabled}: {health}"
                result["mundane_calls_per_second"] = result["mundane_calls"] / result["callback_seconds"]
                print(json.dumps(result), flush=True); results.append(result)
                Path(output).write_text(json.dumps(results, indent=2) + "\n")
                check_combat(f"enabled={enabled} capture")
                if scenario == "empty":
                    client = journey.reconnect_character(port, expected_room="Pos: standing >"); drain(client)
            # Real recovery rebuild and repeated live reload retain the indexed population.
            if scenario in ("travel", "mass-wake"):
                client.send(f"goto {home}"); drain(client, 1)
            copyover()
            drain(client, 2); client.send("properties set world.activity.enabled 0"); drain(client, 1)
            client.send("properties set world.activity.enabled 1"); drain(client, 1)
            status = (runtime / "logs/log/status").read_text(errors="replace")
            matches = re.findall(r"WORLD ACTIVITY: enabled=1 ready=1 indexed_npcs=(\d+) players=(\d+) corpses=(\d+)", status)
            assert matches and int(matches[-1][0]) >= population and int(matches[-1][1]) == 1, matches
            check_population("copyover reconstruction")
            check_runtime_index("copyover reconstruction")
            # Existing copyover links combat only for restored descriptors;
            # independent NPC pairs are idle. Qualify fresh combat after recovery
            # without changing that unrelated persistence behavior.
            start_combat()
            check_combat("fresh post-copyover combat")
            if runtime_index:
                client.send(f"load mob {mob_vnum}"); drain(client, .5)
                check_runtime_index("staff NPC creation")
                client.send("purge activitypredator"); drain(client, .5)
                check_runtime_index("extraction before deferred memory release")
            mode = "population/reload smoke" if smoke else f"matched {scenario} capture"
            print(f"real world activity server: {mode}, normal wandering callbacks, live disable/enable and copyover rebuild passed", flush=True)
        except Exception:
            failure = Path(output).parent / f"activity-failure-{scenario}-{time.time_ns()}"
            failure.mkdir(parents=True)
            shutil.copy2(runtime / "server.out", failure / "server.out")
            shutil.copytree(runtime / "logs", failure / "logs")
            print(f"Disposable fixture diagnostics: {failure}", flush=True)
            print((runtime / "server.out").read_text(errors="replace")[-5000:])
            print(journey.runtime_logs(runtime)); raise
        finally: stop()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("binary", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--smoke", action="store_true")
    parser.add_argument("--runtime-index", action="store_true")
    parser.add_argument("--full-world", action="store_true")
    parser.add_argument("--scenario", choices=("empty", "sparse", "busy", "corpse", "travel", "mass-wake", "recovery"), default="sparse")
    parser.add_argument("--seconds", type=int, default=180)
    parser.add_argument("--population", type=int, default=1000)
    args = parser.parse_args()
    if not args.smoke and args.seconds < 160:
        parser.error("capture at least 160 seconds for a complete steady-state window")
    journey.build_inspector()
    run(args.binary.resolve(), args.output, population=args.population, seconds=args.seconds,
        smoke=args.smoke, runtime_index=args.runtime_index, scenario=args.scenario, full_world=args.full_world)
