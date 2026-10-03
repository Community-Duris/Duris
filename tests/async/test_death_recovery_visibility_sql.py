#!/usr/bin/env python3
"""Executable status journeys after the disposable native death fixture.

Never load .env. Mutation is permitted only with the explicit synthetic gate and
the known fixture databases; ordinary status remains strictly read-only.
"""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
import player_death_restitution as api
import player_death_recovery_visibility as recovery

assert os.environ.get("DURIS_RECOVERY_SYNTHETIC_FIXTURE") == "1"
assert os.environ.get("ENVIRONMENT") == "test"
assert os.environ.get("DB_NAME") in {"death_disposition_test", "recovery570", "recovery570b"}
args = api.parser().parse_args(["status", "--artifact", "/unused"])
policy, _ = api.policy_for_command(args)
db = api.Mysql(policy)
assert db.scalar("SELECT LOWER(HEX(operation_id)) FROM player_death_disposition WHERE pid=2 AND save_revision=1") == "a600000000000000000000000000005a"

with tempfile.TemporaryDirectory(prefix="death-status-") as temporary:
    artifact = Path(temporary) / "status.json"
    command = [sys.executable, str(ROOT / "scripts/player_death_restitution.py"),
               "status", "--artifact", str(artifact), "--overwrite"]

    def read(*extra):
        completed = subprocess.run(command + list(extra), cwd=ROOT, check=True, capture_output=True, text=True)
        # Sensitive case identifiers belong only in the protected artifact.
        assert "item_uid" not in completed.stdout and "operation_id" not in completed.stdout
        assert artifact.stat().st_mode & 0o077 == 0
        return json.loads(artifact.read_text())

    first = read("--limit", "1")
    assert first["next_cursor"] == {"after_pid": 1, "after_revision": 5}
    original = first["cases"][0]
    assert original["death_disposition"] == "completed" and original["recovery_required"]
    assert original["counts"]["quarantine"] == 3 and original["counts"]["unresolved"] == 1
    assert original["unmatched_count"] == 1 and original["correlation"] == recovery.correlation(1, 9001)
    second = read("--after-pid", "1", "--after-revision", "5")
    item_case = second["cases"][0]
    assert {item["item_uid"] for item in item_case["items"]} == {401, 403, 407}
    assert item_case["terminal_custody"] == "quarantine"

    # Synthetic reviewed delivery and explicit destruction afterimages. The
    # production query must retain 407 after its original root relationship ends.
    db.run("INSERT INTO player_items(pid,vnum,quantity,obj_uid) VALUES (2,501,1,401)")
    db.run("UPDATE item_current_owner SET state=1,item_revision=4 WHERE item_uid=401")
    db.run("UPDATE item_current_owner SET state=2,owner_type=8,owner_id=0,root_item_uid=item_uid,parent_item_uid=NULL,item_revision=2 WHERE item_uid IN (403,407)")
    restitution_id = "57000000000000000000000000000001"
    db.run("INSERT INTO player_death_restitution_receipt(restitution_id,source_pid,death_revision,recipient_pid,death_operation_id,evidence_digest,plan_digest,status,actor,reason,candidate_count,delivered_count) VALUES (UNHEX('" + restitution_id + "'),2,1,2,UNHEX('a600000000000000000000000000005a'),UNHEX(REPEAT('00',32)),UNHEX(REPEAT('01',32)),2,'fixture','synthetic delivery',1,1)")
    db.run("INSERT INTO player_death_restitution_item(restitution_id,item_uid,disposition,classification) VALUES (UNHEX('" + restitution_id + "'),401,1,'fixture')")
    db.run("INSERT INTO player_death_restitution_delivery(item_uid,restitution_id,source_pid,death_revision,recipient_pid,source_item_revision,delivered_item_revision,delivered_item_id,metadata_digest,original_payload) SELECT 401,UNHEX('" + restitution_id + "'),2,1,2,3,4,id,UNHEX(REPEAT('00',32)),0x01 FROM player_items WHERE obj_uid=401")
    pending = read("--after-pid", "1", "--after-revision", "5")["cases"][0]
    assert pending["recovery_required"] and pending["verification_requires_review"]
    assert pending["recovery_owner"] == "restitution_verification"
    # Synthetic afterimage of existing protected verify; an applied receipt
    # alone cannot close the recovery case.
    db.run("UPDATE player_death_restitution_receipt SET status=3 WHERE restitution_id=UNHEX('" + restitution_id + "')")
    resolved = read("--include-resolved")
    case = next(case for case in resolved["cases"] if case["pid"] == 2)
    assert case["terminal_custody"] == "restored" and not case["recovery_required"]
    assert case["counts"]["restored"] == 1 and case["counts"]["safely_retired"] == 2
    assert resolved["unresolved_cases"] == 1  # Original wallet remains owed.
    assert all(case["pid"] != 2 for case in read()["cases"])
    # Neither a verified receipt nor a completed death can conceal lost delivery.
    db.run("DELETE FROM player_items WHERE obj_uid=401")
    missing = read("--after-pid", "1", "--after-revision", "5")["cases"][0]
    assert missing["terminal_custody"] == "unresolved"
    assert missing["recovery_owner"] == "custody_reconciliation"
    assert missing["correlation"] == item_case["correlation"]
    db.run("INSERT INTO player_items(pid,vnum,quantity,obj_uid) VALUES (2,501,1,401)")
    assert read("--after-pid", "1", "--after-revision", "5")["cases"] == []

    # Accounting's archive is an existing recovery owner. Its native evidence
    # encoding is readable for status while remaining ineligible for restitution.
    fixture = Path(temporary) / "evidence-fixture"
    subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-Isrc",
                    "tests/async/player_death_conflict_evidence_codec_harness.cpp", "src/player/player_snapshot_codec.c",
                    "-o", str(fixture)], cwd=ROOT, check=True)
    evidence_hex = subprocess.check_output([str(fixture), "conflict-evidence"]).hex()
    evidence = api.decode_payload(bytes.fromhex(evidence_hex), recovery_status=True)
    assert evidence["death"]["conflict_evidence_retained"]
    try:
        api.decode_payload(bytes.fromhex(evidence_hex))
    except api.ToolError:
        pass
    else:
        raise AssertionError("status must not broaden restitution eligibility")
    source_pid, evidence_revision = evidence["pid"], evidence["revision"]
    evidence_operation = evidence["death"]["operation_id_hex"]
    db.run("INSERT INTO player_death_conflict_evidence(operation_id,pid,save_revision,source_revision,corpse_item_uid,request_hash,payload_hash,payload) VALUES (UNHEX('" + evidence_operation + "')," + str(source_pid) + "," + str(evidence_revision) + ",1," + str(evidence["death"]["corpse"][0]["object_uid"]) + ",UNHEX(REPEAT('00',32)),UNHEX(SHA2(UNHEX('" + evidence_hex + "'),256)),UNHEX('" + evidence_hex + "'))")
    archive = read("--after-pid", "2")["cases"][0]
    assert archive["death_disposition"] == "retained_not_completed"
    assert archive["recovery_required"] and archive["recovery_owner"] == "retained_death_conflict"
    assert archive["correlation"] == recovery.correlation(source_pid, evidence["death"]["corpse"][0]["values"][6])
    # A committed non-item disposition with the same evidence is still held.
    db.run("INSERT INTO player_death_disposition(pid,save_revision,operation_id,corpse_item_uid,corpse_room_vnum,wallet_revision,wallet_pile_uid,payload) VALUES (" + str(source_pid) + "," + str(evidence_revision) + ",UNHEX('" + evidence_operation + "')," + str(evidence["death"]["corpse"][0]["object_uid"]) + ",1201,0,0,UNHEX('" + evidence_hex + "'))")
    archive = read("--after-pid", "2")["cases"][0]
    assert archive["death_disposition"] == "completed_with_retained_conflict"
    assert archive["recovery_required"] and archive["recovery_owner"] == "retained_death_conflict"

print("[PASS] protected SQL status, durable-only child, restart, pagination, pending verification, delivery loss and successful reconciliation")
