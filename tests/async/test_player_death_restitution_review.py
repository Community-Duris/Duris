#!/usr/bin/env python3
"""Small regressions for independently reviewed restitution safety gaps."""
from pathlib import Path
import importlib.util
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location("player_death_restitution", ROOT / "scripts/player_death_restitution.py")
assert SPEC and SPEC.loader
cli = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(cli)


class ReviewRegressions(unittest.TestCase):
    def test_god_only_authority_is_not_an_ordinary_item(self):
        item = {"object_uid": 7001, "vnum": 104, "type": 9,
                "extra_flags": 0, "name_hex": "ordinary gloves".encode().hex()}
        authority = {"god": {"104": {"vnum": 104, "owned": "Y"}}}
        self.assertEqual(cli.item_kind(item, authority), "artifact")

    def test_custody_only_competitor_is_collected(self):
        db = mock.Mock()
        def rows(query):
            if "FROM player_death_custody" in query:
                return [["9004", "104", "1", "1", "99", "0", "10", "99", "8"]]
            return []
        db.run.side_effect = rows
        with mock.patch.object(cli, "table_exists", return_value=False):
            artifacts = cli.fetch_artifacts(db, [104])
        competitors = artifacts["competitors"].get("104", [])
        self.assertTrue(any(row["item_uid"] == 9004 for row in competitors))

    def test_no_process_visibility_is_not_quiescence(self):
        db = mock.Mock()
        db.scalar.return_value = "0"
        with self.assertRaisesRegex(cli.ToolError, "PROCESS"):
            cli.check_database_quiescence(db)


if __name__ == "__main__":
    unittest.main()
