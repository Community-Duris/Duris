#!/usr/bin/env python3
"""Real server control gameplay and publication in one fresh allow-listed DB."""
from __future__ import annotations

import argparse
from dataclasses import replace
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
import time

import test_flatfile_combat_journey as journey
from run_telemetry_player_journey import reviewed_property_catalog
from test_telemetry_repository import prepare_sql_fixture, drop_sql_fixture, sql_environment

ROOT = journey.ROOT
sys.path.insert(0, str(ROOT))
from scripts.telemetry import battle_publication, battle_source, battle_result_contract, battle_comparison, control_contract, incident, outage
from scripts.telemetry import progression_context_contract as progression_context, progression_publication
from scripts.telemetry.db_access import ConnectionSettings, PyMySQLConnectionFactory, PyMySQLRollupDatabase
from scripts.telemetry.rollup_definitions import RollupTarget, PUBLICATION_PUBLISHED, PUBLICATION_SUPERSEDED
from scripts.telemetry.rollup_engine import RollupEngine, RollupBounds, BoundsExceeded


def run():
    assert os.environ.get("TELEMETRY_REPOSITORY_DISPOSABLE") == "1"
    _, _, requested_database = sql_environment()
    if len("duris.player.death.restitution." + requested_database) > 64:
        raise RuntimeError("gameplay fixture name exceeds the MySQL runtime exclusion lock budget")
    import pymysql
    environment, command, database = prepare_sql_fixture()
    root = pymysql.connect(host="127.0.0.1", port=int(environment["DB_PORT"]), user=environment["DB_USER"],
        password=environment["DB_PASSWD"], database=database, autocommit=True, cursorclass=pymysql.cursors.DictCursor)
    users, adapters = [], []
    receipt = dict(status="running", actual_running_server=True, synthetic_accounts=9,
        production_or_staging_access=False, engine=os.environ["TELEMETRY_REPOSITORY_DB_IMAGE"])
    result = Path(os.environ["TELEMETRY_CONTROL_JOURNEY_RESULT"])
    token = hashlib.sha256(database.encode()).hexdigest()[:12]
    password = "synthetic-control-" + token

    def query(statement, parameters=()):
        with root.cursor() as cursor:
            cursor.execute(statement, parameters)
            return cursor.fetchall()

    def count(where="1"):
        return query("SELECT COUNT(*) AS n FROM telemetry_interval WHERE " + where)[0]["n"]

    def until(predicate, label, timeout=30):
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            if predicate():
                return
            time.sleep(0.1)
        raise AssertionError(label)

    try:
        for role in ("game", "writer", "rollup", "report", "review"):
            user = "ctl_" + role + "_" + token
            query("CREATE USER %s@'%%' IDENTIFIED BY %s", (user, password))
            users.append(user)
        query(f"GRANT ALL ON `{database}`.* TO %s@'%%'", (users[0],))
        for table in ("telemetry_config", "telemetry_interval", "telemetry_quarantine",
            "telemetry_progression_context", "telemetry_progression_configuration"):
            query(f"GRANT SELECT,INSERT ON `{database}`.`{table}` TO %s@'%%'", (users[1],))
        query(f"GRANT SELECT,INSERT,UPDATE ON `{database}`.telemetry_session TO %s@'%%'", (users[1],))
        # Minimal world boot skips SQL zone publication. Provide its native
        # zone authority before gameplay; objective receipts are never seeded.
        assert query("SELECT COUNT(*) AS n FROM zones WHERE number=1")[0]["n"] == 0
        query("INSERT INTO zones(number,name) VALUES(1,'Minimal World')")
        with tempfile.TemporaryDirectory(prefix="control-gameplay-") as temporary:
            runtime = Path(temporary)
            journey.make_fixture(runtime)
            # Restore ordinary starter damage: this journey needs living PvP
            # participants, rather than the shared fixture's fast NPC kill.
            objects = runtime / "areas_mini/mini.obj"
            objects.write_text(objects.read_text().replace("6 100 1 7 0 0 0 0", "6 1 6 7 0 0 0 0"))
            # One ordinary room exit permits native flee movement and a real
            # disengagement watch. The fixed mini-zone already contains stone
            # prototype 358 with the maintained epic_stone gameplay function.
            world_path = runtime / "areas_mini/mini.wld"
            world = world_path.read_text()
            start = world.index("#22800\n")
            end = world.index("\nS\n", start)
            world = world[:end] + "\nD0\n~\n~\n0 0 22801" + world[end:]
            world = world.replace("$~", "#22801\nThe Regression Refuge~\nA quiet room beyond the arena.\n~\n1 0 0\nD2\n~\n~\n0 0 22800\nS\n$~")
            world_path.write_text(world)
            # Ordinary prototype data only: XP still goes through native combat,
            # assistance, rested, caps, storage and level-threshold decisions.
            mobiles = runtime / "areas_mini/mini.mob"
            mobiles.write_text(mobiles.read_text().replace("$~", """#22802
progression subject~
the progression subject~
A progression subject waits here.
~
~
10 0 0 0 0 0 0 0 S
PH 0 0 -1
10 0 0 1d1+1 1d1+1
0.0.0.0 800
8 8 0
$~"""))
            value = objects.read_text()
            start, end = value.index("#358\n"), value.index("#359\n")
            value = value[:start] + value[start:end].replace("1 4 0 0 0 0 0 0", "1 4 1 0 0 0 0 0") + value[end:]
            objects.write_text(value)
            journey.generate_certificate(runtime)
            reviewed_property_catalog(runtime, runtime / "reviewed-properties.catalog")
            for name in ("logs/log", "journals/players", "journals/critical", "telemetry-ledger", "bin/server"):
                (runtime / name).mkdir(parents=True, exist_ok=True, mode=0o700)
            binary = ROOT / "bin/server/dms_new"
            def executable_hash(path):
                digest = hashlib.sha256()
                with path.open("rb") as source:
                    for block in iter(lambda: source.read(1024 * 1024), b""):
                        digest.update(block)
                return digest.hexdigest()

            binary_hash = executable_hash(binary)
            executable = runtime / "bin/server/dms_new"
            # Bound copying memory and avoid Docker bind-mount sendfile failures.
            with binary.open("rb") as source, executable.open("wb") as destination:
                shutil.copyfileobj(source, destination, length=1024 * 1024)
            shutil.copystat(binary, executable)
            os.link(executable, runtime / "bin/server/dms")
            port, tls, websocket = journey.available_ports()
            state = runtime / "missing-copyover-parent/copyover.dat"
            env = dict(PATH=os.environ.get("PATH", "/usr/bin:/bin"), ENVIRONMENT="local",
                DB_HOST="127.0.0.1", DB_PORT=environment["DB_PORT"], DB_NAME=database,
                DB_ALLOWED_TARGETS="127.0.0.1/" + database, DB_USER=users[0], DB_PASSWD=password, DB_TLS="FALSE",
                PERSISTENCE_MODE="mariadb-primary", PERSISTENCE_BACKEND="mariadb", REDIS="FALSE", CHAOS_MUD="FALSE",
                PLAYER_SAVE_JOURNAL_DIR=str(runtime / "journals/players"), CRITICAL_COMMAND_JOURNAL_DIR=str(runtime / "journals/critical"),
                COPYOVER_STATE_FILE=str(state), LISTEN_ADDRESS="127.0.0.1", DURIS_TLS_PORT=str(tls),
                DURIS_WEBSOCKET_LISTEN_ADDRESS="127.0.0.1", DURIS_WEBSOCKET_PORT=str(websocket),
                TELEMETRY_PROPERTY_CATALOG_FILE=str(runtime / "reviewed-properties.catalog"),
                TELEMETRY_OUTAGE_LEDGER_DIR=str(runtime / "telemetry-ledger"),
                TELEMETRY_ENABLED="false", TELEMETRY_BACKEND="sql", TELEMETRY_DB_USER=users[1], TELEMETRY_DB_PASSWD=password,
                TELEMETRY_INTERVAL_USEC="1000000", TELEMETRY_CHECKPOINT_INTERVAL_USEC="1000000",
                TELEMETRY_ACTIVE_WINDOW_USEC="3000000", TELEMETRY_CONTEXT_SEGMENTS_PER_MINUTE="64")
            process = output = None
            clients = []

            def stop():
                nonlocal process, output
                for client in clients:
                    client.close()
                clients.clear()
                if process and process.poll() is None:
                    process.terminate()
                    try:
                        process.wait(timeout=30)
                    except subprocess.TimeoutExpired:
                        process.kill()
                        process.wait(timeout=5)
                        raise AssertionError("game fixture shutdown budget exceeded")
                if output:
                    output.close()

            def boot(*, specials=False):
                nonlocal process, output
                output = (runtime / "server.out").open("w")
                arguments = [str(runtime / "bin/server/dms"), "--minimal"] + ([] if specials else ["-s"]) + [str(port)]
                process = subprocess.Popen(arguments,
                    cwd=runtime, env=env, stdout=output, stderr=subprocess.STDOUT)
                until(lambda: process.poll() is not None or "Entering game loop." in
                    (runtime / "server.out").read_text(errors="replace"), "server boot timed out", 90)
                assert process.poll() is None, "server exited during boot"
                assert executable_hash(Path(f"/proc/{process.pid}/exe")) == binary_hash

            def reconnect(account, character):
                client = journey.reconnect_character(port, account=account, character=character)
                clients.append(client)
                return client

            def save(client, character):
                started = time.perf_counter_ns()
                client.send("save")
                client.expect("Save complete for " + character + ".", timeout=30)
                return time.perf_counter_ns() - started

            def issue(client, text, response=None):
                while client._receive():
                    pass
                client.pending.clear()
                client.send(text)
                if response:
                    client.expect(response, timeout=30)
                return client.expect(" >", timeout=30)

            def wake_stand(client):
                deadline = time.monotonic() + 30
                while time.monotonic() < deadline:
                    issue(client, "wake")
                    reply = issue(client, "stand")
                    if any(text in reply for text in ("You are already standing.", "You clamber to your feet.",
                        "You rise to your feet.", "You manage to unsteadily get to your feet.", "You tense up and become more alert.")):
                        return
                    # Random native melee stun may refuse or knock down a stand.
                    # Wait for ordinary recovery rather than clearing the flag.
                    time.sleep(0.25)
                raise AssertionError("native wake/stand recovery did not accept standing")

            try:
                boot()
                characters = (("Ctlstaffacct", "Ctlstaff", "w"), ("Ctltargacct", "Ctltarget", "w"),
                    ("Pvpcastacct", "Pvpcast", "sorcerer"), ("Pvphealacct", "Pvpheal", "cleric"),
                    ("Pvpallyacct", "Pvpally", "w"), ("Pvpvictacct", "Pvpvictim", "w"),
                    ("Pvpmateacct", "Pvpmate", "w"), ("Progacct", "Progfirst", "w"),
                    ("Progacct", "Progsecond", "w"), ("Progpeeracct", "Progpeer", "w"))
                created_accounts = set()
                for account, character, class_name in characters:
                    client = journey.MudClient(port)
                    clients.append(client)
                    journey.create_character(client, account=account, character=character, class_name=class_name,
                        email=account.lower() + "@example.invalid", new_account=account not in created_accounts)
                    created_accounts.add(account)
                    save(client, character)
                    client.send("quit")
                    client.expect("ACCOUNT MENU", timeout=30)
                stop()
                assert count() == 0, "disabled telemetry wrote observations"
                query("UPDATE player_data SET level=62 WHERE name='Ctlstaff'")
                query("UPDATE player_data SET level=40 WHERE name='Ctltarget'")
                query("UPDATE player_data SET level=40,highest_level=40,base_hit=50000,hit_diff=0 WHERE name LIKE %s", ("Pvp%",))
                # Its native 24-second effect expires before the existing
                # 30-second battle inactivity censoring boundary.
                query("UPDATE player_data SET level=32,highest_level=32 WHERE name='Pvpcast'")
                query("UPDATE player_data SET level=50,highest_level=50 WHERE name='Pvpmate'")
                # A persistent, non-selected saving penalty makes the native
                # random gate reliably exercisable without bypassing it.
                query("INSERT INTO player_affects(pid,type,duration,flags,modifier,location) "
                    "SELECT pid,17,-1,512,20,20 FROM player_data WHERE name='Pvpvictim'")
                # Only persistent gameplay prerequisites are seeded. Every
                # positive observation below comes from the maintained server's
                # normal cast/attack/effect/lifecycle paths, never telemetry APIs.
                for character, spells in (("Pvpcast", (100, 102, 32)), ("Pvpheal", (4, 14, 16))):
                    for spell in spells:
                        for _ in range(16 if spell == 4 else 4):
                            query("INSERT INTO player_affects(pid,type,duration,flags,modifier) "
                                "SELECT pid,2005,-1,790,%s FROM player_data WHERE name=%s", (spell, character))
                boot()
                staff = reconnect("Ctlstaffacct", "Ctlstaff")
                off_save = [save(staff, "Ctlstaff") for _ in range(10)]
                stop()
                assert count() == 0
                env["TELEMETRY_ENABLED"] = "true"
                boot()
                staff = reconnect("Ctlstaffacct", "Ctlstaff")
                target = reconnect("Ctltargacct", "Ctltarget")
                target_pid = query("SELECT pid FROM player_data WHERE name='Ctltarget'")[0]["pid"]
                staff_pid = query("SELECT pid FROM player_data WHERE name='Ctlstaff'")[0]["pid"]
                until(lambda: query("SELECT COUNT(*) AS n FROM telemetry_session")[0]["n"] == 2, "authenticated sessions absent")
                issue(staff, "instacast 'major paralysis' Ctltarget")
                until(lambda: count("record_kind=13 AND ctl_kind=1 AND ctl_family=3 AND ctl_result=1") > 0,
                    "native major paralysis application absent")
                # Two live native effect owners overlap. Expiry uses the real event
                # clock; no fixture calls telemetry APIs or edits captured rows.
                issue(staff, "instacast 'blindness' Ctltarget")
                until(lambda: count(f"record_kind=13 AND ctl_target_actor_pid={target_pid} AND ctl_after_mask=5") > 0,
                    "native blindness/paralysis overlap absent")
                issue(staff, "instacast 'cure blind' Ctltarget")
                until(lambda: count(f"record_kind=13 AND ctl_target_actor_pid={target_pid} AND ctl_before_mask=5 AND ctl_after_mask=4") > 0,
                    "native cure boundary absent")
                until(lambda: count(f"record_kind=13 AND ctl_target_actor_pid={target_pid} AND ctl_before_mask=4 AND ctl_after_mask=0") > 0,
                    "native paralysis expiry absent", 45)
                issue(staff, "instacast 'major paralysis' Ctltarget")
                # Equipment changes and save rebuilding must not emit a false
                # off/on transition while the same effective status remains.
                staff.send("load obj 678")
                staff.expect("You have created", timeout=30)
                issue(staff, "setbit obj cap aff blind 1")
                issue(staff, "wear cap")
                until(lambda: count(f"record_kind=13 AND ctl_target_actor_pid={staff_pid} AND ctl_after_mask=1") > 0,
                    "equipment selected status absent")
                before_save = count(f"record_kind=13 AND ctl_target_actor_pid={staff_pid} AND ctl_boundary=2")
                on_save = [save(staff, "Ctlstaff") for _ in range(10)]
                time.sleep(1)
                assert count(f"record_kind=13 AND ctl_target_actor_pid={staff_pid} AND ctl_boundary=2") == before_save
                issue(staff, "remove cap")
                until(lambda: count(f"record_kind=13 AND ctl_target_actor_pid={staff_pid} AND ctl_before_mask=1 AND ctl_after_mask=0") > 0,
                    "equipment removal absent")
                staff.send("shutdown copyover")
                staff.expect("Copyover FAILED", timeout=60)
                save(staff, "Ctlstaff")
                old_producers = query("SELECT COUNT(DISTINCT boot_id,process_id) AS n FROM telemetry_interval")[0]["n"]
                state.parent.mkdir()
                staff.send("shutdown copyover")
                staff.expect("Copyover complete!", timeout=90)
                save(staff, "Ctlstaff")
                until(lambda: query("SELECT COUNT(DISTINCT boot_id,process_id) AS n FROM telemetry_interval")[0]["n"] > old_producers,
                    "copyover producer boundary absent")
                assert query("SELECT COUNT(*) AS n FROM telemetry_session")[0]["n"] == 2
                resumed_producer = query("SELECT boot_id,process_id FROM telemetry_interval ORDER BY ingest_id DESC LIMIT 1")[0]
                producer_where = "boot_id={boot_id} AND process_id={process_id}".format(**resumed_producer)
                until(lambda: count("record_kind=5 AND " + producer_where) > 0, "resumed configuration absent")
                before_resumed_control = count("record_kind=13 AND " + producer_where)
                issue(staff, "instacast 'major paralysis' Ctltarget")
                until(lambda: count("record_kind=13 AND " + producer_where) > before_resumed_control,
                    "fresh control baseline after copyover absent")
                until(lambda: count(f"record_kind=13 AND ctl_kind=2 AND ctl_target_actor_pid={target_pid} " +
                    "AND (ctl_after_mask & 4)<>0 AND " + producer_where) > 0,
                    "admitted active control baseline before writer outage absent")
                # Kill only private writer connections after revoking new login.
                # Gameplay save remains authoritative through its distinct role.
                outage_start = time.time_ns() // 1000
                query("ALTER USER %s@'%%' IDENTIFIED BY %s", (users[1], password + "-unavailable"))
                for row in query("SELECT ID FROM information_schema.PROCESSLIST WHERE USER=%s", (users[1],)):
                    query("KILL CONNECTION " + str(int(row["ID"])))
                save(staff, "Ctlstaff")
                issue(staff, "setbit char Ctlstaff aff blind 1")
                time.sleep(2)
                issue(staff, "setbit char Ctlstaff aff blind 0")
                query("ALTER USER %s@'%%' IDENTIFIED BY %s", (users[1], password))
                until(lambda: "state=circuit-open" in journey.runtime_logs(runtime), "private failure did not open circuit")
                save(staff, "Ctlstaff")
                stop()
                failed_evidence = outage.read_evidence(runtime / "telemetry-ledger")
                failed_producer = next(row for row in failed_evidence["observations"] if
                    (row["boot_id"], row["process_id"]) == (resumed_producer["boot_id"], resumed_producer["process_id"]))
                assert failed_producer["circuit_open_count"] > 0 and (
                    failed_producer["unknown_after_last_sample"] or failed_producer["known_abandoned_unattempted_records"] > 0)
                # Terminal circuits require the documented operator lifecycle
                # restart. Original backlog stays loss evidence, never replayed
                # as newly observed activity under the next producer.
                outage_end = time.time_ns() // 1000
                before_recovery = count("record_kind=13")
                control_origin = int(query("SELECT COALESCE(MAX(ingest_id),0) AS n FROM telemetry_interval")[0]["n"])
                # Preserve the original control fixture's special suppression.
                # The separate outcome producer enables native object procedures.
                boot()
                staff = reconnect("Ctlstaffacct", "Ctlstaff")
                target = reconnect("Ctltargacct", "Ctltarget")
                issue(staff, "instacast 'major paralysis' Ctltarget")
                until(lambda: count("record_kind=13") > before_recovery, "fresh producer did not recover control capture", 45)
                issue(staff, "setbit char Ctlstaff aff blind 1")
                issue(staff, "setbit char Ctlstaff aff blind 0")
                until(lambda: count(f"record_kind=13 AND ctl_target_actor_pid={staff_pid} AND ctl_after_mask=1 AND occurrence_utc_usec>={outage_end}") > 0 and
                    count(f"record_kind=13 AND ctl_target_actor_pid={staff_pid} AND ctl_before_mask=1 AND ctl_after_mask=0 AND occurrence_utc_usec>={outage_end}") > 0,
                    "fresh producer status transitions absent")
                save(staff, "Ctlstaff")
                # Fresh ordinary players enter only after the failed producer
                # has stopped. Their positive prefixes belong to the separately
                # witnessed, clean-drained recovery producer.
                pvp = {character: reconnect(account, character) for account, character, _ in characters[2:7]}
                pids = {row["name"]: row["pid"] for row in query("SELECT name,pid FROM player_data WHERE name LIKE %s", ("Pvp%",))}
                caster, healer, ally, victim, mate = (pvp[name] for name in
                    ("Pvpcast", "Pvpheal", "Pvpally", "Pvpvictim", "Pvpmate"))
                for client in pvp.values():
                    issue(client, "remove all")
                    issue(client, "toggle vicious off")
                pvp_producer = query("SELECT boot_id,process_id FROM telemetry_interval ORDER BY ingest_id DESC LIMIT 1")[0]
                pvp_where = "boot_id={boot_id} AND process_id={process_id}".format(**pvp_producer)

                def live_controls():
                    return query("SELECT * FROM telemetry_interval WHERE record_kind=13 AND " + pvp_where +
                        " AND ctl_target_actor_pid=%s ORDER BY record_seq", (pids["Pvpvictim"],))

                def cast_until(client, spell, family, label, *, minimum_ticks=0, attempts=4):
                    for _ in range(attempts):
                        before = max((row["record_seq"] for row in live_controls()), default=0)
                        client.send("cast '" + spell + "' Pvpvictim")
                        until(lambda: any(row["record_seq"] > before and row["ctl_kind"] == 1 and
                            row["ctl_family"] == family for row in live_controls()), label + " resolution absent", 20)
                        applied = [row for row in live_controls() if row["record_seq"] > before and row["ctl_kind"] == row["ctl_result"] == 1 and
                                row["ctl_family"] == family and row["ctl_configured_ticks"] >= minimum_ticks
                        ]
                        if applied:
                            return applied[-1]
                    raise AssertionError(label + " did not apply through ordinary casting")

                solo_start = int(query("SELECT COALESCE(MAX(ingest_id),0) AS n FROM telemetry_interval")[0]["n"])
                print("Ordinary solo PvP: cast, refresh and native expiry", flush=True)
                issue(caster, "kill Pvpvictim")
                cast_until(caster, "minor paralysis", 4, "solo paralysis")
                first_paralysis = next(row for row in reversed(live_controls()) if row["ctl_kind"] == row["ctl_result"] == 1)
                time.sleep(2)
                cast_until(caster, "minor paralysis", 4, "solo refresh")
                refreshed = [row for row in live_controls() if row["ctl_kind"] == row["ctl_result"] == 1 and
                    row["ctl_family"] == 4 and row["record_seq"] > first_paralysis["record_seq"]]
                assert refreshed and refreshed[-1]["ctl_before_mask"] & 8 and refreshed[-1]["ctl_after_mask"] & 8
                until(lambda: any(row["ctl_kind"] == 3 and row["ctl_before_mask"] & 8 and
                    not row["ctl_after_mask"] & 8 and row["record_seq"] > refreshed[-1]["record_seq"]
                    for row in live_controls()), "ordinary solo refreshed effect did not expire", 60)
                expiry = next(row for row in live_controls() if row["ctl_kind"] == 3 and
                    row["ctl_before_mask"] & 8 and not row["ctl_after_mask"] & 8 and
                    row["record_seq"] > refreshed[-1]["record_seq"])
                solo_end = int(query("SELECT MAX(ingest_id) AS n FROM telemetry_interval")[0]["n"])
                # Two actual groups, then a member leaves while control is live.
                # Group admission is unavailable in combat. Supervised native
                # tranquilize resets combat between these separate cases.
                issue(staff, "tranquilize", "entire room nods off")
                for client in pvp.values():
                    wake_stand(client)
                issue(healer, "follow Pvpcast", "now follow")
                issue(ally, "follow Pvpcast", "now follow")
                issue(healer, "consent Pvpcast")
                issue(ally, "consent Pvpcast")
                issue(caster, "group Pvpheal", "now a member of your group")
                issue(caster, "group Pvpally", "now a member of your group")
                issue(mate, "follow Pvpvictim", "now follow")
                issue(mate, "consent Pvpvictim")
                issue(victim, "group Pvpmate", "now a member of your group")
                group_start = int(query("SELECT MAX(ingest_id) AS n FROM telemetry_interval")[0]["n"])
                print("Ordinary group PvP: overlap, participation, cure and departure", flush=True)
                issue(healer, "kill Pvpvictim")
                issue(caster, "kill Pvpvictim")
                cast_until(caster, "slowness", 5, "group slow")
                ally.send("group Pvpally")
                ally.expect("You leave the group.", timeout=20)
                # Blindness randomly configures 4-12 seconds. Select a native
                # application long enough for the ordinary cure cast/lag, using
                # WAIT_SEC=4 only as a prerequisite, never as elapsed evidence.
                group_blindness = cast_until(healer, "blindness", 1, "group blindness", minimum_ticks=32, attempts=16)
                until(lambda: any(row["ctl_after_mask"] & 17 == 17 for row in live_controls()), "ordinary group overlap absent")
                healer.send("cast 'cure blind' Pvpvictim")
                victim.expect("Your vision returns!", timeout=20)
                until(lambda: any(row["ctl_kind"] == 3 and row["ctl_before_mask"] & 17 == 17 and
                    row["ctl_after_mask"] & 17 == 16 and row["record_seq"] > group_blindness["record_seq"]
                    for row in live_controls()), "ordinary group cure boundary absent", 20)
                cure = next(row for row in live_controls() if row["ctl_kind"] == 3 and
                    row["ctl_before_mask"] & 17 == 17 and row["ctl_after_mask"] & 17 == 16 and
                    row["record_seq"] > group_blindness["record_seq"])
                # Native quit supplies an observed actor departure; status on the
                # far side of that boundary cannot extend the battle prefix.
                issue(staff, "tranquilize", "entire room nods off")
                wake_stand(victim)
                departure_retries = 0
                for departure_attempt in range(3):
                    victim.send("quit")
                    response, _ = victim.expect_any(("ACCOUNT MENU", "You're too stunned to think of camping!"), timeout=180)
                    if response == "ACCOUNT MENU":
                        break
                    departure_retries += 1
                    wake_stand(victim)
                else:
                    raise AssertionError("native participant departure remained stun-refused")
                until(lambda: any(row["ctl_kind"] == 3 and row["ctl_boundary"] == 5 for row in live_controls()),
                    "ordinary participant lifecycle cut absent")
                receipt["ordinary_pvp"] = dict(producer=pvp_producer, target_pid=pids["Pvpvictim"],
                    caster_pid=pids["Pvpcast"], healer_pid=pids["Pvpheal"], solo_ingest_range=[solo_start+1, solo_end],
                    group_ingest_range=[group_start+1, int(query("SELECT MAX(ingest_id) AS n FROM telemetry_interval")[0]["n"])], refresh=True, expiry=True, cure=True,
                    changing_group_participation=True, observed_departure=True,
                    native_wake_stand_before_departure=True, stun_refused_quit_retries=departure_retries,
                    expiry_record_seq=expiry["record_seq"], cure_record_seq=cure["record_seq"],
                    positive_capture="ordinary untrusted cast and combat paths",
                    seeded_prerequisites="level, HP, memorized spells, non-selected saving penalty",
                    native_blindness_prerequisite_ticks=32,
                    supervised_combat_reset="native tranquilize between cases and before quit")
                print("Ordinary outcome PvP: equipment, effective support, flee and real escape watch", flush=True)
                # The control and outcome studies each need a complete bounded
                # publication window. Drain the first producer before starting
                # fresh authenticated/association history for the second.
                control_through = int(query("SELECT MAX(ingest_id) AS n FROM telemetry_interval")[0]["n"])
                stop()
                outcome_origin = int(query("SELECT MAX(ingest_id) AS n FROM telemetry_interval")[0]["n"])
                boot(specials=True)
                staff = reconnect("Ctlstaffacct", "Ctlstaff")
                pvp = {character: reconnect(account, character) for account, character, _ in characters[2:7] if character != "Pvpvictim"}
                caster, healer, ally, mate = (pvp[name] for name in ("Pvpcast", "Pvpheal", "Pvpally", "Pvpmate"))
                outcome_producer = query("SELECT boot_id,process_id FROM telemetry_interval ORDER BY ingest_id DESC LIMIT 1")[0]
                assert outcome_producer != pvp_producer
                outcome_where = "boot_id={boot_id} AND process_id={process_id}".format(**outcome_producer)
                outcome_start = int(query("SELECT MAX(ingest_id) AS n FROM telemetry_interval")[0]["n"])
                for client in (caster, healer, ally, mate):
                    wake_stand(client)
                issue(healer, "follow Pvpcast", "now follow")
                issue(healer, "consent Pvpcast")
                issue(caster, "group Pvpheal", "now a member of your group")
                issue(staff, "load obj 678", "You have created")
                issue(staff, "give cap Pvpally")
                issue(staff, "setbit char Pvpally hit 40000")
                issue(ally, "kill Pvpmate")

                def ally_builds():
                    return query("SELECT * FROM telemetry_interval WHERE record_kind=12 AND " + outcome_where +
                        " AND bctx_actor_kind=1 AND bctx_actor_id=%s ORDER BY record_seq", (pids["Pvpally"],))

                until(lambda: any(row["bctx_status"] == 1 and row["bctx_available"] & 32 for row in ally_builds()),
                    "ordinary equipment baseline absent")
                gear_before = next(row for row in reversed(ally_builds()) if row["bctx_status"] == 1 and row["bctx_available"] & 32)
                # The ordinary wear command is forbidden during combat. Keep
                # the prior observed point, change equipment while peaceful,
                # then observe a fresh admitted combat point.
                issue(staff, "tranquilize", "entire room nods off")
                for client in (caster, healer, ally, mate):
                    wake_stand(client)
                issue(ally, "wear cap", "You don")
                issue(ally, "kill Pvpmate")
                until(lambda: any(row["record_seq"] > gear_before["record_seq"] and row["bctx_status"] == 1 and
                    row["bctx_available"] & 32 and row["bctx_equipment_digest"] != gear_before["bctx_equipment_digest"] for row in ally_builds()),
                    "ordinary equipment-change point absent")
                gear_after = next(row for row in ally_builds() if row["record_seq"] > gear_before["record_seq"] and
                    row["bctx_status"] == 1 and row["bctx_available"] & 32 and row["bctx_equipment_digest"] != gear_before["bctx_equipment_digest"])
                issue(staff, "setbit char Pvpally hit 40000")
                issue(healer, "cast 'cure light' Pvpally")
                until(lambda: count("record_kind=10 AND " + outcome_where + " AND battle_relation=2 AND "
                    f"battle_actor_pid={pids['Pvpheal']} AND battle_related_actor_id={pids['Pvpally']}") > 0,
                    "effective ordinary support relation absent")
                support = query("SELECT * FROM telemetry_interval WHERE record_kind=10 AND " + outcome_where +
                    " AND battle_relation=2 AND battle_actor_pid=%s AND battle_related_actor_id=%s ORDER BY record_seq DESC LIMIT 1",
                    (pids["Pvpheal"], pids["Pvpally"]))[0]
                assert support["battle_actor_group_size"] == 2
                issue(healer, "group Pvpheal", "You leave the group.")
                until(lambda: count("record_kind=10 AND " + outcome_where + f" AND battle_actor_pid={pids['Pvpheal']} "
                    f"AND battle_actor_group_size=1 AND record_seq>{support['record_seq']}") > 0,
                    "support participant group departure absent")
                support_departure = query("SELECT * FROM telemetry_interval WHERE record_kind=10 AND " + outcome_where +
                    " AND battle_actor_pid=%s AND battle_actor_group_size=1 AND record_seq>%s ORDER BY record_seq LIMIT 1",
                    (pids["Pvpheal"], support["record_seq"]))[0]
                for _ in range(16):
                    while ally._receive():
                        pass
                    ally.pending.clear()
                    ally.send("flee")
                    accepted, _ = ally.expect_any(("You flee northward!", "You couldn't escape!"), timeout=30)
                    ally.expect(" >", timeout=30)
                    if accepted == "You flee northward!":
                        # SQL detail is asynchronous. Do not send another flee
                        # while waiting for its first accepted movement receipt.
                        until(lambda: count(f"record_kind=14 AND bout_kind=2 AND bout_target_actor_pid={pids['Pvpally']} AND " + outcome_where) > 0,
                            "accepted ordinary flee receipt absent")
                        break
                else:
                    raise AssertionError("ordinary flee did not accept room movement")
                until(lambda: count(f"record_kind=14 AND bout_kind=4 AND bout_target_actor_pid={pids['Pvpally']} AND " + outcome_where) > 0,
                    "native scheduler did not confirm the ordinary escape watch", 50)
                movement = query("SELECT * FROM telemetry_interval WHERE record_kind=14 AND bout_kind=2 AND " + outcome_where +
                    " AND bout_target_actor_pid=%s ORDER BY record_seq DESC LIMIT 1", (pids["Pvpally"],))[0]
                escape = query("SELECT * FROM telemetry_interval WHERE record_kind=14 AND bout_kind=4 AND " + outcome_where +
                    " AND bout_target_actor_pid=%s ORDER BY record_seq DESC LIMIT 1", (pids["Pvpally"],))[0]
                assert escape["bout_parent_sequence"] == movement["bout_sequence"] and movement["bout_from_room_vnum"] == 22800
                assert movement["bout_to_room_vnum"] == 22801 and escape["bout_at_usec"]-escape["bout_start_usec"] >= 30_000_000
                issue(ally, "south")
                issue(staff, "tranquilize", "entire room nods off")
                for client in (caster, healer, ally, mate):
                    wake_stand(client)
                print("Ordinary fatal PvP and supported committed stone objective", flush=True)
                victim = reconnect("Pvpvictacct", "Pvpvictim")
                wake_stand(victim)
                issue(staff, "setbit char Pvpvictim hit 1")
                issue(caster, "toggle vicious on", "and will kill mortally wounded victims")
                issue(caster, "kill Pvpvictim")
                # The sorcerer's untrained melee can miss every swing. Its
                # ordinary memorized missile still runs native cast/damage/die.
                issue(caster, "cast 'magic missile' Pvpvictim")
                until(lambda: count(f"record_kind=14 AND bout_kind=1 AND bout_target_actor_pid={pids['Pvpvictim']} AND " + outcome_where) > 0,
                    "ordinary native death evidence absent", 60)
                death = query("SELECT * FROM telemetry_interval WHERE record_kind=14 AND bout_kind=1 AND " + outcome_where +
                    " AND bout_target_actor_pid=%s ORDER BY record_seq DESC LIMIT 1", (pids["Pvpvictim"],))[0]
                assert death["bout_source_actor_pid"] == pids["Pvpcast"] and death["bout_target_battle_seq"] > 0
                issue(staff, "tranquilize", "entire room nods off")
                for client in (caster, healer, ally, mate):
                    wake_stand(client)
                issue(staff, "load obj 358", "You have created")
                issue(staff, "give stone Pvpmate")
                issue(mate, "touch stone", "You touch")
                until(lambda: count(f"record_kind=14 AND bout_kind=6 AND bout_target_actor_pid={pids['Pvpmate']} AND " + outcome_where) > 0,
                    "native committed objective receipt absent", 45)
                objective = query("SELECT * FROM telemetry_interval WHERE record_kind=14 AND bout_kind=6 AND " + outcome_where +
                    " AND bout_target_actor_pid=%s ORDER BY record_seq DESC LIMIT 1", (pids["Pvpmate"],))[0]
                operation = objective["bout_operation_id"]
                committed = query("SELECT operation_id,committed_at FROM critical_operation_inbox WHERE operation_id=%s", (operation,))
                claim = query("SELECT stone_uid,operation_id FROM epic_stone_claim WHERE stone_uid=%s", (objective["bout_source_object_uid"],))
                credit = query("SELECT * FROM zone_touch_outcome WHERE operation_id=%s", (operation,))
                assert len(committed) == len(claim) == len(credit) == 1 and committed[0]["committed_at"] is not None
                assert claim[0]["operation_id"] == operation and credit[0]["toucher_pid"] == pids["Pvpmate"]
                assert credit[0]["group_size"] == objective["bout_participant_count"] and credit[0]["zone_number"] == objective["bout_credited_zone_vnum"]
                outcome_through = int(query("SELECT MAX(ingest_id) AS n FROM telemetry_interval")[0]["n"])
                receipt["ordinary_outcomes"] = dict(producer=outcome_producer, ingest_range=[outcome_start+1,
                    outcome_through],
                    equipment_actor_pid=pids["Pvpally"], equipment_point_sequences=[gear_before["bctx_sequence"], gear_after["bctx_sequence"]],
                    support_ingest_id=support["ingest_id"], support_record_seq=support["record_seq"],
                    support_departure_ingest_id=support_departure["ingest_id"], changing_support_group_participation=True,
                    movement_sequence=movement["bout_sequence"], escape_sequence=escape["bout_sequence"],
                    escape_observed_usec=escape["bout_at_usec"]-escape["bout_start_usec"], death_sequence=death["bout_sequence"],
                    objective_sequence=objective["bout_sequence"], objective_operation_id=operation.hex(),
                    objective_source_uid=objective["bout_source_object_uid"], objective_receipt_matches_authoritative_claim=True,
                    ordinary_equipment=True, ordinary_effective_support=True, ordinary_flee=True, real_scheduler_escape=True,
                    ordinary_fatal_pvp=True, ordinary_supported_objective=True,
                    native_special_procedures_enabled=True,
                    seeded_prerequisites="native mini-zone SQL authority, memorized cure light and fatal magic missile, objective recipient level 50; staff sets current HP before support and fatal combat",
                    supervised_combat_reset="native tranquilize between outcome cases")
                receipt.update(binary_sha256=binary_hash, disabled_capture_zero=True, accepted_major_paralysis=True,
                    native_overlap=True, native_cure=True, native_expiry=True, equipment_apply_remove=True,
                    save_no_false_transition=True, copyover_failure_and_exec=True, logical_sessions_preserved=True,
                    private_sql_outage_game_save=True, private_sql_recovery=True,
                    private_sql_recovery_mode="operator lifecycle restart after terminal circuit",
                    terminal_circuit_and_abandoned_backlog=True, failed_producer_evidence=failed_producer,
                    copyover_fresh_control_baseline=True,
                    save_latency_ns={"telemetry_off": off_save, "telemetry_on_status_present": on_save})
                stop()
                # Use the actual captured threshold catalog as a gameplay
                # prerequisite, with the server stopped. No telemetry output or
                # earned award is seeded, and every later level gain is native.
                catalog_root = query("SELECT pf.pcfg_boot_id,pf.pcfg_process_id,pf.pcfg_root_record_seq "
                    "FROM telemetry_progression_configuration pf WHERE pcfg_chunk_index=0 "
                    "ORDER BY record_seq DESC LIMIT 1")[0]
                catalog_rows = query("SELECT * FROM telemetry_progression_configuration "
                    "WHERE pcfg_boot_id=%s AND pcfg_process_id=%s AND pcfg_root_record_seq=%s ORDER BY pcfg_chunk_index",
                    (catalog_root["pcfg_boot_id"], catalog_root["pcfg_process_id"], catalog_root["pcfg_root_record_seq"]))
                catalog = progression_context.reconcile_configuration_chunks(
                    [{name: row[name] for name, _, _ in progression_context.CONFIGURATION_CHUNK_LAYOUT}
                        for row in catalog_rows], [(row["boot_id"], row["process_id"], row["record_seq"]) for row in catalog_rows])
                thresholds = {entry["id"]: entry["bits"] for entry in catalog["values"] if 1 <= entry["id"] <= 62}
                assert 1 < thresholds[11] < 2**31
                query("UPDATE player_data SET level=10,highest_level=10,exp=0,base_hit=50000,hit_diff=0 WHERE name LIKE %s", ("Prog%",))
                query("UPDATE player_data SET exp=%s WHERE name='Progfirst'", (thresholds[11]-1,))
                progress_origin = int(query("SELECT MAX(ingest_id) AS n FROM telemetry_interval")[0]["n"])
                # The independently versioned progression producer uses a
                # coarser cadence to keep this several-minute native journey
                # inside the unchanged retained/publication byte budget.
                env["TELEMETRY_INTERVAL_USEC"] = "10000000"
                env["TELEMETRY_CHECKPOINT_INTERVAL_USEC"] = "20000000"
                boot(specials=True)
                staff = reconnect("Ctlstaffacct", "Ctlstaff")
                paging = issue(staff, "toggle paging")
                if "mode on." in paging:
                    paging = issue(staff, "toggle paging")
                assert "mode off." in paging
                first = reconnect("Progacct", "Progfirst")
                peer = reconnect("Progpeeracct", "Progpeer")
                progress_pids = {row["name"]: row["pid"] for row in query("SELECT name,pid FROM player_data WHERE name LIKE %s", ("Prog%",))}

                def progress_points(character=None, through=None):
                    clause = "" if character is None else " AND px.pctx_pid=%s"
                    parameters = (progress_origin,) if character is None else (progress_origin, progress_pids[character])
                    if through is not None:
                        clause += " AND r.ingest_id<=%s"
                        parameters += (through,)
                    return query("SELECT px.*,r.ingest_id FROM telemetry_progression_context px "
                        "JOIN telemetry_interval r USING(boot_id,process_id,record_seq) WHERE r.ingest_id>%s" + clause +
                        " ORDER BY r.ingest_id", parameters)

                receipt["native_progression_attack_attempts"] = []

                def native_kill(client, character, *, wait_for_receipt=True, npc_xp=1000000):
                    before = int(query("SELECT COALESCE(MAX(ingest_id),0) AS n FROM telemetry_interval")[0]["n"])
                    issue(staff, "load mob 22802", "You have created")
                    if npc_xp == 800:
                        # Keep the narrow rate prerequisite at one maximum HP;
                        # regeneration during command prompts must not turn it
                        # into a prolonged fight with changing build exposure.
                        # Native kills of opponents within five levels add a
                        # bloodlust affect after the award. A level-4 target
                        # keeps this level-10 rate scenario in a stable build
                        # stratum; native level-difference XP modifiers apply.
                        issue(staff, "setbit char subject level 4")
                        issue(staff, "setbit char subject basehit 1")
                        issue(staff, "setattr subject hit 1", "OK.")
                    issue(staff, "setbit char subject hit 1")
                    if npc_xp == 800:
                        inspected = re.sub(r"\x1b\[[0-?]*[ -/]*[@-~]", "", issue(staff, "stat char subject"))
                        assert re.search(r"Level:\s*4\(", inspected), inspected
                        assert re.search(r"Hits:\s*\[\s*1/\s*1/\s*1\+", inspected), inspected
                    # convertMob replaces prototype XP during native loading.
                    # Set only the NPC prerequisite; player XP still follows
                    # native damage/kill awards, modifiers, caps and levels.
                    issue(staff, "setbit char subject exp " + str(npc_xp))
                    # The native toggle command flips this preference; it
                    # ignores a trailing "on". Observe its reply and enable
                    # it if the first toggle switched it off.
                    preference = issue(client, "toggle vicious")
                    if "mode off." in preference:
                        preference = issue(client, "toggle vicious")
                    assert "will kill mortally wounded victims." in preference
                    issue(client, "wield mace")
                    transcript_start = len(client.transcript)
                    attempt = dict(character=character, npc_xp=npc_xp, ordinary_command_attempts=1,
                        completion_budget_seconds=180 if wait_for_receipt else 60)
                    receipt["native_progression_attack_attempts"].append(attempt)
                    issue(client, "kill subject")
                    # Native critical misses can stop fighting altogether, not
                    # merely delay a hit. Reissue the same ordinary attack in
                    # the existing bounded wait; never load another subject or
                    # clear an affect. All misses/context cuts stay retained.
                    deadline = time.monotonic() + attempt["completion_budget_seconds"]
                    next_attack = time.monotonic() + 5
                    while b"You receive your share of experience." not in client.transcript[transcript_start:]:
                        assert time.monotonic() < deadline, "ordinary progression kill-share gameplay absent"
                        client._receive()
                        if b"You receive your share of experience." in client.transcript[transcript_start:]:
                            break
                        if time.monotonic() >= next_attack:
                            attempt["ordinary_command_attempts"] += 1
                            issue(client, "kill subject")
                            next_attack = time.monotonic() + 5
                        time.sleep(0.1)
                    attempt["native_kill_share_message_observed"] = True
                    if not wait_for_receipt:
                        return []
                    until(lambda: count("record_kind=6 AND ingest_id>" + str(before) +
                        " AND pid=" + str(progress_pids[character]) + " AND progression_source=3 AND progression_applied_xp>0") > 0,
                        "ordinary progression kill-share receipt absent")
                    awards = query("SELECT * FROM telemetry_interval WHERE record_kind=6 AND ingest_id>%s "
                        "AND pid=%s AND progression_applied_xp>0 ORDER BY ingest_id", (before, progress_pids[character]))
                    sources = {(row["boot_id"], row["process_id"], row["record_seq"]) for row in awards}
                    until(lambda: sources <= {(row["pctx_source_boot_id"], row["pctx_source_process_id"], row["pctx_source_record_seq"])
                        for row in progress_points(character)}, "native award decision contexts absent")
                    return awards

                print("Ordinary progression: solo XP, two completed milestones and rested/group decisions", flush=True)
                native_kill(first, "Progfirst")
                until(lambda: any(row["pctx_boundary"] == 4 and row["pctx_current_level"] == 11
                    for row in progress_points("Progfirst")), "first native level advance absent")
                # Retain the original fixture's segment cap. Spreading fights
                # also exposes real elapsed time between native milestones.
                for attempt in range(4):
                    if any(row["pctx_boundary"] == 4 and row["pctx_current_level"] >= 12
                            for row in progress_points("Progfirst")):
                        break
                    time.sleep(62)
                    native_kill(first, "Progfirst")
                until(lambda: any(row["pctx_boundary"] == 4 and row["pctx_current_level"] == 12
                    for row in progress_points("Progfirst")), "full native level stage did not complete")
                milestone_through = int(query("SELECT MAX(ingest_id) AS n FROM telemetry_interval")[0]["n"])
                issue(staff, "newbsu Progfirst", "Done.")
                issue(peer, "follow Progfirst", "now follow")
                issue(peer, "consent Progfirst")
                issue(first, "group Progpeer", "now a member of your group")
                issue(first, "look")
                issue(peer, "look")
                time.sleep(62)
                group_awards = native_kill(first, "Progfirst")
                until(lambda: any(row["pctx_assistance"] == 2 and row["pctx_eligible_group_size"] == 2
                    for row in progress_points("Progfirst")), "native group-share context absent")
                assert any(row["pctx_assistance"] == 1 for row in progress_points("Progfirst"))
                assert any(row["pctx_rested_application"] in (3, 4) and
                    row["pctx_flags"] & progression_context.APPLICATION_KNOWN for row in progress_points("Progfirst"))
                after_group_award = max(row["ingest_id"] for row in group_awards)
                issue(first, "look")
                issue(peer, "look")
                until(lambda: any(row["ingest_id"] > after_group_award and row["pctx_boundary"] == 2 and
                    row["pctx_quality_flags"] == 0 and row["pctx_flags"] & progression_context.CONTIGUOUS_EXPOSURE
                    for row in progress_points("Progfirst")), "first-character connected exposure before rotation absent", 45)
                group_through = int(query("SELECT MAX(ingest_id) AS n FROM telemetry_interval")[0]["n"])
                # Save evidence is checked against the independent player store;
                # the published telemetry observations remain observed-mutable.
                save(first, "Progfirst")
                saved = query("SELECT level,exp FROM player_data WHERE name='Progfirst'")[0]
                assert saved["level"] >= 12
                first.send("quit")
                # Both mortal character switches preserve the ordinary camp
                # timer rather than changing its property for the fixture.
                first.expect("ACCOUNT MENU", timeout=180)
                time.sleep(1)
                second = reconnect("Progacct", "Progsecond")
                issue(second, "wield mace")
                issue(second, "look")
                issue(peer, "look")
                native_kill(second, "Progsecond", npc_xp=800)
                time.sleep(62)
                # A separate retained window starts after native warmup. It
                # excludes earlier setup/context cuts without omitting unknown
                # exposure from the full milestone/rotation generation.
                until(lambda: any(row["pctx_boundary"] == 2 and row["pctx_quality_flags"] == 0 and
                    row["pctx_flags"] & progression_context.CONTIGUOUS_EXPOSURE
                    for row in progress_points("Progsecond")), "stable second-character exposure absent")
                rate_probe_origin = int(query("SELECT MAX(ingest_id) AS n FROM telemetry_interval")[0]["n"])
                rate_probe_awards = native_kill(second, "Progsecond", npc_xp=800)
                assert all(row["progression_before_level"] == row["progression_after_level"] == 10 for row in rate_probe_awards)
                last_rate_award = max(row["ingest_id"] for row in rate_probe_awards)
                until(lambda: any(row["ingest_id"] > last_rate_award and row["pctx_boundary"] == 2
                    for row in progress_points("Progsecond")), "native rate interval absent")
                # Native no-misfire tags cut the wider fight's exposure. Qualify
                # one declared prefix through the native kill-share boundary.
                # Earlier fight awards/cuts remain in the adjacent rotation
                # generation; subsequent lifecycle/recovery stays in the next
                # generation. This proves a connected-time rate path, never a
                # full-fight rate or active attention. No unknown interval is
                # filtered out of a retained publication window.
                kill_share_awards = [row for row in rate_probe_awards if row["progression_source"] == 3]
                assert len(kill_share_awards) == 1
                first_rate_award = kill_share_awards[0]
                native_exposures = [row for row in progress_points("Progsecond") if
                    row["ingest_id"] > rate_probe_origin and row["pctx_boundary"] == 2 and
                    row["pctx_quality_flags"] == 0 and row["pctx_flags"] & progression_context.CONTIGUOUS_EXPOSURE]
                before_award = [row for row in native_exposures if row["ingest_id"] < first_rate_award["ingest_id"] and
                    row["pctx_at_usec"] <= first_rate_award["at_monotonic_usec"]]
                covering_award = [row for row in native_exposures if row["ingest_id"] > first_rate_award["ingest_id"] and
                    row["pctx_start_usec"] <= first_rate_award["at_monotonic_usec"] <= row["pctx_at_usec"]]
                assert before_award and covering_award, "native kill-share clean exposure prefix absent"
                anchor = before_award[-1]
                prefix = covering_award[0]
                anchor_source = query("SELECT ingest_id,quality_flags FROM telemetry_interval WHERE boot_id=%s "
                    "AND process_id=%s AND record_seq=%s AND record_kind=1",
                    (anchor["pctx_source_boot_id"], anchor["pctx_source_process_id"], anchor["pctx_source_record_seq"]))[0]
                assert anchor_source["quality_flags"] == 0
                rate_origin, rate_through = anchor_source["ingest_id"] - 1, prefix["ingest_id"]
                rate_awards = [row for row in rate_probe_awards if rate_origin < row["ingest_id"] <= rate_through]
                assert rate_awards == [first_rate_award]
                second.send("quit")
                # Mortal quit follows the ordinary camp timer (about 140 s).
                # Preserve that lifecycle rather than changing its property.
                second.expect("ACCOUNT MENU", timeout=180)
                before_reload = int(query("SELECT MAX(ingest_id) AS n FROM telemetry_interval")[0]["n"])
                first = reconnect("Progacct", "Progfirst")
                until(lambda: any(row["ingest_id"] > before_reload and row["pctx_boundary"] == 1
                    for row in progress_points("Progfirst")), "saved progression reload baseline absent")
                loaded = next(row for row in progress_points("Progfirst") if row["ingest_id"] > before_reload and row["pctx_boundary"] == 1)
                assert loaded["pctx_current_level"] == saved["level"] and loaded["pctx_current_exp"] == saved["exp"]
                progress_before_copyover = int(query("SELECT MAX(ingest_id) AS n FROM telemetry_interval")[0]["n"])
                staff.send("shutdown copyover")
                staff.expect("Copyover complete!", timeout=90)
                save(first, "Progfirst")
                issue(first, "look")
                issue(peer, "look")
                until(lambda: any(row["ingest_id"] > progress_before_copyover and row["pctx_boundary"] == 1
                    for row in progress_points("Progfirst")), "progression copyover baseline absent")
                # A real XP decision and player save continue through a private
                # telemetry writer outage. The absent source cannot be rebuilt
                # from that independent save or counted as observed zero XP.
                save(first, "Progfirst")
                before_failure_save = query("SELECT level,exp FROM player_data WHERE name='Progfirst'")[0]
                progression_failed_producer = query("SELECT boot_id,process_id FROM telemetry_interval ORDER BY ingest_id DESC LIMIT 1")[0]
                progression_outage_start = time.time_ns() // 1000
                query("ALTER USER %s@'%%' IDENTIFIED BY %s", (users[1], password + "-progression-unavailable"))
                for row in query("SELECT ID FROM information_schema.PROCESSLIST WHERE USER=%s", (users[1],)):
                    query("KILL CONNECTION " + str(int(row["ID"])))
                native_kill(first, "Progfirst", wait_for_receipt=False)
                save(first, "Progfirst")
                after_failure_save = query("SELECT level,exp FROM player_data WHERE name='Progfirst'")[0]
                assert after_failure_save != before_failure_save
                def progression_circuit_open():
                    health = issue(staff, "world telemetry")
                    producer = "producer={boot_id}:{process_id}".format(**progression_failed_producer)
                    return "state=circuit-open" in health and producer in health

                until(progression_circuit_open, "progression outage did not open the current producer's circuit")
                query("ALTER USER %s@'%%' IDENTIFIED BY %s", (users[1], password))
                stop()
                progression_failed_evidence = outage.read_evidence(runtime / "telemetry-ledger")
                progression_failed_witness = next(row for row in progression_failed_evidence["observations"] if
                    (row["boot_id"], row["process_id"]) == (progression_failed_producer["boot_id"], progression_failed_producer["process_id"]))
                assert progression_failed_witness["circuit_open_count"] > 0 and (
                    progression_failed_witness["unknown_after_last_sample"] or progression_failed_witness["known_abandoned_unattempted_records"] > 0)
                progression_outage_end = time.time_ns() // 1000
                before_progression_recovery = int(query("SELECT MAX(ingest_id) AS n FROM telemetry_interval")[0]["n"])
                boot(specials=True)
                staff = reconnect("Ctlstaffacct", "Ctlstaff")
                first = reconnect("Progacct", "Progfirst")
                recovery_awards = native_kill(first, "Progfirst")
                assert any(row["ingest_id"] > before_progression_recovery and row["progression_source"] == 3 for row in recovery_awards)
                save(first, "Progfirst")
                progress_through = int(query("SELECT MAX(ingest_id) AS n FROM telemetry_interval")[0]["n"])
                progress_capture = progress_points(through=progress_through)
                owners = {name: {row["pctx_account_token"] for row in progress_capture if row["pctx_pid"] == pid and row["pctx_account_token"]}
                    for name, pid in progress_pids.items()}
                assert len(owners["Progfirst"]) == len(owners["Progsecond"]) == len(owners["Progpeer"]) == 1
                assert owners["Progfirst"] == owners["Progsecond"] and owners["Progfirst"] != owners["Progpeer"]
                progress_scope = (progress_capture[0]["pctx_environment_id"], progress_capture[0]["pctx_season_id"])
                progress_clocks = query("SELECT MIN(CASE WHEN record_kind=1 THEN start_utc_usec ELSE occurrence_utc_usec END) AS first_clock,"
                    "MAX(occurrence_utc_usec) AS through_clock FROM telemetry_interval WHERE ingest_id>%s AND ingest_id<=%s "
                    "AND record_kind IN (1,2,6,9,15,16) AND occurrence_utc_usec>=0", (progress_origin, progress_through))[0]
                progress_times = [progress_clocks["first_clock"], progress_clocks["through_clock"]]
                assert progress_times
                receipt["ordinary_progression"] = dict(origin=progress_origin, watermark=progress_through,
                    scope=list(progress_scope), character_pids=progress_pids, saved_player_readback=saved,
                    actual_threshold_catalog_digest=progression_context.configuration_digest(catalog),
                    captured_contexts=len(progress_capture), account_tokens={name: next(iter(tokens)) for name, tokens in owners.items()},
                    group_award_receipts=[[row["boot_id"], row["process_id"], row["record_seq"]] for row in group_awards],
                    reviewed_clock_range=[min(progress_times)-1,max(progress_times)+1],
                    actual_native_kills=True, native_threshold_consumption=True, actual_rested_application=True,
                    actual_solo_and_group_share=True, sequential_same_account_switch=True, overlapping_accounts=True,
                    normal_save_readback=True, successful_progression_copyover=True,
                    private_writer_outage_native_XP_and_save=True,
                    failure_player_store_before=before_failure_save, failure_player_store_after=after_failure_save,
                    failure_native_source_reconstructed=False, failure_XP_counted_as_observed_zero=False,
                    failed_producer=progression_failed_producer,
                    private_writer_outage_clock_range=[progression_outage_start, progression_outage_end],
                    private_writer_recovery="fresh producer after terminal-circuit lifecycle restart",
                    recovery_kill_award_receipts=[[row["boot_id"], row["process_id"], row["record_seq"]] for row in recovery_awards if row["progression_source"] == 3],
                    XP_award_committed=False, telemetry_certifies_character_save=False,
                    seeded_prerequisites="stopped server: level 10, HP, first character's XP one below the actual native level-11 threshold; native NPC prototype; staff sets NPC current HP to 1 and NPC XP to 1000000 for milestones or 800, level 4 and one verified maximum HP for the narrow rate window before each ordinary kill; native level-difference XP modifiers and mortality apply; ordinary staff rested buff; ordinary mortal camp before switching back",
                    rate_input_origin=rate_origin, rate_input_watermark=rate_through,
                    rate_window_selection="native connected exposure through the kill-share decision; earlier fight cuts and awards retained in adjacent generations; no whole-fight or active-attention rate",
                    rate_probe_origin=rate_probe_origin,
                    rate_probe_award_receipts=[[row["boot_id"], row["process_id"], row["record_seq"]] for row in rate_probe_awards],
                    publication_windows=[dict(name="milestones", generation=1, origin=progress_origin, watermark=milestone_through),
                        dict(name="group_decisions", generation=2, origin=milestone_through, watermark=group_through),
                        dict(name="rotation", generation=3, origin=group_through, watermark=rate_probe_origin),
                        dict(name="uncertain_fight_exposure", generation=4, origin=rate_probe_origin, watermark=rate_origin),
                        dict(name="comparable_rates", generation=5, origin=rate_origin, watermark=rate_through),
                        dict(name="persistence_and_recovery", generation=6, origin=rate_through, watermark=progress_through)],
                    rate_award_receipts=[[row["boot_id"], row["process_id"], row["record_seq"]] for row in rate_awards],
                    budget_evidence="original 64-segment fixture cap, 10-second progression intervals, native fights separated by 62 seconds")
                stop()
                receipt["worker_outage_evidence"] = outage.read_evidence(runtime / "telemetry-ledger")
                assert receipt["worker_outage_evidence"]["ledger_version"] == 7
                witness = next(row for row in receipt["worker_outage_evidence"]["observations"] if
                    (row["boot_id"], row["process_id"]) == (pvp_producer["boot_id"], pvp_producer["process_id"]))
                assert witness["phase"] == "clean_drained" and not witness["unknown_after_last_sample"]
                assert all(witness[name] == 0 for name in ("rejected_detail_admissions", "rejected_control_admissions",
                    "quarantined_records", "invalid_records", "conflict_records", "sequence_gap_count", "unclosed_tail_count"))
                receipt["ordinary_pvp"]["independent_delivery_witness"] = witness
                outcome_witness = next(row for row in receipt["worker_outage_evidence"]["observations"] if
                    (row["boot_id"], row["process_id"]) == (outcome_producer["boot_id"], outcome_producer["process_id"]))
                assert outcome_witness["phase"] == "clean_drained" and not outcome_witness["unknown_after_last_sample"]
                assert all(outcome_witness[name] == 0 for name in ("rejected_detail_admissions", "rejected_control_admissions",
                    "quarantined_records", "invalid_records", "conflict_records", "sequence_gap_count", "unclosed_tail_count"))
                receipt["ordinary_outcomes"]["independent_delivery_witness"] = outcome_witness
            except Exception:
                result.parent.mkdir(parents=True, exist_ok=True)
                (result.parent / (result.stem + "-failure-controls.json")).write_text(
                    json.dumps(query("SELECT * FROM telemetry_interval WHERE record_kind IN (10,12,13,14) ORDER BY ingest_id"), default=str, indent=2) + "\n")
                for client in clients:
                    try:
                        while client._receive():
                            pass
                    except (AssertionError, OSError):
                        # Earlier quit/restart cases retain closed clients in
                        # the transcript list. They must not hide the failure.
                        pass
                (result.parent / (result.stem + "-server-failure.log")).write_text(
                    (runtime / "server.out").read_text(errors="replace") + "\n" + journey.runtime_logs(runtime) +
                    "\n" + "\n".join(bytes(client.transcript[-6000:]).decode(errors="replace").replace(journey.PASSWORD, "[redacted]")
                        for client in clients), encoding="utf-8")
                raise
            finally:
                stop()
        controls = query("SELECT * FROM telemetry_interval WHERE record_kind=13 AND ingest_id<=%s ORDER BY ingest_id", (control_through,))
        assert controls and any(row["ctl_kind"] == 3 for row in controls)
        target_scope = RollupTarget(7, 1, controls[0]["ctl_environment_id"], controls[0]["ctl_season_id"])
        tables = tuple(battle_source.table(table, target_scope.scope_tuple) for table in (
            "telemetry_battle_source", "telemetry_battle_input", "telemetry_rollup_battle_coverage", "telemetry_rollup_battle_row"))
        review_tables = incident.schema_contract(6)[2:]
        grants = {
            2: {"telemetry_rollup_state": "SELECT,INSERT,UPDATE", "telemetry_generation_identity": "SELECT,INSERT",
                "telemetry_rollup_session": "SELECT,INSERT,UPDATE", "telemetry_player_day": "SELECT,INSERT,UPDATE",
                "telemetry_cohort_day": "SELECT,INSERT,UPDATE", "telemetry_cohort_member": "SELECT,INSERT,UPDATE",
                "telemetry_interval": "SELECT", "telemetry_config": "SELECT",
                "telemetry_progression_context": "SELECT", "telemetry_progression_configuration": "SELECT", "telemetry_identity_registry": "SELECT",
                "telemetry_identity_association": "SELECT", review_tables[0]: "SELECT", review_tables[1]: "SELECT",
                tables[0]: "SELECT,INSERT,UPDATE", tables[1]: "SELECT,INSERT", tables[2]: "SELECT,INSERT", tables[3]: "SELECT,INSERT",
                "telemetry_rollup_incident_coverage": "SELECT,INSERT", "telemetry_rollup_incident": "SELECT,INSERT"},
            3: {"telemetry_rollup_state": "SELECT", "telemetry_generation_identity": "SELECT", tables[2]: "SELECT", tables[3]: "SELECT",
                "telemetry_rollup_incident_coverage": "SELECT", "telemetry_rollup_incident": "SELECT"},
            4: {"telemetry_interval": "SELECT",
                "telemetry_progression_context": "SELECT", "telemetry_progression_configuration": "SELECT", review_tables[0]: "SELECT,INSERT", review_tables[1]: "SELECT,INSERT"},
        }
        for index, permissions in grants.items():
            for table, permission in permissions.items():
                query(f"GRANT {permission} ON `{database}`.`{table}` TO %s@'%%'", (users[index],))
            adapters.append(PyMySQLRollupDatabase(PyMySQLConnectionFactory(ConnectionSettings(host="127.0.0.1",
                port=int(environment["DB_PORT"]), database=database, user=users[index], password=password))))
        rollup, reporter, reviewer = adapters
        result_through = outcome_through
        recovery_rows = query("SELECT boot_id,process_id,MIN(record_seq) AS first_seq FROM telemetry_interval WHERE ingest_id>%s "
            "AND ingest_id<=%s GROUP BY boot_id,process_id", (outcome_origin, result_through))
        assert len(recovery_rows) == 1 and recovery_rows[0]["first_seq"] == 1
        assert all(recovery_rows[0][name] == outcome_producer[name] for name in ("boot_id", "process_id"))
        through = control_through
        control_recovery = query("SELECT boot_id,process_id,MIN(record_seq) AS first_seq FROM telemetry_interval WHERE ingest_id>%s "
            "AND ingest_id<=%s GROUP BY boot_id,process_id", (control_origin, control_through))
        assert len(control_recovery) == 1 and control_recovery[0]["first_seq"] == 1
        assert all(control_recovery[0][name] == pvp_producer[name] for name in ("boot_id", "process_id"))
        def prepare(scope, *, origin=0, watermark=through):
            # Resume the maintained committed cursor in bounded invocations.
            # Keep default byte/page limits as this longer journey adds rows.
            previous = origin
            for invocation in range(1, 65):
                try:
                    completed = RollupEngine(rollup).run(scope, through_ingest_id=watermark, origin_ingest_id=origin,
                        bounds=RollupBounds(page_size=64, max_rows=128, max_runtime_s=60))
                except BoundsExceeded as error:
                    assert str(error) == "rollup invocation exceeded max_rows before reaching snapshot", str(error)
                    cursor = rollup.read_state(scope)["input_watermark"]
                    assert previous < cursor <= watermark
                    previous = cursor
                else:
                    assert completed.complete and completed.final_cursor == watermark
                    receipt.setdefault("bounded_preparation", []).append(dict(generation=scope.generation,
                        definition=scope.definition_version, invocations=invocation, max_rows_per_invocation=128,
                        max_bytes_per_invocation=RollupBounds().max_total_bytes, input_origin=origin, input_watermark=watermark))
                    prepared = (rollup.read_progression_source(scope) if scope.definition_version == 9 else
                        rollup.read_battle_source(scope))
                    source_count = len(prepared.inputs) if scope.definition_version == 9 else len(prepared.facts)
                    reserved_bytes = (progression_publication.HEADER_BYTE_BOUND +
                        (prepared.header["source_fact_count"] + prepared.header["reference_count"] + 2) *
                        (progression_publication.INPUT_ROW_BYTE_BOUND + progression_publication.PUBLICATION_INPUT_BYTE_BOUND)
                        if scope.definition_version == 9 else prepared.reserved_bytes)
                    print(json.dumps(dict(phase="source_prepared", definition=scope.definition_version,
                        generation=scope.generation, input_origin=origin, input_watermark=watermark,
                        retained_inputs=source_count, reserved_bytes=reserved_bytes)), flush=True)
                    return
            raise AssertionError("bounded source preparation did not complete")

        fields = battle_source.SOURCE_COLUMNS[13]
        expected_controls = {row["ingest_id"]: {name: row[name] for name in fields} for row in controls}
        # Adjacent windows preserve every source row while separating the
        # earlier outage producers from the complete recovery-study prefix.
        # SQL buffering and publication share the original 32 MiB budget.
        control_windows = ((0, control_origin), (control_origin, control_through))
        unreviewed_controls, prepared_controls = [], []
        for generation, (origin, watermark) in enumerate(control_windows, 1):
            unreviewed_scope = replace(target_scope, generation=generation)
            rollup.reserve_identity_generation(unreviewed_scope.scope_tuple, None)
            prepare(unreviewed_scope, origin=origin, watermark=watermark)
            retained = rollup.read_battle_source(unreviewed_scope)
            assert {key: row for key, row in expected_controls.items() if origin < key <= watermark} == {
                row["ingest_id"]: row for row in retained.facts if row["record_kind"] == 13}
            assert rollup.publish_generation(unreviewed_scope, bounds=RollupBounds(max_runtime_s=60))["status"] == "published"
            unreviewed = reporter.read_report(unreviewed_scope, "battle_control_states", max_rows=1024)
            assert unreviewed.rows and not unreviewed.truncated
            assert all(row["qualified_status_usec"] == [None] * 8 for row in unreviewed.rows)
            unreviewed_controls.append((unreviewed_scope, unreviewed))
            prepared_controls.append(retained)
        # The real outage is reviewed as uncertainty. Retained report prefixes
        # must never turn its unobserved activity into measured zero duration.
        # Review the entire retained association prefix, including kind-10
        # clocks before the first control. Independent worker/lifecycle evidence
        # witnesses delivery; an unclosed producer still has an unknown tail.
        times = [row[name] for window in prepared_controls for row in window.facts for name in row if
            name.endswith("utc_usec") and row[name] is not None and row[name] != control_contract.UTC_UNKNOWN]
        evidence_digest = hashlib.sha256(json.dumps(receipt["worker_outage_evidence"], sort_keys=True).encode()).hexdigest()
        loss = dict(incident.template(6)["incidents"][0], producer_boot_id=resumed_producer["boot_id"],
            producer_process_id=resumed_producer["process_id"], start_utc_usec=outage_start-1_000_003,
            end_utc_usec=outage_end+1_000_003,
            backlog_disposition="abandoned", observation_provenance="original_observation",
            evidence_digest=evidence_digest)
        losses = [loss]
        for observation in receipt["worker_outage_evidence"]["observations"]:
            if observation["unknown_after_last_sample"]:
                losses.append(dict(incident.template(6)["incidents"][0], incident_id=len(losses)+1,
                    producer_boot_id=observation["boot_id"], producer_process_id=observation["process_id"],
                    start_utc_usec=observation["observed_utc_usec"], end_utc_usec=None,
                    first_record_seq=observation["last_committed_record_seq"]+1, last_record_seq=None,
                    observation_provenance="original_observation", evidence_digest=evidence_digest))
        packet = dict(incident.template(6), incidents=losses, environment_id=target_scope.environment_id,
            season_id=target_scope.season_id, reviewer_token="a" * 64, review_evidence_digest=evidence_digest,
            reviewed_from_utc_usec=min(times)-1, reviewed_through_utc_usec=max(times)+1)
        reviewer.register_incident_packet(packet)
        reviewed_states, reviewed_operations = [], []
        for generation, (origin, watermark) in enumerate(control_windows, 3):
            target_scope = replace(target_scope, generation=generation)
            rollup.reserve_identity_generation(target_scope.scope_tuple, None)
            prepare(target_scope, origin=origin, watermark=watermark)
            retained = rollup.read_battle_source(target_scope)
            assert {key: row for key, row in expected_controls.items() if origin < key <= watermark} == {
                row["ingest_id"]: row for row in retained.facts if row["record_kind"] == 13}
            assert rollup.publish_generation(target_scope, bounds=RollupBounds(max_runtime_s=60))["status"] == "published"
            states = reporter.read_report(target_scope, "battle_control_states", max_rows=1024)
            operations = reporter.read_report(target_scope, "battle_control_operations", max_rows=1024)
            assert not states.truncated and not operations.truncated
            reviewed_states.extend(states.rows)
            reviewed_operations.extend(operations.rows)
            assert states.coverage.incident_coverage["quality_flags"] & incident.QUALITY_INCIDENT_GAP
            assert states.coverage.incident_coverage["zero_activity_implied"] is False
        assert len(reviewed_states) + len(reviewed_operations) == len(controls)
        assert expected_controls == {
            row["ingest_id"]: {name: row[name] for name in fields} for row in (*reviewed_states, *reviewed_operations)}
        assert all(row["proven_action_restriction_usec"] is None and row["caster_attributed_duration_usec"] is None
            for row in reviewed_states)
        gap_states = [row for row in reviewed_states if row["publication_quality_flags"] & incident.QUALITY_INCIDENT_GAP]
        assert all(row["qualified_status_usec"] == [None] * 8 for row in gap_states)
        failed_control_rows = [row for row in (*reviewed_states, *reviewed_operations) if
            (row["ctl_boot_id"], row["ctl_process_id"]) == (failed_producer["boot_id"], failed_producer["process_id"])]
        active_baselines = [row for row in failed_control_rows if row["ctl_kind"] == 2 and row["ctl_after_mask"] & 4]
        # The writer can lose every endpoint after an admitted active baseline.
        # No retained prefix then overlaps the reviewed tail, so gap_states may
        # be empty. Qualify the actual open baseline and missing endpoint rather
        # than requiring the worker to have persisted a row inside the outage.
        assert active_baselines and all(row["qualified_status_usec"] == [None] * 8 for row in active_baselines)
        assert all(row["record_seq"] <= failed_producer["last_committed_record_seq"] for row in failed_control_rows)
        tail = next(row for row in states.coverage.incident_coverage["incidents"] if
            (row["producer_boot_id"], row["producer_process_id"]) ==
            (failed_producer["boot_id"], failed_producer["process_id"]) and row["end_utc_usec"] is None)
        assert tail["first_record_seq"] == failed_producer["last_committed_record_seq"] + 1
        assert tail["observation_provenance"] == "original_observation"
        receipt["native_outage_control_evidence"] = dict(active_baseline_ingest_ids=[row["ingest_id"] for row in active_baselines],
            active_baseline_durations_unknown=True, overlapping_gap_state_count=len(gap_states),
            last_committed_record_seq=failed_producer["last_committed_record_seq"],
            first_unknown_record_seq=tail["first_record_seq"], missing_endpoints_not_synthesized=True)
        for unreviewed_scope, unreviewed in unreviewed_controls:
            assert reporter.read_report(unreviewed_scope, "battle_control_states", max_rows=1024).rows == unreviewed.rows
        receipt["control_publication_windows"] = [dict(origin=origin, watermark=watermark,
            unreviewed_generation=index+1, reviewed_generation=index+3) for index, (origin, watermark) in enumerate(control_windows)]
        receipt["adjacent_control_windows_cover_original_source"] = True
        # Tampering with bindings alone must first fail the original digest.
        # A separately sealed in-memory negative copy then proves missing
        # configuration stays NULL. Original stored source is never changed.
        try:
            battle_publication.build_publication(replace(retained, configurations=(None,) * len(retained.facts)),
                None, states.coverage.incident_coverage)
        except battle_publication.PublicationError as error:
            assert str(error) == "battle_public_source_changed", str(error)
        else:
            raise AssertionError("changed configuration escaped the sealed source digest")
        unknown_inputs = [battle_source.retain_input(row, target_scope.scope_tuple, quality)
            for row, quality in zip(retained.facts, retained.projection_qualities, strict=True)]
        unknown_header = battle_source.advance_header(battle_source.initial_header(target_scope.scope_tuple,
            retained.header["input_origin"]), unknown_inputs, through)
        unknown_config = battle_source.verify_source(unknown_header, unknown_inputs,
            expected_scope=target_scope.scope_tuple, expected_watermark=through, expected_origin=retained.header["input_origin"])
        unknown_publication = battle_publication.build_publication(unknown_config, None, states.coverage.incident_coverage)
        unknown_states = [battle_publication.decode_row(target_scope.scope_tuple, row)
            for row in unknown_publication.rows if row["row_kind"] == 8]
        assert unknown_states and all(row["qualified_status_usec"] == [None] * 8 for row in unknown_states)
        receipt["negative_evidence"] = dict(unreviewed_native_generation=unreviewed_scope.generation,
            unreviewed_state_rows=sum(len(report.rows) for _, report in unreviewed_controls), immutable_unreviewed_generation=True,
            missing_configuration_publication_probe_rows=len(unknown_states),
            configuration_digest_tamper_refused=True,
            real_private_writer_loss_preserved=True,
            context_clock_capacity="native executable and definition-7 regression fault cases in the maintained command")
        live = receipt["ordinary_pvp"]
        live_states = [row for row in states.rows if row["ctl_target_actor_pid"] == live["target_pid"] and
            (row["boot_id"], row["process_id"]) == (live["producer"]["boot_id"], live["producer"]["process_id"])]
        prefixes = [row for row in live_states if row["ctl_kind"] == 3 and row["observed_prefix_usec"] > 0]
        diagnostic_fields = ("record_seq", "ctl_boundary", "ctl_before_mask", "ctl_after_mask", "qualified_duration_mask",
            "clock_status", "target_link_status", "chain_status", "configuration_status", "publication_quality_flags")
        live["control_prefix_diagnostics"] = [{name: row[name] for name in diagnostic_fields} for row in prefixes]
        for phase in ("solo", "group"):
            first, last = live[phase+"_ingest_range"]
            phase_rows = [row for row in prefixes if first <= row["ingest_id"] <= last]
            assert any(row["qualified_duration_mask"] == 255 and row["ctl_before_mask"] for row in phase_rows), (
                phase, [(row["ctl_boundary"], row["clock_status"], row["target_link_status"],
                    row["publication_quality_flags"]) for row in phase_rows])
        qualified = [row for row in prefixes if row["qualified_duration_mask"] == 255]
        per_status = [sum(row["qualified_status_usec"][bit] for row in qualified) for bit in range(8)]
        assert all(per_status[bit] > 0 for bit in (0, 3, 4))
        selected = sorted((row["ctl_start_usec"], row["ctl_at_usec"]) for row in qualified if row["ctl_before_mask"])
        union, last = 0, 0
        for first, end in selected:
            union += max(0, end - max(first, last))
            last = max(last, end)
        assert union == sum(row["observed_prefix_usec"] for row in qualified if row["ctl_before_mask"])
        assert sum(per_status) > union > 0
        assert any(row["ctl_kind"] == 3 and row["ctl_boundary"] == 5 for row in live_states)
        assert all(any(row["record_seq"] == live[name + "_record_seq"] and
            row["qualified_duration_mask"] == 255 and row["ctl_before_mask"] for row in qualified)
            for name in ("expiry", "cure"))
        assert any(row["ctl_boundary"] == 5 and row["ctl_before_mask"] and
            row["qualified_duration_mask"] == 255 for row in qualified)
        target_operations = [row for row in operations.rows if row["ctl_target_actor_pid"] == live["target_pid"] and
            (row["boot_id"], row["process_id"]) == (live["producer"]["boot_id"], live["producer"]["process_id"])]
        live_operations = [row for row in target_operations if row["ctl_source_actor_pid"] in (live["caster_pid"], live["healer_pid"])]
        live["other_source_operation_count"] = len(target_operations)-len(live_operations)
        assert live_operations and all(row["ctl_source_actor_kind"] == 1 and row["ctl_flags"] & 7 == 0 for row in live_operations), [
            {name: row[name] for name in ("record_seq", "ctl_source_actor_pid", "ctl_source_actor_kind", "ctl_family", "ctl_flags")}
            for row in live_operations]
        assert any(row["ctl_source_group_size"] == 1 and row["ctl_target_group_size"] == 1 for row in live_operations)
        assert any(row["ctl_source_group_size"] == 3 and row["ctl_target_group_size"] == 2 for row in live_operations)
        group_contexts = [row for row in retained.facts if row["record_kind"] == 10 and
            row["battle_actor_id"] in (live["caster_pid"], live["healer_pid"]) and
            (row["boot_id"], row["process_id"]) == (live["producer"]["boot_id"], live["producer"]["process_id"]) and
            row["battle_actor_group_size"] in (2, 3)]
        assert {row["battle_actor_group_size"] for row in group_contexts} == {2, 3}
        assert any(a["battle_actor_group_key"] == b["battle_actor_group_key"] and
            a["battle_actor_group_size"] == 3 and b["battle_actor_group_size"] == 2 and
            a["battle_actor_group_revision"] < b["battle_actor_group_revision"] for a in group_contexts for b in group_contexts)
        assert any(row["qualified_duration_mask"] == 255 and row["ctl_before_mask"] & 17 == 17 for row in prefixes)
        witness = live["independent_delivery_witness"]
        assert witness["registered_utc_usec"] <= min(row["ctl_start_utc_usec"] for row in prefixes)
        assert max(row["ctl_decision_utc_usec"] for row in prefixes) <= witness["observed_utc_usec"]
        live.update(qualified_prefixes=len(qualified), qualified_status_usec=per_status,
            selected_status_union_usec=union, disjoint_prefix_union_matches=True,
            ordinary_operation_count=len(live_operations), unknown_prefixes=len(prefixes)-len(qualified),
            independent_review_range=[packet["reviewed_from_utc_usec"], packet["reviewed_through_utc_usec"]])
        # The complete recovery producer publishes reviewed dimensions, typed
        # evidence and denominators atomically under independent version 8.
        # Keep the preceding control window and its default budgets unchanged.
        result_scope = RollupTarget(8, 1, target_scope.environment_id, target_scope.season_id)
        result_tables = tuple(battle_source.table(table, result_scope.scope_tuple) for table in (
            "telemetry_battle_source", "telemetry_battle_input", "telemetry_rollup_battle_coverage", "telemetry_rollup_battle_row"))
        result_review = incident.schema_contract(7)[2:]
        extra_grants = {
            2: {result_tables[0]: "SELECT,INSERT,UPDATE", result_tables[1]: "SELECT,INSERT",
                result_tables[2]: "SELECT,INSERT", result_tables[3]: "SELECT,INSERT", result_review[0]: "SELECT", result_review[1]: "SELECT"},
            3: {result_tables[2]: "SELECT", result_tables[3]: "SELECT"},
            4: {result_review[0]: "SELECT,INSERT", result_review[1]: "SELECT,INSERT"},
        }
        for index, permissions in extra_grants.items():
            for table, permission in permissions.items():
                query(f"GRANT {permission} ON `{database}`.`{table}` TO %s@'%%'", (users[index],))
        frozen_control = reporter.read_report(target_scope, "battle_control_states", max_rows=1024)
        rollup.reserve_identity_generation(result_scope.scope_tuple, None)
        prepare(result_scope, origin=outcome_origin, watermark=result_through)
        assert rollup.publish_generation(result_scope, bounds=RollupBounds(max_runtime_s=60))["status"] == "published"
        unreviewed_results = reporter.read_report(result_scope, "battle_outcomes", max_rows=1024)
        assert unreviewed_results.rows and not unreviewed_results.truncated
        assert all(not row["event_evidence_qualified"] for row in unreviewed_results.rows)
        result_scope = replace(result_scope, generation=2)
        rollup.reserve_identity_generation(result_scope.scope_tuple, None)
        prepare(result_scope, origin=outcome_origin, watermark=result_through)
        result_source = rollup.read_battle_source(result_scope)
        result_times = [row[name] for row in result_source.facts for name in row if
            name.endswith("utc_usec") and row[name] is not None and row[name] != control_contract.UTC_UNKNOWN]
        result_packet = dict(packet, registry_schema_version=7,
            reviewed_from_utc_usec=min(packet["reviewed_from_utc_usec"], min(result_times)-1),
            reviewed_through_utc_usec=max(packet["reviewed_through_utc_usec"], max(result_times)+1),
            incidents=[dict(loss, record_kind_mask=incident.schema_contract(7)[0]) for loss in packet["incidents"]])
        reviewer.register_incident_packet(result_packet)
        assert rollup.publish_generation(result_scope, bounds=RollupBounds(max_runtime_s=60))["status"] == "published"
        outcomes = reporter.read_report(result_scope, "battle_outcomes", max_rows=1024)
        comparisons = reporter.read_report(result_scope, "battle_build_comparisons", max_rows=1024)
        assert not outcomes.truncated and not comparisons.truncated
        assert outcomes.coverage.battle_coverage == comparisons.coverage.battle_coverage
        assert comparisons.rows == reporter.read_report(result_scope, "battle_build_points", max_rows=1024).rows
        assert comparisons.rows and all(row["comparison"] == battle_publication._comparison_value(row) for row in comparisons.rows)
        native = receipt["ordinary_outcomes"]
        producer = (native["producer"]["boot_id"], native["producer"]["process_id"])
        associations = reporter.read_report(result_scope, "battle_associations", max_rows=1024)
        assert not associations.truncated and associations.coverage.battle_coverage == outcomes.coverage.battle_coverage
        support = next(row for row in associations.rows if row["ingest_id"] == native["support_ingest_id"])
        raw_support = query("SELECT * FROM telemetry_interval WHERE ingest_id=%s", (native["support_ingest_id"],))[0]
        assert all(support[name] == raw_support[name] for name in battle_source.SOURCE_COLUMNS[10])
        assert (support["boot_id"], support["process_id"]) == producer and support["record_seq"] == native["support_record_seq"]
        assert support["battle_relation"] == 2 and support["battle_actor_pid"] == receipt["ordinary_pvp"]["healer_pid"]
        assert support["battle_related_actor_id"] == native["equipment_actor_pid"]
        departure = next(row for row in associations.rows if row["ingest_id"] == native["support_departure_ingest_id"])
        assert (departure["boot_id"], departure["process_id"]) == producer
        assert departure["battle_actor_pid"] == support["battle_actor_pid"] and (
            support["battle_actor_group_size"], departure["battle_actor_group_size"]) == (2, 1)
        observed = [row for row in outcomes.rows if (row["boot_id"], row["process_id"]) == producer]
        for field, kind in (("movement_sequence", 2), ("escape_sequence", 4), ("death_sequence", 1), ("objective_sequence", 6)):
            point = next(row for row in observed if row["bout_sequence"] == native[field])
            assert point["bout_kind"] == kind and point["event_evidence_qualified"], (
                field, point["target_link_status"], point["clock_status"], point["chain_status"], point["objective_status"],
                point["configuration_status"], point["publication_quality_flags"], point["event_quality_flags"])
            assert not point["whole_battle_victory_implied"] and not point["full_zone_clear_implied"]
            if kind in (1, 2, 4):
                assert point["battle_context_qualified"] and point["observed_pre_roster"]["observed_pre_owner_count"] >= 2
            else:
                assert point["bout_operation_id"] == native["objective_operation_id"] and point["bout_source_object_uid"] == native["objective_source_uid"]
        gear = [row for row in comparisons.rows if (row["boot_id"], row["process_id"]) == producer and
            row["bctx_actor_kind"] == 1 and row["bctx_actor_id"] == native["equipment_actor_pid"] and
            row["bctx_sequence"] in native["equipment_point_sequences"]]
        assert len(gear) == 2 and all(row["point_context_verified"] and row["comparison"]["dimensions"]["equipment"]["usable_for_matching"] for row in gear), (
            [(row["link_status"], row["point_clock_status"], row["configuration_status"], row["publication_quality_flags"]) for row in gear])
        assert gear[0]["bctx_equipment_digest"] != gear[1]["bctx_equipment_digest"]
        match = battle_comparison.compare_points(*sorted(gear, key=lambda row: row["bctx_sequence"]), ("level", "classes", "equipment"))
        assert match["matched_dimensions"] == ["level", "classes"] and match["different_dimensions"] == ["equipment"]
        assert not match["unknown_dimensions"] and not match["combat_strength_equivalence_implied"]
        for table in (*result_tables[:2], "telemetry_interval", "telemetry_config",
            "telemetry_progression_context", "telemetry_progression_configuration", *result_review):
            try:
                reporter._execute("SELECT COUNT(*) AS n FROM " + table)
            except Exception as error:
                assert getattr(error.__cause__, "args", (None,))[0] == 1142 or getattr(error, "args", (None,))[0] == 1142
            else:
                raise AssertionError("result report role can read private input")
        assert reporter.read_report(target_scope, "battle_control_states", max_rows=1024) == frozen_control
        frozen_results = reporter.read_report(replace(result_scope, generation=1), "battle_outcomes", max_rows=1024)
        assert frozen_results.rows == unreviewed_results.rows and all(not row["event_evidence_qualified"] for row in frozen_results.rows)
        raw_results = query("SELECT * FROM telemetry_interval WHERE record_kind=14 AND ingest_id>%s AND ingest_id<=%s ORDER BY ingest_id",
            (outcome_origin, result_through))
        assert {row["ingest_id"]: {name: row[name] for name in battle_source.SOURCE_COLUMNS[14]} for row in raw_results} == {
            row["ingest_id"]: row for row in result_source.facts if row["record_kind"] == 14}
        # A separately verified missing-binding value fixture cannot qualify
        # the actual gameplay. It never alters captured or retained SQL rows.
        unknown_inputs = [battle_source.retain_input(row, result_scope.scope_tuple, quality) for row, quality in
            zip(result_source.facts, result_source.projection_qualities, strict=True)]
        unknown_header = battle_source.advance_header(battle_source.initial_header(result_scope.scope_tuple,
            result_source.header["input_origin"]), unknown_inputs, result_source.header["input_watermark"])
        unknown_source = battle_source.verify_source(unknown_header, unknown_inputs, expected_scope=result_scope.scope_tuple,
            expected_watermark=result_source.header["input_watermark"], expected_origin=result_source.header["input_origin"])
        unknown_output = battle_publication.build_publication(unknown_source, None, outcomes.coverage.incident_coverage)
        assert unknown_output.header["qualified_result_evidence_count"] == unknown_output.header["qualified_build_points"] == 0
        receipt["comparability_outcome_publication"] = dict(definition=8, incident_schema=7,
            source_origin=result_source.header["input_origin"], source_watermark=result_source.header["input_watermark"],
            source_digest=result_source.header["source_digest"].hex(),
            snapshot_digest=outcomes.coverage.battle_coverage["snapshot_digest"], exact_retained_results=len(raw_results),
            published_result_rows=len(outcomes.rows), published_comparison_points=len(comparisons.rows),
            qualified_result_evidence=outcomes.coverage.battle_coverage["qualified_result_evidence_count"],
            qualified_battle_context=outcomes.coverage.battle_coverage["qualified_result_context_count"],
            qualified_build_points=outcomes.coverage.battle_coverage["qualified_build_points"],
            qualified_gear_change_comparison=match, unreviewed_generation_immutable=True, definition_seven_unchanged=True,
            exact_effective_support_relation=True, changing_support_group_participation=True, generic_buff_origin_unknown=True,
            restricted_role=True, missing_configuration_fixture=True, missing_configuration_fixture_mutates_sql=False,
            same_atomic_coverage=True, complete_win_or_zone_clear_claimed=False, controller_identity_unknown=True)
        # The native progression window uses independent definition 9. The
        # explicit synthetic-controller declaration belongs to this owned
        # gameplay fixture; account names and network addresses prove no link.
        import test_telemetry_identity_history as identity_review_fixture
        from scripts.telemetry import identity_history
        progression_target = RollupTarget(9, 1, *progress_scope)
        private_progression = ("telemetry_progression_source_v9", "telemetry_progression_input_v9", "telemetry_progression_reference_v9")
        public_progression = ("telemetry_rollup_progression_coverage_v9", "telemetry_rollup_progression_row_v9")
        progression_incidents = incident.schema_contract(8)[2:]
        progression_grants = {
            2: {private_progression[0]: "SELECT,INSERT,UPDATE",
                **{table: "SELECT,INSERT" for table in (*private_progression[1:], *public_progression)},
                **{table: "SELECT" for table in progression_incidents}},
            3: {table: "SELECT" for table in public_progression},
            4: {"telemetry_identity_registry": "SELECT,INSERT", "telemetry_identity_association": "SELECT,INSERT",
                "telemetry_identity_reviewer": "SELECT", **{table: "SELECT,INSERT" for table in progression_incidents}},
        }
        for index, permissions in progression_grants.items():
            for table, permission in permissions.items():
                query(f"GRANT {permission} ON `{database}`.`{table}` TO %s@'%%'", (users[index],))
        query(f"GRANT SELECT (environment_id,season_id,account_token) ON `{database}`.telemetry_account_token TO %s@'%%'", (users[4],))
        principal_name = reviewer._execute("SELECT CURRENT_USER() AS principal")[0][0]["principal"]
        query("INSERT INTO telemetry_identity_reviewer VALUES(%s,%s,%s,%s,1)",
            (*progress_scope, principal_name, bytes.fromhex("b"*64)))
        first_clock, through_clock = receipt["ordinary_progression"]["reviewed_clock_range"]
        declared_tokens = sorted({value for value in receipt["ordinary_progression"]["account_tokens"].values()})
        registry_packet = identity_review_fixture.packet([
            identity_review_fixture.association(index+1, value, 901, first_clock, through_clock)
            for index, value in enumerate(declared_tokens)], environment_id=progress_scope[0], season_id=progress_scope[1],
            reviewed_from_utc_usec=first_clock, reviewed_through_utc_usec=through_clock,
            reviewed_at_utc_usec=through_clock, review_evidence_digest=hashlib.sha256(
                b"owned native gameplay fixture: explicitly declared controller 901 for two synthetic accounts").hexdigest())
        assert reviewer.register_identity_packet(registry_packet)["status"] == "registered"
        registry = identity_history.Registry.from_packet(registry_packet)
        progression_producers = {(row["boot_id"], row["process_id"]) for row in query(
            "SELECT DISTINCT boot_id,process_id FROM telemetry_interval WHERE ingest_id>%s AND ingest_id<=%s",
            (progress_origin, progress_through))}
        progression_losses, progression_witnesses = [], []
        delivery_digest = hashlib.sha256(json.dumps(receipt["worker_outage_evidence"], sort_keys=True).encode()).hexdigest()
        failed_progression_producer = receipt["ordinary_progression"]["failed_producer"]
        for witness in receipt["worker_outage_evidence"]["observations"]:
            if (witness["boot_id"], witness["process_id"]) not in progression_producers:
                continue
            progression_witnesses.append(witness)
            if (witness["boot_id"], witness["process_id"]) == (failed_progression_producer["boot_id"], failed_progression_producer["process_id"]):
                start_clock, end_clock = receipt["ordinary_progression"]["private_writer_outage_clock_range"]
                progression_losses.append(dict(incident.template(8)["incidents"][0], incident_id=len(progression_losses)+1,
                    producer_boot_id=witness["boot_id"], producer_process_id=witness["process_id"],
                    start_utc_usec=start_clock-1_000_003, end_utc_usec=end_clock+1_000_003,
                    backlog_disposition="abandoned", observation_provenance="original_observation", evidence_digest=delivery_digest))
            else:
                assert all(witness[name] == 0 for name in ("rejected_detail_admissions", "rejected_control_admissions",
                    "quarantined_records", "invalid_records", "conflict_records", "sequence_gap_count", "unclosed_tail_count"))
            if witness["unknown_after_last_sample"]:
                progression_losses.append(dict(incident.template(8)["incidents"][0], incident_id=len(progression_losses)+1,
                    producer_boot_id=witness["boot_id"], producer_process_id=witness["process_id"],
                    start_utc_usec=witness["observed_utc_usec"], end_utc_usec=None,
                    first_record_seq=witness["last_committed_record_seq"]+1, last_record_seq=None,
                    observation_provenance="original_observation", evidence_digest=delivery_digest))
        assert len(progression_witnesses) == len(progression_producers)
        reviewed_progression = dict(incident.template(8), incidents=progression_losses,
            environment_id=progress_scope[0], season_id=progress_scope[1],
            reviewed_from_utc_usec=first_clock, reviewed_through_utc_usec=through_clock,
            reviewer_token="a"*64, review_evidence_digest=hashlib.sha256(json.dumps(
                receipt["ordinary_progression"], sort_keys=True).encode()).hexdigest())
        reviewer.register_incident_packet(reviewed_progression)
        # Adjacent immutable generations cover the whole native journey under
        # the original 32 MiB limit. Context churn is preserved, never filtered
        # away or combined into a population-wide rate or milestone estimate.
        specifications = receipt["ordinary_progression"]["publication_windows"]
        assert specifications[0]["origin"] == progress_origin and specifications[-1]["watermark"] == progress_through
        assert all(left["watermark"] == right["origin"] for left, right in zip(specifications, specifications[1:]))
        published_progression, progression_windows = {}, []
        for specification in specifications:
            target = replace(progression_target, generation=specification["generation"])
            rollup.reserve_identity_generation(target.scope_tuple, registry.registry_version)
            prepare(target, origin=specification["origin"], watermark=specification["watermark"])
            assert rollup.publish_generation(target, bounds=RollupBounds(max_runtime_s=60))["status"] == "published"
            reports = {name: reporter.read_report(target, name, max_rows=progression_publication.MAX_OUTPUT_ROWS)
                for name in progression_publication.ROW_KINDS}
            assert all(snapshot.rows and not snapshot.truncated for snapshot in reports.values())
            coverage = reports["progression_context"].coverage.progression_coverage
            for snapshot in reports.values():
                assert snapshot.coverage.progression_coverage == coverage
                assert all(not row.get("character_save_committed", False) and not row.get("XP_award_committed", False)
                    for row in snapshot.rows)
            retained = rollup.read_progression_source(target)
            assert retained.header["source_fact_count"] == coverage["source_fact_count"]
            published_progression[specification["name"]] = (target, reports)
            progression_windows.append(dict(specification, coverage=coverage,
                report_counts={name: len(snapshot.rows) for name, snapshot in reports.items()}))
        progression_reports = published_progression["milestones"][1]
        progression_coverage = progression_reports["progression_context"].coverage.progression_coverage
        assert progression_coverage["completed_milestone_count"] >= 2
        assert progression_coverage["unfinished_milestone_count"] > 0
        assert progression_coverage["qualified_full_stage_count"] > 0
        rotations = published_progression["rotation"][1]["character_rotation"].rows
        first_account = receipt["ordinary_progression"]["account_tokens"]["Progfirst"]
        assert any(row["basis"] == "account" and row["identity_token"] == first_account and
            row["sequential_switches"] for row in rotations)
        assert any(row["basis"] == "controller" and row["identity_token"] == 901 and
            (row["maximum_observed_simultaneous_sessions"] or 0) >= 2 for row in rotations)
        assert any(row["basis"] == "unknown_controller" for row in rotations)
        group_contexts = published_progression["group_decisions"][1]["progression_context"].rows
        assert any(row["pctx_assistance"] == 2 and row["pctx_eligible_group_size"] == 2 for row in group_contexts)
        assert reporter.read_report(target_scope, "battle_control_states", max_rows=1024) == frozen_control
        assert reporter.read_report(result_scope, "battle_outcomes", max_rows=1024).rows == outcomes.rows
        receipt["progression_publication"] = dict(definition=9, independent_incident_schema=8,
            native_source=True, restricted_role=True, source_origin=progress_origin, source_watermark=progress_through,
            coverage=progression_coverage, windows=progression_windows, adjacent_windows_cover_journey=True,
            dated_confirmed_controller_fixture=True, controller_links_inferred=False,
            unknown_controller_population_preserved=True, old_battle_generations_preserved=True,
            independent_delivery_witnesses=progression_witnesses,
            public_readback_uses_private_source=False, economic_authority_dependency_issue=487)
        rate_target, rate_reports = published_progression["comparable_rates"]
        rate_report = rate_reports["progression_portfolio"]
        assert not rate_report.truncated
        rate_pid = progress_pids["Progsecond"]
        subject = query("SELECT pctx_subject_id AS subject FROM telemetry_progression_context WHERE pctx_pid=%s LIMIT 1", (rate_pid,))[0]["subject"]
        qualified_rates = [row for row in rate_report.rows if row["basis"] == "character" and row["identity_token"] == subject and
            row["observed_earned_positive_xp"] > 0 and row["observed_earned_xp_per_connected_hour"] is not None]
        assert qualified_rates, "native progression window produced no comparable observed XP rate"
        uncertain_fight_rates = published_progression["uncertain_fight_exposure"][1]["progression_portfolio"].rows
        assert any(row["basis"] == "character" and row["identity_token"] == subject and
            row["observed_earned_positive_xp"] > 0 and "unclassified_stratum_exposure" in row["unknown"] and
            row["observed_earned_xp_per_heuristic_active_hour"] is None and
            row["observed_earned_xp_per_connected_hour"] is None for row in uncertain_fight_rates), \
            "native earlier fight cuts must remain retained with unknown rates"
        for target, reports in published_progression.values():
            for name, snapshot in reports.items():
                reread = reporter.read_report(target, name, max_rows=progression_publication.MAX_OUTPUT_ROWS)
                expected_status = (PUBLICATION_PUBLISHED if target.generation == specifications[-1]["generation"] else
                    PUBLICATION_SUPERSEDED)
                assert reread.coverage.publication_status == expected_status
                # Supersession changes freshness metadata, never the retained
                # report values, source bounds or atomic coverage snapshot.
                assert replace(reread, coverage=replace(reread.coverage,
                    publication_status=snapshot.coverage.publication_status)) == snapshot
        receipt["progression_publication"]["native_rate_window"] = dict(generation=rate_target.generation, source_origin=rate_origin,
            source_watermark=rate_through, qualified_character_rates=qualified_rates,
            coverage=rate_report.coverage.progression_coverage, earlier_generation_preserved=True)
        receipt["progression_publication"]["native_rate_window"].update(
            bounded_observed_prefix=True, complete_kill_rate_claimed=False,
            denominator_basis="connected", heuristic_active_rate_required=False,
            earlier_fight_awards_and_unknown_rates_retained=True)
        receipt.update(status="passed", raw_controls=len(controls), published_states=len(reviewed_states),
            published_operations=len(reviewed_operations), qualified_prefixes=states.coverage.battle_coverage["qualified_control_prefixes"],
            exact_retained_inputs=True, report_definition=7, private_incident_schema=6,
            report_uses_restricted_role=True, action_restriction_and_caster_duration_unknown=True,
            uncertainty_preserved=True, migration_steps=int(environment["TELEMETRY_REPOSITORY_MIGRATION_COUNT"]))
        result.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
        print(json.dumps({key: receipt[key] for key in ("status", "actual_running_server", "raw_controls", "published_states", "published_operations", "qualified_prefixes")}))
    finally:
        if receipt["status"] != "passed":
            failure = sys.exc_info()[1]
            receipt.update(status="failed", failure=str(failure))
            result.parent.mkdir(parents=True, exist_ok=True)
            result.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
            def diagnostic_value(value):
                if isinstance(value, bytes):
                    return {"bytes_hex": value.hex()}
                raise TypeError(type(value).__name__)
            (result.parent / (result.stem + "-failure-source.json")).write_text(json.dumps(dict(
                raw=query("SELECT * FROM telemetry_interval ORDER BY ingest_id"),
                configuration=query("SELECT * FROM telemetry_config"),
                progression_context=query("SELECT * FROM telemetry_progression_context ORDER BY boot_id,process_id,record_seq"),
                progression_configuration=query("SELECT * FROM telemetry_progression_configuration ORDER BY boot_id,process_id,record_seq")),
                default=diagnostic_value) + "\n", encoding="utf-8")
        for adapter in adapters:
            adapter.close()
        for user in users:
            query("DROP USER IF EXISTS %s@'%%'", (user,))
        root.close()
        drop_sql_fixture(environment, command, database)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sql-fixture", action="store_true")
    if not parser.parse_args().sql_fixture:
        parser.error("--sql-fixture and an owned disposable loopback database are required")
    run()
