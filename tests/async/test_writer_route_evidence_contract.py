#!/usr/bin/env python3
"""Contract regression verifying systematic writer route evidence linking."""
from pathlib import Path
import json
import subprocess
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]


class TestWriterRouteEvidenceContract(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        path = ROOT / "docs/persistence/economy_accounting/writers.json"
        cls.inventory = json.loads(path.read_text(encoding="utf-8"))
        matrix = ROOT / "docs/persistence/economy_accounting/writer_coverage_matrix.json"
        cls.matrix = json.loads(matrix.read_text(encoding="utf-8"))

    def test_all_writers_have_identifiable_routes_and_valid_test_candidates(self):
        writers = self.inventory["writers"]
        self.assertEqual(len(writers), self.matrix["counts"]["draft_registry_rows"])
        self.assertEqual(len({w["id"] for w in writers}), len(writers))
        for w in writers:
            for test in w["test_candidates"]:
                self.assertTrue(
                    (ROOT / test).is_file(),
                    f"Writer {w['id']} candidate test file missing: {test}",
                )
            self.assertEqual(set(w["backends"]), {"mysql", "mariadb", "flatfile"})
        if not self.matrix["coverage_complete"]:
            self.assertEqual(self.matrix["playable_release_status"], "BLOCKED")

    def test_economy_accounting_validator_passes(self):
        cmd = [sys.executable, str(ROOT / "scripts/validate_economy_accounting.py")]
        res = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True)
        self.assertEqual(
            res.returncode, 0, f"Validator failed: {res.stderr}\n{res.stdout}"
        )
        self.assertIn(f"{len(self.inventory['writers'])} writer routes", res.stdout)


if __name__ == "__main__":
    unittest.main()
