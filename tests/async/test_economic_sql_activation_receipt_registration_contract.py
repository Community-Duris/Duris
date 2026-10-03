#!/usr/bin/env python3
"""Offline registration checks; these do not qualify SQL execution or activation."""

import hashlib
import json
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
import migration_runner  # noqa: E402

RECEIPT_ID = "0036_economic_sql_activation_receipt"
PRIOR_ID = "0035_player_item_dynamic_state"


class EconomicSqlActivationReceiptRegistrationContract(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.raw = json.loads((ROOT / "migrations/migration_manifest.json").read_text())

    def test_receipt_is_registered_once_immediately_after_item_state(self):
        entries = self.raw["migrations"]
        matches = [(index, entry) for index, entry in enumerate(entries)
                   if entry["id"] == RECEIPT_ID]
        self.assertEqual(len(matches), 1, "receipt migration must be registered once")
        index, entry = matches[0]
        self.assertGreater(index, 0)
        self.assertEqual(entries[index - 1]["id"], PRIOR_ID)
        self.assertEqual(entries[index - 1]["sequence"], 35)
        self.assertEqual(entry["sequence"], 36)
        self.assertEqual(entry["compatibility"], "mysql8-mariadb10")
        for field, suffix in (("apply", "sql"), ("verify", "sh")):
            path = f"immutable/{RECEIPT_ID}.{suffix}"
            self.assertEqual(entry[field], path)
            actual = hashlib.sha256((ROOT / "migrations" / path).read_bytes()).hexdigest()
            self.assertEqual(entry[f"{field}_checksum"], actual)

    def test_canonical_loader_includes_receipt_with_exact_paths(self):
        manifest = migration_runner.load_manifest()
        matches = [step for step in manifest.migrations if step.migration_id == RECEIPT_ID]
        self.assertEqual(len(matches), 1, "canonical runner omits receipt migration")
        step = matches[0]
        self.assertEqual(step.sequence, 36)
        self.assertEqual(step.apply_path, ROOT / "migrations/immutable" / f"{RECEIPT_ID}.sql")
        self.assertEqual(step.verify_path, ROOT / "migrations/immutable" / f"{RECEIPT_ID}.sh")

    def test_reader_object_is_registered_in_server(self):
        makefile = (ROOT / "src/Makefile").read_text()
        self.assertEqual(makefile.count("persistence/economic_sql_activation_receipt.o"), 1,
                         "receipt reader must be linked exactly once in the server")
        self.assertTrue((ROOT / "src/persistence/economic_sql_activation_receipt.c").is_file())

    def test_receipt_is_post_baseline_not_a_rewritten_adoption_contract(self):
        baseline = self.raw["baseline"]
        self.assertEqual(baseline["id"], "duris-schema-2026-08-27-session11")
        self.assertEqual(baseline["required_table_count"], 170)
        self.assertEqual(baseline["required_table_fingerprint"],
                         "db13d7a42bf82bcbd32bac8d83224913c755fefd000ade6d4e798b1bd4f494dd")
        self.assertNotIn("economic_sql_activation_receipt", baseline["required_tables"])


if __name__ == "__main__":
    unittest.main()
