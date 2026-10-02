#!/usr/bin/env python3
"""Verify diagnostic classification, protected output and read-only SQL capture."""

import json
import os
from pathlib import Path
import stat
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
import persistence_doctor as doctor


def report():
    return {"schema_version": 1, "report_type": "duris-persistence-diagnostic",
            "backend": "mariadb-primary", "target": {"kind": "player", "value": "9001"},
            "state": {"quarantined": 1, "archive_available": 1,
                      "journal_available": 1, "pipeline_available": 1},
            "history": {"available": 1, "events": [], "incidents": [
                {"stage": "save_result", "sequence": 1, "error": 10001, "diagnosis": 6,
                 "witness": {"item_uid": 7001}}]}}


class DoctorTest(unittest.TestCase):
    @unittest.skipUnless(os.environ.get("PERSISTENCE_DIAGNOSTICS_MYSQL_TEST") == "1",
                         "requires an explicitly disposable SQL fixture")
    def test_native_sql_graph_and_unchanged_authority(self):
        config = {key: os.environ.get(key, "") for key in
                  ("DB_HOST", "DB_PORT", "DB_NAME", "DB_USER", "DB_PASSWD", "DB_ALLOWED_TARGETS")}
        self.assertEqual(config["DB_HOST"], "127.0.0.1")
        self.assertTrue(config["DB_NAME"].startswith("economic_schema_test_"))
        self.assertEqual(os.environ.get("TEST_DB_DISPOSABLE"), "1")
        self.assertTrue(config["DB_PORT"].isdigit())
        client = ["mysql", "--no-defaults", "--protocol=tcp", "--host", config["DB_HOST"],
                  "--port", config["DB_PORT"], "--user", config["DB_USER"], "--batch",
                  "--skip-column-names", config["DB_NAME"]]

        def sql(statement):
            return subprocess.check_output(client, input=statement, text=True,
                                           env=doctor.process_environment(config))

        pid, foreign, first, second = 900991, 900992, 800900991, 800900992
        sql(f"INSERT INTO player_data(pid,name,save_revision) VALUES "
            f"({pid},'DiagnosticFixture',7),({foreign},'DiagnosticForeignFixture',3);"
            f"INSERT INTO item_owner_revision(owner_type,owner_id,owner_context_id,revision) "
            f"VALUES(1,{pid},0,4),(1,{foreign},0,5);"
            f"INSERT INTO item_current_owner(item_uid,root_item_uid,owner_type,owner_id,"
            f"item_revision,vnum,equipment_slot) VALUES "
            f"({first},{first},1,{foreign},1,73,0),({second},{second},1,{pid},2,74,0);"
            f"INSERT INTO player_items(pid,obj_uid,vnum,equip_slot) VALUES({pid},{first},73,0);")
        state_sql = (f"SELECT save_revision FROM player_data WHERE pid IN ({pid},{foreign}) ORDER BY pid;"
                     f"SELECT item_uid,owner_id,item_revision FROM item_current_owner "
                     f"WHERE item_uid IN ({first},{second}) ORDER BY item_uid;"
                     f"SELECT obj_uid,vnum FROM player_items WHERE pid={pid};")
        try:
            before = sql(state_sql)
            native = doctor.capture_sql(config, doctor.target("player", str(pid)))
            observed = {row["uid"]: row for row in native["rows"] if row["kind"] == "custody"}
            self.assertEqual(observed[first]["owner_id"], foreign)
            categories = {(row["category"], row["uid"]) for row in doctor.findings(native)}
            self.assertEqual(categories, {("payload_owner_mismatch", first),
                                          ("custody_without_payload", second)})
            self.assertTrue(native["consistent_read"])
            self.assertFalse(doctor.assess(None, native)["recovery_verified"])
            doctor.capture_sql(config, doctor.target("item", str(first)))
            doctor.capture_sql(config, doctor.target("operation", "ab" * 16))
            self.assertEqual(sql(state_sql), before)
        finally:
            sql(f"DELETE FROM player_items WHERE pid={pid};"
                f"DELETE FROM item_current_owner WHERE item_uid IN ({first},{second});"
                f"DELETE FROM item_owner_revision WHERE owner_type=1 AND owner_id IN ({pid},{foreign});"
                f"DELETE FROM player_data WHERE pid IN ({pid},{foreign});")

    def test_recovery_requires_original_proof(self):
        assessment = doctor.assess(report(), None)
        self.assertFalse(assessment["recovery_verified"])
        self.assertEqual(assessment["diagnosis"], "active_custody_absent_from_snapshot")
        self.assertEqual(assessment["action"], "stopped_native_recovery_evaluation_required")
        self.assertIn("native_authority_not_captured", assessment["gaps"])
        self.assertTrue(any("envelopes" in value for value in assessment["required_recovery_evidence"]))

    def test_history_loss_is_explicit(self):
        fixture = report()
        fixture["history"].update(overwritten=1, dropped=1, matching_events=3)
        self.assertIn("runtime_history_loss", doctor.assess(fixture, None)["gaps"])
        self.assertIn("runtime_history_window_truncated", doctor.assess(fixture, None)["gaps"])

    def test_large_cycle_and_duplicate_equipment(self):
        rows = [{"kind": "custody", "uid": uid, "root": 1,
                 "parent": uid + 1 if uid < doctor.MAX_ROWS else 1,
                 "owner_type": 1, "owner_id": 9001, "owner_context": 0,
                 "state": 1, "inline_coin": 1, "slot": 0}
                for uid in range(1, doctor.MAX_ROWS + 1)]
        cycles = doctor.findings({"rows": rows})
        self.assertEqual(len(cycles), doctor.MAX_ROWS)
        self.assertTrue(all(row["category"] == "custody_cycle" for row in cycles))
        for row in rows[:2]:
            row.update(parent=0, root=row["uid"], slot=2)
        self.assertEqual(sum(row["category"] == "duplicate_custody_equipment_slot"
                             for row in doctor.findings({"rows": rows[:2]})), 2)

    def test_graph_findings_and_valid_inline_coins(self):
        native = {"rows": [
            {"kind": "custody", "uid": 1, "root": 1, "parent": 0, "owner_type": 1,
             "owner_id": 9001, "owner_context": 0, "state": 1, "inline_coin": 0, "vnum": 100},
            {"kind": "custody", "uid": 2, "root": 2, "parent": 0, "owner_type": 1,
             "owner_id": 9001, "owner_context": 0, "state": 1, "inline_coin": 1, "vnum": 3},
            {"kind": "payload", "source": "player_items", "uid": 3, "owner_type": 1,
             "owner_id": 9001, "owner_context": 0, "vnum": 100, "parent": 0, "broken_parent": 0},
        ]}
        findings = doctor.findings(native)
        self.assertEqual({item["category"] for item in findings},
                         {"custody_without_payload", "payload_without_custody"})
        self.assertFalse(any(item["uid"] == 2 for item in findings))

    def test_one_consistent_read_without_writes_or_payload_prose(self):
        for kind, value in (("player", "9001"), ("item", "7001"), ("operation", "ab" * 16)):
            sql = doctor.sql_snapshot_statement(doctor.target(kind, value))
            self.assertEqual(sql.count("START TRANSACTION"), 1)
            self.assertIn("CONSISTENT SNAPSHOT, READ ONLY", sql)
            self.assertTrue(sql.endswith("ROLLBACK;"))
            for forbidden in ("FOR UPDATE", "DELETE ", "INSERT ", "UPDATE ", "REPLACE ", "short_descr", "account_name"):
                self.assertNotIn(forbidden, sql)
        with self.assertRaises(doctor.DoctorError):
            doctor.target("player", "1; DELETE FROM player_data")

    def test_explicit_port_and_redacted_sql_failure(self):
        config = {"DB_HOST": "127.0.0.1", "DB_PORT": "3407", "DB_NAME": "test_diagnostic",
                  "DB_USER": "test", "DB_PASSWD": "secret", "DB_ALLOWED_TARGETS": "127.0.0.1/test_diagnostic"}
        failed = subprocess.CompletedProcess([], 1, "", "ERROR 1146: secret private sql")
        with patch.object(doctor, "preferred_mysql_ssl_arguments", return_value=()), \
                patch.object(doctor.subprocess, "run", return_value=failed) as run:
            with self.assertRaisesRegex(doctor.DoctorError, r"code 1146") as error:
                doctor.capture_sql(config, doctor.target("player", "9001"))
            self.assertNotIn("secret", str(error.exception))
            self.assertIn("3407", run.call_args.args[0])
            self.assertNotIn("secret", run.call_args.args[0])

    def test_owner_only_files_no_overwrite_and_cli(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source, output = root / "native.json", root / "incident.json"
            source.write_text(json.dumps(report()))
            source.chmod(0o600)
            self.assertEqual(doctor.read_report(source)["target"]["value"], "9001")
            run = subprocess.run([sys.executable, str(ROOT / "scripts/persistence_doctor.py"),
                                  "--report", str(source), "--output", str(output)],
                                 capture_output=True, text=True)
            self.assertEqual(run.returncode, 0, run.stderr)
            self.assertNotIn("9001", run.stdout)
            self.assertNotIn("7001", run.stdout)
            before = output.read_bytes()
            with self.assertRaises(FileExistsError):
                doctor.save_private(output, {})
            self.assertEqual(before, output.read_bytes())
            self.assertEqual(stat.S_IMODE(output.stat().st_mode), 0o600)
            source.write_text("[]")
            refused = subprocess.run([sys.executable, str(ROOT / "scripts/persistence_doctor.py"),
                                      "--report", str(source), "--output", str(root / "refused.json")],
                                     capture_output=True, text=True)
            self.assertEqual(refused.returncode, 2)
            self.assertNotIn("Traceback", refused.stderr)
            self.assertFalse((root / "refused.json").exists())
            source.chmod(0o644)
            with self.assertRaises(doctor.DoctorError):
                doctor.read_report(source)


if __name__ == "__main__":
    unittest.main()
