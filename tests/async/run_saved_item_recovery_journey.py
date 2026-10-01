#!/usr/bin/env python3
"""Exercise saved-item replay across real full-world MariaDB recovery boots.

Requires make world, a MariaDB server and TEST_DB_HOST/PORT/USER/PASSWORD
pointing to a disposable loopback database. Each case creates and drops its
own uniquely named schema.
"""
import os
from pathlib import Path
import signal
import subprocess
import sys
import tempfile
import time
import uuid

import test_flatfile_combat_journey as journey

ROOT = Path(__file__).resolve().parents[2]


def run(binary: Path, count: int, expect_abort: bool, malformed: bool = False,
        boot_only: bool = False, baseline_loss: bool = False,
        nested_replay: bool = False, fault_stage: str | None = None,
        reject_child: bool = False, concurrent_save: bool = False,
        concurrent_child: bool = False,
        malformed_child: bool = False) -> None:
    host = os.environ.get("TEST_DB_HOST", "127.0.0.1")
    port = os.environ.get("TEST_DB_PORT", "3306")
    assert host in ("127.0.0.1", "localhost", "::1")
    database = "restore_allocator_" + uuid.uuid4().hex[:12]
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

    sql("CREATE DATABASE " + database, False)
    try:
        sql((ROOT / "migrations/bootstrap_multithread_safe.sql").read_text())
        for args in (("adopt", "--kind", "fresh_bootstrap"), ("run",)):
            subprocess.run(["python3", "scripts/migration_runner.py", *args],
                           cwd=ROOT, env=env, check=True, capture_output=True)
        if count:
            sql("INSERT INTO item_owner_revision(owner_type,owner_id,revision)"
                " VALUES (3,22800,1)")
            for number, vnum, noun in ((1, 48, "backpack"), (2, 5, "note"))[:count]:
                uid = 800000000000 + number
                key = "restore_allocator_" + str(number)
                sql("INSERT INTO item_current_owner"
                    "(item_uid,root_item_uid,owner_type,owner_id,item_revision,vnum,state)"
                    f" VALUES ({uid},{uid},3,22800,1,{vnum},1);"
                    "INSERT INTO saved_items"
                    "(item_key,room_vnum,vnum,obj_uid,name,short_descr,description)"
                    f" VALUES ('{key}',22800,{vnum},{uid},"
                    f"'recovery {noun}','a recovery {noun}',"
                    f"'A recovery {noun} lies here.')")
        if malformed:
            sql("INSERT INTO saved_items(item_key,room_vnum,vnum) VALUES "
                "('invalid_room',999999,48),('invalid_vnum',22800,999999)")
        if nested_replay:
            assert count == 1
            root_id = sql("SELECT id FROM saved_items "
                          "WHERE item_key='restore_allocator_1'")
            sql("UPDATE saved_items SET weight=4,cost=215,value0=120 "
                "WHERE id=" + root_id + ";"
                "INSERT INTO saved_items"
                "(item_key,room_vnum,vnum,container_id,obj_uid,name,short_descr,"
                "description,cost) VALUES "
                f"('restore_allocator_1',22800,5,{root_id},800000000002,"
                "'recovery note','a recovery note','A recovery note lies here.',150);"
                "INSERT INTO item_current_owner"
                "(item_uid,root_item_uid,parent_item_uid,owner_type,owner_id,"
                "item_revision,vnum,state) VALUES "
                "(800000000002,800000000001,800000000001,3,22800,1,5,1);"
                f"INSERT INTO saved_item_affects(item_id,location,modifier)"
                f" VALUES ({root_id},1,3);"
                f"INSERT INTO saved_item_extra_descr(item_id,keyword,description)"
                f" VALUES ({root_id},'recovery stitching','The stitches are intact.')")
        if reject_child:
            sql("CREATE TRIGGER reject_saved_child BEFORE INSERT ON saved_items "
                "FOR EACH ROW SET NEW.item_key=IF("
                "NEW.container_id IS NOT NULL AND NEW.item_key LIKE 'item.uid.%',"
                "NULL,NEW.item_key)")
        if malformed_child:
            sql("UPDATE saved_items SET vnum=999999 "
                "WHERE item_key='restore_allocator_1' AND container_id IS NOT NULL")
        with tempfile.TemporaryDirectory(prefix="restore-allocator-") as temporary:
            game = Path(temporary)
            (game / "logs/log").mkdir(parents=True)
            journey.make_fixture(game)
            journey.generate_certificate(game)
            for kind in ("players", "critical"):
                (game / "journals" / kind).mkdir(parents=True, mode=0o700)
            game_port, tls_port, websocket_port = journey.available_ports()
            env.update(DURIS_TLS_PORT=str(tls_port),
                       DURIS_WEBSOCKET_PORT=str(websocket_port),
                       PLAYER_SAVE_JOURNAL_DIR=str(game / "journals/players"),
                       CRITICAL_COMMAND_JOURNAL_DIR=str(game / "journals/critical"))
            if fault_stage:
                env["DURIS_SAVED_ITEM_RESTORE_FAULT_STAGE"] = fault_stage
            if concurrent_save:
                env["DURIS_SAVED_ITEM_RESTORE_PAUSE_STAGE"] = "after_publication"
            if concurrent_child:
                env["DURIS_SAVED_ITEM_RESTORE_PAUSE_STAGE"] = "after_acknowledgment"
            output_path = game / "server.out"
            with output_path.open("w") as output:
                process = subprocess.Popen(
                    [str(binary), "-d", str(game), str(game_port)],
                    cwd=game, env=env, stdout=output, stderr=subprocess.STDOUT)
            client = None
            try:
                deadline = time.monotonic() + 120
                replaced_source = False
                while time.monotonic() < deadline and process.poll() is None:
                    if (concurrent_child and not replaced_source and
                            "injected pause stage=after_acknowledgment" in
                            journey.runtime_logs(game)):
                        sql("START TRANSACTION;"
                            "DELETE FROM saved_items WHERE "
                            "item_key='restore_allocator_1' AND "
                            "container_id IS NOT NULL;"
                            "INSERT INTO saved_items"
                            "(item_key,room_vnum,vnum,container_id,obj_uid,name,"
                            "short_descr,description,cost) VALUES "
                            f"('restore_allocator_1',22800,5,{root_id},"
                            "800000000002,'newer recovery note',"
                            "'a newer recovery note',"
                            "'A newer recovery note lies here.',150);"
                            "COMMIT")
                        replaced_source = True
                    if (concurrent_save and not replaced_source and
                            "injected pause stage=after_publication" in
                            journey.runtime_logs(game)):
                        sql("START TRANSACTION;"
                            f"DELETE FROM saved_items WHERE id={root_id};"
                            "INSERT INTO saved_items"
                            "(item_key,room_vnum,vnum,obj_uid,name,short_descr,"
                            "description,cost) VALUES "
                            "('restore_allocator_1',22800,48,800000000001,"
                            "'newer recovery backpack','a newer recovery backpack',"
                            "'A newer recovery backpack lies here.',215);"
                            "SET @new_root=LAST_INSERT_ID();"
                            "INSERT INTO saved_items"
                            "(item_key,room_vnum,vnum,container_id,obj_uid,name,"
                            "short_descr,description,cost) VALUES "
                            "('restore_allocator_1',22800,5,@new_root,800000000002,"
                            "'recovery note','a recovery note',"
                            "'A recovery note lies here.',150);"
                            "COMMIT")
                        replaced_source = True
                    if "Entering game loop." in output_path.read_text(errors="replace"):
                        break
                    time.sleep(.1)
                logs = output_path.read_text(errors="replace") + journey.runtime_logs(game)
                if fault_stage:
                    assert process.wait(timeout=30) == 80, logs[-2500:]
                    assert f"injected stop stage={fault_stage}" in logs, logs[-2500:]
                    source_count = sql("SELECT COUNT(*) FROM saved_items "
                                       "WHERE item_key='restore_allocator_1'")
                    destination_count = sql("SELECT COUNT(*) FROM saved_items "
                                            "WHERE item_key='item.uid.800000000001'")
                    receipt_count = sql("SELECT COUNT(*) FROM saved_item_recovery_handoff")
                    if fault_stage in ("before_materialization", "after_materialization",
                                       "after_publication", "before_acknowledgment"):
                        assert (source_count, destination_count, receipt_count) == (
                            "2", "0", "0")
                    elif fault_stage == "after_retirement_commit":
                        assert (source_count, destination_count, receipt_count) == (
                            "0", "2", "1")
                    else:
                        assert (source_count, destination_count, receipt_count) == (
                            "2", "2", "1")
                    env.pop("DURIS_SAVED_ITEM_RESTORE_FAULT_STAGE")
                    game_port, tls_port, websocket_port = journey.available_ports()
                    env.update(DURIS_TLS_PORT=str(tls_port),
                               DURIS_WEBSOCKET_PORT=str(websocket_port))
                    with output_path.open("w") as output:
                        process = subprocess.Popen(
                            [str(binary), "-d", str(game), str(game_port)],
                            cwd=game, env=env, stdout=output,
                            stderr=subprocess.STDOUT)
                    deadline = time.monotonic() + 120
                    while time.monotonic() < deadline and process.poll() is None:
                        if "Entering game loop." in output_path.read_text(errors="replace"):
                            break
                        time.sleep(.1)
                    assert process.poll() is None, output_path.read_text()[-2500:]
                    client = journey.MudClient(game_port)
                    journey.create_character(client, expected_room=None)
                    client.pending.clear()
                    client.send("look")
                    room = client.expect("Pos: standing >", timeout=20)
                    assert room.count("recovery backpack") == 1, room
                    client.pending.clear()
                    client.send("look in backpack")
                    inside = client.expect("Pos: standing >", timeout=20)
                    assert inside.count("recovery note") == 1, inside
                    assert sql("SELECT COUNT(*) FROM saved_items "
                               "WHERE item_key='restore_allocator_1'") == "0"
                    assert sql("SELECT COUNT(*) FROM saved_items "
                               "WHERE item_key='item.uid.800000000001'") == "2"
                    assert sql("SELECT COUNT(*) FROM saved_item_recovery_handoff "
                               "WHERE retired_at IS NOT NULL") == "1"
                    print(f"fault replay: {fault_stage} preserved one nested UID graph",
                          flush=True)
                    return
                if expect_abort:
                    assert process.wait(timeout=20) == -signal.SIGABRT, logs[-2000:]
                    assert "free(): invalid pointer" in logs, logs[-2000:]
                    print("baseline: nonempty full-world MariaDB recovery boot aborted at allocator cleanup",
                          flush=True)
                    return
                assert process.poll() is None and "Entering game loop." in logs, logs[-2000:]
                if count and not boot_only and not malformed_child:
                    client = journey.MudClient(game_port)
                    journey.create_character(client, expected_room=None)
                    client.pending.clear()
                    client.send("look")
                    room = client.expect("Pos: standing >", timeout=20)
                    assert "recovery backpack" in room, room
                    if count == 2:
                        assert "recovery note" in room, room
                if malformed_child:
                    assert sql("SELECT COUNT(*) FROM saved_items "
                               "WHERE item_key='restore_allocator_1'") == "2"
                    assert sql("SELECT COUNT(*) FROM saved_item_recovery_handoff") == "0"
                    client = journey.MudClient(game_port)
                    journey.create_character(client, expected_room=None)
                    client.pending.clear()
                    client.send("look")
                    room = client.expect("Pos: standing >", timeout=20)
                    assert "recovery backpack" not in room, room
                    sql("UPDATE saved_items SET vnum=5 "
                        "WHERE item_key='restore_allocator_1' "
                        "AND container_id IS NOT NULL")
                    client.close()
                    client = None
                    process.terminate()
                    assert process.wait(timeout=30) == 0
                    game_port, tls_port, websocket_port = journey.available_ports()
                    env.update(DURIS_TLS_PORT=str(tls_port),
                               DURIS_WEBSOCKET_PORT=str(websocket_port))
                    with output_path.open("w") as output:
                        process = subprocess.Popen(
                            [str(binary), "-d", str(game), str(game_port)],
                            cwd=game, env=env, stdout=output,
                            stderr=subprocess.STDOUT)
                    deadline = time.monotonic() + 120
                    while time.monotonic() < deadline and process.poll() is None:
                        if "Entering game loop." in output_path.read_text(errors="replace"):
                            break
                        time.sleep(.1)
                    assert process.poll() is None, output_path.read_text()[-2500:]
                    client = journey.reconnect_character(game_port, expected_room=None)
                    client.pending.clear()
                    client.send("look")
                    room = client.expect("Pos: standing >", timeout=20)
                    assert room.count("recovery backpack") == 1, room
                    client.pending.clear()
                    client.send("look in backpack")
                    inside = client.expect("Pos: standing >", timeout=20)
                    assert inside.count("recovery note") == 1, inside
                    assert sql("SELECT COUNT(*) FROM saved_items "
                               "WHERE item_key='restore_allocator_1'") == "0"
                    assert sql("SELECT COUNT(*) FROM saved_items "
                               "WHERE item_key='item.uid.800000000001'") == "2"
                    print("malformed child: source retained, repaired graph replayed once",
                          flush=True)
                    return
                if concurrent_save:
                    assert replaced_source, "fixture did not replace the source during pause"
                    assert sql("SELECT COUNT(*) FROM saved_items "
                               "WHERE item_key='restore_allocator_1'") == "2"
                    assert sql("SELECT COUNT(*) FROM saved_items "
                               "WHERE item_key='item.uid.800000000001'") == "0"
                    assert sql("SELECT COUNT(*) FROM saved_item_recovery_handoff") == "0"
                    client.close()
                    client = None
                    process.terminate()
                    assert process.wait(timeout=30) == 0
                    env.pop("DURIS_SAVED_ITEM_RESTORE_PAUSE_STAGE")
                    game_port, tls_port, websocket_port = journey.available_ports()
                    env.update(DURIS_TLS_PORT=str(tls_port),
                               DURIS_WEBSOCKET_PORT=str(websocket_port))
                    with output_path.open("w") as output:
                        process = subprocess.Popen(
                            [str(binary), "-d", str(game), str(game_port)],
                            cwd=game, env=env, stdout=output,
                            stderr=subprocess.STDOUT)
                    deadline = time.monotonic() + 120
                    while time.monotonic() < deadline and process.poll() is None:
                        if "Entering game loop." in output_path.read_text(errors="replace"):
                            break
                        time.sleep(.1)
                    assert process.poll() is None, output_path.read_text()[-2500:]
                    client = journey.reconnect_character(game_port, expected_room=None)
                    client.pending.clear()
                    client.send("look")
                    room = client.expect("Pos: standing >", timeout=20)
                    assert room.count("newer recovery backpack") == 1, room
                    assert "a recovery backpack" not in room, room
                    assert sql("SELECT COUNT(*) FROM saved_items "
                               "WHERE item_key='restore_allocator_1'") == "0"
                    assert sql("SELECT COUNT(*) FROM saved_items "
                               "WHERE item_key='item.uid.800000000001'") == "2"
                    print("concurrent save: newer source generation retained and replayed once",
                          flush=True)
                    return
                if concurrent_child:
                    assert replaced_source, "fixture did not replace child during pause"
                    assert sql("SELECT COUNT(*) FROM saved_items "
                               "WHERE item_key='restore_allocator_1'") == "2"
                    assert sql("SELECT COUNT(*) FROM saved_items "
                               "WHERE item_key='item.uid.800000000001'") == "2"
                    assert sql("SELECT COUNT(*) FROM saved_item_recovery_handoff "
                               "WHERE retired_at IS NULL") == "1"
                    client.close()
                    client = None
                    process.terminate()
                    assert process.wait(timeout=30) == 0
                    env.pop("DURIS_SAVED_ITEM_RESTORE_PAUSE_STAGE")
                    game_port, tls_port, websocket_port = journey.available_ports()
                    env.update(DURIS_TLS_PORT=str(tls_port),
                               DURIS_WEBSOCKET_PORT=str(websocket_port))
                    with output_path.open("w") as output:
                        process = subprocess.Popen(
                            [str(binary), "-d", str(game), str(game_port)],
                            cwd=game, env=env, stdout=output,
                            stderr=subprocess.STDOUT)
                    deadline = time.monotonic() + 120
                    while time.monotonic() < deadline and process.poll() is None:
                        if "Entering game loop." in output_path.read_text(errors="replace"):
                            break
                        time.sleep(.1)
                    assert process.poll() is None, output_path.read_text()[-2500:]
                    client = journey.reconnect_character(game_port, expected_room=None)
                    client.pending.clear()
                    client.send("look")
                    room = client.expect("Pos: standing >", timeout=20)
                    assert room.count("recovery backpack") == 1, room
                    client.pending.clear()
                    client.send("look in backpack")
                    inside = client.expect("Pos: standing >", timeout=20)
                    assert inside.count("recovery note") == 1, inside
                    assert sql("SELECT COUNT(*) FROM saved_items "
                               "WHERE item_key='restore_allocator_1'") == "2"
                    assert sql("SELECT COUNT(*) FROM saved_item_recovery_handoff "
                               "WHERE retired_at IS NULL") == "1"
                    print("concurrent child replacement: exact source generation retained",
                          flush=True)
                    return
                if reject_child:
                    assert sql("SELECT COUNT(*) FROM saved_items "
                               "WHERE item_key='restore_allocator_1'") == "2"
                    assert sql("SELECT COUNT(*) FROM saved_items "
                               "WHERE item_key='item.uid.800000000001'") == "0"
                    assert sql("SELECT COUNT(*) FROM saved_item_recovery_handoff") == "0"
                    assert "handoff deferred; original source retained" in logs, logs[-2500:]
                    sql("DROP TRIGGER reject_saved_child")
                    client.close()
                    client = None
                    process.terminate()
                    assert process.wait(timeout=30) == 0
                    game_port, tls_port, websocket_port = journey.available_ports()
                    env.update(DURIS_TLS_PORT=str(tls_port),
                               DURIS_WEBSOCKET_PORT=str(websocket_port))
                    with output_path.open("w") as output:
                        process = subprocess.Popen(
                            [str(binary), "-d", str(game), str(game_port)],
                            cwd=game, env=env, stdout=output,
                            stderr=subprocess.STDOUT)
                    deadline = time.monotonic() + 120
                    while time.monotonic() < deadline and process.poll() is None:
                        if "Entering game loop." in output_path.read_text(errors="replace"):
                            break
                        time.sleep(.1)
                    assert process.poll() is None, output_path.read_text()[-2500:]
                    client = journey.reconnect_character(game_port, expected_room=None)
                    client.pending.clear()
                    client.send("look")
                    client.expect("Pos: standing >", timeout=20)
                    client.pending.clear()
                    client.send("look in backpack")
                    inside = client.expect("Pos: standing >", timeout=20)
                    assert inside.count("recovery note") == 1, inside
                    assert sql("SELECT COUNT(*) FROM saved_items "
                               "WHERE item_key='restore_allocator_1'") == "0"
                    assert sql("SELECT COUNT(*) FROM saved_items "
                               "WHERE item_key='item.uid.800000000001'") == "2"
                    print("partial SQL failure: source and nested graph survived, retry acknowledged",
                          flush=True)
                    return
                if malformed:
                    assert sql("SELECT COUNT(*) FROM saved_items "
                               "WHERE item_key IN ('invalid_room','invalid_vnum')") == "2"
                    assert sql("SELECT COUNT(*) FROM saved_items "
                               "WHERE item_key LIKE 'item.uid.%'") == str(count)
                if nested_replay:
                    assert sql("SELECT COUNT(*) FROM saved_items "
                               "WHERE item_key='restore_allocator_1'") == "0"
                    assert sql("SELECT COUNT(*) FROM saved_items "
                               "WHERE item_key='item.uid.800000000001'") == "2"
                    assert sql("SELECT GROUP_CONCAT(obj_uid ORDER BY id) "
                               "FROM saved_items WHERE item_key="
                               "'item.uid.800000000001'") == (
                               "800000000001,800000000002")
                    assert sql("SELECT COUNT(*) FROM saved_item_recovery_handoff "
                               "WHERE retired_at IS NOT NULL") == "1"
                    assert sql("SELECT COUNT(*) FROM saved_item_affects a "
                               "JOIN saved_items i ON i.id=a.item_id "
                               "WHERE i.obj_uid=800000000001 AND a.location=1 "
                               "AND a.modifier=3") == "1"
                    assert sql("SELECT COUNT(*) FROM saved_item_extra_descr e "
                               "JOIN saved_items i ON i.id=e.item_id "
                               "WHERE i.obj_uid=800000000001 "
                               "AND e.keyword='recovery stitching'") == "1"
                    if boot_only:
                        print("sanitizer boot: nested UID graph and metadata handoff passed",
                              flush=True)
                        return
                    client.pending.clear()
                    client.send("look in backpack")
                    inside = client.expect("Pos: standing >", timeout=20)
                    assert "recovery note" in inside, inside
                    for restart in (1, 2):
                        client.close()
                        client = None
                        process.terminate()
                        assert process.wait(timeout=30) == 0
                        game_port, tls_port, websocket_port = journey.available_ports()
                        env.update(DURIS_TLS_PORT=str(tls_port),
                                   DURIS_WEBSOCKET_PORT=str(websocket_port))
                        with output_path.open("w") as output:
                            process = subprocess.Popen(
                                [str(binary), "-d", str(game), str(game_port)],
                                cwd=game, env=env, stdout=output,
                                stderr=subprocess.STDOUT)
                        deadline = time.monotonic() + 120
                        while time.monotonic() < deadline and process.poll() is None:
                            if "Entering game loop." in output_path.read_text(errors="replace"):
                                break
                            time.sleep(.1)
                        assert process.poll() is None, output_path.read_text()[-2000:]
                        client = journey.reconnect_character(game_port, expected_room=None)
                        client.pending.clear()
                        client.send("look")
                        room = client.expect("Pos: standing >", timeout=20)
                        assert room.count("recovery backpack") == 1, room
                        client.pending.clear()
                        client.send("look in backpack")
                        inside = client.expect("Pos: standing >", timeout=20)
                        assert inside.count("recovery note") == 1, inside
                        assert sql("SELECT COUNT(*) FROM saved_items "
                                   "WHERE item_key='item.uid.800000000001'") == "2"
                        assert sql("SELECT COUNT(*) FROM saved_items "
                                   "WHERE item_key='restore_allocator_1'") == "0"
                    print("fixed: nested UID graph, affects and metadata survived two restarts",
                          flush=True)
                    return
                if baseline_loss:
                    assert count == 1
                    assert sql("SELECT COUNT(*) FROM saved_items") == "0", (
                        "baseline unexpectedly retained its only durable source")
                    client.close()
                    client = None
                    process.terminate()
                    assert process.wait(timeout=30) == 0
                    game_port, tls_port, websocket_port = journey.available_ports()
                    env.update(DURIS_TLS_PORT=str(tls_port),
                               DURIS_WEBSOCKET_PORT=str(websocket_port))
                    with output_path.open("w") as output:
                        process = subprocess.Popen(
                            [str(binary), "-d", str(game), str(game_port)],
                            cwd=game, env=env, stdout=output,
                            stderr=subprocess.STDOUT)
                    deadline = time.monotonic() + 120
                    while time.monotonic() < deadline and process.poll() is None:
                        if "Entering game loop." in output_path.read_text(errors="replace"):
                            break
                        time.sleep(.1)
                    logs = output_path.read_text(errors="replace") + journey.runtime_logs(game)
                    assert process.poll() is None, logs[-2000:]
                    assert sql("SELECT COUNT(*) FROM saved_items") == "0"
                    client = journey.reconnect_character(game_port, expected_room=None)
                    client.pending.clear()
                    client.send("look")
                    room = client.expect("Pos: standing >", timeout=20)
                    assert "recovery backpack" not in room, room
                    print("baseline: first boot published root and deleted source; restart lost item",
                          flush=True)
                    return
                print(f"fixed: {count} restored roots, boot passed",
                      flush=True)
            except Exception:
                print("server tail:", output_path.read_text(errors="replace")[-2000:],
                      flush=True)
                print("logs tail:", journey.runtime_logs(game)[-2000:], flush=True)
                if client:
                    print("game tail:", client.transcript.decode(errors="replace")[-2000:],
                          flush=True)
                raise
            finally:
                if client:
                    client.close()
                if process.poll() is None:
                    process.terminate()
                    process.wait(timeout=20)
    finally:
        sql("DROP DATABASE " + database, False)


