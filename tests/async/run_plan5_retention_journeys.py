#!/usr/bin/env python3
"""Inactive native deletion preserves seeded non-personal economic evidence.

Requires the explicit disposable Linux gate and an existing server binary.
The existing menu journeys own their behavior. A test observer seeds structurally
native-compatible zero-effect history and reads it without changing that behavior.
This does not qualify active typed erasure, writer authority or complete capture.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import secrets
import socket
import struct
import subprocess
import sys
import tempfile
from unittest import mock

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))


def native_fixture() -> dict[str, Path]:
    work = ROOT / "bin/tests/plan5-retention-qualified-native"
    work.mkdir(mode=0o700, parents=True, exist_ok=True)
    source = work / "probe.cpp"
    source.write_text(r'''#include "economy/economic_accounting_intent.h"
#include <cassert>
#include <cstdlib>
#include <iostream>
critical_operation_id id(uint8_t value) { critical_operation_id result; result.bytes.fill(value); return result; }
critical_operation_id op_id(uint8_t value,uint32_t pid) {
    auto result=id(value);
    for (size_t i=0;i<4;++i) result.bytes[12+i]=uint8_t(pid>>(i*8));
    return result;
}
void output(const std::vector<uint8_t> &bytes) {
    for (size_t i=0;i<4;++i) std::cout.put(static_cast<char>(bytes.size()>>(i*8)));
    std::cout.write(reinterpret_cast<const char *>(bytes.data()),bytes.size());
}
int main(int argc,char **argv) {
    assert(argc==2);
    const uint64_t pid=std::strtoull(argv[1],nullptr,10); assert(pid>0 && pid<=UINT32_MAX);
    economic_accounting_plan plan;
    auto &m=plan.metadata;
    m.lineage=id(0x11); m.epoch=id(0x22); m.operation_id=op_id(0x80,pid);
    m.actor_kind=economic_actor_kind::domain; m.actor_id=pid; m.writer_id=1;
    m.reason=economic_reason::coin_transfer;
    m.source_event={economic_source_kind::lifecycle,id(0x71),id(0x72),pid,0};
    for (size_t step=0;step<2;++step) {
        economic_frozen_intent intent;
        intent.admission.metadata=m; intent.command_binding.fill(0x21); intent.domain_digest.fill(0x22);
        std::vector<uint8_t> frozen,encoded;
        assert(economic_intent_encode(intent,&frozen)==economic_accounting_error::ok);
        assert(economic_intent_digest(intent,&m.intent_digest)==economic_accounting_error::ok);
        m.domain_digest=intent.domain_digest;
        assert(economic_plan_encode(plan,&encoded)==economic_accounting_error::ok);
        economic_accounting_plan decoded;
        assert(economic_plan_decode(encoded,&decoded)==economic_accounting_error::ok);
        output(frozen); output(encoded);
        m.original_operation_id=m.operation_id; m.operation_id=op_id(0x81,pid); m.source_event->slot=1;
    }
}
''')
    sources = ["src/economy/economic_accounting_plan.c", "src/economy/economic_accounting_types.c",
               "src/economy/economic_accounting_intent.c", "src/persistence/critical_command.c",
               "src/item/item_transfer_command.c", "src/item/craft_pouch_mutation.c",
               "src/combat/chaos_pouch_ledger.c", "src/player/player_snapshot_codec.c"]
    result = {}
    for mode in ("sql", "flatfile"):
        binary = work / ("probe-" + mode)
        command = ["g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O1", "-g",
                   "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie", "-Isrc",
                   str(source), *(str(ROOT / name) for name in sources), "-lcrypto", "-o", str(binary)]
        if mode == "flatfile":
            command.insert(1, "-D__NO_MYSQL__")
        subprocess.run(command, cwd=ROOT, check=True)
        result[mode] = binary
        print("RETENTION_NATIVE " + json.dumps({"mode": mode,
              "binary_sha256": hashlib.sha256(binary.read_bytes()).hexdigest()}), flush=True)
    return result


def run(server: Path) -> None:
    if os.environ.get("DURIS_RUN_PLAN5_RETENTION_INTEGRATION") != "1" or sys.platform != "linux":
        raise RuntimeError("requires explicit disposable Linux retention invocation")
    if not server.is_file() or not server.is_relative_to(ROOT / "bin"):
        raise RuntimeError("select an existing workspace/bin SQL server")
    import pymysql
    import persistence_restore as restore
    from economic_sql_audit_snapshot import read_evidence
    from reconcile_economy_accounting import Reconciler, view
    import run_mysql_account_deletion_journey as account_journey
    import run_mysql_deletion_journey as character_journey
    import test_flatfile_combat_journey as journey

    binaries = native_fixture()
    print("RETENTION_SERVER " + hashlib.sha256(server.read_bytes()).hexdigest(), flush=True)
    real_popen = subprocess.Popen
    real_check_output = subprocess.check_output
    real_create = journey.create_character
    sanitizer_env = dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                         UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
    completed = []
    with tempfile.TemporaryDirectory(prefix="duris-plan5-retention-") as directory:
        base = Path(directory)
        for engine in ("mariadb", "mysql"):
            candidate = base / engine
            candidate.mkdir(mode=0o700)
            with socket.socket() as reservation:
                reservation.bind(("127.0.0.1", 0))
                port = reservation.getsockname()[1]

            def loopback_server(args, *positional, **kwargs):
                if "--skip-networking" in args:
                    assert "--datadir=" + str(candidate / "mysql") in args
                    assert candidate.is_relative_to(base)
                    args = [arg for arg in args if arg != "--skip-networking"]
                    args += ["--bind-address=127.0.0.1", "--port=" + str(port)]
                    if engine == "mysql":
                        args.append("--default-authentication-plugin=mysql_native_password")
                return real_popen(args, *positional, **kwargs)

            with mock.patch.object(subprocess, "Popen", side_effect=loopback_server):
                with restore.private_database(candidate, engine) as env:
                    admin = pymysql.connect(unix_socket=env["DB_SOCKET"], user="root", autocommit=True,
                                            cursorclass=pymysql.cursors.DictCursor)
                    password = secrets.token_hex(24)
                    try:
                        with admin.cursor() as cursor:
                            cursor.execute("CREATE USER 'fixture_owner'@'127.0.0.1' IDENTIFIED BY %s", (password,))
                            cursor.execute("GRANT ALL ON *.* TO 'fixture_owner'@'127.0.0.1' WITH GRANT OPTION")
                            cursor.execute("SELECT VERSION() AS version")
                            version = cursor.fetchone()["version"]
                        for name, module in (("account", account_journey), ("character", character_journey)):
                            state = {"seeded": False, "captures": 0, "cold_restarts": 0,
                                     "verified": False, "pids": [], "op_ids": []}
                            reader_password = secrets.token_hex(24)
                            reader_name = "retention_" + name

                            def retained_cut(label, fixture_append=False):
                                schema = state["schema"]
                                reader = pymysql.connect(unix_socket=env["DB_SOCKET"], user=reader_name,
                                    password=reader_password, database=schema, autocommit=True,
                                    cursorclass=pymysql.cursors.DictCursor)
                                connection = mock.Mock(wraps=reader)
                                cursor = mock.Mock(wraps=reader.cursor())
                                try:
                                    cursor.execute("SET TRANSACTION ISOLATION LEVEL REPEATABLE READ")
                                    cursor.execute("START TRANSACTION WITH CONSISTENT SNAPSHOT, READ ONLY")
                                    cut = read_evidence(cursor, b"\x11" * 16, b"\x22" * 16, True)
                                    rows = []
                                    for table in ("economic_lineage_state", "economic_epoch",
                                                  "economic_accounting_operation", "economic_accounting_source_claim"):
                                        cursor.execute("SELECT * FROM " + table + " ORDER BY 1,2")
                                        rows.append(cursor.fetchall())
                                    op_ids = (b"\x07" * 16, *state["op_ids"])
                                    cursor.execute("SELECT * FROM critical_operation_inbox WHERE operation_id IN (" +
                                                   ",".join(["%s"] * len(op_ids)) + ") ORDER BY operation_id", op_ids)
                                    rows.append(cursor.fetchall())
                                    cursor.execute("SELECT COUNT(*) AS active FROM economic_lineage_state WHERE active_epoch IS NOT NULL")
                                    assert cursor.fetchone()["active"] == 0
                                    cursor.execute("SELECT COUNT(*) AS staged FROM economic_sql_lifecycle_installation WHERE phase IN (1,2)")
                                    assert cursor.fetchone()["staged"] == 0
                                finally:
                                    connection.rollback()
                                    cursor.close()
                                    reader.close()
                                connection.rollback.assert_called_once_with()
                                cursor.close.assert_called_once_with()
                                assert all(call.args[0].upper().startswith(("SELECT", "SET TRANSACTION", "START TRANSACTION"))
                                           for call in cursor.execute.call_args_list)
                                snapshot = {"schema_version": 1, "lineage": "11" * 16, "epoch": "22" * 16,
                                            "complete": False, "quiescent": False, "backend": "retention_fixture",
                                            "native": {"holdings": [], "items": []}, **cut}
                                original = json.dumps(snapshot, sort_keys=True)
                                report = Reconciler().audit(snapshot)
                                assert report["exception_counts"] == {"evidence_loss": 1, "unfenced_snapshot": 1}, report
                                for limit in (0, 1, 100):
                                    output = view(snapshot, report, "operation", limit, operation_id=state["op_ids"][0].hex())
                                    assert output["coverage"]["exception_count"] == 2 and not output["coverage"]["complete"]
                                    assert len(output["rows"]) <= limit
                                assert json.dumps(snapshot, sort_keys=True) == original
                                if "retained" in state:
                                    previous_cut, previous_rows = state["retained"]
                                    if fixture_append:
                                        for collection, previous in previous_cut.items():
                                            if isinstance(previous, list):
                                                assert all(row in cut[collection] for row in previous), (label, collection)
                                        assert all(all(row in current for row in previous)
                                                   for current, previous in zip(rows, previous_rows)), label
                                    else:
                                        assert (cut, rows) == state["retained"], label
                                state["retained"] = (cut, rows)
                                assert len(cut["operations"]) == len(state["pids"]) * 2
                                assert len(cut["source_claims"]) == len(state["pids"])
                                state["captures"] += 1
                                print("RETENTION_CAPTURE " + json.dumps({"engine": engine, "journey": name,
                                      "label": label, "roots": len(cut["operations"]), "claims": len(cut["source_claims"]),
                                      "prior_rows_unchanged": True, "fixture_append": fixture_append,
                                      "select_only": True, "rollback": 1, "cursor_close": 1,
                                      "incomplete_refusals": report["exception_counts"]}), flush=True)

                            def create_with_history(client, *args, **kwargs):
                                real_create(client, *args, **kwargs)
                                if kwargs.get("character", journey.CHARACTER) != journey.CHARACTER:
                                    return
                                with admin.cursor() as cursor:
                                    cursor.execute("SELECT SCHEMA_NAME AS name FROM information_schema.SCHEMATA WHERE SCHEMA_NAME LIKE 'deletion_test_%'")
                                    schemas = cursor.fetchall()
                                    assert len(schemas) == 1 and re.fullmatch(r"deletion_test_[0-9a-f]{12}", schemas[0]["name"])
                                    schema = schemas[0]["name"]
                                    cursor.execute("SELECT pid FROM " + schema + ".player_data WHERE name=%s", (journey.CHARACTER,))
                                    pid_rows = cursor.fetchall()
                                    assert len(pid_rows) == 1
                                    pid = pid_rows[0]["pid"]
                                    cursor.execute("SELECT sequence_number,migration_id FROM " + schema + ".mud_schema_history ORDER BY sequence_number DESC LIMIT 1")
                                    assert cursor.fetchone() == {"sequence_number": 56, "migration_id": "0056_spell_ward_durability"}
                                    if not state["seeded"]:
                                        cursor.execute("CREATE USER '" + reader_name + "'@'localhost' IDENTIFIED BY %s", (reader_password,))
                                        cursor.execute("GRANT SELECT ON " + schema + ".* TO '" + reader_name + "'@'localhost'")
                                assert pid not in state["pids"], "native name reuse recycled an economic actor PID"
                                if state["seeded"]:
                                    assert state["schema"] == schema
                                    retained_cut("before-next-identity-seed")
                                outputs = [real_check_output([str(binary), str(pid)], env=sanitizer_env) for binary in binaries.values()]
                                assert outputs[0] == outputs[1]
                                artifact = ROOT / "bin/tests/plan5-retention-qualified-native" / (engine + "-" + name + "-" + str(pid) + ".bin")
                                artifact.write_bytes(outputs[0])
                                print("RETENTION_NATIVE_CASE " + json.dumps({"engine": engine, "journey": name, "pid": pid,
                                      "encoded_sha256": hashlib.sha256(outputs[0]).hexdigest(),
                                      "encoded_bytes": len(outputs[0]), "native_modes_agree": True}), flush=True)
                                blocks = []
                                offset = 0
                                while offset < len(outputs[0]):
                                    size, = struct.unpack_from("<I", outputs[0], offset)
                                    offset += 4
                                    blocks.append(outputs[0][offset:offset + size])
                                    offset += size
                                assert len(blocks) == 4 and offset == len(outputs[0])
                                owner = pymysql.connect(unix_socket=env["DB_SOCKET"], user="root", database=schema,
                                    autocommit=True, cursorclass=pymysql.cursors.DictCursor)
                                try:
                                    def insert(table, fields):
                                        with owner.cursor() as cursor:
                                            cursor.execute("INSERT INTO " + table + " (" + ",".join(fields) + ") VALUES (" +
                                                           ",".join(["%s"] * len(fields)) + ")", tuple(fields.values()))
                                    creator = b"\x07" * 16
                                    if not state["seeded"]:
                                        insert("critical_operation_inbox", dict(operation_id=creator, command_hash=b"\x01" * 32,
                                               keys_hash=b"\x02" * 32, command_type=1, schema_version=1, payload_version=1,
                                               status=1, result_code=0, result_payload=b""))
                                        insert("economic_epoch", dict(lineage=b"\x11" * 16, epoch=b"\x22" * 16, ordinal=1,
                                               transition_kind=1, transition_digest=b"\x03" * 32, creating_operation_id=creator))
                                        insert("economic_lineage_state", dict(lineage=b"\x11" * 16, active_epoch=None))
                                    for index, (frozen, plan) in enumerate(zip(blocks[:4:2], blocks[1:4:2])):
                                        assert frozen[:4] == b"EAI1" and plan[:4] == b"EAP1"
                                        assert struct.unpack_from("<Q", plan, 76)[0] == pid
                                        assert plan[152:184] == hashlib.sha256(b"DURIS-ECONOMIC-INTENT-V1\0" + frozen).digest()
                                        op = plan[40:56]
                                        insert("critical_operation_inbox", dict(operation_id=op, command_hash=hashlib.sha256(frozen).digest(),
                                               keys_hash=b"\x04" * 32, command_type=3, schema_version=2, payload_version=1,
                                               status=1, result_code=9 if index else 0, durable_revision=index + 1, result_payload=b""))
                                        with owner.cursor() as cursor:
                                            cursor.execute("UPDATE critical_operation_inbox SET committed_at=CURRENT_TIMESTAMP(6) WHERE operation_id=%s", (op,))
                                        insert("economic_accounting_operation", dict(operation_id=op, lineage=plan[8:24], epoch=plan[24:40],
                                               original_operation_id=plan[56:72] if index else None, accounting_version=1,
                                               writer_id=1, policy_version=1, compiler_version=1, actor_kind=1, actor_id=pid,
                                               reason=3, source_event=plan[104:152], intent_digest=plan[152:184], domain_digest=plan[184:216],
                                               plan_digest=None if index else hashlib.sha256(plan).digest(), canonical_intent=frozen,
                                               canonical_plan=None if index else plan, outcome=2 if index else 1, result_code=9 if index else 0,
                                               account_count=0, posting_count=0, child_count=0, item_event_count=0,
                                               before_witness_count=0, after_witness_count=0))
                                        if not index:
                                            insert("economic_accounting_source_claim", dict(lineage=plan[8:24], source_event=plan[104:152],
                                                   operation_id=op, outcome=1))
                                        state["op_ids"].append(op)
                                finally:
                                    owner.close()
                                state["pids"].append(pid)
                                append = state["seeded"]
                                state.update(seeded=True, schema=schema)
                                if not append:
                                    permission = pymysql.connect(unix_socket=env["DB_SOCKET"], user=reader_name,
                                        password=reader_password, database=schema, autocommit=True)
                                    try:
                                        with permission.cursor() as cursor:
                                            try:
                                                cursor.execute("UPDATE economic_accounting_operation SET actor_id=actor_id WHERE 1=0")
                                            except pymysql.MySQLError as error:
                                                assert error.args[0] == 1142
                                            else:
                                                raise AssertionError("retention observer is not SELECT-only")
                                    finally:
                                        permission.close()
                                retained_cut("after-identity-seed" if append else "before-deletion", fixture_append=append)

                            def observe_popen(args, *positional, **kwargs):
                                if args and str(args[0]) == str(server) and state["seeded"]:
                                    retained_cut("before-cold-restart")
                                    state["cold_restarts"] += 1
                                return loopback_server(args, *positional, **kwargs)

                            def observe_output(args, *positional, **kwargs):
                                text = kwargs.get("input")
                                if state["seeded"] and isinstance(text, str) and text == "DROP DATABASE " + state["schema"]:
                                    retained_cut("after-deletion-and-restart")
                                    with admin.cursor() as cursor:
                                        cursor.execute("SELECT COUNT(*) AS players FROM " + state["schema"] + ".player_data WHERE pid IN (" +
                                                       ",".join(["%s"] * len(state["pids"])) + ")", tuple(state["pids"]))
                                        assert cursor.fetchone()["players"] == 0
                                        cursor.execute("SELECT COUNT(*) AS accounts FROM " + state["schema"] + ".accounts WHERE account_name=%s", (journey.ACCOUNT,))
                                        assert cursor.fetchone()["accounts"] == (1 if name == "character" else 0)
                                    assert state["cold_restarts"] >= 1
                                    assert len(state["pids"]) == (2 if name == "character" else 1)
                                    state["verified"] = True
                                return real_check_output(args, *positional, **kwargs)

                            test_env = dict(env, TEST_DB_HOST="127.0.0.1", TEST_DB_PORT=str(port),
                                            TEST_DB_USER="fixture_owner", TEST_DB_PASSWORD=password)
                            with mock.patch.dict(os.environ, test_env, clear=True), \
                                    mock.patch.object(journey, "create_character", side_effect=create_with_history), \
                                    mock.patch.object(subprocess, "Popen", side_effect=observe_popen), \
                                    mock.patch.object(subprocess, "check_output", side_effect=observe_output):
                                module.run(server)
                            assert state["seeded"] and state["verified"]
                            result = {"engine": engine, "version": version, "journey": name,
                                      "captures": state["captures"], "cold_restarts": state["cold_restarts"],
                                      "pids": state["pids"], "retained_roots": len(state["op_ids"]),
                                      "retained_claims": len(state["pids"]), "inactive": True,
                                      "native_erasure_boundary": True, "seeded_evidence": True,
                                      "typed_active_erasure_qualified": False}
                            completed.append(result)
                            print("RETENTION_JOURNEY " + json.dumps(result, sort_keys=True), flush=True)
                    finally:
                        admin.close()
    assert len(completed) == 4
    print("PLAN5_RETENTION_QUALIFIED " + json.dumps({"journeys": completed,
          "engines": 2, "native_modes": 2, "accounting_activated": False,
          "real_native_menu_paths": True, "seeded_retained_evidence": True,
          "full_R8_qualified": False}, sort_keys=True), flush=True)


def run_flatfile(server: Path, inspector: Path) -> None:
    if os.environ.get("DURIS_RUN_PLAN5_RETENTION_INTEGRATION") != "1" or sys.platform != "linux":
        raise RuntimeError("requires explicit disposable Linux retention invocation")
    for binary in (server, inspector):
        if not binary.is_file() or not binary.is_relative_to(ROOT / "bin"):
            raise RuntimeError("select existing workspace/bin flatfile binaries")
    import builtins
    import fcntl
    import stat
    import time
    import run_flatfile_deletion_journey as deletion
    import test_flatfile_combat_journey as journey
    from test_flatfile_restore_economic_authority import build_fixture

    work = ROOT / "bin/tests/plan5-flat-retention-native"
    work.mkdir(mode=0o700, parents=True, exist_ok=True)
    fixture = build_fixture(work / "fixture")
    source = work / "audit.cpp"
    source.write_text('''#include "qualify_flatfile_economic_records.h"
#include <iostream>
int main(int argc, char **argv) {
    if (argc != 2) return 2;
    try { restore_economic_records::checker(argv[1]).run(); return 0; }
    catch (...) { std::cerr << "native_restore_qualification_failed\\n"; return 1; }
}
''')
    audit = work / "audit"
    subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                    "-O1", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
                    "-fno-pie", "-no-pie", "-I" + str(ROOT / "scripts"), str(source),
                    "-lcrypto", "-o", str(audit)], check=True)
    environment = dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                       UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
    real_popen, real_reconnect, real_print = subprocess.Popen, journey.reconnect_character, builtins.print
    summaries = []
    for name in ("character", "durable", "uncertain"):
        state = {"seeded": False, "captures": 0, "cold_restarts": 0, "verified": False}

        def capture(label):
            root = state["root"]
            descriptor = os.open(root / "domains/.critical-authority.lock", os.O_RDONLY | os.O_NOFOLLOW | os.O_CLOEXEC)
            try:
                lock = os.fstat(descriptor)
                assert stat.S_ISREG(lock.st_mode) and lock.st_nlink == 1 and lock.st_uid == os.geteuid()
                assert not lock.st_mode & 0o077
                deadline = time.monotonic() + 30
                while True:
                    try:
                        fcntl.flock(descriptor, fcntl.LOCK_SH | fcntl.LOCK_NB)
                        break
                    except BlockingIOError:
                        if time.monotonic() >= deadline:
                            raise RuntimeError("native retention authority lock deadline")
                        time.sleep(0.01)
                directory = root / "economic-evidence"
                def inventory():
                    rows = {}
                    for path in sorted(directory.iterdir()):
                        info = path.lstat()
                        assert stat.S_ISREG(info.st_mode) and info.st_nlink == 1 and info.st_uid == os.geteuid()
                        assert not info.st_mode & 0o077
                        rows[path.name] = (info.st_mode, info.st_nlink, path.read_bytes())
                    return rows
                before = inventory()
                result = subprocess.run([str(audit), str(root)], env=environment, text=True,
                                        capture_output=True, timeout=30)
                assert result.returncode == 0 and not result.stdout and not result.stderr, result.stderr
                assert inventory() == before, "independent reader changed retained evidence"
                control = before["authority.eal"][2]
                assert control[:8] == b"DURECA1\0" and len(control) == 16600
                assert control[48 + 64:48 + 80] == b"\0" * 16, "accounting became active"
                assert struct.unpack_from("<I", control, 48 + 96)[0] == 2
                assert sum(key.startswith("source-claim-") for key in before) == 2
                if state["seeded"]:
                    assert before == state["retained"], label + ": retained evidence changed"
                state["retained"] = before
                state["captures"] += 1
                pending = (root / "domains/.critical-authority-transaction").exists()
                manifest = {key: {"mode": mode, "links": links, "bytes": len(value),
                                 "sha256": hashlib.sha256(value).hexdigest()}
                            for key, (mode, links, value) in before.items()}
                (work / (name + "-cut-" + str(state["captures"]) + ".json")).write_text(json.dumps(manifest, sort_keys=True))
                real_print("FLAT_RETENTION_CAPTURE " + json.dumps({"journey": name, "label": label,
                           "files": len(before), "claims": 2, "epochs": 2, "inactive": True,
                           "shared_lock": True, "reader_unchanged": True, "prior_bytes_unchanged": True,
                           "native_pending_journal": pending}), flush=True)
            finally:
                os.close(descriptor)

        def observe_popen(args, *positional, **kwargs):
            if args and str(args[0]) == str(server):
                root = Path(kwargs["env"]["FLATFILE_STATE_DIR"]).resolve()
                assert root.name == "state" and root.parent.name.startswith("flatfile-deletion-")
                assert root.is_relative_to(Path(tempfile.gettempdir()).resolve())
                if "root" in state:
                    assert root == state["root"]
                state["root"] = root
                if state["seeded"]:
                    capture("before-cold-restart")
                    state["cold_restarts"] += 1
            return real_popen(args, *positional, **kwargs)

        def observe_reconnect(*args, **kwargs):
            client = real_reconnect(*args, **kwargs)
            # The existing initial absent-store fault uses a stray marker. Seed
            # only after that refusal has passed and its marker was removed.
            if not state["seeded"]:
                assert (state["root"] / "players/1.snapshot").is_file()
                subprocess.run([str(fixture), str(state["root"]), "retention"], env=environment, check=True)
                capture("before-deletion")
                state["seeded"] = True
            return client

        def observe_print(*args, **kwargs):
            if args and isinstance(args[0], str) and args[0].startswith("[PASS]"):
                assert state["seeded"] and not state["verified"]
                capture("after-deletion-and-restart")
                assert not (state["root"] / "players/1.snapshot").exists()
                state["verified"] = True
            real_print(*args, **kwargs)

        with mock.patch.object(subprocess, "Popen", side_effect=observe_popen), \
                mock.patch.object(journey, "reconnect_character", side_effect=observe_reconnect), \
                mock.patch.object(builtins, "print", side_effect=observe_print):
            deletion.run(server, inspector, None if name == "character" else name)
        assert state["seeded"] and state["verified"]
        assert state["cold_restarts"] == (1 if name == "character" else 3)
        row = {"journey": name, "captures": state["captures"], "cold_restarts": state["cold_restarts"],
               "retained_roots": 4, "claims": 2, "epochs": 2, "inactive": True,
               "actual_native_menu": True, "seeded_history": True, "full_R8_qualified": False}
        summaries.append(row)
        real_print("FLAT_RETENTION_JOURNEY " + json.dumps(row, sort_keys=True), flush=True)
    real_print("PLAN5_FLAT_RETENTION_QUALIFIED " + json.dumps({"journeys": summaries,
               "server_sha256": hashlib.sha256(server.read_bytes()).hexdigest(),
               "inspector_sha256": hashlib.sha256(inspector.read_bytes()).hexdigest(),
               "fixture_sha256": hashlib.sha256(fixture.read_bytes()).hexdigest(),
               "audit_sha256": hashlib.sha256(audit.read_bytes()).hexdigest(),
               "accounting_activated": False, "full_R8_qualified": False}, sort_keys=True), flush=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--server", type=Path, required=True)
    parser.add_argument("--backend", choices=("sql", "flatfile"), default="sql")
    parser.add_argument("--inspector", type=Path)
    arguments = parser.parse_args()
    if arguments.backend == "flatfile":
        if arguments.inspector is None:
            parser.error("flatfile requires --inspector")
        run_flatfile(arguments.server.resolve(), arguments.inspector.resolve())
    else:
        run(arguments.server.resolve())
