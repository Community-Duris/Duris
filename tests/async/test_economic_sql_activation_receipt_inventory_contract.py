#!/usr/bin/env python3
"""Source-only inventory contract for the SQL activation receipt."""

from pathlib import Path
import json
import re
import unittest


ROOT = Path(__file__).resolve().parents[2]
LIFECYCLE_MANIFEST = ROOT / "migrations/data_lifecycle_manifest.json"
MIGRATION = ROOT / "migrations/immutable/0036_economic_sql_activation_receipt.sql"
RUNTIME_VERIFIER = ROOT / "migrations/verify_runtime_compatibility.sh"
RECEIPT_TABLE = "economic_sql_activation_receipt"
SUPPLEMENTAL_TABLES = {
    "economic_baseline_control",
    "economic_baseline_reservation",
    "economic_baseline_witness",
    "economic_sql_lifecycle_installation",
    "economic_sql_global_activation",
    "sql_room_item_payload",
    RECEIPT_TABLE,
}
EXPECTED_COLUMNS = {
    "operation_id",
    "lineage",
    "epoch",
    "baseline_operation_id",
    "baseline_revision",
    "source_capture_digest",
    "native_boundary_digest",
    "activation_scope",
    "coverage_contract_version",
    "coverage_evidence_digest",
    "activation_digest",
    "receipt_version",
    "created_at",
}
EXPECTED_CHECKS = {
    "ck_economic_sql_activation_operation_nonzero",
    "ck_economic_sql_activation_lineage_nonzero",
    "ck_economic_sql_activation_epoch_nonzero",
    "ck_economic_sql_activation_baseline_operation_nonzero",
    "ck_economic_sql_activation_baseline_revision",
    "ck_economic_sql_activation_scope",
    "ck_economic_sql_activation_coverage_version",
    "ck_economic_sql_activation_receipt_version",
}
EXPECTED_KEYS = {
    "PRIMARY KEY (operation_id)",
    "UNIQUE KEY uq_economic_sql_activation_lineage (lineage)",
    "KEY idx_economic_sql_activation_install_binding "
    "(operation_id, lineage, epoch, baseline_operation_id)",
    "KEY idx_economic_sql_activation_epoch_revision "
    "(lineage, epoch, baseline_revision)",
    "KEY idx_economic_sql_activation_baseline_operation (baseline_operation_id)",
}


class EconomicSqlActivationReceiptInventoryContract(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.manifest = json.loads(LIFECYCLE_MANIFEST.read_text())
        cls.migration = MIGRATION.read_text()
        cls.verifier = RUNTIME_VERIFIER.read_text()

    def test_lifecycle_entry_matches_protected_replay_inventory_contract(self):
        matching = [
            entry for entry in self.manifest["entries"]
            if entry["id"] == f"database:{RECEIPT_TABLE}"
        ]
        self.assertEqual(len(matching), 1)
        receipt = matching[0]
        lifecycle_installation = next(
            entry for entry in self.manifest["entries"]
            if entry["id"] == "database:economic_sql_lifecycle_installation"
        )

        self.assertEqual(
            set(receipt),
            {
                "id", "kind", "locator", "data_category", "data_subject_key",
                "technical_purpose", "controller_decision", "season_action",
                "active_retention", "archive_retention", "terminal_action",
                "exception", "protected_record", "dependencies", "export_rule",
            },
        )
        for field in (
            "kind", "data_category", "data_subject_key", "season_action",
            "active_retention", "archive_retention", "terminal_action", "exception",
            "protected_record",
        ):
            with self.subTest(field=field):
                self.assertEqual(receipt[field], lifecycle_installation[field])
        self.assertEqual(receipt["locator"], RECEIPT_TABLE)
        self.assertEqual(
            receipt["dependencies"],
            [
                "database:critical_operation_inbox",
                "database:economic_baseline_witness",
                "database:economic_epoch",
                "database:economic_sql_lifecycle_installation",
            ],
        )
        self.assertIn(
            "no independent gameplay activation authority", receipt["technical_purpose"]
        )
        self.assertEqual(
            receipt["controller_decision"],
            lifecycle_installation["controller_decision"],
        )
        self.assertEqual(
            receipt["export_rule"],
            {
                "disposition": "pending",
                "subject_route": "operation_domain",
                "decision": {
                    "status": "pending",
                    "reference": "PENDING-SHARED-DISCLOSURE-DECISION",
                },
                "excluded_fields": [
                    "source_capture_digest",
                    "native_boundary_digest",
                    "coverage_evidence_digest",
                    "activation_digest",
                ],
                "shared_fields": [],
            },
        )

    def test_receipt_columns_and_key_sets_are_source_pinned(self):
        table = re.search(
            rf"CREATE TABLE IF NOT EXISTS {RECEIPT_TABLE}\s*\((.*?)\)\s*ENGINE=",
            self.migration,
            re.DOTALL,
        )
        self.assertIsNotNone(table, "receipt table DDL is absent")
        assert table is not None
        body = table.group(1)
        columns = set(re.findall(
            r"^\s{4}([a-z][a-z0-9_]*)\s+(?:BINARY|BIGINT|TINYINT|SMALLINT|TIMESTAMP)\b",
            body,
            re.MULTILINE,
        ))
        self.assertEqual(columns, EXPECTED_COLUMNS)

        actual_keys = {
            re.sub(r"\s+", " ", key).strip()
            for key in re.findall(
                r"^ {4}((?:PRIMARY KEY|(?:UNIQUE )?KEY)\b.*?\))",
                body,
                re.MULTILINE | re.DOTALL,
            )
        }
        self.assertEqual(actual_keys, EXPECTED_KEYS)
        check_names = set(re.findall(
            r"\bCONSTRAINT\s+(ck_[a-z0-9_]+)\s+CHECK\b", body
        ))
        self.assertEqual(check_names, EXPECTED_CHECKS)

    def test_runtime_supplemental_metadata_inventories_include_receipt(self):
        def tables_for(tag):
            lines = [
                line for line in self.verifier.splitlines()
                if f"CONCAT('{tag}'" in line
            ]
            self.assertEqual(
                len(lines), 1, f"expected one {tag} supplemental metadata query"
            )
            query = lines[0].split(f"CONCAT('{tag}'", 1)[1]
            match = re.search(r"table_name IN \(([^)]*)\)", query)
            self.assertIsNotNone(match, f"{tag} query has no table inventory")
            assert match is not None
            return re.findall(r"'([^']+)'", match.group(1))

        for tag in ("X", "K", "E"):
            with self.subTest(metadata_tag=tag):
                listed = tables_for(tag)
                self.assertEqual(set(listed), SUPPLEMENTAL_TABLES)
                self.assertEqual(len(listed), len(SUPPLEMENTAL_TABLES))


if __name__ == "__main__":
    unittest.main()
