#!/usr/bin/env python3
"""Real server corpse haul at the item-count cap on both persistence backends."""

from pathlib import Path
import os
import signal
import subprocess
import sys
import tempfile
import time
import uuid

import test_flatfile_combat_journey as journey

ROOT = Path(__file__).resolve().parents[2]


def drain(client, duration=1):
    deadline = time.monotonic() + duration
    while time.monotonic() < deadline:
        client._receive()
        time.sleep(.02)
    result = client.pending.decode(errors="replace")
    client.pending.clear()
    return result


def add_stock(runtime):
    objects_path = runtime / "areas_mini/mini.obj"
    objects = objects_path.read_text()
    start, end = objects.index("#15\n"), objects.index("#16\n")
    banana = objects[start:end]
    assert "banana~" in banana
    extra = ""
    for vnum, name in ((22810, "ruby"), (22811, "sapphire"), (22812, "marker")):
        extra += banana.replace("#15\n", f"#{vnum}\n", 1).replace(
            "banana", name).replace("Banana", name.capitalize())
    objects_path.write_text(objects.replace("$~", extra + "$~"))

    zone_path = runtime / "areas_mini/mini.zon"
    zone = zone_path.read_text()
    anchor = "G 1 15 1 0 100 0 0 0 * corpse-loot marker\n"
    assert anchor in zone
    extra_resets = (
        "G 1 22810 1 0 100 0 0 0 * second corpse root\n"
        "G 1 22811 1 0 100 0 0 0 * third corpse root\n"
    )
    fillers = "O 0 22812 30 22800 100 0 0 0 * count-cap filler\n" * 20
    zone_path.write_text(zone.replace(anchor, anchor + extra_resets + fillers))


