#!/usr/bin/env python3
"""Source-derived epic-zone seed acceptance tests (no private dump required)."""
import importlib.util
import io
import json
from contextlib import redirect_stderr, redirect_stdout
from unittest import mock
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "scripts/epic_zone_seed.py"


class EpicZoneSeedTest(unittest.TestCase):
    def cli(self, *args):
        return subprocess.run([sys.executable, str(SCRIPT), *args], cwd=ROOT,
                              text=True, capture_output=True)

    def test_source_coverage_and_known_values(self):
        result = self.cli("manifest")
        self.assertEqual(result.returncode, 0, result.stderr)
        manifest = json.loads(result.stdout)
        rows = {r["number"]: r for r in manifest["zones"]}
        self.assertEqual(len(rows), 110)
        self.assertEqual(sum(r["stone_loads"] for r in rows.values()), 119)
        self.assertEqual(sum(r["status"] == "known" for r in rows.values()), 107)
        for number, payout in [(14, 80), (24, 225), (68, 600), (140, 90)]:
            self.assertEqual(rows[number]["epic_payout"], payout)
            self.assertEqual(rows[number]["suggested_group_size"], 100)
        self.assertEqual(rows[1389]["status"], "disabled")
        self.assertEqual(rows[1389]["epic_payout"], 0)
        self.assertIsNone(rows[1389]["suggested_group_size"])
        self.assertEqual(sum(r["status"] == "unresolved" for r in rows.values()), 2)
        self.assertTrue(all(r["epic_payout"] is None for r in rows.values()
                            if r["status"] == "unresolved"))
        self.assertNotIn("world.zon", manifest["sources"])
        self.assertIn("migrations/epic-zone-payout.sql", manifest["sources"])
        self.assertEqual(manifest["unseeded_assignments"], [{
            "number": 4200, "epic_payout": 850,
            "reason": "no stone load in active area sources"}])

    def test_deterministic_and_checked_in_manifest(self):
        first = self.cli("manifest")
        self.assertEqual(first.returncode, 0, first.stderr)
        self.assertEqual(first.stdout, self.cli("manifest").stdout)
        self.assertEqual(first.stdout, (ROOT / "migrations/seeds/epic_zone_payouts.json").read_text())
        self.assertEqual(self.cli("check").returncode, 0)

    def test_apply_requires_exact_explicit_database_and_stopped_ack(self):
        self.assertNotEqual(self.cli("sql-apply").returncode, 0)
        self.assertNotEqual(self.cli("sql-apply", "--database", "seed_qa").returncode, 0)
        self.assertNotEqual(self.cli("sql-apply", "--database", "bad'; DROP TABLE zones;--",
                                     "--server-stopped").returncode, 0)
        result = self.cli("sql-apply", "--database", "seed_qa", "--server-stopped")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("CAST(DATABASE() AS BINARY) = CAST('seed_qa' AS BINARY)", result.stdout)
        self.assertIn("START TRANSACTION", result.stdout)
        self.assertIn("FOR UPDATE", result.stdout)
        self.assertIn("COMMIT", result.stdout)

    def test_stale_manifest_refuses_application_without_emitting_sql(self):
        spec = importlib.util.spec_from_file_location("epic_zone_seed", SCRIPT)
        assert spec is not None and spec.loader is not None
        mod = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(mod)
        with tempfile.TemporaryDirectory() as directory:
            stale = Path(directory) / "seed.json"
            stale.write_text("{}\n")
            output, errors = io.StringIO(), io.StringIO()
            with mock.patch.object(mod, "MANIFEST", stale), \
                    mock.patch.object(sys, "argv", [str(SCRIPT), "sql-apply", "--database", "seed_qa", "--server-stopped"]), \
                    redirect_stdout(output), redirect_stderr(errors):
                self.assertEqual(mod.main(), 1)
            self.assertEqual(output.getvalue(), "")
            self.assertIn("manifest differs", errors.getvalue())

    def test_audit_is_read_only(self):
        result = self.cli("sql-audit")
        self.assertEqual(result.returncode, 0, result.stderr)
        for word in ["UPDATE ", "INSERT ", "DELETE ", "CREATE ", "DROP "]:
            self.assertNotIn(word, result.stdout)
        self.assertIn("SELECT 4200,", result.stdout)
        self.assertIn("outside-active-source-no-seed", result.stdout)

    def test_parser_rejects_duplicate_payout_and_unrecognized_sql(self):
        spec = importlib.util.spec_from_file_location("epic_zone_seed", SCRIPT)
        assert spec is not None and spec.loader is not None
        mod = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(mod)
        valid = "UPDATE zones SET suggested_group_size = 100 WHERE epic_type != '0';\n"
        valid += "UPDATE zones SET epic_payout = 80 WHERE number = 14; --kobold\n"
        self.assertEqual(mod.parse_payouts(valid), {14: 80})
        with self.assertRaises(ValueError):
            mod.parse_payouts(valid + "UPDATE zones SET epic_payout = 90 WHERE number = 14;\n")
        with self.assertRaises(ValueError):
            mod.parse_payouts(valid + "UPDATE zones SET epic_payout = 90 WHERE number IN (24);\n")
        with self.assertRaises(ValueError):
            mod.parse_payouts(valid.replace("= 100 WHERE", "= 50 WHERE"))

    def test_zone_parser_rejects_ambiguous_headers_and_counts_loads(self):
        spec = importlib.util.spec_from_file_location("epic_zone_seed", SCRIPT)
        assert spec is not None and spec.loader is not None
        mod = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(mod)
        text = "#14\nKobold~\n1546 2 0 15 25 1\nO 0 358 1 1400 100 0 0 0\nS\n"
        stones = mod.parse_stones((ROOT / "src/world/epic.h").read_text())
        self.assertEqual(stones, {358: 1, 359: 2, 360: 3})
        rows = mod.parse_zones(text, "fixture.zon", stones)
        self.assertEqual(rows[14]["stones"], [358])
        with self.assertRaises(ValueError):
            mod.parse_zones(text + text, "fixture.zon", stones)


if __name__ == "__main__":
    unittest.main()
