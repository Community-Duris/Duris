#!/usr/bin/env python3
"""Contract regression verifying systematic writer route evidence linking."""
from pathlib import Path
import json
import subprocess
import unittest

ROOT = Path(__file__).resolve().parents[2]


class TestWriterRouteEvidenceContract(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        path = ROOT / "docs/persistence/economy_accounting/writers.json"
        cls.inventory = json.loads(path.read_text(encoding="utf-8"))

    def test_all_writers_have_evidence_and_test_candidates(self):
        writers = self.inventory["writers"]
        self.assertEqual(len(writers), 115)
        for w in writers:
            self.assertTrue(
                bool(w.get("test_candidates")),
                f"Writer {w['id']} lacks test_candidates",
            )
            self.assertTrue(
                bool(w.get("evidence")),
                f"Writer {w['id']} lacks evidence",
            )
            for test in w["test_candidates"]:
                self.assertTrue(
                    (ROOT / test).is_file(),
                    f"Writer {w['id']} candidate test file missing: {test}",
                )
            for backend_name, backend in w["backends"].items():
                self.assertTrue(
                    bool(backend.get("evidence")),
                    f"Writer {w['id']} backend {backend_name} lacks evidence",
                )

    def test_economy_accounting_validator_passes(self):
        cmd = ["python3", str(ROOT / "scripts/validate_economy_accounting.py")]
        res = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True)
        self.assertEqual(
            res.returncode, 0, f"Validator failed: {res.stderr}\n{res.stdout}"
        )
        self.assertIn("115 writer routes", res.stdout)


if __name__ == "__main__":
    unittest.main()
