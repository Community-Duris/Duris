#!/usr/bin/env python3
"""Executable status journeys after the disposable native death fixture.

Never load .env. Mutation is permitted only with the explicit synthetic gate and
the two known fixture databases; ordinary status remains strictly read-only.
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
    resolved = read("--include-resolved")
    case = next(case for case in resolved["cases"] if case["pid"] == 2)
    assert case["terminal_custody"] == "restored" and not case["recovery_required"]
    assert case["counts"]["restored"] == 1 and case["counts"]["safely_retired"] == 2
    assert resolved["unresolved_cases"] == 1  # Original wallet remains owed.
    assert all(case["pid"] != 2 for case in read()["cases"])
    # Neither an applied receipt nor a completed death can conceal lost delivery.
    db.run("DELETE FROM player_items WHERE obj_uid=401")
    missing = read("--after-pid", "1", "--after-revision", "5")["cases"][0]
    assert missing["terminal_custody"] == "unresolved"
    assert missing["recovery_owner"] == "custody_reconciliation"
    assert missing["correlation"] == item_case["correlation"]
    db.run("INSERT INTO player_items(pid,vnum,quantity,obj_uid) VALUES (2,501,1,401)")
    assert read("--after-pid", "1", "--after-revision", "5")["cases"] == []

print("[PASS] protected SQL status, durable-only child, restart, pagination, delivery loss and successful reconciliation")
