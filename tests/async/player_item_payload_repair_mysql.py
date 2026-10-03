#!/usr/bin/env python3
"""Run only through run_player_item_payload_repair_mysql.sh in synthetic containers."""
from __future__ import annotations

import copy
import json
import os
from pathlib import Path
import subprocess
import struct
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
import player_death_restitution as api
import player_item_payload_repair as repair


def main() -> None:
    # Explicit fixture fence: no developer or production database can enter.
    assert os.environ["ENVIRONMENT"] == "test"
    assert os.environ["DB_NAME"] == "duris_payload_repair_test"
    db = api.Mysql()
    binary = ROOT / "bin/tests/repair-runtime"
    db.run((ROOT / "migrations/bootstrap_multithread_safe.sql").read_text())
    # Resolve immutable migrations by the current branch's manifest. Accounting
    # uses a different sequence and retains additional load/save authorities.
    suffixes = {"player_death_disposition", "pet_restore_state", "pet_custody",
                "player_death_restitution", "player_item_runtime_state",
                "economy_accounting", "player_death_conflict_evidence",
                "item_equipment_slot", "item_extra_description_fulltext_unique"}
    manifest = json.loads((ROOT / "migrations/migration_manifest.json").read_text())
    for migration in manifest["migrations"]:
        if migration["id"].split("_", 1)[1] in suffixes:
            db.run((ROOT / "migrations" / migration["apply"]).read_text())
    # Existing schema-only boot data is unrelated to item recovery.
    db.run("INSERT INTO accounts(account_name) VALUES('acct43')")
    db.run("INSERT INTO player_data(pid,name) VALUES(44,'synthetic-other-player')")
    raw = subprocess.check_output([str(binary), "--repair-seed"]).decode().strip()
    evidence = {"format": repair.EVIDENCE_FORMAT, "snapshots_hex": [raw]}
    with tempfile.TemporaryDirectory(prefix="duris-synthetic-payload-repair-") as temporary:
        folder = Path(temporary)
        for variable in ("PLAYER_SAVE_JOURNAL_DIR", "CRITICAL_COMMAND_JOURNAL_DIR"):
            directory = folder / variable
            directory.mkdir(mode=0o700)
            os.environ[variable] = str(directory)
        evidence_path = folder / "evidence.json"
        api.atomic_write_json(evidence_path, evidence)
        proof = folder / "proof"
        proof.write_text("format=duris-death-restitution-quiescence-v3\ndatabase=duris_payload_repair_test\n"
                         "boundary=mysql-advisory-exclusion\nguard=duris.player.death.restitution\n"
                         "expires_at=2099-01-01T00:00:00Z\n")
        proof.chmod(0o600)
        target = api.identify_target(db, db.policy)
        backup_path = folder / "backup.json"
        backup = api.create_backup(db, db.policy, backup_path, lambda: api.check_quiescence(db, proof))

        def wait_for_quiescence():
            # MariaDB can briefly retain a just-disconnected transaction in the
            # visibility cache. Wait for the real maintenance guard to clear.
            for _ in range(100):
                if db.scalar("SELECT COUNT(*) FROM information_schema.innodb_trx") == "0" and \
                        db.scalar("SELECT COUNT(*) FROM information_schema.processlist WHERE ID<>CONNECTION_ID()") == "0":
                    return
                time.sleep(0.1)
            raise AssertionError("synthetic transactions did not drain")

        def cli(arguments):
            wait_for_quiescence()
            return api.main(arguments)

        def plan_for(uid=52601, source=evidence):
            return repair.prepare(api, db, uid, source, target, backup, backup["maintenance_boundary"])

        def refuse(label, uid=52601, source=evidence):
            plan = plan_for(uid, source)
            assert not plan["applyable"] and plan["classification"] == label, (plan["classification"], plan["note"])
            assert db.scalar("SELECT COUNT(*) FROM player_death_restitution_item WHERE classification='payload_repair'") == "0"
            print("refusal:", label)

        def assert_no_write(expected_projection="0"):
            assert db.scalar("SELECT COUNT(*) FROM player_items WHERE obj_uid=52601") == expected_projection
            assert db.scalar("SELECT COUNT(*) FROM player_death_restitution_receipt") == "0"

        refuse("existing_payload_conflict")
        db.run("DELETE FROM player_items WHERE obj_uid=52601")
        refuse("missing_evidence", source=None)
        refuse("insufficient_evidence", source={"format": repair.EVIDENCE_FORMAT, "snapshots_hex": []})
        absent = codec_snapshot_without_uid()
        refuse("missing_uid_evidence", source={"format": repair.EVIDENCE_FORMAT, "snapshots_hex": [absent]})
        refuse("corrupt_or_unsupported_encoding", source={"format": repair.EVIDENCE_FORMAT, "snapshots_hex": ["00000000"]})
        conflict = bytearray.fromhex(raw)
        # Change the generated key in the original complete frame, then validate
        # the changed frame through the production codec before testing conflict.
        original_key = (52601 * 77).to_bytes(8, "little", signed=True)
        position = conflict.index(original_key)
        conflict[position:position + 8] = (987654).to_bytes(8, "little", signed=True)
        conflicting_evidence = {"format": repair.EVIDENCE_FORMAT, "snapshots_hex": [raw, conflict.hex()]}
        refuse("conflicting_evidence", source=conflicting_evidence)
        artifact_frame = bytearray.fromhex(raw)
        flags_at = artifact_frame.index(struct.pack("<IIIII", 1, 22, 23, 24, 25)) + 4
        artifact_frame[flags_at:flags_at + 4] = api.ITEM_ARTIFACT.to_bytes(4, "little")
        refuse("artifact_reconciliation_required", source={"format": repair.EVIDENCE_FORMAT, "snapshots_hex": [artifact_frame.hex()]})
        # Refuse changed/retired/unsupported custody and pending authority.
        for sql, undo, label in (
            ("UPDATE item_current_owner SET state=2 WHERE item_uid=52601", "UPDATE item_current_owner SET state=1 WHERE item_uid=52601", "retired_or_inactive"),
            ("UPDATE item_current_owner SET owner_type=5 WHERE item_uid=52601", "UPDATE item_current_owner SET owner_type=1 WHERE item_uid=52601", "unsupported_owner"),
            ("UPDATE item_current_owner SET vnum=99 WHERE item_uid=52601", "UPDATE item_current_owner SET vnum=52601 WHERE item_uid=52601", "identity_conflict"),
            ("UPDATE item_current_owner SET parent_item_uid=52603 WHERE item_uid=52602", "UPDATE item_current_owner SET parent_item_uid=NULL WHERE item_uid=52602", "invalid_topology"),
            ("UPDATE item_current_owner SET parent_item_uid=52603,root_item_uid=52602 WHERE item_uid=52601", "UPDATE item_current_owner SET parent_item_uid=NULL,root_item_uid=52601 WHERE item_uid=52601", "invalid_topology"),
            ("UPDATE player_items SET item_type=5 WHERE obj_uid=52602", "UPDATE player_items SET item_type=15 WHERE obj_uid=52602", "invalid_topology"),
            ("INSERT INTO player_items(pid,vnum,obj_uid) VALUES(44,52601,52601)", "DELETE FROM player_items WHERE obj_uid=52601", "competing_instance"),
            ("INSERT INTO saved_items(item_key,vnum,obj_uid) VALUES('synthetic-competitor',52601,52601)", "DELETE FROM saved_items WHERE obj_uid=52601", "competing_instance"),
            ("UPDATE item_current_owner SET coin_payload=UNHEX('00') WHERE item_uid=52601", "UPDATE item_current_owner SET coin_payload=NULL WHERE item_uid=52601", "currency_refused"),
        ):
            db.run(sql)
            refuse(label)
            db.run(undo)
        # A 33-object chain exceeds the production codec's root-inclusive depth.
        parent_uid = 52602
        parent_id = int(db.scalar("SELECT id FROM player_items WHERE obj_uid=52602"))
        for extra_uid in range(52701, 52732):
            db.run(f"INSERT INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,owner_type,owner_id,owner_context_id,vnum,item_revision,state) VALUES({extra_uid},52602,{parent_uid},1,43,0,{extra_uid},8,1)")
            db.run(f"INSERT INTO player_items(pid,vnum,obj_uid,item_type,container_id) VALUES(43,{extra_uid},{extra_uid},15,{parent_id})")
            parent_uid = extra_uid
            parent_id = int(db.scalar(f"SELECT id FROM player_items WHERE obj_uid={extra_uid}"))
        db.run(f"UPDATE item_current_owner SET parent_item_uid={parent_uid},root_item_uid=52602 WHERE item_uid=52601")
        refuse("invalid_topology")
        db.run("UPDATE item_current_owner SET parent_item_uid=NULL,root_item_uid=52601 WHERE item_uid=52601")
        db.run("DELETE FROM player_items WHERE obj_uid BETWEEN 52701 AND 52731 ORDER BY id DESC")
        db.run("DELETE FROM item_current_owner WHERE item_uid BETWEEN 52701 AND 52731 ORDER BY item_uid DESC")
        journal = Path(os.environ["CRITICAL_COMMAND_JOURNAL_DIR"]) / "critical-command.journal"
        journal.write_bytes(b"CCJ1pending")
        journal.chmod(0o600)
        refuse("pending_authority")
        journal.unlink()
        db.run("INSERT INTO critical_operation_inbox(operation_id,command_hash,keys_hash,command_type,schema_version,payload_version,status,result_payload) "
               "VALUES(UNHEX(REPEAT('ab',16)),UNHEX(REPEAT('ab',32)),UNHEX(REPEAT('ab',32)),1,1,1,0,'')")
        refuse("pending_authority")
        db.run("DELETE FROM critical_operation_inbox")
        db.run("INSERT INTO critical_operation_inbox(operation_id,command_hash,keys_hash,command_type,schema_version,payload_version,status,result_payload) "
               "VALUES(UNHEX(REPEAT('ac',16)),UNHEX(REPEAT('ac',32)),UNHEX(REPEAT('ac',32)),1,1,1,1,'')")
        db.run("INSERT INTO critical_outbox(operation_id,event_index,destination,event_type,payload_version,payload,status) VALUES(UNHEX(REPEAT('ac',16)),0,1,1,1,'',2)")
        refuse("pending_authority")
        db.run("DELETE FROM critical_outbox")
        db.run("DELETE FROM critical_operation_inbox")
        plan = plan_for()
        assert plan["applyable"], (plan["classification"], plan["note"])
        # Direct transaction tests prove the fences are inside SQL, independent
        # of the Python preflight. Mutate after preparation and require rollback.
        for sql, undo in (
            ("UPDATE item_current_owner SET item_revision=9 WHERE item_uid=52601", "UPDATE item_current_owner SET item_revision=8 WHERE item_uid=52601"),
            ("UPDATE item_owner_revision SET revision=21 WHERE owner_type=1 AND owner_id=43", "UPDATE item_owner_revision SET revision=20 WHERE owner_type=1 AND owner_id=43"),
            ("UPDATE player_data SET save_revision=99 WHERE pid=43", "UPDATE player_data SET save_revision=2 WHERE pid=43"),
            ("UPDATE item_current_owner SET owner_id=42 WHERE item_uid=52601", "UPDATE item_current_owner SET owner_id=43 WHERE item_uid=52601"),
            ("UPDATE item_current_owner SET state=2 WHERE item_uid=52601", "UPDATE item_current_owner SET state=1 WHERE item_uid=52601"),
            ("UPDATE player_items SET item_type=5 WHERE obj_uid=52602", "UPDATE player_items SET item_type=15 WHERE obj_uid=52602"),
            ("UPDATE player_items SET container_id=NULL WHERE obj_uid=52603", "UPDATE player_items SET container_id=(SELECT id FROM (SELECT id FROM player_items WHERE obj_uid=52602) p) WHERE obj_uid=52603"),
            ("INSERT INTO player_items(pid,vnum,obj_uid) VALUES(44,52601,52601)", "DELETE FROM player_items WHERE obj_uid=52601"),
        ):
            db.run(sql)
            expected_projection = db.scalar("SELECT COUNT(*) FROM player_items WHERE obj_uid=52601")
            wait_for_quiescence()
            result = db.run(repair.build_sql(api, db, plan, "synthetic-operator", "stale plan test"))
            assert ["DURIS_REPAIR|0|0"] in result, result[-3:]
            assert_no_write(expected_projection)
            db.run(undo)
        # Native state can exceed a scalar SQL column even when the production
        # codec accepts it. A permissive server must never commit clipped data.
        overflow_timer = bytearray.fromhex(raw)
        timer_at = overflow_timer.index(struct.pack("<qqqqqq", -1, 123, 456, 789, 1011, 1213))
        overflow_timer[timer_at:timer_at + 8] = struct.pack("<q", 2**31)
        overflow_name = bytearray.fromhex(raw)
        old_name = b"synthetic generated item"
        name_at = overflow_name.index(old_name) - 4
        assert overflow_name[name_at:name_at + 4] == struct.pack("<I", len(old_name))
        long_name = b"x" * 512 + b" "  # trailing-space clipping warns even in strict mode
        overflow_name[name_at:name_at + 4 + len(old_name)] = struct.pack("<I", len(long_name)) + long_name
        for lossy_frame in (overflow_timer, overflow_name):
            lossy_plan = plan_for(source={"format": repair.EVIDENCE_FORMAT, "snapshots_hex": [lossy_frame.hex()]})
            assert lossy_plan["applyable"], (lossy_plan["classification"], lossy_plan["note"])
            wait_for_quiescence()
            try:
                result = db.run("SET SESSION sql_mode='';\n" +
                               repair.build_sql(api, db, lossy_plan, "synthetic-operator", "lossy SQL conversion test"))
            except api.ToolError:
                pass  # Strict-mode errors also disconnect and roll back.
            else:
                assert ["DURIS_REPAIR|0|0"] in result, result[-3:]
            assert_no_write()
        # SQL failure after projection insertion rolls the complete transaction back.
        db.run("CREATE TRIGGER fail_repair BEFORE INSERT ON player_item_runtime_state FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='synthetic interruption'")
        try:
            wait_for_quiescence()
            db.run(repair.build_sql(api, db, plan, "synthetic-operator", "rollback test"))
            raise AssertionError("interrupted transaction committed")
        except api.ToolError:
            assert_no_write()
        db.run("DROP TRIGGER fail_repair")
        # Kill the actual client after the projection statement, before COMMIT.
        wait_for_quiescence()
        sql = repair.build_sql(api, db, plan, "synthetic-operator", "client interruption test")
        cutoff = sql.index("INSERT INTO player_item_runtime_state")
        progress = folder / "interrupted-client-output"
        with progress.open("wb") as output:
            client = subprocess.Popen([*db.base, "--unbuffered", db.database], stdin=subprocess.PIPE,
                                      stdout=output, stderr=subprocess.DEVNULL, env=db.env)
            try:
                client.stdin.write((sql[:cutoff] +
                    "\nSELECT CONCAT('REPAIR_INSERTED|',@repair_ok,'|',@repair_projection,'|',@repair_receipt);\n").encode())
                client.stdin.flush()
                # Keep stdin open so the client waits with an uncommitted
                # transaction. Its flushed marker proves both inserts happened.
                deadline = time.monotonic() + 10
                while "REPAIR_INSERTED|1|1|1" not in progress.read_text():
                    if client.poll() is not None or time.monotonic() >= deadline:
                        raise AssertionError("client did not reach the uncommitted repair inserts")
                    time.sleep(0.05)
            finally:
                if client.poll() is None:
                    client.kill()
                client.wait(timeout=5)
                client.stdin.close()
        # Killing the client disconnects its SQL socket; wait for rollback to finish.
        for _ in range(50):
            if db.scalar("SELECT COUNT(*) FROM information_schema.processlist WHERE ID<>CONNECTION_ID()") == "0": break
            time.sleep(0.1)
        wait_for_quiescence()
        assert_no_write()
        before_authority = db.run("SELECT item_uid,item_revision,owner_id,root_item_uid,parent_item_uid FROM item_current_owner ORDER BY item_uid")

        for uid in (52601, 52603):
            if uid == 52603:
                db.run("DELETE FROM player_items WHERE obj_uid=52603")
            plan_path = folder / f"plan-{uid}.json"
            cli(["repair-prepare", "--item-uid", str(uid), "--evidence", str(evidence_path),
                      "--backup-receipt", str(backup_path), "--artifact", str(plan_path)])
            plan = repair.load_plan(api, plan_path)
            assert plan["applyable"], (plan["classification"], plan["note"])
            controls = ["--plan", str(plan_path), "--evidence", str(evidence_path), "--offline-proof", str(proof),
                        "--actor", "synthetic-operator", "--reason", "synthetic exact UID repair"]
            try:
                cli(["repair-apply", *controls])
                raise AssertionError("apply without approval was accepted")
            except api.ToolError as error:
                assert "authorization_required" in str(error)
            cli(["repair-apply", *controls, "--approve"])
            assert db.scalar("SELECT status FROM player_death_restitution_receipt ORDER BY applied_at DESC LIMIT 1") == "2"
            db.run(f"UPDATE player_items SET cost=1 WHERE obj_uid={uid}")
            try:
                cli(["repair-verify", *controls])
                raise AssertionError("failed verification recorded a verified receipt")
            except api.ToolError as error:
                assert "payload_fidelity" in str(error)
            assert db.scalar("SELECT status FROM player_death_restitution_receipt ORDER BY applied_at DESC LIMIT 1") == "2"
            db.run(f"UPDATE player_items SET cost=9876 WHERE obj_uid={uid}")
            if uid == 52603:
                db.run("UPDATE item_current_owner SET root_item_uid=52601 WHERE item_uid=52602")
                try:
                    cli(["repair-verify", *controls])
                    raise AssertionError("invalid ancestor chain was certified")
                except api.ToolError as error:
                    assert "invalid_topology" in str(error)
                assert db.scalar("SELECT status FROM player_death_restitution_receipt ORDER BY applied_at DESC LIMIT 1") == "2"
                db.run("UPDATE item_current_owner SET root_item_uid=52602 WHERE item_uid=52602")
            cli(["repair-apply", *controls, "--approve"])
            # Fresh native processes exercise the real production load and save,
            # then cold load again with the previous process's memory gone.
            subprocess.run([str(binary), "--repair-save"], check=True)
            subprocess.run([str(binary), "--repair-check"], check=True)
            cli(["repair-verify", *controls])
            assert db.scalar("SELECT COUNT(*) FROM player_death_restitution_delivery") == "0"
            changed = copy.deepcopy(evidence)
            changed["snapshots_hex"] = [conflict.hex()]
            after = plan_for(uid, changed)
            assert after["classification"] == "conflicting_repair_receipt", after
            # Fidelity failures cannot record verified state.
            db.run(f"UPDATE player_items SET cost=1 WHERE obj_uid={uid}")
            try:
                repair.verify(api, db, plan)
                raise AssertionError("changed serialized state was certified")
            except api.ToolError as error:
                assert "payload_fidelity" in str(error)
            db.run(f"UPDATE player_items SET cost=9876 WHERE obj_uid={uid}")
        assert db.run("SELECT item_uid,item_revision,owner_id,root_item_uid,parent_item_uid FROM item_current_owner ORDER BY item_uid") == before_authority
        assert db.scalar("SELECT COUNT(*) FROM player_death_restitution_receipt WHERE status=3") == "2"
        assert db.scalar("SELECT COUNT(*) FROM item_ownership_ledger") == "0"
        assert db.scalar("SELECT COUNT(*) FROM currency_ledger") == "0"
        print("successful generated/nested repair, no authority side effects, all refusals, stale transaction fences, rollback, interruption, idempotency and process restart fidelity passed")


def codec_snapshot_without_uid() -> str:
    # A complete supported death fixture without this UID is independent native
    # identity evidence. It is not a hand-built replacement serialization.
    binary = ROOT / "bin/tests/repair-absent-fixture"
    subprocess.run(["g++", "-std=c++20", "-Isrc", "tests/async/player_death_restitution_fixture.cpp",
                    "src/player/player_snapshot_codec.c", "-o", str(binary)], cwd=ROOT, check=True)
    return subprocess.check_output([str(binary)]).decode().splitlines()[0]


if __name__ == "__main__":
    main()
