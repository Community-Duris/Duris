#!/usr/bin/env python3
"""Telemetry migration contracts that guard signed units, scope and lifecycle."""
import json
import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SQL = ROOT / "migrations/immutable/0014_telemetry_storage.sql"
TABLES = {"telemetry_session", "telemetry_interval", "telemetry_config",
          "telemetry_player_day", "telemetry_cohort_day", "telemetry_rollup_state"}


class TelemetrySchemaTest(unittest.TestCase):
    def setUp(self):
        self.sql = SQL.read_text()
        self.tables = dict(re.findall(
            r"CREATE TABLE IF NOT EXISTS (\w+) \((.*?)\) ENGINE=InnoDB",
            self.sql, re.S))

    def test_exact_six_stores_with_no_gameplay_dependencies(self):
        self.assertEqual(set(self.tables), TABLES)
        self.assertEqual(self.sql.count("COLLATE=utf8mb4_unicode_ci"), 6)
        self.assertNotRegex(self.sql.upper(), r"FOREIGN KEY|CREATE TRIGGER|CREATE EVENT|PARTITION BY")
        self.assertNotRegex(self.sql.upper(), r"\b(?:JSON|BLOB|TEXT|VARCHAR)\b")

    def test_global_replay_and_session_scope_and_checkpoint_lookup(self):
        facts = self.tables["telemetry_interval"]
        self.assertIn("UNIQUE KEY uq_telemetry_replay (boot_id,process_id,record_seq)", facts)
        self.assertIn("KEY idx_telemetry_checkpoint (environment_id,season_id,session_boot_id,session_process_id,session_seq,checkpoint_revision)", facts)
        session = self.tables["telemetry_session"]
        self.assertIn("PRIMARY KEY (environment_id,season_id,session_boot_id,session_process_id,session_seq)", session)
        self.assertIn("UNIQUE KEY uq_telemetry_session_identity (session_boot_id,session_process_id,session_seq)", session)
        self.assertIn("PRIMARY KEY (environment_id,config_id)", self.tables["telemetry_config"])

    def test_record_kinds_preserve_typed_units_and_absent_fields(self):
        facts = self.tables["telemetry_interval"]
        for name in ("occurrence_utc_usec", "ingested_utc_usec", "start_utc_usec", "end_utc_usec", "at_utc_usec", "effective_utc_usec"):
            self.assertRegex(facts, rf"\b{name} BIGINT (?:NOT )?NULL")
        for name in ("duration_usec", "start_monotonic_usec", "end_monotonic_usec", "checkpoint_revision", "first_missing_record_seq", "last_missing_record_seq", "dropped_records", "config_revision", "connected_usec", "resident_usec"):
            self.assertIn(name + " BIGINT UNSIGNED NULL", facts)
        for name in ("record_kind", "lifecycle", "gap_reason"):
            self.assertRegex(facts, rf"\b{name} TINYINT UNSIGNED (?:NOT )?NULL")
        self.assertIn("fingerprint BINARY(32) NULL", facts)
        self.assertIn("fingerprint BINARY(32) NOT NULL", self.tables["telemetry_config"])
        self.assertNotIn("payload_fingerprint", facts)

    def test_lifecycle_inventory_retains_and_requires_decisions(self):
        manifest = json.loads((ROOT / "migrations/data_lifecycle_manifest.json").read_text())
        entries = {e["locator"]: e for e in manifest["entries"] if e["locator"] in TABLES}
        self.assertEqual(set(entries), TABLES)
        self.assertFalse(manifest["controller_approval"]["destructive_rules_enabled"])
        for entry in entries.values():
            self.assertEqual(entry["season_action"], "retain")
            self.assertEqual(entry["terminal_action"], "retain")
            self.assertEqual(entry["controller_decision"]["status"], "pending")
            self.assertEqual(entry["export_rule"]["disposition"], "pending")
        for name in ("telemetry_interval", "telemetry_config", "telemetry_rollup_state"):
            self.assertTrue(entries[name]["protected_record"])


if __name__ == "__main__":
    unittest.main()
