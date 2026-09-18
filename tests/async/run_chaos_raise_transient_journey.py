#!/usr/bin/env python3
"""Cast Greater Dracolich against a disposable persisted player corpse."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time
import uuid

import test_flatfile_combat_journey as journey

ROOT = Path(__file__).resolve().parents[2]
TRANSIENT = 524288
UIDS = (880000000001, 880000000002, 880000000003)
OWNER_PID, SAVE_ID, ROOM = 880001, 1004, 22800


def run(binary: Path, expect_refusal: bool, with_coins: bool) -> None:
    host = os.environ.get("TEST_DB_HOST", "127.0.0.1")
    port = os.environ.get("TEST_DB_PORT", "3307")
    assert host in ("127.0.0.1", "localhost", "::1")
    database = "chaos_raise_" + uuid.uuid4().hex[:12]
    env = dict(os.environ, ENVIRONMENT="local", DB_HOST=host, DB_PORT=port,
               DB_NAME=database, DB_USER=os.environ["TEST_DB_USER"],
               DB_PASSWD=os.environ["TEST_DB_PASSWORD"],
               MYSQL_PWD=os.environ["TEST_DB_PASSWORD"],
               DB_ALLOWED_TARGETS=host + "/" + database,
               PERSISTENCE_MODE="mariadb-primary", DB_TLS="FALSE",
               REDIS="FALSE", CHAOS_MUD="FALSE", LISTEN_ADDRESS="127.0.0.1",
               DURIS_WEBSOCKET_LISTEN_ADDRESS="127.0.0.1")
    mysql = ["mysql", "--protocol=tcp", "-h", host, "-P", port,
             "-u", env["DB_USER"], "-N", "-B", "--unbuffered"]

    def sql(statement: str, selected: bool = True) -> str:
        return subprocess.check_output(mysql + ([database] if selected else []),
                                       input=statement, text=True, env=env,
                                       stderr=subprocess.DEVNULL).strip()

    def ready(process: subprocess.Popen, output_path: Path) -> None:
        deadline = time.monotonic() + 120
        while time.monotonic() < deadline and process.poll() is None:
            if "Entering game loop." in output_path.read_text(errors="replace"):
                return
            time.sleep(.1)
        raise AssertionError(output_path.read_text(errors="replace")[-2500:])

    sql("CREATE DATABASE " + database, False)
    try:
        sql((ROOT / "migrations/bootstrap_multithread_safe.sql").read_text())
        for args in (("adopt", "--kind", "fresh_bootstrap"), ("run",)):
            subprocess.run(["python3", "scripts/migration_runner.py", *args],
                           cwd=ROOT, env=env, check=True, capture_output=True)
        with tempfile.TemporaryDirectory(prefix="chaos-raise-") as temporary:
            game = Path(temporary)
            (game / "logs/log").mkdir(parents=True)
            journey.make_fixture(game)
            journey.generate_certificate(game)
            for kind in ("players", "critical"):
                (game / "journals" / kind).mkdir(parents=True, mode=0o700)
            env.update(PLAYER_SAVE_JOURNAL_DIR=str(game / "journals/players"),
                       CRITICAL_COMMAND_JOURNAL_DIR=str(game / "journals/critical"))
            output_path = game / "server.out"

            def start(chaos: bool) -> tuple[subprocess.Popen, int]:
                game_port, tls_port, websocket_port = journey.available_ports()
                env.update(CHAOS_MUD="TRUE" if chaos else "FALSE",
                           DURIS_TLS_PORT=str(tls_port),
                           DURIS_WEBSOCKET_PORT=str(websocket_port))
                output = output_path.open("w")
                process = subprocess.Popen(
                    [str(binary), "-d", str(game), str(game_port)], cwd=game,
                    env=env, stdout=output, stderr=subprocess.STDOUT)
                output.close()
                ready(process, output_path)
                return process, game_port

            process, game_port = start(False)
            client = None
            try:
                client = journey.MudClient(game_port)
                journey.create_character(client, expected_room=None, class_name="n",
                                         hometown="p")
                client.send("save")
                client.expect(f"Save complete for {journey.CHARACTER}.", timeout=30)
                client.close()
                client = None
                process.terminate()
                assert process.wait(timeout=30) == 0

                pid = int(sql("SELECT pid FROM player_data WHERE name='" +
                              journey.CHARACTER + "'"))
                assert pid > 0
                sql(f"UPDATE player_data SET level=56,highest_level=56,"
                    f"exp=999999999 WHERE pid={pid};"
                    f"DELETE FROM player_skills WHERE pid={pid} AND skill_id=380;"
                    f"INSERT INTO player_skills(pid,skill_id,learned,taught)"
                    f" VALUES({pid},380,100,100);"
                    f"DELETE FROM player_undead_slots WHERE pid={pid} AND circle=12;"
                    f"INSERT INTO player_undead_slots(pid,circle,slots)"
                    f" VALUES({pid},12,1);"
                    f"INSERT INTO player_affects"
                    f"(pid,type,duration,flags,modifier,location,level)"
                    f" VALUES({pid},2005,-1,16,380,0,56)")

                if "--npc-corpse" in sys.argv[2:]:
                    npc_uid = UIDS[0]
                    sql("INSERT INTO item_owner_revision"
                        "(owner_type,owner_id,owner_context_id,revision)"
                        f" VALUES(3,{ROOM},0,1);"
                        "INSERT INTO item_current_owner"
                        "(item_uid,root_item_uid,owner_type,owner_id,"
                        "owner_context_id,item_revision,vnum,state)"
                        f" VALUES({npc_uid},{npc_uid},3,{ROOM},0,1,2,1);"
                        "INSERT INTO saved_items"
                        "(item_key,room_vnum,vnum,item_type,obj_uid,value1,value2,"
                        "name,short_descr,description)"
                        f" VALUES('ordinary_npc_corpse',{ROOM},2,24,{npc_uid},4,56,"
                        "'ordinary beast corpse _npcorpse_',"
                        "'the corpse of an ordinary beast',"
                        "'The corpse of an ordinary beast is lying here.')")
                    process, game_port = start(True)
                    client = journey.reconnect_character(game_port,
                                                         expected_room=None)
                    client.pending.clear()
                    client.send("look")
                    room = client.expect("Pos: standing >", timeout=20)
                    assert "corpse of an ordinary beast" in room, room
                    client.send("cast 'create greater dracolich' corpse")
                    outcome, transcript = client.expect_any(
                        ("The corpse summons a greater",
                         "You can't animate", "This spell requires"), timeout=120)
                    assert outcome.startswith("The corpse summons"), transcript
                    assert sql("SELECT COUNT(*) FROM critical_operation_inbox "
                               "WHERE command_type=16") == "0"
                    client.pending.clear()
                    client.send("look")
                    raised_room = client.expect("Pos: standing >", timeout=20)
                    assert raised_room.lower().count("dracolich") == 1, raised_room
                    assert "corpse of an ordinary beast" not in raised_room
                    print("ordinary NPC corpse: one live follower, no player-corpse "
                          "durable command", flush=True)
                    return

                corpse_owner = (OWNER_PID << 32) | SAVE_ID
                coin_row = ("INSERT INTO corpse_items"
                            "(corpse_id,vnum,item_type,container_id,weight,"
                            "value0,value1,value2,value3,name,short_descr)"
                            " VALUES(@corpse_id,3,20,@root_id,2,1,2,3,4,"
                            "'coins','some coins')") if with_coins else ""
                sql("INSERT INTO corpses"
                    "(player_name,save_id,corpse_revision,room_vnum,short_descr,"
                    "description,name,weight,value2,value3,value5)"
                    f" VALUES('FixtureFallen',{SAVE_ID},1,{ROOM},"
                    "'the corpse of FixtureFallen',"
                    "'The corpse of FixtureFallen is lying here.',"
                    "'corpse fixturefallen',20,56,"
                    f"{OWNER_PID},0);"
                    "SET @corpse_id=LAST_INSERT_ID();"
                    "INSERT INTO corpse_items"
                    "(corpse_id,vnum,weight,cost,obj_uid,name,short_descr,description)"
                    f" VALUES(@corpse_id,48,10,215,{UIDS[0]},"
                    "'fixture backpack','a fixture backpack',"
                    "'A fixture backpack lies here.');"
                    "SET @root_id=LAST_INSERT_ID();"
                    "INSERT INTO corpse_items"
                    "(corpse_id,vnum,container_id,weight,cost,obj_uid,"
                    "name,short_descr,description)"
                    f" VALUES(@corpse_id,5,@root_id,2,150,{UIDS[1]},"
                    "'fixture note','a fixture note',"
                    "'A fixture note lies here.');"
                    "INSERT INTO corpse_items"
                    "(corpse_id,vnum,container_id,weight,cost,extra_flags,"
                    "obj_uid,name,short_descr,description)"
                    f" VALUES(@corpse_id,5,@root_id,1,150,{TRANSIENT},"
                    f"{UIDS[2]},'fixture fading note','a fixture fading note',"
                    "'A fixture fading note lies here.');" + coin_row)
                sql("INSERT INTO item_owner_revision"
                    "(owner_type,owner_id,owner_context_id,revision)"
                    f" VALUES(4,{corpse_owner},0,1)")
                for uid, parent, vnum in ((UIDS[0], "NULL", 48),
                                          (UIDS[1], str(UIDS[0]), 5),
                                          (UIDS[2], str(UIDS[0]), 5)):
                    sql("INSERT INTO item_current_owner"
                        "(item_uid,root_item_uid,parent_item_uid,owner_type,"
                        "owner_id,owner_context_id,item_revision,vnum,state)"
                        f" VALUES({uid},{UIDS[0]},{parent},4,"
                        f"{corpse_owner},0,5,{vnum},1)")

                process, game_port = start("--chaos-off" not in sys.argv[2:])
                client = journey.reconnect_character(game_port, expected_room=None)
                client.pending.clear()
                client.send("look")
                room = client.expect("Pos: standing >", timeout=20)
                assert "corpse of FixtureFallen" in room, room
                client.send("stand")
                client.expect("Pos: standing >", timeout=15)
                wallet_before = tuple(map(int, sql(
                    f"SELECT copper,silver,gold,platinum FROM player_data WHERE pid={pid}"
                ).split("\t")))
                client.pending.clear()
                client.send("cast 'create greater dracolich' corpse")
                outcome, transcript = client.expect_any(
                    ("The corpse resists the raising and remains intact.",
                     "The corpse summons a greater",
                     "The raising committed, but its live effects needed recovery.",
                     "You cannot control any more greater dracoliches!",
                     "This spell requires the corpse of a more powerful being!",
                     "You do not know that spell",
                     "You don't have that spell memorized."),
                    timeout=120)
                assert "You cannot control" not in outcome and \
                       "requires the corpse" not in outcome and \
                       "do not know" not in outcome and \
                       "memorized" not in outcome, transcript
                receipt = sql("SELECT result_code FROM critical_operation_inbox "
                              "WHERE command_type=16 ORDER BY created_at DESC LIMIT 1")
                print("cast outcome:", outcome, "receipt:", receipt, flush=True)
                if expect_refusal:
                    assert outcome.startswith("The corpse resists"), transcript
                    assert receipt == "116", receipt
                    assert sql("SELECT COUNT(*) FROM corpses") == "1"
                    assert sql("SELECT COUNT(*) FROM corpse_items") == (
                        "4" if with_coins else "3")
                    assert sql("SELECT COUNT(*) FROM item_current_owner WHERE "
                               f"item_uid IN {UIDS} AND owner_type=4") == "3"
                    assert tuple(map(int, sql(
                        f"SELECT copper,silver,gold,platinum FROM player_data WHERE pid={pid}"
                    ).split("\t"))) == wallet_before
                    print("baseline chaos cast: ESTALE refusal retained full corpse graph",
                          flush=True)
                else:
                    assert outcome.startswith("The corpse summons"), transcript
                    assert receipt == "0", receipt
                    assert sql("SELECT COUNT(*) FROM corpses") == "0"
                    assert sql("SELECT COUNT(*) FROM corpse_items") == "0"
                    assert sql("SELECT COUNT(*) FROM player_items WHERE "
                               f"obj_uid IN ({UIDS[0]},{UIDS[1]})") == "2"
                    assert sql("SELECT CONCAT(owner_type,':',state) FROM "
                               f"item_current_owner WHERE item_uid={UIDS[2]}") == "8:2"
                    assert sql("SELECT COUNT(*) FROM player_items WHERE "
                               f"obj_uid={UIDS[2]}") == "0"
                    wallet_after = tuple(map(int, sql(
                        f"SELECT copper,silver,gold,platinum FROM player_data WHERE pid={pid}"
                    ).split("\t")))
                    expected_money = (1, 2, 3, 4) if with_coins else (0, 0, 0, 0)
                    assert wallet_after == tuple(before + delta for before, delta in
                                                 zip(wallet_before, expected_money))
                    client.pending.clear()
                    client.send("look")
                    raised_room = client.expect("Pos: standing >", timeout=20)
                    assert raised_room.lower().count("dracolich") == 1, raised_room
                    assert raised_room.count("(minion)") == 1, raised_room
                    client.send("save")
                    client.expect(f"Save complete for {journey.CHARACTER}.", timeout=30)
                    assert sql("SELECT COUNT(*) FROM player_pets WHERE "
                               f"owner_pid={pid}") == "1"
                    mode = "chaos-off" if "--chaos-off" in sys.argv[2:] else "chaos-on"
                    print(f"fixed {mode} cast: one follower, legitimate nested graph,"
                          " transient UID destroyed", flush=True)
                print("commands: look; stand; cast 'create greater dracolich' corpse",
                      flush=True)
                print("result:", outcome, "receipt:", receipt, flush=True)
            except Exception:
                print("diagnostic logs:", "\n".join(
                    line for line in journey.runtime_logs(game).splitlines()
                    if any(term in line.lower() for term in
                           ("durable_raise", "corpse_lifecycle", "raise_submission",
                            "critical_command", "mysql")))[-6000:], flush=True)
                print("server tail:", output_path.read_text(errors="replace")[-2500:],
                      flush=True)
                print("logs tail:", journey.runtime_logs(game)[-2500:], flush=True)
                if client:
                    print("game tail:", client.transcript.decode(errors="replace")[-2000:],
                          flush=True)
                raise
            finally:
                if client:
                    client.close()
                if process.poll() is None:
                    process.terminate()
                    process.wait(timeout=30)
    finally:
        sql("DROP DATABASE " + database, False)


if __name__ == "__main__":
    run(Path(sys.argv[1]).resolve(), "--expect-refusal" in sys.argv[2:],
        "--coinless" not in sys.argv[2:])
