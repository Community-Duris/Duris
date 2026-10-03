#!/usr/bin/env python3
"""An ordinary bandage retires one original UID through save and two restarts."""

import argparse
import os
from pathlib import Path
import re
import signal
import subprocess
import tempfile
import time
import uuid

import test_flatfile_combat_journey as journey
import test_static_quest_reward_journey as quest


ROOT = Path(__file__).resolve().parents[2]
BANDAGE_VNUM = 393
WOUNDED_VNUM = 29303


def fixture(run_root: Path) -> None:
    journey.make_fixture(run_root)
    mini = run_root / "areas_mini"
    mobiles = mini / "mini.mob"
    content = mobiles.read_text()
    wounded = """#29303
wounded human~
a wounded human~
A wounded human lies here, barely breathing.
~
~
10 0 0 0 0 0 0 0 S
PH 0 0 -1
1 0 0 1d1+1 1d1+1
0.0.0.0 0
8 8 0
"""
    assert content.count("$~") == 1
    mobiles.write_text(content.replace("$~", wounded + "$~"))
    zone = mini / "mini.zon"
    content = re.sub(r"^[MG] .*\n", "", zone.read_text(), flags=re.M)
    assert content.count("\nS\n") == 1
    zone.write_text(content.replace("\nS\n",
                    f"\nM 0 {WOUNDED_VNUM} 1 22800 100 0 0 0 * wounded human\nS\n"))


