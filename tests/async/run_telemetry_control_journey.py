#!/usr/bin/env python3
"""Real server control gameplay and publication in one fresh allow-listed DB."""
from __future__ import annotations

import argparse
from dataclasses import replace
import hashlib
import json
import os
from pathlib import Path
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
from scripts.telemetry import battle_publication, battle_source, control_contract, incident, outage
from scripts.telemetry.db_access import ConnectionSettings, PyMySQLConnectionFactory, PyMySQLRollupDatabase
from scripts.telemetry.rollup_definitions import RollupTarget
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
    receipt = dict(status="running", actual_running_server=True, synthetic_accounts=7,
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
        for table in ("telemetry_config", "telemetry_interval", "telemetry_quarantine"):
            query(f"GRANT SELECT,INSERT ON `{database}`.`{table}` TO %s@'%%'", (users[1],))
        query(f"GRANT SELECT,INSERT,UPDATE ON `{database}`.telemetry_session TO %s@'%%'", (users[1],))
        with tempfile.TemporaryDirectory(prefix="control-gameplay-") as temporary:
            runtime = Path(temporary)
            journey.make_fixture(runtime)
            # Restore ordinary starter damage: this journey needs living PvP
            # participants, rather than the shared fixture's fast NPC kill.
            objects = runtime / "areas_mini/mini.obj"
            objects.write_text(objects.read_text().replace("6 100 1 7 0 0 0 0", "6 1 6 7 0 0 0 0"))
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

            def boot():
                nonlocal process, output
                output = (runtime / "server.out").open("w")
                process = subprocess.Popen([str(runtime / "bin/server/dms"), "--minimal", "-s", str(port)],
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
                client.expect(" >", timeout=30)

            try:
                boot()
                characters = (("Ctlstaffacct", "Ctlstaff", "w"), ("Ctltargacct", "Ctltarget", "w"),
                    ("Pvpcastacct", "Pvpcast", "sorcerer"), ("Pvphealacct", "Pvpheal", "cleric"),
                    ("Pvpallyacct", "Pvpally", "w"), ("Pvpvictacct", "Pvpvictim", "w"),
                    ("Pvpmateacct", "Pvpmate", "w"))
                for account, character, class_name in characters:
                    client = journey.MudClient(port)
                    clients.append(client)
                    journey.create_character(client, account=account, character=character, class_name=class_name,
                        email=account.lower() + "@example.invalid")
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
                # A persistent, non-selected saving penalty makes the native
                # random gate reliably exercisable without bypassing it.
                query("INSERT INTO player_affects(pid,type,duration,flags,modifier,location) "
                    "SELECT pid,17,-1,512,20,20 FROM player_data WHERE name='Pvpvictim'")
                # Only persistent gameplay prerequisites are seeded. Every
                # positive observation below comes from the maintained server's
                # normal cast/attack/effect/lifecycle paths, never telemetry APIs.
                for character, spells in (("Pvpcast", (100, 102)), ("Pvpheal", (4, 14))):
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
                pvp = {character: reconnect(account, character) for account, character, _ in characters[2:]}
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
                    issue(client, "wake")
                    issue(client, "stand")
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
                victim.send("quit")
                victim.expect("ACCOUNT MENU", timeout=30)
                until(lambda: any(row["ctl_kind"] == 3 and row["ctl_boundary"] == 5 for row in live_controls()),
                    "ordinary participant lifecycle cut absent")
                receipt["ordinary_pvp"] = dict(producer=pvp_producer, target_pid=pids["Pvpvictim"],
                    caster_pid=pids["Pvpcast"], healer_pid=pids["Pvpheal"], solo_ingest_range=[solo_start+1, solo_end],
                    group_ingest_range=[group_start+1, int(query("SELECT MAX(ingest_id) AS n FROM telemetry_interval")[0]["n"])], refresh=True, expiry=True, cure=True,
                    changing_group_participation=True, observed_departure=True,
                    expiry_record_seq=expiry["record_seq"], cure_record_seq=cure["record_seq"],
                    positive_capture="ordinary untrusted cast and combat paths",
                    seeded_prerequisites="level, HP, memorized spells, non-selected saving penalty",
                    native_blindness_prerequisite_ticks=32,
                    supervised_combat_reset="native tranquilize between cases and before quit")
                receipt.update(binary_sha256=binary_hash, disabled_capture_zero=True, accepted_major_paralysis=True,
                    native_overlap=True, native_cure=True, native_expiry=True, equipment_apply_remove=True,
                    save_no_false_transition=True, copyover_failure_and_exec=True, logical_sessions_preserved=True,
                    private_sql_outage_game_save=True, private_sql_recovery=True,
                    private_sql_recovery_mode="operator lifecycle restart after terminal circuit",
                    terminal_circuit_and_abandoned_backlog=True, failed_producer_evidence=failed_producer,
                    copyover_fresh_control_baseline=True,
                    save_latency_ns={"telemetry_off": off_save, "telemetry_on_status_present": on_save})
                stop()
                receipt["worker_outage_evidence"] = outage.read_evidence(runtime / "telemetry-ledger")
                assert receipt["worker_outage_evidence"]["ledger_version"] == 5
                witness = next(row for row in receipt["worker_outage_evidence"]["observations"] if
                    (row["boot_id"], row["process_id"]) == (pvp_producer["boot_id"], pvp_producer["process_id"]))
                assert witness["phase"] == "clean_drained" and not witness["unknown_after_last_sample"]
                assert all(witness[name] == 0 for name in ("rejected_detail_admissions", "rejected_control_admissions",
                    "quarantined_records", "invalid_records", "conflict_records", "sequence_gap_count", "unclosed_tail_count"))
                receipt["ordinary_pvp"]["independent_delivery_witness"] = witness
            except Exception:
                result.parent.mkdir(parents=True, exist_ok=True)
                (result.parent / (result.stem + "-failure-controls.json")).write_text(
                    json.dumps(query("SELECT * FROM telemetry_interval WHERE record_kind IN (10,13) ORDER BY ingest_id"), default=str, indent=2) + "\n")
                (result.parent / (result.stem + "-server-failure.log")).write_text(
                    (runtime / "server.out").read_text(errors="replace") + "\n" + journey.runtime_logs(runtime) +
                    "\n" + "\n".join(bytes(client.transcript[-6000:]).decode(errors="replace").replace(journey.PASSWORD, "[redacted]")
                        for client in clients), encoding="utf-8")
                raise
            finally:
                stop()
        controls = query("SELECT * FROM telemetry_interval WHERE record_kind=13 ORDER BY ingest_id")
        assert controls and any(row["ctl_kind"] == 3 for row in controls)
        target_scope = RollupTarget(7, 1, controls[0]["ctl_environment_id"], controls[0]["ctl_season_id"])
        tables = tuple(battle_source.table(table, target_scope.scope_tuple) for table in (
            "telemetry_battle_source", "telemetry_battle_input", "telemetry_rollup_battle_coverage", "telemetry_rollup_battle_row"))
        review_tables = incident.schema_contract(6)[2:]
        grants = {
            2: {"telemetry_rollup_state": "SELECT,INSERT,UPDATE", "telemetry_generation_identity": "SELECT,INSERT",
                "telemetry_rollup_session": "SELECT,INSERT,UPDATE", "telemetry_player_day": "SELECT,INSERT,UPDATE",
                "telemetry_cohort_day": "SELECT,INSERT,UPDATE", "telemetry_cohort_member": "SELECT,INSERT,UPDATE",
                "telemetry_interval": "SELECT", "telemetry_config": "SELECT", "telemetry_identity_registry": "SELECT",
                "telemetry_identity_association": "SELECT", review_tables[0]: "SELECT", review_tables[1]: "SELECT",
                tables[0]: "SELECT,INSERT,UPDATE", tables[1]: "SELECT,INSERT", tables[2]: "SELECT,INSERT", tables[3]: "SELECT,INSERT",
                "telemetry_rollup_incident_coverage": "SELECT,INSERT", "telemetry_rollup_incident": "SELECT,INSERT"},
            3: {"telemetry_rollup_state": "SELECT", "telemetry_generation_identity": "SELECT", tables[2]: "SELECT", tables[3]: "SELECT",
                "telemetry_rollup_incident_coverage": "SELECT", "telemetry_rollup_incident": "SELECT"},
            4: {"telemetry_interval": "SELECT", review_tables[0]: "SELECT,INSERT", review_tables[1]: "SELECT,INSERT"},
        }
        for index, permissions in grants.items():
            for table, permission in permissions.items():
                query(f"GRANT {permission} ON `{database}`.`{table}` TO %s@'%%'", (users[index],))
            adapters.append(PyMySQLRollupDatabase(PyMySQLConnectionFactory(ConnectionSettings(host="127.0.0.1",
                port=int(environment["DB_PORT"]), database=database, user=users[index], password=password))))
        rollup, reporter, reviewer = adapters
        through = query("SELECT MAX(ingest_id) AS n FROM telemetry_interval")[0]["n"]
        def prepare(scope):
            # Resume the maintained committed cursor in bounded invocations.
            # Keep default byte/page limits as this longer journey adds rows.
            previous = 0
            for invocation in range(1, 65):
                try:
                    completed = RollupEngine(rollup).run(scope, through_ingest_id=through,
                        bounds=RollupBounds(page_size=64, max_rows=128, max_runtime_s=60))
                except BoundsExceeded as error:
                    assert str(error) == "rollup invocation exceeded max_rows before reaching snapshot", str(error)
                    cursor = rollup.read_state(scope)["input_watermark"]
                    assert previous < cursor <= through
                    previous = cursor
                else:
                    assert completed.complete and completed.final_cursor == through
                    receipt.setdefault("bounded_preparation", []).append(dict(generation=scope.generation,
                        invocations=invocation, max_rows_per_invocation=128,
                        max_bytes_per_invocation=RollupBounds().max_total_bytes, input_watermark=through))
                    return
            raise AssertionError("bounded source preparation did not complete")

        rollup.reserve_identity_generation(target_scope.scope_tuple, None)
        prepare(target_scope)
        retained = rollup.read_battle_source(target_scope)
        fields = battle_source.SOURCE_COLUMNS[13]
        expected_controls = {row["ingest_id"]: {name: row[name] for name in fields} for row in controls}
        assert expected_controls == {row["ingest_id"]: row for row in retained.facts if row["record_kind"] == 13}
        # Freeze an actual-source generation without independent review first.
        # Delivering every row alone must not qualify any elapsed duration.
        unreviewed_scope = target_scope
        assert rollup.publish_generation(unreviewed_scope, bounds=RollupBounds(max_runtime_s=60))["status"] == "published"
        unreviewed = reporter.read_report(unreviewed_scope, "battle_control_states", max_rows=1024)
        assert unreviewed.rows and not unreviewed.truncated
        assert all(row["qualified_status_usec"] == [None] * 8 for row in unreviewed.rows)
        target_scope = RollupTarget(7, 2, target_scope.environment_id, target_scope.season_id)
        rollup.reserve_identity_generation(target_scope.scope_tuple, None)
        prepare(target_scope)
        retained = rollup.read_battle_source(target_scope)
        assert expected_controls == {row["ingest_id"]: row for row in retained.facts if row["record_kind"] == 13}
        # The real outage is reviewed as uncertainty. Retained report prefixes
        # must never turn its unobserved activity into measured zero duration.
        # Review the entire retained association prefix, including kind-10
        # clocks before the first control. Independent worker/lifecycle evidence
        # witnesses delivery; an unclosed producer still has an unknown tail.
        times = [row[name] for row in retained.facts for name in row if
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
        assert rollup.publish_generation(target_scope, bounds=RollupBounds(max_runtime_s=60))["status"] == "published"
        states = reporter.read_report(target_scope, "battle_control_states", max_rows=1024)
        operations = reporter.read_report(target_scope, "battle_control_operations", max_rows=1024)
        assert len(states.rows) + len(operations.rows) == len(controls) and not states.truncated and not operations.truncated
        assert expected_controls == {
            row["ingest_id"]: {name: row[name] for name in fields} for row in (*states.rows, *operations.rows)}
        assert all(row["proven_action_restriction_usec"] is None and row["caster_attributed_duration_usec"] is None
            for row in states.rows)
        assert states.coverage.incident_coverage["quality_flags"] & incident.QUALITY_INCIDENT_GAP
        assert states.coverage.incident_coverage["zero_activity_implied"] is False
        assert all(row["qualified_status_usec"] == [None] * 8 for row in states.rows if
            row["publication_quality_flags"] & incident.QUALITY_INCIDENT_GAP)
        assert reporter.read_report(unreviewed_scope, "battle_control_states", max_rows=1024).rows == unreviewed.rows
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
            unreviewed_state_rows=len(unreviewed.rows), immutable_unreviewed_generation=True,
            missing_configuration_publication_probe_rows=len(unknown_states),
            configuration_digest_tamper_refused=True,
            real_private_writer_loss_preserved=True,
            context_clock_capacity="native executable and definition-7 regression fault cases in the maintained command")
        live = receipt["ordinary_pvp"]
        live_states = [row for row in states.rows if row["ctl_target_actor_pid"] == live["target_pid"] and
            (row["boot_id"], row["process_id"]) == (live["producer"]["boot_id"], live["producer"]["process_id"])]
        prefixes = [row for row in live_states if row["ctl_kind"] == 3 and row["observed_prefix_usec"] > 0]
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
        live_operations = [row for row in operations.rows if row["ctl_target_actor_pid"] == live["target_pid"]]
        assert live_operations and all(row["ctl_flags"] & 7 == 0 for row in live_operations)
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
        receipt.update(status="passed", raw_controls=len(controls), published_states=len(states.rows),
            published_operations=len(operations.rows), qualified_prefixes=states.coverage.battle_coverage["qualified_control_prefixes"],
            exact_retained_inputs=True, report_definition=7, private_incident_schema=6,
            report_uses_restricted_role=True, action_restriction_and_caster_duration_unknown=True,
            uncertainty_preserved=True, migration_steps=int(environment["TELEMETRY_REPOSITORY_MIGRATION_COUNT"]))
        result.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
        print(json.dumps({key: receipt[key] for key in ("status", "actual_running_server", "raw_controls", "published_states", "published_operations", "qualified_prefixes")}))
    finally:
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
