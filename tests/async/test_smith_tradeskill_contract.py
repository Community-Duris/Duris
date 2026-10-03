#!/usr/bin/env python3
"""Contract and behavior tests for atomic compound tradeskill smith recipe."""

import json
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[2]


class SmithTradeskillContract(unittest.TestCase):
    def test_smith_source_implements_atomic_compound_transaction(self):
        source = (ROOT / "src/economy/tradeskill.c").read_text(encoding="utf-8")
        smith_idx = source.find("int smith(P_char ch, P_char pl, int cmd, char *arg)")
        self.assertGreater(smith_idx, 0, "smith function should be present")

        smith_body = source[smith_idx:smith_idx + 4500]

        # Verify money debit precedes ore extraction and forge creation
        sub_money = smith_body.find("SUB_MONEY(pl, price, 0)")
        forge_create = smith_body.find("forge_create(choice")
        grant_item = smith_body.find("grant_tradeskill_item(pl, tobj)")
        extract_ore = smith_body.find("extract_obj(needed_ore[j], TRUE)")

        self.assertGreater(sub_money, 0)
        self.assertGreater(forge_create, sub_money, "forge_create must follow money payment")
        self.assertGreater(grant_item, forge_create, "grant_tradeskill_item must follow creation")
        self.assertGreater(extract_ore, grant_item, "ore extraction must only happen after successful grant")

        # Verify rollback refund if creation or grant fails
        self.assertIn("ADD_MONEY(pl, price);", smith_body, "must refund money on creation/grant failure")

    def test_writers_inventory_routes_smith_as_crafting_cost(self):
        writers_path = ROOT / "docs/persistence/economy_accounting/writers.json"
        writers = json.loads(writers_path.read_text(encoding="utf-8"))
        entry = next((w for w in writers["writers"] if w["id"] == "crafting.smith"), None)
        self.assertIsNotNone(entry)
        self.assertEqual(entry["reason"], "crafting_cost")
        self.assertEqual(entry["symbol"], "smith")


if __name__ == "__main__":
    unittest.main()