def run(binary: Path) -> None:
    if os.environ.get("TEST_DB_DISPOSABLE") != "1":
        raise RuntimeError("TEST_DB_DISPOSABLE=1 is required")
    host = os.environ["TEST_DB_HOST"]
    if host not in ("127.0.0.1", "localhost"):
        raise RuntimeError("use a disposable loopback database")
    port = os.environ.get("TEST_DB_PORT", "3306")
    database = "bandage_journey_test_" + uuid.uuid4().hex[:12]
    environment = {
        "PATH": os.environ.get("PATH", "/usr/bin:/bin"),
        "ENVIRONMENT": "local", "DB_HOST": host, "DB_PORT": port,
        "DB_NAME": database, "DB_USER": os.environ["TEST_DB_USER"],
        "DB_PASSWD": os.environ["TEST_DB_PASSWORD"],
        "MYSQL_PWD": os.environ["TEST_DB_PASSWORD"],
        "DB_ALLOWED_TARGETS": host + "/" + database,
        "PERSISTENCE_MODE": "mariadb-primary", "DB_TLS": "FALSE",
        "LISTEN_ADDRESS": "127.0.0.1",
        "DURIS_WEBSOCKET_LISTEN_ADDRESS": "127.0.0.1",
        "REDIS": "FALSE", "CHAOS_MUD": "FALSE",
    }
    if runtime_library_path := os.environ.get("LD_LIBRARY_PATH"):
        environment["LD_LIBRARY_PATH"] = runtime_library_path
    mysql = ["mysql", "--protocol=tcp", "-h", host, "-P", port,
             "-u", environment["DB_USER"], "-N", "-B"]

    def sql(query: str, selected: bool = True) -> str:
        args = mysql + ([database] if selected else [])
        return subprocess.check_output(args, input=query, text=True,
                                       env=environment).strip()

    sql("CREATE DATABASE " + database + " CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci", False)
    try:
        sql((ROOT / "migrations/bootstrap_multithread_safe.sql").read_text())
        subprocess.run(["python3", "scripts/migration_runner.py", "adopt",
                        "--kind", "fresh_bootstrap"], cwd=ROOT, env=environment,
                       check=True, timeout=300)
        subprocess.run(["python3", "scripts/migration_runner.py", "run"],
                       cwd=ROOT, env=environment, check=True, timeout=600)
        with tempfile.TemporaryDirectory(prefix="duris-bandage-sql-") as run_tmp:
            run_root = Path(run_tmp)
            (run_root / "logs/log").mkdir(parents=True)
            (run_root / "logs/log/.gitignore").write_text("*\n!.gitignore\n")
            fixture(run_root)
            journey.generate_certificate(run_root)
            (run_root / "Players").mkdir(mode=0o700)
            journals = run_root / "journals"
            (journals / "players").mkdir(parents=True, mode=0o700)
            (journals / "critical").mkdir(mode=0o700)
            game_port, tls_port, websocket_port = journey.available_ports()
            environment.update(
                PLAYER_SAVE_JOURNAL_DIR=str(journals / "players"),
                CRITICAL_COMMAND_JOURNAL_DIR=str(journals / "critical"),
                DURIS_TLS_PORT=str(tls_port), DURIS_WEBSOCKET_PORT=str(websocket_port),
            )
            first_uids = set()
            for phase in ("initial", "restart", "second_restart"):
                output_path = run_root / f"{phase}.out"
                process, output = quest.boot(binary, run_root, environment,
                                             game_port, output_path)
                client = None
                try:
                    if phase == "initial":
                        client = journey.MudClient(game_port)
                        journey.create_character(client, expected_room=None,
                                                 class_name="d", hometown="p")
                        client.send("look")
                        client.expect("A wounded human is lying here", timeout=15)
                        player_id = int(sql("SELECT pid FROM player_data WHERE name='" +
                                            journey.CHARACTER + "'"))
                        def active_bandages() -> set[int]:
                            rows = sql("SELECT item_uid FROM item_current_owner WHERE "
                                       f"owner_type=1 AND owner_id={player_id} AND state=1 "
                                       f"AND vnum={BANDAGE_VNUM}")
                            return {int(row) for row in rows.splitlines() if row}
                        first_uids = active_bandages()
                        journey.require(first_uids, "starter kit has no durable bandage")
                        time.sleep(3)
                        client.send("bandage human")
                        client.expect("You begin using the bandage.", timeout=15)
                        client.expect("You attempt to", timeout=30)
                        deadline = time.monotonic() + 30
                        while len(active_bandages()) != len(first_uids) - 1:
                            journey.require(time.monotonic() < deadline,
                                            "bandage custody was not retired")
                            time.sleep(0.1)
                        consumed = first_uids - active_bandages()
                        journey.require(len(consumed) == 1,
                                        f"expected one consumed UID, found {consumed}")
                        consumed_uid = next(iter(consumed))
                        remaining_uids = first_uids - consumed
                    else:
                        client = journey.reconnect_character(game_port)
                        journey.require(active_bandages() == remaining_uids,
                                        "original bandage UIDs changed on restart")
                    client.send("save")
                    client.expect(f"Save complete for {journey.CHARACTER}.", timeout=30)
                    count = int(sql(f"SELECT COUNT(*) FROM player_items WHERE pid={player_id} "
                                    f"AND obj_uid={consumed_uid}"))
                    journey.require(count == 0, "consumed bandage remained in saved items")
                    saved_uids = {int(row) for row in sql(
                        f"SELECT obj_uid FROM player_items WHERE pid={player_id} "
                        f"AND vnum={BANDAGE_VNUM}").splitlines() if row}
                    journey.require(saved_uids == remaining_uids,
                                    "saved bandages do not match the original surviving UIDs")
                    journey.require(active_bandages() == remaining_uids,
                                    "saving changed original bandage custody")
                    state = sql("SELECT owner_type,state FROM item_current_owner WHERE "
                                f"item_uid={consumed_uid}")
                    journey.require(state == "8\t2", f"wrong bandage tombstone: {state}")
                    retirement = sql(
                        "SELECT HEX(operation_id),item_revision FROM item_ownership_ledger "
                        f"WHERE item_uid={consumed_uid} AND to_owner_type=8")
                    rows = retirement.splitlines()
                    journey.require(len(rows) == 1 and len(rows[0].split("\t")[0]) == 32,
                                    "bandage must have exactly one operation-scoped retirement")
                    if phase == "initial":
                        original_retirement = retirement
                    else:
                        journey.require(retirement == original_retirement,
                                        "restart changed the bandage retirement operation/revision")
                    client.send("quit")
                    client.expect("ACCOUNT MENU", timeout=30)
                    client.send("0")
                    client.close()
                    client = None
                    quest.stop(process, output)
                except Exception as error:
                    output.flush()
                    raise AssertionError(
                        f"{phase}: {error}\n--- client ---\n"
                        f"{bytes(client.transcript).decode('utf-8', errors='replace')[-5000:] if client else ''}"
                        f"\n--- server ---\n{output_path.read_text(errors='replace')[-5000:]}"
                        f"\n--- logs ---\n{journey.runtime_logs(run_root)}"
                    ) from error
                finally:
                    if client is not None:
                        client.close()
                    if process.poll() is None:
                        process.kill()
                        process.wait(timeout=5)
                    if not output.closed:
                        output.close()
    finally:
        sql("DROP DATABASE " + database, False)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--server", type=Path, required=True)
    args = parser.parse_args()
    run(args.server)
    print("bandage original UIDs, single retirement operation, save and two cold restarts passed")
