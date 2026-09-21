#!/usr/bin/env python3
"""Cast Greater Dracolich against a disposable persisted player corpse."""
import os
from pathlib import Path
import subprocess
import struct
import sys
import tempfile
import time
import uuid

import test_flatfile_combat_journey as journey

ROOT = Path(__file__).resolve().parents[2]
TRANSIENT = 524288
UIDS = (880000000001, 880000000002, 880000000003)
GIVE_UIDS = (880000000004, 880000000005)
PROC_UID = 880000000006
NPC_GEAR_UIDS = (880000000007, 880000000008, 880000000009)
NPC_TRANSIENT_UID = 880000000010
NPC_MONEY_UID = 880000000011
OWNER_PID, SAVE_ID, ROOM = 880001, 1004, 22800


def run(binary: Path, expect_refusal: bool, with_coins: bool) -> bool:
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
        schema = subprocess.run(
            ["bash", "migrations/verify_runtime_compatibility.sh"], cwd=ROOT,
            env=env, capture_output=True, text=True)
        raise AssertionError(output_path.read_text(errors="replace")[-2500:] +
                             journey.runtime_logs(game)[-4500:] +
                             schema.stderr[-1800:])

    sql("CREATE DATABASE " + database, False)
    try:
        sql((ROOT / "migrations/bootstrap_multithread_safe.sql").read_text())
        for args in (("adopt", "--kind", "fresh_bootstrap"), ("run",)):
            subprocess.run(["python3", "scripts/migration_runner.py", *args],
                           cwd=ROOT, env=env, check=True, capture_output=True)
        sql((ROOT / "migrations/immutable/0028_pet_custody.sql").read_text())
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
                if any(flag in sys.argv[2:] for flag in
                       ("--pet-give-probe", "--pet-give-stale-return",
                        "--pet-give-stale-player")):
                    # New characters begin with more carried starter objects than
                    # their ordinary item limit. Clear this disposable inventory
                    # so the give path tests custody instead of capacity refusal.
                    sql(f"DELETE FROM player_items WHERE pid={pid};"
                        "DELETE FROM item_current_owner WHERE owner_type=1 "
                        f"AND owner_id={pid} AND owner_context_id=0;")
                sql(f"UPDATE player_data SET level=56,highest_level=56,"
                    f"base_dex=90,base_str=90,"
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
                    if "--npc-charmed-give-control" in sys.argv[2:]:
                        sql("INSERT INTO item_owner_revision"
                            "(owner_type,owner_id,owner_context_id,revision)"
                            f" VALUES(1,{pid},0,1) ON DUPLICATE KEY UPDATE "
                            "revision=GREATEST(revision,1);"
                            "INSERT INTO player_items"
                            "(pid,vnum,weight,wear_flags,obj_uid,name,short_descr)"
                            f" VALUES({pid},5,1,1,{GIVE_UIDS[1]},"
                            "'control trinket','a control trinket');"
                            "INSERT INTO item_current_owner"
                            "(item_uid,root_item_uid,owner_type,owner_id,"
                            "owner_context_id,item_revision,vnum,state)"
                            f" VALUES({GIVE_UIDS[1]},{GIVE_UIDS[1]},1,{pid},0,1,5,1)")
                    sql("INSERT INTO item_owner_revision"
                        "(owner_type,owner_id,owner_context_id,revision)"
                        f" VALUES(3,{ROOM},0,1);"
                        "INSERT INTO item_current_owner"
                        "(item_uid,root_item_uid,owner_type,owner_id,"
                        "owner_context_id,item_revision,vnum,state)"
                        f" VALUES({npc_uid},{npc_uid},3,{ROOM},0,1,2,1);"
                        "INSERT INTO saved_items"
                        "(item_key,room_vnum,vnum,item_type,obj_uid,timer,value1,value2,"
                        "name,short_descr,description)"
                        f" VALUES('ordinary_npc_corpse',{ROOM},2,24,{npc_uid},1000000,4,56,"
                        "'ordinary beast corpse _npcorpse_',"
                        "'the corpse of an ordinary beast',"
                        "'The corpse of an ordinary beast is lying here.')")
                    if "--npc-gear-probe" in sys.argv[2:]:
                        sql("SET @corpse_row=(SELECT id FROM saved_items WHERE "
                            f"obj_uid={npc_uid});"
                            "INSERT INTO saved_items(item_key,room_vnum,vnum,"
                            "container_id,obj_uid,weight,wear_flags,name,short_descr)"
                            f" VALUES('ordinary_npc_corpse',{ROOM},48,@corpse_row,"
                            f"{NPC_GEAR_UIDS[0]},5,1,'ordinary backpack',"
                            "'an ordinary backpack');"
                            "SET @ordinary_row=LAST_INSERT_ID();"
                            "INSERT INTO saved_items(item_key,room_vnum,vnum,"
                            "container_id,obj_uid,weight,wear_flags,name,short_descr)"
                            f" VALUES('ordinary_npc_corpse',{ROOM},48,@ordinary_row,"
                            f"{NPC_GEAR_UIDS[1]},2,0,'no-take token',"
                            "'a no-take token');"
                            "SET @no_take_row=LAST_INSERT_ID();"
                            "INSERT INTO saved_items(item_key,room_vnum,vnum,"
                            "container_id,obj_uid,weight,extra_flags,wear_flags,name,short_descr)"
                            f" VALUES('ordinary_npc_corpse',{ROOM},5,@no_take_row,"
                            f"{NPC_GEAR_UIDS[2]},1,2050,0,'no-show token',"
                            "'a no-show token');"
                            "INSERT INTO saved_items(item_key,room_vnum,vnum,"
                            "container_id,obj_uid,weight,extra_flags,wear_flags,name,short_descr)"
                            f" VALUES('ordinary_npc_corpse',{ROOM},5,@ordinary_row,"
                            f"{NPC_TRANSIENT_UID},1,{TRANSIENT},1,'fading token',"
                            "'a fading token');"
                            "INSERT INTO saved_items(item_key,room_vnum,vnum,item_type,"
                            "container_id,obj_uid,weight,value0,value1,value2,value3,"
                            "name,short_descr)"
                            f" VALUES('ordinary_npc_corpse',{ROOM},3,20,@corpse_row,"
                            f"{NPC_MONEY_UID},2,1,2,3,4,'coins','some coins');"
                            "INSERT INTO item_current_owner(item_uid,root_item_uid,"
                            "parent_item_uid,owner_type,owner_id,owner_context_id,"
                            "item_revision,vnum,state) VALUES"
                            f"({NPC_GEAR_UIDS[0]},{npc_uid},{npc_uid},3,{ROOM},0,1,48,1),"
                            f"({NPC_GEAR_UIDS[1]},{npc_uid},{NPC_GEAR_UIDS[0]},3,{ROOM},0,1,48,1),"
                            f"({NPC_GEAR_UIDS[2]},{npc_uid},{NPC_GEAR_UIDS[1]},3,{ROOM},0,1,5,1),"
                            f"({NPC_TRANSIENT_UID},{npc_uid},{NPC_GEAR_UIDS[0]},3,{ROOM},0,1,5,1),"
                            f"({NPC_MONEY_UID},{npc_uid},{npc_uid},3,{ROOM},0,1,3,1)")
                    process, game_port = start("--chaos-off" not in sys.argv[2:])
                    client = journey.reconnect_character(game_port,
                                                         expected_room=None)
                    client.pending.clear()
                    client.send("look")
                    room = client.expect("Pos: standing >", timeout=20)
                    assert "corpse of an ordinary beast" in room, room
                    client.send("cast 'create greater dracolich' corpse")
                    outcome, transcript = client.expect_any(
                        ("The corpse summons a greater",
                         "The corpse resists the raising and remains intact.",
                         "You abort your spell before it's done!",
                         "You can't animate", "This spell requires"), timeout=120)
                    if outcome.startswith(("You abort your spell", "The corpse resists")):
                        return False
                    if "--npc-gear-probe" in sys.argv[2:]:
                        assert outcome.startswith("The corpse summons"), transcript
                        assert sql("SELECT COUNT(*) FROM critical_operation_inbox "
                                   "WHERE command_type=16 AND result_code=0") == "1"
                        tracked = (f"({npc_uid},{NPC_GEAR_UIDS[0]},"
                                   f"{NPC_GEAR_UIDS[1]},{NPC_GEAR_UIDS[2]},"
                                   f"{NPC_TRANSIENT_UID},{NPC_MONEY_UID})")
                        assert sql("SELECT COUNT(*) FROM saved_items WHERE obj_uid IN "
                                   f"{tracked}") == "0"
                        if sql("SELECT COUNT(*) FROM player_pets WHERE "
                               f"owner_pid={pid} AND pet_uid={npc_uid}") == "0":
                            assert sql("SELECT COUNT(*) FROM item_current_owner WHERE "
                                       f"item_uid IN {tracked} AND owner_type=8 "
                                       "AND owner_id=0 AND state=2") == "6"
                            assert sql("SELECT COUNT(*) FROM player_items WHERE obj_uid IN "
                                       f"{tracked}") == "0"
                            assert sql("SELECT COUNT(*) FROM player_pet_items WHERE obj_uid IN "
                                       f"{tracked}") == "0"
                            print("hostile NPC corpse raise destroyed the complete graph; "
                                  "retrying for friendly custody", flush=True)
                            return False
                        assert sql("SELECT COUNT(*) FROM player_items WHERE obj_uid IN "
                                   f"{tracked}") == "0"
                        assert sql("SELECT COUNT(*) FROM player_pet_items WHERE obj_uid IN "
                                   f"({NPC_GEAR_UIDS[0]},{NPC_GEAR_UIDS[1]},"
                                   f"{NPC_GEAR_UIDS[2]})") == "3"
                        assert sql("SELECT COUNT(*) FROM item_current_owner WHERE "
                                   f"item_uid IN ({NPC_GEAR_UIDS[0]},"
                                   f"{NPC_GEAR_UIDS[1]},{NPC_GEAR_UIDS[2]}) "
                                   f"AND owner_type=11 AND owner_id={npc_uid} "
                                   f"AND owner_context_id={pid} AND state=1") == "3"
                        assert sql("SELECT CONCAT(root_item_uid,':',"
                                   "COALESCE(parent_item_uid,0)) FROM item_current_owner "
                                   f"WHERE item_uid={NPC_GEAR_UIDS[2]}") == \
                               f"{NPC_GEAR_UIDS[0]}:{NPC_GEAR_UIDS[1]}"
                        assert sql("SELECT CONCAT(extra_flags,':',wear_flags) FROM "
                                   "player_pet_items WHERE obj_uid="
                                   f"{NPC_GEAR_UIDS[2]}") == "2050:0"
                        assert sql("SELECT COUNT(*) FROM item_current_owner WHERE "
                                   f"item_uid IN ({npc_uid},{NPC_TRANSIENT_UID},"
                                   f"{NPC_MONEY_UID}) AND owner_type=8 AND owner_id=0 "
                                   "AND state=2") == "3"
                        client.pending.clear()
                        client.send("look")
                        room = client.expect("Pos: standing >", timeout=20)
                        assert "corpse of an ordinary beast" not in room, room
                        assert room.lower().count("dracolich") == 1, room
                        client.send("save")
                        client.expect(f"Save complete for {journey.CHARACTER}.", timeout=30)
                        client.close()
                        client = None
                        process.terminate()
                        assert process.wait(timeout=30) == 0
                        process, game_port = start(True)
                        client = journey.reconnect_character(game_port,
                                                             expected_room=None)
                        client.pending.clear()
                        client.send("look")
                        room = client.expect("Pos: standing >", timeout=20)
                        assert "corpse of an ordinary beast" not in room, room
                        assert room.lower().count("dracolich") == 1, room
                        assert sql("SELECT COUNT(*) FROM player_pet_items WHERE obj_uid IN "
                                   f"({NPC_GEAR_UIDS[0]},{NPC_GEAR_UIDS[1]},"
                                   f"{NPC_GEAR_UIDS[2]})") == "3"
                        assert sql("SELECT COUNT(DISTINCT obj_uid) FROM player_pet_items "
                                   "WHERE obj_uid IN "
                                   f"({NPC_GEAR_UIDS[0]},{NPC_GEAR_UIDS[1]},"
                                   f"{NPC_GEAR_UIDS[2]})") == "3"
                        assert sql("SELECT CONCAT(root_item_uid,':',"
                                   "COALESCE(parent_item_uid,0),':',owner_type,':',"
                                   "owner_id,':',owner_context_id) FROM "
                                   f"item_current_owner WHERE item_uid={NPC_GEAR_UIDS[2]}") == \
                               (f"{NPC_GEAR_UIDS[0]}:{NPC_GEAR_UIDS[1]}:11:"
                                f"{npc_uid}:{pid}")
                        print("equipped NPC corpse raise: ordinary, no-take, and "
                              "no-show nested gear retained once under pet custody "
                              "across save/restart", flush=True)
                        return True
                    assert outcome.startswith("The corpse summons"), transcript
                    assert sql("SELECT COUNT(*) FROM critical_operation_inbox "
                               "WHERE command_type=16 AND result_code=0") == "1"
                    if sql("SELECT COUNT(*) FROM player_pets WHERE "
                           f"owner_pid={pid} AND pet_uid={npc_uid}") == "0":
                        return False
                    client.pending.clear()
                    client.send("look")
                    client.expect("Adrift in the Plane of Life", timeout=20)
                    raised_room = client.expect("Pos: standing >", timeout=20)
                    assert raised_room.lower().count("dracolich") == 1, raised_room
                    assert "corpse of an ordinary beast" not in raised_room
                    if "--npc-charmed-give-control" in sys.argv[2:]:
                        client.pending.clear()
                        client.send("give trinket dracolich")
                        client.expect("Ok.", timeout=20)
                        assert sql("SELECT CONCAT(owner_type,':',owner_id,':',state) "
                                   "FROM item_current_owner WHERE item_uid=" +
                                   str(GIVE_UIDS[1])) == f"11:{npc_uid}:1"
                        print("ordinary NPC-corpse charmed follower accepted a durable "
                              "item under pet custody", flush=True)
                    print("ordinary NPC corpse: one live follower and one durable "
                          "world-corpse command", flush=True)
                    return True

                corpse_owner = (OWNER_PID << 32) | SAVE_ID
                coin_row = ("INSERT INTO corpse_items"
                            "(corpse_id,vnum,item_type,container_id,weight,"
                            "value0,value1,value2,value3,name,short_descr)"
                            " VALUES(@corpse_id,3,20,@root_id,2,1,2,3,4,"
                            "'coins','some coins')") if with_coins else ""
                give_probe = "--give-probe" in sys.argv[2:]
                pet_give_probe = any(flag in sys.argv[2:] for flag in
                                     ("--pet-give-probe", "--pet-give-stale-return",
                                      "--pet-give-stale-player"))
                pet_probe = "--pet-probe" in sys.argv[2:] or pet_give_probe
                fixture_probe = give_probe or pet_probe
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
                    "(corpse_id,vnum,weight,cost,wear_flags,obj_uid,name,short_descr,description)"
                    f" VALUES(@corpse_id,48,10,215,1,{UIDS[0]},"
                    "'fixture backpack','a fixture backpack',"
                    "'A fixture backpack lies here.');"
                    "SET @root_id=LAST_INSERT_ID();"
                    "INSERT INTO corpse_items"
                    "(corpse_id,vnum,container_id,weight,cost,obj_uid,"
                    "name,short_descr,description)"
                    f" VALUES(@corpse_id,5,@root_id,2,150,{UIDS[1]},"
                    "'fixture note','a fixture note',"
                    "'A fixture note lies here.');"
                    "SET @note_id=LAST_INSERT_ID();"
                    "INSERT INTO corpse_item_affects(item_id,location,modifier)"
                    " VALUES(@note_id,1,7);"
                    "INSERT INTO corpse_item_extra_descr(item_id,keyword,description)"
                    " VALUES(@root_id,'fixture-mark','The maker marked this bag.');"
                    "INSERT INTO corpse_items"
                    "(corpse_id,vnum,container_id,weight,cost,extra_flags,"
                    "obj_uid,name,short_descr,description)"
                    f" VALUES(@corpse_id,5,@root_id,1,150,{TRANSIENT},"
                    f"{UIDS[2]},'fixture fading note','a fixture fading note',"
                    "'A fixture fading note lies here.');" +
                    ("INSERT INTO corpse_items"
                     "(corpse_id,vnum,weight,cost,wear_flags,obj_uid,name,short_descr,description)"
                     f" VALUES(@corpse_id,48,1,215,1,{GIVE_UIDS[0]},"
                     "'empty satchel','an empty satchel',"
                     "'An empty satchel lies here.');"
                     "INSERT INTO corpse_items"
                     "(corpse_id,vnum,weight,cost,wear_flags,obj_uid,name,"
                     "short_descr,description)"
                     f" VALUES(@corpse_id,5,1,150,3,{GIVE_UIDS[1]},"
                     "'ordinary trinket','an ordinary trinket',"
                     "'An ordinary trinket lies here.');"
                     "INSERT INTO corpse_items"
                     "(corpse_id,vnum,weight,cost,extra_flags,wear_flags,obj_uid,"
                     "name,short_descr,description)"
                     f" VALUES(@corpse_id,5,1,150,2050,0,{PROC_UID},"
                     "'hidden proc token','a hidden proc token',"
                     "'A hidden proc token lies here.');" if fixture_probe else "") + coin_row)
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
                if fixture_probe:
                    for uid, vnum in ((GIVE_UIDS[0], 48), (GIVE_UIDS[1], 5),
                                      (PROC_UID, 5)):
                        sql("INSERT INTO item_current_owner"
                            "(item_uid,root_item_uid,owner_type,owner_id,"
                            "owner_context_id,item_revision,vnum,state)"
                            f" VALUES({uid},{uid},4,{corpse_owner},0,5,{vnum},1)")

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
                save_revision_before = int(sql(
                    f"SELECT save_revision FROM player_data WHERE pid={pid}"))
                stale_player = "--stale-player" in sys.argv[2:]
                if stale_player:
                    sql("UPDATE item_owner_revision SET revision=revision+1 "
                        f"WHERE owner_type=1 AND owner_id={pid} AND "
                        "owner_context_id=0")
                client.pending.clear()
                client.send("cast 'create greater dracolich' corpse")
                outcome, transcript = client.expect_any(
                    ("The corpse resists the raising and remains intact.",
                     "The corpse summons a greater",
                     "The raising committed, but its live effects needed recovery.",
                     "You abort your spell before it's done!",
                     "You cannot control any more greater dracoliches!",
                     "This spell requires the corpse of a more powerful being!",
                     "You do not know that spell",
                     "You don't have that spell memorized."),
                    timeout=120)
                if outcome.startswith("You abort your spell"):
                    assert sql("SELECT COUNT(*) FROM corpses") == "1"
                    assert sql("SELECT COUNT(*) FROM player_pets WHERE "
                               f"owner_pid={pid}") == "0"
                    print("interrupted cast: corpse retained; retrying a fresh "
                          "fixture", flush=True)
                    return False
                assert "You cannot control" not in outcome and \
                       "requires the corpse" not in outcome and \
                       "do not know" not in outcome and \
                       "memorized" not in outcome, transcript
                receipt = sql("SELECT result_code FROM critical_operation_inbox "
                              "WHERE command_type=16 ORDER BY created_at DESC LIMIT 1")
                print("cast outcome:", outcome, "receipt:", receipt, flush=True)
                if expect_refusal or stale_player:
                    assert outcome.startswith("The corpse resists"), transcript
                    assert receipt == "116", receipt
                    assert sql("SELECT COUNT(*) FROM corpses") == "1"
                    assert sql("SELECT COUNT(*) FROM corpse_items") == str(
                        (4 if with_coins else 3) + (3 if fixture_probe else 0))
                    assert sql("SELECT COUNT(*) FROM item_current_owner WHERE "
                               f"item_uid IN {UIDS} AND owner_type=4") == "3"
                    assert tuple(map(int, sql(
                        f"SELECT copper,silver,gold,platinum FROM player_data WHERE pid={pid}"
                    ).split("\t"))) == wallet_before
                    assert sql("SELECT COUNT(*) FROM player_pets WHERE "
                               f"owner_pid={pid}") == "0"
                    print("baseline chaos cast: ESTALE refusal retained full corpse graph",
                          flush=True)
                else:
                    assert outcome.startswith("The corpse summons"), transcript
                    assert receipt == "0", receipt
                    assert sql("SELECT COUNT(*) FROM corpses") == "0"
                    assert sql("SELECT COUNT(*) FROM corpse_items") == "0"
                    if pet_probe and sql("SELECT COUNT(*) FROM player_pets WHERE "
                                         f"owner_pid={pid}") == "0":
                        assert sql("SELECT COUNT(*) FROM player_items WHERE "
                                   f"obj_uid IN ({UIDS[0]},{UIDS[1]})") == "2"
                        assert sql("SELECT COUNT(*) FROM player_pet_items WHERE "
                                   f"obj_uid IN ({UIDS[0]},{UIDS[1]})") == "0"
                        assert sql("SELECT CONCAT(owner_type,':',owner_id) FROM "
                                   "item_current_owner WHERE "
                                   f"item_uid={UIDS[0]}") == f"1:{pid}"
                        assert sql("SELECT COUNT(*) FROM player_pets WHERE "
                                   f"owner_pid={pid}") == "0"
                        assert sql("SELECT CONCAT(owner_type,':',state) FROM "
                                   "item_current_owner WHERE "
                                   f"item_uid={UIDS[2]}") == "8:2"
                        wallet_after_hostile = tuple(map(int, sql(
                            "SELECT copper,silver,gold,platinum FROM player_data "
                            f"WHERE pid={pid}").split("\t")))
                        money = (1, 2, 3, 4) if with_coins else (0, 0, 0, 0)
                        assert wallet_after_hostile == tuple(
                            before + delta for before, delta in
                            zip(wallet_before, money))
                        print("hostile raise: committed graph belongs to caster; "
                              "no durable follower record", flush=True)
                        return "--expect-hostile" in sys.argv[2:]
                    if pet_probe and "--expect-hostile" in sys.argv[2:]:
                        print("follower raise; retrying for hostile outcome", flush=True)
                        return False
                    item_table = "player_pet_items" if pet_probe else "player_items"
                    assert sql(f"SELECT COUNT(*) FROM {item_table} WHERE "
                               f"obj_uid IN ({UIDS[0]},{UIDS[1]})") == "2"
                    if pet_probe:
                        assert sql("SELECT COUNT(*) FROM player_items WHERE "
                                   f"obj_uid IN ({UIDS[0]},{UIDS[1]})") == "0"
                        assert sql("SELECT CONCAT(owner_type,':',owner_id,':',"
                                   "owner_context_id) FROM item_current_owner WHERE "
                                   f"item_uid={UIDS[0]}") == f"11:{corpse_owner}:{pid}"
                        assert sql("SELECT CONCAT(root_item_uid,':',"
                                   "parent_item_uid,':',owner_type,':',owner_id) "
                                   "FROM item_current_owner WHERE "
                                   f"item_uid={UIDS[1]}") == \
                                   f"{UIDS[0]}:{UIDS[0]}:11:{corpse_owner}"
                        assert sql("SELECT CONCAT(child.container_id,':',root.id) "
                                   "FROM player_pet_items child JOIN "
                                   "player_pet_items root ON root.obj_uid="
                                   f"{UIDS[0]} WHERE child.obj_uid={UIDS[1]}") == \
                                   sql("SELECT CONCAT(id,':',id) FROM "
                                       "player_pet_items WHERE "
                                       f"obj_uid={UIDS[0]}")
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
                    equip_probe = "--equip-probe" in sys.argv[2:]
                    if equip_probe:
                        assert pet_probe
                        client.pending.clear()
                        client.send("order dracolich wear trinket")
                        client.expect("Pos: standing >", timeout=20)
                        client.send("save")
                        client.expect(f"Save complete for {journey.CHARACTER}.",
                                      timeout=30)
                        assert int(sql("SELECT equip_slot FROM player_pet_items "
                                       f"WHERE obj_uid={GIVE_UIDS[1]}")) > 0
                    if "--give-probe" in sys.argv[2:]:
                        client.pending.clear()
                        client.send("give backpack dracolich")
                        refusal = client.expect(
                            "That item cannot be given to a pet or mob because its "
                            "custody cannot be saved yet.", timeout=20)
                        assert "custody cannot be saved yet" in refusal
                        assert sql("SELECT COUNT(*) FROM player_items WHERE "
                                   f"pid={pid} AND obj_uid={UIDS[0]}") == "1"
                        assert sql("SELECT CONCAT(owner_type,':',owner_id,':',state) "
                                   "FROM item_current_owner WHERE "
                                   f"item_uid={UIDS[0]}") == f"1:{pid}:1"
                        print("give backpack dracolich: first leg refused; "
                              "full graph stayed with player", flush=True)
                        client.send("give satchel dracolich")
                        client.expect("custody cannot be saved yet.", timeout=20)
                        print("give satchel dracolich (empty): refused", flush=True)
                        client.send("give trinket dracolich")
                        client.expect("custody cannot be saved yet.", timeout=20)
                        print("give trinket dracolich: refused", flush=True)
                        client.pending.clear()
                        client.send(f"order dracolich give backpack {journey.CHARACTER}")
                        ordered = client.expect("Pos: standing >", timeout=20)
                        print("order dracolich give backpack:",
                              ordered.strip()[-180:], flush=True)
                        assert sql("SELECT COUNT(*) FROM player_items WHERE "
                                   f"pid={pid} AND obj_uid IN ({UIDS[0]},{UIDS[1]})") == "2"
                        assert sql("SELECT COUNT(*) FROM player_pet_items WHERE "
                                   f"obj_uid IN ({UIDS[0]},{UIDS[1]})") == "0"
                    if pet_give_probe:
                        def custody(uid: int, owner_type: int, owner_id: int,
                                    table: str) -> None:
                            other = "player_items" if table == "player_pet_items" \
                                else "player_pet_items"
                            assert sql("SELECT CONCAT(owner_type,':',owner_id,':',"
                                       "state) FROM item_current_owner WHERE "
                                       f"item_uid={uid}") == \
                                   f"{owner_type}:{owner_id}:1"
                            assert sql(f"SELECT COUNT(*) FROM {table} WHERE "
                                       f"obj_uid={uid}") == "1"
                            assert sql(f"SELECT COUNT(*) FROM {other} WHERE "
                                       f"obj_uid={uid}") == "0"
                            if uid == UIDS[0]:
                                suffix = "player_pet_item_extra_descr" if \
                                    table == "player_pet_items" else \
                                    "player_item_extra_descr"
                                assert sql(f"SELECT CONCAT(ed.keyword,':',ed.description) "
                                           f"FROM {suffix} ed JOIN {table} item "
                                           f"ON item.id=ed.item_id WHERE item.obj_uid={uid}") \
                                       == "fixture-mark:The maker marked this bag."
                            if uid == UIDS[1]:
                                suffix = "player_pet_item_affects" if \
                                    table == "player_pet_items" else \
                                    "player_item_affects"
                                assert sql(f"SELECT CONCAT(af.location,':',af.modifier) "
                                           f"FROM {suffix} af JOIN {table} item "
                                           f"ON item.id=af.item_id WHERE item.obj_uid={uid}") \
                                       == "1:7"

                        def command(text: str, receipt: str) -> None:
                            client.pending.clear()
                            client.send(text)
                            result = client.expect(receipt, timeout=40)
                            print(text, ":", result.strip()[-160:], flush=True)

                        if "--pet-give-stale-return" in sys.argv[2:]:
                            sql("UPDATE item_owner_revision SET revision=revision+1 "
                                f"WHERE owner_type=11 AND owner_id={corpse_owner} "
                                f"AND owner_context_id={pid}")
                            command(f"order dracolich give backpack "
                                    f"{journey.CHARACTER}",
                                    "The pet transfer did not commit")
                            for uid in (UIDS[0], UIDS[1]):
                                custody(uid, 11, corpse_owner, "player_pet_items")
                            print("stale pet revision: rejected return retained "
                                  "nested pet graph", flush=True)
                            return True
                        command(f"order dracolich give backpack {journey.CHARACTER}",
                                "gives you a fixture backpack")
                        for uid in (UIDS[0], UIDS[1]):
                            custody(uid, 1, pid, "player_items")
                        assert sql("SELECT CONCAT(child.container_id,':',root.id) "
                                   "FROM player_items child JOIN player_items root "
                                   f"ON root.obj_uid={UIDS[0]} WHERE child.obj_uid="
                                   f"{UIDS[1]}") == \
                                   sql("SELECT CONCAT(id,':',id) FROM player_items "
                                       f"WHERE obj_uid={UIDS[0]}")
                        if "--pet-give-stale-player" in sys.argv[2:]:
                            sql("UPDATE item_owner_revision SET revision=revision+1 "
                                f"WHERE owner_type=1 AND owner_id={pid} "
                                "AND owner_context_id=0")
                            command("give backpack dracolich",
                                    "The pet transfer did not commit")
                            for uid in (UIDS[0], UIDS[1]):
                                custody(uid, 1, pid, "player_items")
                            print("stale player revision: rejected give retained "
                                  "nested player graph", flush=True)
                            return True
                        command("give backpack dracolich", "Ok.")
                        for uid in (UIDS[0], UIDS[1]):
                            custody(uid, 11, corpse_owner, "player_pet_items")

                        for keyword, uid in (("satchel", GIVE_UIDS[0]),
                                             ("trinket", GIVE_UIDS[1])):
                            command(f"order dracolich give {keyword} "
                                    f"{journey.CHARACTER}", "gives you")
                            custody(uid, 1, pid, "player_items")
                            command(f"give {keyword} dracolich", "Ok.")
                            custody(uid, 11, corpse_owner, "player_pet_items")
                        command("order dracolich wear trinket", "Pos: standing >")
                        client.send("save")
                        client.expect(f"Save complete for {journey.CHARACTER}.",
                                      timeout=30)
                        assert int(sql("SELECT equip_slot FROM player_pet_items "
                                       f"WHERE obj_uid={GIVE_UIDS[1]}")) > 0
                        print("full and empty backpack, ordinary item, and auto-equip "
                              "completed round trips with one UID each", flush=True)
                    restart_before_save = "--restart-before-save" in sys.argv[2:]
                    if equip_probe and restart_before_save:
                        raise AssertionError("equipment save and pre-save restart are separate journeys")
                    if not restart_before_save and not equip_probe:
                        client.send("save")
                        client.expect(f"Save complete for {journey.CHARACTER}.", timeout=30)
                    assert sql("SELECT COUNT(*) FROM player_pets WHERE "
                               f"owner_pid={pid}") == "1"
                    if pet_probe:
                        state_hex = sql("SELECT restore_state FROM player_pets "
                                        f"WHERE owner_pid={pid}")
                        state = bytes.fromhex(state_hex)
                        charm_deadline, death_deadline = struct.unpack_from(
                            "<qq", state, 8)
                        assert int(sql("SELECT charm_duration FROM player_pets "
                                       f"WHERE owner_pid={pid}")) > 0
                        assert time.time() < charm_deadline < death_deadline
                    if "--live-death-probe" in sys.argv[2:]:
                        assert pet_probe
                        expiring_state = bytearray.fromhex(state_hex)
                        now = int(time.time())
                        struct.pack_into("<qq", expiring_state, 8, now + 45, now + 55)
                        client.close()
                        client = None
                        process.terminate()
                        assert process.wait(timeout=30) == 0
                        sql("UPDATE player_pets SET restore_state='" +
                            expiring_state.hex() + "' WHERE owner_pid=" + str(pid))
                        process, game_port = start(True)
                        client = journey.reconnect_character(game_port,
                                                             expected_room=None)
                        deadline = time.monotonic() + 75
                        saw_follower = False
                        while True:
                            client.pending.clear()
                            client.send("look")
                            client.expect("Adrift in the Plane of Life", timeout=20)
                            visible = client.expect("Pos: standing >", timeout=20)
                            if "dracolich" not in visible.lower():
                                assert saw_follower, visible
                                for item_name in ("fixture backpack", "empty satchel",
                                                  "ordinary trinket"):
                                    assert item_name not in visible, visible
                                break
                            saw_follower = True
                            assert time.monotonic() < deadline, "pet did not die"
                            time.sleep(1)
                        client.send("save")
                        client.expect(f"Save complete for {journey.CHARACTER}.",
                                      timeout=30)
                        assert sql("SELECT hold_reason FROM player_pets WHERE "
                                   f"owner_pid={pid}") == "6"
                        assert sql("SELECT COUNT(*) FROM player_pet_items WHERE "
                                   f"pet_id IN (SELECT id FROM player_pets WHERE "
                                   f"owner_pid={pid})") == "5"
                        assert sql("SELECT COUNT(*) FROM item_current_owner WHERE "
                                   "owner_type=11 AND "
                                   f"owner_context_id={pid} AND state=1") == "5"
                        print("live charm expiry and pet death: no room gear; five "
                              "UIDs held exactly once", flush=True)
                        return True
                    if "--dismiss-probe" in sys.argv[2:]:
                        assert pet_probe
                        if "--dismiss-after-restore" in sys.argv[2:]:
                            client.close()
                            client = None
                            process.terminate()
                            assert process.wait(timeout=30) == 0
                            process, game_port = start(True)
                            client = journey.reconnect_character(game_port,
                                                                 expected_room=None)
                            client.pending.clear()
                            client.send("look")
                            active_room = client.expect("Pos: standing >", timeout=20)
                            assert active_room.lower().count("dracolich") == 1, active_room
                        client.pending.clear()
                        client.send("dismiss dracolich")
                        client.expect("Pos: standing >", timeout=20)
                        client.send("look")
                        dismissed_room = client.expect("Pos: standing >", timeout=20)
                        assert "dracolich" not in dismissed_room.lower(), dismissed_room
                        assert "fixture backpack" not in dismissed_room, dismissed_room
                        client.send("save")
                        client.expect(f"Save complete for {journey.CHARACTER}.",
                                      timeout=30)
                        assert sql("SELECT hold_reason FROM player_pets WHERE "
                                   f"owner_pid={pid}") == "6"
                        assert sql("SELECT COUNT(*) FROM player_pet_items WHERE "
                                   f"pet_id IN (SELECT id FROM player_pets WHERE "
                                   f"owner_pid={pid})") == "5"
                        assert sql("SELECT COUNT(*) FROM item_current_owner WHERE "
                                   "owner_type=11 AND "
                                   f"owner_context_id={pid} AND state=1") == "5"
                        client.close()
                        client = None
                        process.terminate()
                        assert process.wait(timeout=30) == 0
                        process, game_port = start(True)
                        client = journey.reconnect_character(game_port,
                                                             expected_room=None)
                        client.pending.clear()
                        client.send("look")
                        restarted_room = client.expect("Pos: standing >", timeout=20)
                        assert "dracolich" not in restarted_room.lower(), restarted_room
                        assert "fixture backpack" not in restarted_room, restarted_room
                        assert sql("SELECT COUNT(*) FROM player_pet_items WHERE "
                                   f"obj_uid IN ({UIDS[0]},{UIDS[1]},"
                                   f"{GIVE_UIDS[0]},{GIVE_UIDS[1]},{PROC_UID})") == "5"
                        print("dismiss/restart: no room gear or follower; five UIDs "
                              "held exactly once under pet custody", flush=True)
                        return True
                    if fixture_probe:
                        if restart_before_save:
                            save_revision_at_crash = int(sql(
                                "SELECT save_revision FROM player_data WHERE "
                                f"pid={pid}"))
                            print("player save revisions before cast / at crash:",
                                  save_revision_before, save_revision_at_crash,
                                  flush=True)
                        expire_before_restart = "--expire-before-restart" in sys.argv[2:]
                        if expire_before_restart:
                            assert pet_probe
                            expired_state = bytearray.fromhex(state_hex)
                            expired_at = int(time.time()) - 1
                            struct.pack_into("<qq", expired_state, 8,
                                             expired_at, expired_at)
                            sql("UPDATE player_pets SET restore_state='" +
                                expired_state.hex() + "' WHERE owner_pid=" +
                                str(pid))
                        if restart_before_save:
                            process.kill()
                            assert process.wait(timeout=30) != 0
                        client.close()
                        client = None
                        if not restart_before_save:
                            process.terminate()
                            assert process.wait(timeout=30) == 0
                        if expire_before_restart:
                            assert sql("SELECT restore_state FROM player_pets "
                                       f"WHERE owner_pid={pid}") == \
                                       expired_state.hex()
                        process, game_port = start(True)
                        client = journey.reconnect_character(game_port,
                                                             expected_room=None)
                        client.pending.clear()
                        client.send("look")
                        restored = client.expect("Pos: standing >", timeout=20)
                        if expire_before_restart:
                            assert "dracolich" not in restored.lower(), restored
                            client.send("save")
                            client.expect(
                                f"Save complete for {journey.CHARACTER}.",
                                timeout=30)
                            assert sql("SELECT hold_reason FROM player_pets "
                                       f"WHERE owner_pid={pid}") == "3"
                            assert sql("SELECT COUNT(*) FROM player_pet_items "
                                       f"WHERE pet_id IN (SELECT id FROM player_pets "
                                       f"WHERE owner_pid={pid})") == "5"
                            assert sql("SELECT COUNT(*) FROM item_current_owner "
                                       "WHERE owner_type=11 AND "
                                       f"owner_context_id={pid} AND state=1") == "5"
                            print("expired follower held: no live pet; all five "
                                  "UIDs retained under pet custody", flush=True)
                            return True
                        assert restored.lower().count("dracolich") == 1, restored
                        if restart_before_save:
                            client.send("save")
                            client.expect(f"Save complete for {journey.CHARACTER}.", timeout=30)
                        tracked = f"({UIDS[0]},{UIDS[1]},{GIVE_UIDS[0]},{GIVE_UIDS[1]},{PROC_UID})"
                        if pet_probe:
                            assert sql("SELECT COUNT(*) FROM player_pet_items WHERE "
                                       f"obj_uid IN {tracked}") == "5"
                            assert sql("SELECT COUNT(DISTINCT obj_uid) FROM "
                                       f"player_pet_items WHERE obj_uid IN {tracked}") == "5"
                            assert sql("SELECT COUNT(*) FROM player_items WHERE "
                                       f"obj_uid IN {tracked}") == "0"
                            restored_state = bytes.fromhex(sql(
                                "SELECT restore_state FROM player_pets "
                                f"WHERE owner_pid={pid}"))
                            assert struct.unpack_from("<qq", restored_state, 8) == \
                                   (charm_deadline, death_deadline)
                            assert sql("SELECT CONCAT(root_item_uid,':',"
                                       "parent_item_uid,':',owner_type,':',owner_id) "
                                       "FROM item_current_owner WHERE "
                                       f"item_uid={UIDS[1]}") == \
                                       f"{UIDS[0]}:{UIDS[0]}:11:{corpse_owner}"
                            assert sql("SELECT CONCAT(pi.extra_flags,':',pi.wear_flags,':',"
                                       "own.owner_type,':',own.owner_id) FROM "
                                       "player_pet_items pi JOIN item_current_owner own "
                                       "ON own.item_uid=pi.obj_uid "
                                       f"WHERE pi.obj_uid={PROC_UID}") == \
                                       f"2050:0:11:{corpse_owner}"
                            if equip_probe or pet_give_probe:
                                assert int(sql("SELECT equip_slot FROM player_pet_items "
                                               f"WHERE obj_uid={GIVE_UIDS[1]}")) > 0
                            print("restart: follower and all five UIDs restored "
                                  "under pet custody", flush=True)
                        else:
                            assert sql("SELECT COUNT(*) FROM player_items WHERE "
                                       f"obj_uid IN {tracked}") == "5"
                            assert sql("SELECT COUNT(*) FROM player_pet_items WHERE "
                                       f"obj_uid IN {tracked}") == "0"
                            assert sql("SELECT CONCAT(pi.extra_flags,':',pi.wear_flags,':',"
                                       "own.owner_type,':',own.owner_id) FROM player_items pi "
                                       "JOIN item_current_owner own ON own.item_uid=pi.obj_uid "
                                       f"WHERE pi.obj_uid={PROC_UID}") == f"2050:0:1:{pid}"
                            print("restart: no-show/proc-flag/no-take fixture was "
                                  "player-owned; all refused UIDs remain with player",
                                  flush=True)
                    mode = "chaos-off" if "--chaos-off" in sys.argv[2:] else "chaos-on"
                    print(f"fixed {mode} cast: one follower, legitimate nested graph,"
                          " transient UID destroyed", flush=True)
                print("commands: look; stand; cast 'create greater dracolich' corpse",
                      flush=True)
                print("result:", outcome, "receipt:", receipt, flush=True)
                return True
            except Exception:
                print("diagnostic logs:", "\n".join(
                    line for line in journey.runtime_logs(game).splitlines()
                    if any(term in line.lower() for term in
                           ("durable_raise", "corpse_lifecycle", "corpse_trace",
                            "raise_submission",
                            "critical_command", "pet_transfer", "item_movement",
                            "sql_load_saved_item_contents", "ownership", "mysql")))[-6000:],
                      flush=True)
                print("server tail:", output_path.read_text(errors="replace")[-2500:],
                      flush=True)
                runtime_log = journey.runtime_logs(game)
                print("load diagnostics:", "\n".join(
                    line for line in runtime_log.splitlines()
                    if "player_load" in line or "PERSISTENCE:" in line)[-5000:],
                    flush=True)
                print("logs tail:", runtime_log[-2500:], flush=True)
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
    attempts = 20 if "--expect-hostile" in sys.argv[2:] else 5
    for attempt in range(attempts):
        if run(Path(sys.argv[1]).resolve(), "--expect-refusal" in sys.argv[2:],
               "--coinless" not in sys.argv[2:]):
            break
        print(f"retrying a fresh cast after outcome {attempt + 1}", flush=True)
    else:
        raise AssertionError("desired pet or hostile outcome did not occur")
