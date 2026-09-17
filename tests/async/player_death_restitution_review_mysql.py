#!/usr/bin/env python3
"""Run only inside the disposable reconciliation runner's synthetic database."""
from pathlib import Path
import importlib.util
import json
import os
import subprocess
import sys
from unittest import mock

ROOT = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location("player_death_restitution", ROOT / "scripts/player_death_restitution.py")
assert SPEC and SPEC.loader
cli = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(cli)


def main():
    if os.environ.get("ENVIRONMENT") != "test" or os.environ.get("DB_NAME") != "duris_issue_331_test":
        raise RuntimeError("requires the disposable reconciliation fixture")
    plan_path = Path(sys.argv[1])
    temp = Path(sys.argv[2])
    db = cli.Mysql()
    plan = json.loads(plan_path.read_text())

    # Missing mortal/native/domain signals must not bypass a god authority row.
    assert db.scalar("SELECT COUNT(*) FROM artifacts WHERE vnum=101") == "0"
    db.run("INSERT INTO artifacts(vnum,owned,location,timer,type,lastUpdate,locType) "
           "VALUES(101,'Y',42,FROM_UNIXTIME(1700000456),2,FROM_UNIXTIME(1700000456),3)")
    try:
        inspection = temp / "god-only-inspection.json"
        reviewed = temp / "god-only-plan.json"
        subprocess.run([sys.executable, str(ROOT / "scripts/player_death_restitution.py"),
                        "inspect", "--pid", "42", "--death-revision", "7",
                        "--recipient-pid", "42", "--artifact", str(inspection)], check=True)
        subprocess.run([sys.executable, str(ROOT / "scripts/player_death_restitution.py"),
                        "plan", "--inspect", str(inspection), "--artifact", str(reviewed),
                        "--approve-artifact-reconciliation"], check=True)
        candidate = next(r for r in json.loads(reviewed.read_text())["items"] if r["item_uid"] == 1001)
        assert candidate["kind"] == "artifact" and candidate["artifact_reconciliation_required"], candidate
        assert candidate["artifact_reconciliation"]["legacy_before"]["god"]["timer"] == 1700000456
        assert candidate["artifact_after"]["timer_epoch"] is None
        assert candidate["artifact_after"]["timer_epoch_mode"] == "delivery_epoch_plus_usable_lifetime"
        assert candidate["artifact_after"]["usable_lifetime_seconds"] == 456
    finally:
        db.run("DELETE FROM artifacts WHERE vnum=101")
    print("god-only legacy authority is fenced, not recovered as an ordinary item")

    # Generate SQL before the competing custody identity appears. This separately
    # proves the SQL fence, not just Python's stale-plan check.
    prepared_sql = cli.build_apply_sql(plan, "review-test", "custody-fence")
    assert db.scalar("SELECT COUNT(*) FROM player_death_custody WHERE item_uid=9004") == "0"
    db.run("INSERT INTO player_death_custody(pid,save_revision,item_uid,root_item_uid,"
           "parent_item_uid,item_revision,vnum,state,owner_type,owner_id,owner_context_id,owner_revision) "
           "VALUES(99,8,9004,9004,0,1,104,1,1,99,0,1)")
    db.run("DELETE FROM player_items WHERE obj_uid=3000")
    try:
        artifacts = cli.fetch_artifacts(db, [104])
        assert any(r["item_uid"] == 9004 and r.get("source_table") == "player_death_custody"
                   for r in artifacts["competitors"]["104"])
        attempt = subprocess.run([sys.executable, str(ROOT / "scripts/player_death_restitution.py"),
                                  "apply", "--plan", str(plan_path), "--offline-proof", str(temp / "proof"),
                                  "--approve", "--approve-artifact-reconciliation",
                                  "--actor", "review-test", "--reason", "custody-only-competitor"],
                                 capture_output=True, text=True)
        assert attempt.returncode != 0 and "stale" in attempt.stderr.lower(), attempt.stderr
        output = db.run(prepared_sql)
        assert any("DURIS_RESULT|0|" in cell for row in output for cell in row), output
        assert db.scalar("SELECT COUNT(*) FROM player_death_restitution_receipt") == "0"
        assert db.scalar("SELECT COUNT(*) FROM player_death_restitution_delivery") == "0"
        assert db.scalar("SELECT COUNT(*) FROM player_items WHERE obj_uid BETWEEN 1000 AND 1004") == "0"
        assert db.scalar("SELECT COUNT(*) FROM artifact_domain_state WHERE vnum=104") == "0"
    finally:
        db.run("DELETE FROM player_death_custody WHERE pid=99 AND save_revision=8 AND item_uid=9004")
        db.run("INSERT INTO player_items(pid,vnum,equip_slot,container_id,quantity,weight,cost,timer,"
               "extra_flags,wear_flags,item_type,value0,value1,value2,value3,value4,value5,value6,value7,"
               "name,short_descr,description,action_descr,bitvector1,bitvector2,bitvector3,bitvector4,"
               "bitvector5,item_material,obj_uid,item_condition) VALUES "
               "(42,300,0,NULL,1,1,5,123,0,0,1,0,0,0,0,0,0,0,0,"
               "'newer item','newer item','newer item','newer item',0,0,0,0,0,1,3000,99)")
    print("custody-only identity: inspection, stale-plan and real transaction refusal verified; zero recovery writes")

    # Empty PROCESSLIST is not proof of visibility. Require a verifiable direct
    # global PROCESS grant before interpreting empty session/transaction views.
    db.run("CREATE USER 'issue331_visibility'@'%' IDENTIFIED BY 'issue331-visibility-test-only'")
    try:
        db.run("GRANT SELECT ON duris_issue_331_test.* TO 'issue331_visibility'@'%'")
        with mock.patch.dict(os.environ, {"DB_USER": "issue331_visibility",
                                          "DB_PASSWD": "issue331-visibility-test-only",
                                          "MYSQL_PWD": "issue331-visibility-test-only"}):
            limited = cli.Mysql()
            try:
                cli.check_database_quiescence(limited)
            except cli.ToolError as exc:
                assert "requires a direct global PROCESS grant" in str(exc), str(exc)
            else:
                raise AssertionError("restricted account falsely established quiescence")
        db.run("GRANT PROCESS ON *.* TO 'issue331_visibility'@'%'")
        with mock.patch.dict(os.environ, {"DB_USER": "issue331_visibility",
                                          "DB_PASSWD": "issue331-visibility-test-only",
                                          "MYSQL_PWD": "issue331-visibility-test-only"}):
            cli.check_database_quiescence(cli.Mysql())
    finally:
        db.run("DROP USER 'issue331_visibility'@'%'")
    print("restricted recovery account refused; explicit PROCESS visibility accepted")


if __name__ == "__main__":
    main()
