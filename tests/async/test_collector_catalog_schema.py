#!/usr/bin/env python3
"""Offline registration contracts for the immutable collector catalog schema."""

from __future__ import annotations

import hashlib
import json
import stat
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
MIGRATION = ROOT / "migrations/immutable/0018_collector_catalog.sql"
VERIFIER = ROOT / "migrations/immutable/0018_collector_catalog.sh"
WRAPPER = ROOT / "tests/async/run_collector_catalog_schema_mysql.sh"


class CollectorCatalogSchemaTest(unittest.TestCase):
    def test_immutable_step_is_checksum_sealed_and_registered(self) -> None:
        manifest = json.loads((ROOT / "migrations/migration_manifest.json").read_text())
        step = next(item for item in manifest["migrations"]
                    if item["id"] == "0018_collector_catalog")
        self.assertEqual((step["id"], step["sequence"]),
                         ("0018_collector_catalog", 18))
        self.assertEqual(step["apply_checksum"],
                         hashlib.sha256(MIGRATION.read_bytes()).hexdigest())
        self.assertEqual(step["verify_checksum"],
                         hashlib.sha256(VERIFIER.read_bytes()).hexdigest())

    def test_authority_shape_is_in_fresh_bootstrap_and_immutable_upgrade(self) -> None:
        migration = MIGRATION.read_text()
        bootstrap = (ROOT / "migrations/bootstrap_multithread_safe.sql").read_text()
        for table in ("collector_catalog_state", "collector_deaths",
                      "collector_listings", "collector_ledger",
                      "collector_reconciliation_quarantine"):
            self.assertIn(f"CREATE TABLE IF NOT EXISTS {table}", migration)
            self.assertIn(f"CREATE TABLE `{table}`", bootstrap)
        self.assertIn("record_blob VARBINARY(154) NOT NULL", migration)
        self.assertIn("OCTET_LENGTH(record_blob) = 154", migration)
        self.assertNotIn("record_blob BINARY(154)", migration)
        self.assertIn("collector_listing_death_fk", migration)
        self.assertNotIn("collector_ledger_operation_fk", migration)
        self.assertIn("PRIMARY KEY (operation_id,listing_id)", migration)

    def test_verifier_and_disposable_dual_engine_wrapper_are_wired(self) -> None:
        verifier = VERIFIER.read_text()
        wrapper = WRAPPER.read_text()
        makefile = (ROOT / "Makefile").read_text()
        self.assertIn("singleton_rows", verifier)
        self.assertNotIn("1:0:1", verifier)
        self.assertIn("COLLECTOR_CATALOG_DB_IMAGE", wrapper)
        self.assertIn("never reads .env", wrapper)
        self.assertIn("run_collector_catalog_schema_mysql.sh", makefile)
        self.assertTrue(WRAPPER.stat().st_mode & stat.S_IXUSR)

    def test_runtime_and_lifecycle_inventories_include_all_collector_tables(self) -> None:
        runtime = json.loads(
            (ROOT / "migrations/runtime_compatibility_manifest.json").read_text()
        )
        lifecycle = json.loads(
            (ROOT / "migrations/data_lifecycle_manifest.json").read_text()
        )
        lifecycle_ids = {entry["id"] for entry in lifecycle["entries"]}
        for table in ("collector_catalog_state", "collector_deaths",
                      "collector_listings", "collector_ledger",
                      "collector_reconciliation_quarantine", "offline_message_receipts"):
            self.assertIn(f"'{table}'", runtime["runtime_table_sql_list"])
            self.assertIn(f"database:{table}", lifecycle_ids)
        self.assertEqual(runtime["current_table_count"], 198)
        self.assertEqual(runtime["migration_head"]["id"],
                         "0021_collector_notification_identity")


if __name__ == "__main__":
    unittest.main()
