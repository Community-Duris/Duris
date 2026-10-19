#!/usr/bin/env python3
"""Real lethal corpse trap, reconnect, and restart journey on both backends."""

from pathlib import Path
import json
import os
import signal
import shutil
import subprocess
import sys
import tempfile
import time
import uuid

import test_flatfile_combat_journey as journey


ROOT = Path(__file__).resolve().parents[2]
BUILDER_ACCOUNT = "Trapacct"
BUILDER = "Trapbuilder"
SOURCE_ACCOUNT = "Sourceacct"
SOURCE = "Sourcemortal"
OBSERVER_ACCOUNT = "Destacct"
OBSERVER = "Destmortal"
EXECUTIONER_VNUM = 800000
TARGET_VNUMS = (800001, 800002, 800003)
TRAP_VNUM = TARGET_VNUMS[-1]
TRAP_ROOM = 95514
SAFE_ROOM = 95513
TRAP_STATE_KEYWORD = "DURIS_TRAP_STATE_V1"


def drain(client, duration=0.5):
    deadline = time.monotonic() + duration
    while time.monotonic() < deadline:
        client._receive()
        time.sleep(0.02)
    output = client.pending.decode(errors="replace")
    client.pending.clear()
    return output


def clone_banana(block, vnum, keys, short_description, room_description, wear_flags=None):
    clone = block.replace("#15\n", f"#{vnum}\n", 1)
    clone = clone.replace("banana~", keys + "~", 1)
    clone = clone.replace("&+Ya ba&+yna&+Yna&N~", short_description + "~", 1)
    clone = clone.replace(
        "&+YA banana lies here on the ground, turning &+ybrown.&N~",
        room_description + "~",
        1,
    )
    if wear_flags is not None:
        clone = clone.replace("8192 8193 0 0 0", f"8192 {wear_flags} 0 0 0", 1)
    assert keys in clone and short_description in clone and room_description in clone
    return clone


def add_fixture(runtime):
    areas = runtime / "areas"
    areas.unlink()
    areas.mkdir()
    for entry in (ROOT / "areas").iterdir():
        destination = areas / entry.name
        if entry.name in ("world.mob", "world.obj"):
            shutil.copy2(entry, destination)
        else:
            destination.symlink_to(entry, target_is_directory=entry.is_dir())

    objects_path = areas / "world.obj"
    objects = objects_path.read_text()
    assert all(f"#{vnum}\n" not in objects for vnum in TARGET_VNUMS)
    start, end = objects.index("#15\n"), objects.index("#16\n")
    banana = objects[start:end]
    ordinary = clone_banana(
        banana,
        TARGET_VNUMS[0],
        "ruby ordinary marker",
        "a corpse-test ruby",
        "A corpse-test ruby lies here.",
    )
    ioun = clone_banana(
        banana,
        TARGET_VNUMS[1],
        "ioun stone marker",
        "a corpse-test ioun stone",
        "A corpse-test ioun stone floats here.",
        268435457,
    )
    trap = clone_banana(
        banana,
        TRAP_VNUM,
        "trapstone trapped marker",
        "a trapped trapstone",
        "A trapped trapstone lies here.",
    )
    trap += "T\n6 2 1 100\n"
    terminator = objects.rfind("#9999999\n")
    assert terminator > 0
    objects_path.write_text(objects[:terminator] + ordinary + ioun + trap + objects[terminator:])

    mobile_path = areas / "world.mob"
    mobiles = mobile_path.read_text()
    assert f"#{EXECUTIONER_VNUM}\n" not in mobiles
    executioner = f"""#{EXECUTIONER_VNUM}
regression executioner~
the regression executioner~
The regression executioner waits here to test the player death path.
~
~
10 0 0 0 0 0 0 0 S
PH 0 0 -1
60 0 -250 1000d1+50000 10d10+200
0.0.0.0 0
8 8 0
"""
    terminator = mobiles.rfind("#9999999\n")
    assert terminator > 0
    mobile_path.write_text(
        mobiles[:terminator] + executioner + mobiles[terminator:]
    )
    with (runtime / "lib/random_equipment.cfg").open("a") as config:
        config.write("drop.piece.percentage=0\ndrop.equipment.percentage=0\n")


