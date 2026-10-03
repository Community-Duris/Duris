#!/usr/bin/env python3
"""Contract and behavior test for retired money changer procedure."""

import json
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[2]


class CurrencyExchangeRetiredContract(unittest.TestCase):
    def test_writers_inventory_marks_feature_unsupported_and_refused(self):
        writers_path = ROOT / "docs/persistence/economy_accounting/writers.json"
        writers = json.loads(writers_path.read_text(encoding="utf-8"))
        entry = next((w for w in writers["writers"] if w["id"] == "special.money_changer"), None)
        self.assertIsNotNone(entry)
        self.assertEqual(entry["coverage"], "unsupported")
        for backend, info in entry["backends"].items():
            self.assertEqual(info["status"], "refused", f"Backend {backend} should be refused")

    def test_source_contains_no_raw_cash_mutations(self):
        source_path = ROOT / "src/economy/currency_exchange_proc.c"
        source = source_path.read_text(encoding="utf-8")
        # Ensure direct cash assignments are completely removed
        self.assertNotIn("ch->points.cash[from] -=", source)
        self.assertNotIn("ch->points.cash[to] +=", source)
        self.assertNotIn("RATE_TO_LOWER", source)
        self.assertNotIn("RATE_TO_PLATINUM", source)
        # Ensure polite redirection to royal bank tellers is in place
        self.assertIn("The Royal Bank now handles all coin exchanges with zero surcharge", source)

    def test_census_has_no_sites_in_currency_exchange(self):
        writers_path = ROOT / "docs/persistence/economy_accounting/writers.json"
        writers = json.loads(writers_path.read_text(encoding="utf-8"))
        exchange_sites = [s for s in writers["census"] if s["path"] == "src/economy/currency_exchange_proc.c"]
        self.assertEqual(len(exchange_sites), 0)


if __name__ == "__main__":
    unittest.main()