if __name__ == "__main__":
    binary = Path(sys.argv[1]).resolve()
    if "--nested-boot-only" in sys.argv[2:]:
        run(binary, 1, False, nested_replay=True, boot_only=True)
        sys.exit(0)
    fault = next((arg.split("=", 1)[1] for arg in sys.argv[2:]
                  if arg.startswith("--fault-stage=")), None)
    if fault:
        run(binary, 1, False, nested_replay=True, fault_stage=fault)
        sys.exit(0)
    if "--reject-child" in sys.argv[2:]:
        run(binary, 1, False, nested_replay=True, reject_child=True)
        sys.exit(0)
    if "--concurrent-save" in sys.argv[2:]:
        run(binary, 1, False, nested_replay=True, concurrent_save=True)
        sys.exit(0)
    if "--concurrent-child" in sys.argv[2:]:
        run(binary, 1, False, nested_replay=True, concurrent_child=True)
        sys.exit(0)
    if "--malformed-child" in sys.argv[2:]:
        run(binary, 1, False, nested_replay=True, malformed_child=True)
        sys.exit(0)
    if "--nested-replay" in sys.argv[2:]:
        run(binary, 1, False, nested_replay=True)
        sys.exit(0)
    if "--baseline-loss" in sys.argv[2:]:
        run(binary, 1, False, baseline_loss=True)
        sys.exit(0)
    abort = "--expect-abort" in sys.argv[2:]
    boot_only = "--boot-only" in sys.argv[2:]
    for entries in ([1] if abort else [0, 1, 2]):
        run(binary, entries, abort, boot_only=boot_only)
    if not abort:
        run(binary, 2, False, malformed=True, boot_only=boot_only)