def run(binary, backend):
    assert backend in ("mariadb", "flatfile")
    database = "corpse_trap_" + uuid.uuid4().hex[:12]
    host = os.environ.get("TEST_DB_HOST", "127.0.0.1")
    assert host in ("127.0.0.1", "localhost")
    database_port = os.environ.get("TEST_DB_PORT", "3307")
    env = dict(
        PATH=os.environ.get("PATH", "/usr/bin:/bin"),
        ENVIRONMENT="local",
        PERSISTENCE_MODE=backend + "-primary",
        REDIS="FALSE",
        CHAOS_MUD="FALSE",
        LISTEN_ADDRESS="127.0.0.1",
        DURIS_WEBSOCKET_LISTEN_ADDRESS="127.0.0.1",
    )
    if value := os.environ.get("LD_LIBRARY_PATH"):
        env["LD_LIBRARY_PATH"] = value
    state = None
    state_root = None
    sql = None
    if backend == "mariadb":
        password = os.environ["TEST_DB_PASSWORD"]
        env.update(
            DB_HOST=host,
            DB_PORT=database_port,
            DB_NAME=database,
            DB_USER=os.environ.get("TEST_DB_USER", "root"),
            DB_PASSWD=password,
            MYSQL_PWD=password,
            DB_ALLOWED_TARGETS=host + "/" + database,
            DB_TLS="FALSE",
        )
        mysql = [
            "mysql",
            "--protocol=tcp",
            "-h",
            host,
            "-P",
            database_port,
            "-u",
            env["DB_USER"],
            "-N",
            "-B",
            "--unbuffered",
        ]

        def sql(statement, selected=True):
            return subprocess.check_output(
                mysql + ([database] if selected else []), input=statement, text=True, env=env
            ).strip()

        sql("CREATE DATABASE " + database, False)
    else:
        (ROOT / "bin/tests").mkdir(parents=True, exist_ok=True)
        subprocess.run(
            [
                "python3",
                "tests/async/test_flatfile_player_repository.py",
                "--build-inspector",
                str(journey.INSPECTOR),
            ],
            cwd=ROOT,
            check=True,
            timeout=300,
        )
        state = tempfile.TemporaryDirectory(prefix="corpse-trap-state-")
        state_root = Path(state.name)
        (state_root / "domains").mkdir(mode=0o700)
        subprocess.run([str(journey.INSPECTOR), str(state_root), "seed-combat"], check=True)
        env["FLATFILE_STATE_DIR"] = str(state_root)

    def promote_builder():
        if sql:
            sql(
                "UPDATE player_data SET level=62,highest_level=62 "
                f"WHERE name='{BUILDER}'"
            )
        else:
            journey.make_overlord(state_root, BUILDER)

    def source_rows():
        if sql:
            rows = sql(
                "SELECT ci.vnum,ci.obj_uid FROM corpse_items ci JOIN corpses c "
                f"ON c.id=ci.corpse_id WHERE c.player_name='{SOURCE}' "
                f"AND ci.vnum IN {TARGET_VNUMS} ORDER BY ci.vnum"
            )
            return [
                {"vnum": int(vnum), "uid": int(uid)}
                for vnum, uid in (line.split("\t") for line in rows.splitlines())
            ]
        result = subprocess.check_output(
            [
                str(journey.INSPECTOR),
                str(state_root),
                "inspect-world-items",
                *(str(vnum) for vnum in TARGET_VNUMS),
            ],
            text=True,
        )
        return json.loads(result)["items"]

    def source_trap_state():
        if sql:
            return sql(
                "SELECT ed.description FROM corpse_item_extra_descr ed "
                "JOIN corpse_items ci ON ci.id=ed.item_id "
                "JOIN corpses c ON c.id=ci.corpse_id "
                f"WHERE c.player_name='{SOURCE}' AND ci.vnum={TRAP_VNUM} "
                f"AND ed.keyword='{TRAP_STATE_KEYWORD}'"
            )
        trap = next(row for row in source_rows() if row["vnum"] == TRAP_VNUM)
        return trap.get("trap_state", "")

    try:
        if sql:
            sql((ROOT / "migrations/bootstrap_multithread_safe.sql").read_text())
            for args in (("adopt", "--kind", "fresh_bootstrap"), ("run",)):
                subprocess.run(
                    ["python3", "scripts/migration_runner.py", *args],
                    cwd=ROOT,
                    env=env,
                    check=True,
                )
        (ROOT / "bin/tests").mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(
            prefix="corpse-trap-", dir=ROOT / "bin/tests"
        ) as temporary:
            runtime = Path(temporary)
            journey.make_fixture(runtime)
            add_fixture(runtime)
            journey.generate_certificate(runtime)
            (runtime / "logs/log").mkdir(parents=True)
            (runtime / "journals/critical").mkdir(parents=True, mode=0o700)
            game_port, tls_port, ws_port = journey.available_ports()
            env.update(
                CRITICAL_COMMAND_JOURNAL_DIR=str(runtime / "journals/critical"),
                DURIS_TLS_PORT=str(tls_port),
                DURIS_WEBSOCKET_PORT=str(ws_port),
            )
            clients = []
            process = None
            output = None
            boot_number = 0

            def boot():
                nonlocal process, output, boot_number
                boot_number += 1
                output = (runtime / f"server-{boot_number}.out").open("w")
                process = subprocess.Popen(
                    [str(binary), "-s", "-d", str(runtime), str(game_port)],
                    cwd=runtime,
                    env=env,
                    stdout=output,
                    stderr=subprocess.STDOUT,
                )
                deadline = time.monotonic() + 120
                while "Entering game loop." not in (
                    runtime / f"server-{boot_number}.out"
                ).read_text(errors="replace"):
                    assert process.poll() is None and time.monotonic() < deadline, "boot failed"
                    time.sleep(0.1)

            def stop():
                nonlocal process, output
                for client in clients:
                    client.close()
                clients.clear()
                if process and process.poll() is None:
                    process.send_signal(signal.SIGTERM)
                    process.wait(timeout=30)
                if output:
                    output.close()
                process = None
                output = None

            try:
                boot()
                builder = journey.MudClient(game_port)
                clients.append(builder)
                journey.create_character(
                    builder,
                    expected_room=None,
                    account=BUILDER_ACCOUNT,
                    character=BUILDER,
                    email="trapbuilder@example.invalid",
                )
                builder.send("save")
                builder.expect(f"Save complete for {BUILDER}.", timeout=30)
                builder.send("quit")
                builder.expect("ACCOUNT MENU", timeout=30)
                stop()

                promote_builder()
                boot()

                actor = journey.MudClient(game_port)
                clients.append(actor)
                journey.create_character(actor, expected_room=None)
                actor.send("toggle boon")
                actor.expect("You will no longer be affected by boons.")
                source = journey.MudClient(game_port)
                clients.append(source)
                journey.create_character(
                    source,
                    expected_room=None,
                    account=SOURCE_ACCOUNT,
                    character=SOURCE,
                    email="source@example.invalid",
                )
                source.send("toggle boon")
                source.expect("You will no longer be affected by boons.")
                source.send("drop all")
                source.expect("You drop a steel long sword.", timeout=20)
                builder = journey.reconnect_character(
                    game_port,
                    expected_room=None,
                    account=BUILDER_ACCOUNT,
                    character=BUILDER,
                )
                clients.append(builder)
                observer = journey.MudClient(game_port)
                clients.append(observer)
                journey.create_character(
                    observer,
                    expected_room=None,
                    account=OBSERVER_ACCOUNT,
                    character=OBSERVER,
                    email="observer@example.invalid",
                )
                builder.send(f"goto {TRAP_ROOM}")
                builder.expect("Guard Post", timeout=30)
                builder.send("transfer Taverek")
                builder.expect("Ok.", timeout=30)
                actor.expect("demands your presence NOW!")
                builder.send(f"transfer {SOURCE}")
                builder.expect("Ok.", timeout=30)
                source.expect("demands your presence NOW!")
                builder.send(f"transfer {OBSERVER}")
                builder.expect("Ok.", timeout=30)
                observer.expect("demands your presence NOW!")
                for vnum, keyword, description in (
                    (TRAP_VNUM, "trapstone", "a trapped trapstone"),
                    (TARGET_VNUMS[0], "ruby", "a corpse-test ruby"),
                    (TARGET_VNUMS[1], "ioun", "a corpse-test ioun stone"),
                ):
                    builder.send(f"load object {vnum}")
                    builder.expect("You have created")
                    drain(source)
                    builder.send(f"give {keyword} {SOURCE}")
                    source.expect("gives you " + description)
                builder.send(f"load mobile {EXECUTIONER_VNUM}")
                builder.expect("You have created the regression executioner")
                builder.send(f"goto {SAFE_ROOM}")
                builder.expect("Inside the Gates of Kimordril", timeout=30)
                source.send("inventory")
                inventory = source.expect("Pos: standing >") + drain(source)
                assert inventory.index("ioun stone") < inventory.index("ruby") < inventory.index(
                    "trapstone"
                ), inventory
                journey.attack_until_death(source)
                source.expect("ACCOUNT MENU", timeout=45)
                source.close()
                clients.remove(source)

                actor.send("drop all")
                actor.expect("You drop a steel long sword.", timeout=20)
                actor.send(f"look in {SOURCE}")
                contents = actor.expect("trapstone") + drain(actor)
                assert "corpse-test ioun stone" in contents and "corpse-test ruby" in contents
                drain(observer)
                drain(builder)
                drain(actor)
                actor.send(f"get all {SOURCE}")
                death = actor.expect("ACCOUNT MENU", timeout=45) + drain(actor)
                assert "finish sorting your haul" not in death
                assert "corpse-test ioun stone" not in death and "corpse-test ruby" not in death
                assert process.poll() is None, "the lethal corpse trap stopped the server"
                bystander = observer.expect("ACCOUNT MENU", timeout=45) + drain(observer)
                assert "fireball" in bystander.lower(), bystander
                builder.send("look")
                builder.expect("Inside the Gates of Kimordril", timeout=30)
                builder.send(f"goto {TRAP_ROOM}")
                room = builder.expect("Guard Post", timeout=30) + drain(builder)
                assert "regression executioner" in room, room
                builder.send(f"goto {SAFE_ROOM}")
                builder.expect("Inside the Gates of Kimordril", timeout=30)

                actor.close()
                clients.remove(actor)
                actor = journey.reconnect_character(game_port, expected_room=None)
                clients.append(actor)
                actor.expect("Pos: standing >", timeout=30)
                builder.send(f"goto {TRAP_ROOM}")
                builder.expect("Guard Post", timeout=30)
                builder.send("transfer Taverek")
                builder.expect("Ok.", timeout=30)
                actor.expect("demands your presence NOW!")
                builder.send(f"goto {SAFE_ROOM}")
                builder.expect("Inside the Gates of Kimordril", timeout=30)
                actor.send(f"look in {SOURCE}")
                contents = actor.expect("trapstone") + drain(actor)
                assert "corpse-test ioun stone" in contents and "corpse-test ruby" in contents
                actor.send("get sword")
                actor.expect("You get a steel long sword.")
                assert "finish sorting" not in drain(actor), "an abandoned haul replayed"
                actor.send("save")
                actor.expect("Save complete for Taverek.", timeout=30)
                stop()
                before_restart = source_rows()
                assert len(before_restart) == 3, before_restart
                assert source_trap_state() == "6,2,0,100"
                if sql:
                    assert sql(
                        "SELECT COUNT(*) FROM corpse_items ci JOIN corpses c "
                        "ON c.id=ci.corpse_id WHERE c.player_name='Taverek' "
                        f"AND ci.vnum IN {TARGET_VNUMS}"
                    ) == "0"

                boot()
                builder = journey.reconnect_character(
                    game_port,
                    expected_room=None,
                    account=BUILDER_ACCOUNT,
                    character=BUILDER,
                )
                clients.append(builder)
                builder.expect("Pos: standing >", timeout=30)
                actor = journey.reconnect_character(game_port, expected_room=None)
                clients.append(actor)
                actor.expect("Pos: standing >", timeout=30)
                builder.send(f"goto {TRAP_ROOM}")
                builder.expect("Guard Post", timeout=30)
                builder.send("transfer Taverek")
                builder.expect("Ok.", timeout=30)
                actor.expect("demands your presence NOW!")
                builder.send(f"goto {SAFE_ROOM}")
                builder.expect("Inside the Gates of Kimordril", timeout=30)
                actor.send(f"look in {SOURCE}")
                restored = actor.expect("trapstone") + drain(actor)
                assert "corpse-test ioun stone" in restored and "corpse-test ruby" in restored
                actor.send(f"get trapstone {SOURCE}")
                outcome, pickup = actor.expect_any(
                    ("You get a trapped trapstone from", "ACCOUNT MENU"), timeout=45
                )
                assert outcome == "You get a trapped trapstone from", (
                    "the consumed corpse trap rearmed across restart\n" + pickup
                )
                actor.send("inventory")
                actor.expect("a trapped trapstone")
                actor.send("save")
                actor.expect("Save complete for Taverek.", timeout=30)
                stop()
                after_restart = source_rows()
                assert len(after_restart) == 2, after_restart
                before_uids = {row["uid"] for row in before_restart}
                after_uids = {row["uid"] for row in after_restart}
                assert after_uids < before_uids and len(before_uids - after_uids) == 1
                print(
                    f"PASS: {backend} lethal corpse trap cancellation, bystander effect, "
                    "server survival, reconnect, custody and spent state across restart",
                    flush=True,
                )
            except Exception:
                if output:
                    output.flush()
                for path in sorted(runtime.glob("server-*.out")):
                    print(f"--- {path.name} ---\n{path.read_text(errors='replace')[-5000:]}")
                print(journey.runtime_logs(runtime)[-12000:])
                for client in clients:
                    print(client.transcript.decode(errors="replace")[-5000:])
                raise
            finally:
                stop()
    finally:
        if sql:
            sql("DROP DATABASE " + database, False)
        if state:
            state.cleanup()


if __name__ == "__main__":
    if len(sys.argv) == 1:
        (ROOT / "bin/tests").mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(
            prefix="corpse-trap-build-", dir=ROOT / "bin/tests"
        ) as build:
            run(journey.build_flatfile_server(Path(build)), "flatfile")
    elif len(sys.argv) in (2, 3):
        run(Path(sys.argv[1]).resolve(), sys.argv[2] if len(sys.argv) == 3 else "mariadb")
    else:
        raise SystemExit(
            "usage: test_corpse_trap_restart_journey.py [SERVER [mariadb|flatfile]]"
        )
