#!/usr/bin/env python3
"""Contract and validation tests for player death restitution double-entry item custody accounting."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


class PlayerDeathRestitutionAccountingContract(unittest.TestCase):
    def test_repository_wiring_contract(self):
        sql_source = (ROOT / "src/persistence/player_death_restitution_repository.c").read_text(encoding="utf-8")
        self.assertNotIn("economic_accounting_item_reference_insert(", sql_source)
        self.assertIn("INSERT INTO item_ownership_ledger", sql_source)
        self.assertIn("critical_command_legacy_execution_supported(command)", sql_source)


if __name__ == "__main__":
    unittest.main()
