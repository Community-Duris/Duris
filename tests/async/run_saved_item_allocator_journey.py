#!/usr/bin/env python3
"""Exercise saved-item cleanup during a real full-world MariaDB recovery boot.

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
        boot_only: bool = False) -> None:
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
            output_path = game / "server.out"
            with output_path.open("w") as output:
                process = subprocess.Popen(
                    [str(binary), "-d", str(game), str(game_port)],
                    cwd=game, env=env, stdout=output, stderr=subprocess.STDOUT)
            client = None
            try:
                deadline = time.monotonic() + 120
                while time.monotonic() < deadline and process.poll() is None:
                    if "Entering game loop." in output_path.read_text(errors="replace"):
                        break
                    time.sleep(.1)
                logs = output_path.read_text(errors="replace") + journey.runtime_logs(game)
                if expect_abort:
                    assert process.wait(timeout=20) == -signal.SIGABRT, logs[-2000:]
                    assert "free(): invalid pointer" in logs, logs[-2000:]
                    print("baseline: nonempty full-world MariaDB recovery boot aborted at allocator cleanup",
                          flush=True)
                    return
                assert process.poll() is None and "Entering game loop." in logs, logs[-2000:]
                assert f"sql_restore_saved_items: loaded {count} items" in logs, logs[-2000:]
                if count and not boot_only:
                    client = journey.MudClient(game_port)
                    journey.create_character(client, expected_room=None)
                    client.pending.clear()
                    client.send("look")
                    room = client.expect("Pos: standing >", timeout=20)
                    assert "recovery backpack" in room, room
                    if count == 2:
                        assert "recovery note" in room, room
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
    abort = "--expect-abort" in sys.argv[2:]
    boot_only = "--boot-only" in sys.argv[2:]
    for entries in ([1] if abort else [0, 1, 2]):
        run(binary, entries, abort, boot_only=boot_only)
    if not abort:
        run(binary, 2, False, malformed=True, boot_only=boot_only)