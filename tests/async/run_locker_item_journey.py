#!/usr/bin/env python3
"""Exercise nested SQL locker custody and cold-reload recovery through gameplay.

Requires a disposable loopback MySQL instance, a built server and generated
areas/world.* files. The script creates and drops its own random schema.
"""

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
ROOT_UID = 880000000101
CHILD_UID = 880000000102
BANK_ROOM = 5384
BANK_NAME = "The Bank of Fort Marigot"


def run(binary: Path) -> None:
    host = os.environ.get("TEST_DB_HOST", "")
    assert os.environ.get("TEST_DB_DISPOSABLE") == "1"
    assert host in ("127.0.0.1", "localhost", "::1")
    port = os.environ["TEST_DB_PORT"]
    database = "item_locker_" + uuid.uuid4().hex[:12]
    env = dict(os.environ, ENVIRONMENT="local", DB_HOST=host, DB_PORT=port,
               DB_NAME=database, DB_USER=os.environ["TEST_DB_USER"],
               DB_PASSWD=os.environ["TEST_DB_PASSWORD"],
               MYSQL_PWD=os.environ["TEST_DB_PASSWORD"],
               DB_ALLOWED_TARGETS=host + "/" + database,
               PERSISTENCE_MODE="mariadb-primary", DB_TLS="FALSE", REDIS="FALSE",
               CHAOS_MUD="FALSE", LISTEN_ADDRESS="127.0.0.1",
               DURIS_WEBSOCKET_LISTEN_ADDRESS="127.0.0.1")
    mysql = ["mysql", "--no-defaults", "--protocol=tcp", "-h", host,
             "-P", port, "-u", env["DB_USER"], "-N", "-B", "--unbuffered"]

    def sql(statement: str, *, selected: bool = True) -> str:
        return subprocess.check_output(mysql + ([database] if selected else []),
                                       input=statement, text=True, env=env,
                                       stderr=subprocess.DEVNULL).strip()

    sql("CREATE DATABASE " + database, selected=False)
    try:
        sql((ROOT / "migrations/bootstrap_multithread_safe.sql").read_text())
        for args in (("adopt", "--kind", "fresh_bootstrap"), ("run",)):
            subprocess.run(["python3", "scripts/migration_runner.py", *args],
                           cwd=ROOT, env=env, check=True, capture_output=True)
        with tempfile.TemporaryDirectory(prefix="item-locker-") as temporary:
            game = Path(temporary)
            (game / "logs/log").mkdir(parents=True)
            (game / "Players").mkdir()
            journey.make_fixture(game)
            journey.generate_certificate(game)
            for kind in ("players", "critical"):
                (game / "journals" / kind).mkdir(parents=True, mode=0o700)
            env.update(PLAYER_SAVE_JOURNAL_DIR=str(game / "journals/players"),
                       CRITICAL_COMMAND_JOURNAL_DIR=str(game / "journals/critical"))
            assert sql("SELECT COUNT(*) FROM economic_lineage_state "
                       "WHERE active_epoch IS NOT NULL") == "0"
            output_path = game / "server.out"
            process = None
            client = None

            def start() -> int:
                nonlocal process
                game_port, tls_port, websocket_port = journey.available_ports()
                env.update(DURIS_TLS_PORT=str(tls_port),
                           DURIS_WEBSOCKET_PORT=str(websocket_port))
                with output_path.open("w") as output:
                    process = subprocess.Popen(
                        [str(binary), "-d", str(game), str(game_port)], cwd=game,
                        env=env, stdout=output, stderr=subprocess.STDOUT)
                deadline = time.monotonic() + 180
                while time.monotonic() < deadline and process.poll() is None:
                    if "Entering game loop." in output_path.read_text(errors="replace"):
                        return game_port
                    time.sleep(0.1)
                raise AssertionError("locker server did not boot:\n" +
                                     output_path.read_text(errors="replace")[-4000:])

            def stop(*, clean: bool = True) -> None:
                nonlocal process
                if process and process.poll() is None:
                    process.send_signal(signal.SIGTERM)
                    try:
                        result = process.wait(timeout=30)
                    except subprocess.TimeoutExpired:
                        process.kill()
                        process.wait(timeout=10)
                        if clean:
                            raise AssertionError("locker server did not shut down")
                    else:
                        if clean:
                            assert result == 0
                process = None

            def verify_custody(owner_type: int, owner_id: int,
                               context_id: int) -> None:
                for uid, root, parent in ((ROOT_UID, ROOT_UID, "NULL"),
                                          (CHILD_UID, ROOT_UID, str(ROOT_UID))):
                    assert sql("SELECT CONCAT(root_item_uid,':',"
                               "COALESCE(parent_item_uid,'NULL'),':',owner_type,':',"
                               "owner_id,':',owner_context_id,':',state) "
                               f"FROM item_current_owner WHERE item_uid={uid}") == \
                           f"{root}:{parent}:{owner_type}:{owner_id}:{context_id}:1"

            def verify_revision(revision: int) -> None:
                assert sql("SELECT COUNT(*) FROM item_current_owner WHERE "
                           f"item_uid IN ({ROOT_UID},{CHILD_UID}) "
                           f"AND item_revision={revision}") == "2"

            def history(transfers: int) -> str:
                rows = sql("SELECT HEX(operation_id),event_index,item_uid,root_item_uid,"
                           "COALESCE(parent_item_uid,0),from_owner_type,from_owner_id,"
                           "from_owner_context_id,to_owner_type,to_owner_id,"
                           "to_owner_context_id,item_revision FROM item_ownership_ledger "
                           f"WHERE item_uid IN ({ROOT_UID},{CHILD_UID}) "
                           "ORDER BY item_revision,item_uid")
                fields = [row.split("\t") for row in rows.splitlines()]
                assert len(fields) == transfers * 2, rows
                for transfer in range(transfers):
                    root, child = fields[transfer * 2:transfer * 2 + 2]
                    assert len(root[0]) == 32 and int(root[0], 16) > 0, rows
                    assert root[0] == child[0] and root[1] != child[1], rows
                    assert [int(root[2]), int(child[2])] == [ROOT_UID, CHILD_UID], rows
                    assert [int(root[3]), int(child[3])] == [ROOT_UID, ROOT_UID], rows
                    assert [int(root[4]), int(child[4])] == [0, ROOT_UID], rows
                    assert root[5:11] == child[5:11], rows
                    assert [int(root[11]), int(child[11])] == [6 + transfer] * 2, rows
                    assert [int(root[5]), int(root[8])] == (
                        [1, 5] if transfer == 0 else [5, 1]), rows
                    assert sql("SELECT COUNT(*) FROM critical_operation_inbox WHERE "
                               f"operation_id=UNHEX('{root[0]}') AND status=1 "
                               "AND result_code=0 AND failure_stage=0 "
                               "AND committed_at IS NOT NULL") == "1", rows
                if transfers == 2:
                    assert fields[0][0] != fields[2][0], rows
                    assert fields[0][5:8] == fields[2][8:11], rows
                    assert fields[0][8:11] == fields[2][5:8], rows
                return rows

            def verify_native(table: str) -> None:
                for uid in (ROOT_UID, CHILD_UID):
                    assert sql(f"SELECT COUNT(*) FROM {table} WHERE obj_uid={uid}") == "1"
                other = "locker_items" if table == "player_items" else "player_items"
                assert sql(f"SELECT COUNT(*) FROM {other} WHERE "
                           f"obj_uid IN ({ROOT_UID},{CHILD_UID})") == "0"
                assert sql(f"SELECT CONCAT(child.container_id,':',root.id) FROM "
                           f"{table} child JOIN {table} root ON root.obj_uid={ROOT_UID} "
                           f"WHERE child.obj_uid={CHILD_UID}") == \
                           sql(f"SELECT CONCAT(id,':',id) FROM {table} "
                               f"WHERE obj_uid={ROOT_UID}")
                prefix = "locker_item_" if table == "locker_items" else "player_item_"
                assert sql(f"SELECT CONCAT(ed.keyword,':',ed.description) FROM "
                           f"{prefix}extra_descr ed JOIN {table} item "
                           f"ON item.id=ed.item_id WHERE item.obj_uid={ROOT_UID}") == \
                           "locker-mark:The maker marked this bag."
                assert sql(f"SELECT CONCAT(af.location,':',af.modifier) FROM "
                           f"{prefix}affects af JOIN {table} item "
                           f"ON item.id=af.item_id WHERE item.obj_uid={CHILD_UID}") == "1:7"

            def verify(owner_type: int, owner_id: int, context_id: int,
                       table: str) -> None:
                verify_custody(owner_type, owner_id, context_id)
                verify_native(table)

            try:
                port_number = start()
                client = journey.MudClient(port_number)
                journey.create_character(client, expected_room=None, class_name="w")
                client.send("save")
                client.expect(f"Save complete for {journey.CHARACTER}.", timeout=30)
                client.close()
                client = None
                stop()

                pid = int(sql("SELECT pid FROM player_data WHERE name='" +
                              journey.CHARACTER + "'"))
                assert pid > 0
                sql(f"DELETE FROM player_items WHERE pid={pid};"
                    "DELETE FROM item_current_owner WHERE owner_type=1 "
                    f"AND owner_id={pid} AND owner_context_id=0;"
                    f"UPDATE player_data SET last_room={BANK_ROOM},copper=100 "
                    f"WHERE pid={pid};"
                    "INSERT INTO item_owner_revision"
                    "(owner_type,owner_id,owner_context_id,revision) "
                    f"VALUES(1,{pid},0,1) ON DUPLICATE KEY UPDATE "
                    "revision=GREATEST(revision,1);"
                    "INSERT INTO player_items"
                    "(pid,vnum,weight,cost,wear_flags,obj_uid,name,short_descr,description)"
                    f" VALUES({pid},48,10,215,1,{ROOT_UID},'locker backpack',"
                    "'a locker backpack','A locker backpack lies here.');"
                    "SET @root_id=LAST_INSERT_ID();"
                    "INSERT INTO player_items"
                    "(pid,vnum,container_id,weight,cost,obj_uid,name,short_descr,description)"
                    f" VALUES({pid},5,@root_id,2,150,{CHILD_UID},'locker note',"
                    "'a locker note','A locker note lies here.');"
                    "SET @child_id=LAST_INSERT_ID();"
                    "INSERT INTO player_item_affects(item_id,location,modifier)"
                    " VALUES(@child_id,1,7);"
                    "INSERT INTO player_item_extra_descr(item_id,keyword,description)"
                    " VALUES(@root_id,'locker-mark','The maker marked this bag.');"
                    "INSERT INTO item_current_owner"
                    "(item_uid,root_item_uid,owner_type,owner_id,owner_context_id,"
                    "item_revision,vnum,state)"
                    f" VALUES({ROOT_UID},{ROOT_UID},1,{pid},0,5,48,1);"
                    "INSERT INTO item_current_owner"
                    "(item_uid,root_item_uid,parent_item_uid,owner_type,owner_id,"
                    "owner_context_id,item_revision,vnum,state)"
                    f" VALUES({CHILD_UID},{ROOT_UID},{ROOT_UID},1,{pid},0,5,5,1)")

                port_number = start()
                client = journey.reconnect_character(port_number, expected_room=BANK_NAME)
                client.send("look")
                client.expect(BANK_NAME, timeout=20)
                verify(1, pid, 0, "player_items")
                verify_revision(5)
                assert history(0) == ""
                client.pending.clear()
                client.send("enter locker")
                client.expect("escorts you to the locker", timeout=30)
                client.send("drop backpack")
                client.expect("You drop", timeout=30)
                owner = sql(f"SELECT CONCAT(owner_id,':',owner_context_id) "
                            f"FROM item_current_owner WHERE item_uid={ROOT_UID}")
                locker_id, chest_id = map(int, owner.split(":"))
                assert locker_id > 0 and chest_id > 0
                verify_custody(5, locker_id, chest_id)
                verify_revision(6)
                deposited_history = history(1)
                client.send("open door")
                client.expect("Ok.", timeout=15)
                client.send("north")
                client.expect(BANK_NAME, timeout=30)
                client.send("save")
                client.expect(f"Save complete for {journey.CHARACTER}.", timeout=30)
                client.close()
                client = None
                stop()
                verify(5, locker_id, chest_id, "locker_items")
                verify_revision(6)
                assert history(1) == deposited_history

                port_number = start()
                client = journey.reconnect_character(port_number, expected_room=BANK_NAME)
                client.send("enter locker")
                client.expect("escorts you to the locker", timeout=30)
                verify(5, locker_id, chest_id, "locker_items")
                verify_revision(6)
                assert history(1) == deposited_history
                client.send("get backpack")
                client.expect("You get", timeout=30)
                verify_custody(1, pid, 0)
                verify_revision(7)
                withdrawn_history = history(2)
                client.send("open door")
                client.expect("Ok.", timeout=15)
                client.send("north")
                client.expect(BANK_NAME, timeout=30)
                client.send("save")
                client.expect(f"Save complete for {journey.CHARACTER}.", timeout=30)
                client.close()
                client = None
                stop()
                verify(1, pid, 0, "player_items")
                verify_revision(7)
                assert history(2) == withdrawn_history
                port_number = start()
                client = journey.reconnect_character(port_number, expected_room=BANK_NAME)
                client.send("look in backpack")
                client.expect("a locker note", timeout=20)
                verify(1, pid, 0, "player_items")
                verify_revision(7)
                assert history(2) == withdrawn_history
                client.send("save")
                client.expect(f"Save complete for {journey.CHARACTER}.", timeout=30)
                client.close()
                client = None
                stop()
                verify(1, pid, 0, "player_items")
                verify_revision(7)
                assert history(2) == withdrawn_history
                assert sql("SELECT COUNT(*) FROM economic_lineage_state "
                           "WHERE active_epoch IS NOT NULL") == "0"
                print("SQL locker: original nested UIDs, exact revisions/receipts, "
                      "deposit/reload, withdrawal/reload and metadata passed",
                      flush=True)
            except Exception:
                print("server tail:", output_path.read_text(errors="replace")[-3500:],
                      flush=True)
                print("runtime logs:", journey.runtime_logs(game)[-3500:], flush=True)
                if client:
                    print("game tail:", client.transcript.decode(errors="replace")[-1800:],
                          flush=True)
                raise
            finally:
                if client:
                    client.close()
                stop(clean=False)
    finally:
        sql("DROP DATABASE " + database, selected=False)


if __name__ == "__main__":
    run(Path(sys.argv[1]).resolve())