def run(binary, backend):
    assert backend in ("mariadb", "flatfile")
    database = "haul_cap_" + uuid.uuid4().hex[:12]
    host = os.environ.get("TEST_DB_HOST", "127.0.0.1")
    assert host in ("127.0.0.1", "localhost")
    port = os.environ.get("TEST_DB_PORT", "3307")
    env = dict(PATH=os.environ.get("PATH", "/usr/bin:/bin"), ENVIRONMENT="local",
               PERSISTENCE_MODE=backend + "-primary", REDIS="FALSE",
               CHAOS_MUD="FALSE", LISTEN_ADDRESS="127.0.0.1",
               DURIS_WEBSOCKET_LISTEN_ADDRESS="127.0.0.1")
    if value := os.environ.get("LD_LIBRARY_PATH"):
        env["LD_LIBRARY_PATH"] = value
    state = None
    state_root = None
    sql = None
    if backend == "mariadb":
        password = os.environ["TEST_DB_PASSWORD"]
        env.update(DB_HOST=host, DB_PORT=port, DB_NAME=database,
                   DB_USER=os.environ.get("TEST_DB_USER", "root"), DB_PASSWD=password,
                   MYSQL_PWD=password, DB_ALLOWED_TARGETS=host + "/" + database,
                   DB_TLS="FALSE")
        mysql = ["mysql", "--protocol=tcp", "-h", host, "-P", port,
                 "-u", env["DB_USER"], "-N", "-B", "--unbuffered"]

        def sql(statement, selected=True):
            return subprocess.check_output(mysql + ([database] if selected else []),
                                           input=statement, text=True, env=env).strip()

        sql("CREATE DATABASE " + database, False)
    else:
        (ROOT / "bin/tests").mkdir(parents=True, exist_ok=True)
        subprocess.run(["python3", "tests/async/test_flatfile_player_repository.py",
                        "--build-inspector", str(journey.INSPECTOR)],
                       cwd=ROOT, check=True, timeout=180)
        state = tempfile.TemporaryDirectory(prefix="haul-cap-state-")
        state_root = Path(state.name)
        (state_root / "domains").mkdir(mode=0o700)
        subprocess.run([str(journey.INSPECTOR), str(state_root), "seed-combat"], check=True)
        env["FLATFILE_STATE_DIR"] = str(state_root)

    def owner_count(pid):
        if sql:
            return int(sql(f"SELECT COUNT(*) FROM item_current_owner WHERE owner_type=1 AND owner_id={pid} AND vnum IN (15,22810,22811) AND state=1"))
        return sum(item["vnum"] in (15, 22810, 22811)
                   for item in journey.inspect_authority(state_root)["player_items"])

    def wallet(pid):
        if sql:
            return [int(value) for value in sql(
                f"SELECT copper,silver,gold,platinum FROM player_data WHERE pid={pid}").split()]
        return journey.inspect_authority(state_root)["wallet"]

    try:
        if sql:
            sql((ROOT / "migrations/bootstrap_multithread_safe.sql").read_text())
            for args in (("adopt", "--kind", "fresh_bootstrap"), ("run",)):
                subprocess.run(["python3", "scripts/migration_runner.py", *args],
                               cwd=ROOT, env=env, check=True)
        (ROOT / "bin/tests").mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="haul-cap-", dir=ROOT / "bin/tests") as temporary:
            runtime = Path(temporary)
            journey.make_fixture(runtime, True)
            add_stock(runtime)
            journey.generate_certificate(runtime)
            (runtime / "logs/log").mkdir(parents=True)
            for name in ("players", "critical"):
                (runtime / "journals" / name).mkdir(parents=True, mode=0o700)
            game_port, tls_port, ws_port = journey.available_ports()
            env.update(PLAYER_SAVE_JOURNAL_DIR=str(runtime / "journals/players"),
                       CRITICAL_COMMAND_JOURNAL_DIR=str(runtime / "journals/critical"),
                       DURIS_TLS_PORT=str(tls_port), DURIS_WEBSOCKET_PORT=str(ws_port))
            output = (runtime / "server.out").open("w")
            process = None
            actor = None
            try:
                process = subprocess.Popen([str(binary), "--minimal", "-s", "-d",
                                            str(runtime), str(game_port)], cwd=runtime,
                                           env=env, stdout=output, stderr=subprocess.STDOUT)
                deadline = time.monotonic() + 120
                while "Entering game loop." not in (runtime / "server.out").read_text(errors="replace"):
                    assert process.poll() is None and time.monotonic() < deadline, "boot failed"
                    time.sleep(.1)
                actor = journey.MudClient(game_port)
                journey.create_character(actor)
                actor.send("toggle boon")
                actor.expect("You will no longer be affected by boons.")
                actor.send("wield mace")
                actor.expect("You wield")
                actor.send("drop all")
                actor.expect("You drop a steel long sword.", timeout=20)
                drain(actor)
                actor.send("get all.marker")
                full = actor.expect("You can't carry anything more.", timeout=60) + drain(actor)
                actor.send("inventory")
                inventory = actor.expect("Pos: standing >", timeout=20) + drain(actor)
                assert "marker" in inventory, inventory
                actor.send("kill raoul")
                deadline = time.monotonic() + 45
                while True:
                    event, _ = actor.expect_any(("is dead! R.I.P.",
                                                 "You stumble, but recover in time!",
                                                 "You stumble in your attack, and jab at",
                                                 "You stumble in your attack, and hit yourself!"),
                                                timeout=max(1, deadline - time.monotonic()))
                    if event == "is dead! R.I.P.":
                        break
                    assert time.monotonic() < deadline
                    actor.send("kill raoul")
                actor.send("look in corpse")
                contents = actor.expect("sapphire", timeout=20) + drain(actor)
                assert "banana" in contents and "ruby" in contents, contents
                drain(actor)
                actor.send("get all corpse")
                first = actor.expect("You finish sorting your haul", timeout=60) + drain(actor)
                print("NPC corpse at cap:", first, flush=True)
                assert first.count("You can't carry any more.") == 1, first
                assert "3s" in first and "Nothing acquired." not in first, first
                actor.send("save")
                actor.expect("Save complete for Taverek.", timeout=30)
                pid = int(sql("SELECT pid FROM player_data WHERE name='Taverek'")) if sql else 1
                assert wallet(pid) == [0, 3, 0, 0]
                assert owner_count(pid) == 0
                actor.send("look in corpse")
                remaining = actor.expect("sapphire") + drain(actor)
                assert "banana" in remaining and "ruby" in remaining
                print("PASS: NPC corpse coin persisted and all three roots retained at cap", flush=True)
                actor.send("drop marker")
                actor.expect("You drop a marker.", timeout=20)
                drain(actor)
                actor.send("get all corpse")
                partial = actor.expect("You finish sorting your haul", timeout=60) + drain(actor)
                print("NPC corpse with one free slot:", partial, flush=True)
                assert partial.count("You can't carry any more.") == 1, partial
                assert "Haul:\r\n  a " in partial, partial
                assert owner_count(pid) == 1
                actor.send("save")
                actor.expect("Save complete for Taverek.", timeout=30)
                actor.send("quit")
                actor.expect("ACCOUNT MENU", timeout=30)
                actor.close()
                actor = journey.reconnect_character(game_port)
                actor.send("inventory")
                inventory = actor.expect("Pos: standing >") + drain(actor)
                assert "marker" in inventory and any(
                    name in inventory for name in ("banana", "ruby", "sapphire")), inventory
                assert owner_count(pid) == 1
                journey.attack_until_death(actor)
                actor.expect("ACCOUNT MENU", timeout=45)
                actor.close()
                actor = journey.reconnect_character(game_port)
                actor.send("look")
                actor.expect("The corpse of a Human is lying here.")
                drain(actor)
                actor.send("get all Taverek")
                player = actor.expect("You finish sorting your haul", timeout=60) + drain(actor)
                print("Player corpse at cap:", player, flush=True)
                assert player.count("You can't carry any more.") == 1, player
                assert "Haul:" in player and "Some contents were not acquired." in player
                if "The coin transfer could not start; nothing changed." in player:
                    # An independent transaction admission may refuse a coin
                    # after the durable roots commit. It must remain in the
                    # corpse and be retrievable by a later capped command.
                    assert wallet(pid) == [0, 0, 0, 0]
                    actor.send("look in Taverek")
                    assert "coins" in actor.expect("coins", timeout=20) + drain(actor)
                    actor.send("get coins Taverek")
                    coin_retry = actor.expect("You get 3s.", timeout=30) + drain(actor)
                    print("Separate coin retry:", coin_retry, flush=True)
                actor.send("save")
                actor.expect("Save complete for Taverek.", timeout=30)
                assert wallet(pid) == [0, 3, 0, 0]
                print(f"PASS: {backend} NPC/player corpse count notice, partial haul, saved custody, wallet and reconnect", flush=True)
            except Exception:
                output.flush()
                print((runtime / "server.out").read_text(errors="replace")[-2000:])
                if actor:
                    print(actor.transcript.decode(errors="replace")[-3000:])
                raise
            finally:
                if actor:
                    actor.close()
                if process and process.poll() is None:
                    process.send_signal(signal.SIGTERM)
                    process.wait(timeout=30)
                output.close()
    finally:
        if sql:
            sql("DROP DATABASE " + database, False)
        if state:
            state.cleanup()


if __name__ == "__main__":
    run(Path(sys.argv[1]).resolve(), sys.argv[2] if len(sys.argv) > 2 else "mariadb")
