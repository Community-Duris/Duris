#!/usr/bin/env python3
"""Keep hidden-pet relinking visible without claiming accounting qualification."""
from collections import Counter
import json
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
import validate_economy_accounting as validator


class HiddenPetEquipmentCensusContract(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        folder = ROOT / "docs/persistence/economy_accounting"
        cls.inventory = json.loads((folder / "writers.json").read_text())
        cls.matrix = json.loads((folder / "writer_coverage_matrix.json").read_text())
        cls.current = validator.scan_sources(ROOT)
        cls.owners = {}
        for writer in cls.inventory["writers"]:
            for site in writer.get("sites", []):
                cls.owners.setdefault(tuple(site), set()).add(writer["id"])

    def test_source_census_matches_and_all_current_sites_remain_mapped(self):
        signature = lambda rows: Counter((r["path"], r["family"], r["excerpt"]) for r in rows)
        self.assertEqual(signature(self.inventory["census"]), signature(self.current))
        sites = {(r["path"], r["line"], r["family"]) for r in self.current}
        self.assertEqual(set(self.owners), sites)
        self.assertEqual(self.matrix["lexical_census"]["unmapped_current_unique_sites"], 0)

    def test_pet_normalizer_is_explicit_unqualified_same_pet_projection(self):
        route_id = "item.pet_hidden_equipment_relink"
        writers = {r["id"]: r for r in self.inventory["writers"]}
        self.assertIn(route_id, set(writers))
        writer = writers[route_id]
        self.assertEqual(writer["symbol"], "item_restrict_player_pet_equipment")
        self.assertEqual(len(writer["sites"]), 2)
        for site in writer["sites"]:
            self.assertEqual(self.owners[tuple(site)], {route_id})
        self.assertEqual(writer["evidence"], [])
        self.assertTrue(all(b["status"] == "unverified" and not b["evidence"]
                            for b in writer["backends"].values()))
        route = next(r for r in self.matrix["routes"] if r["id"] == route_id)
        self.assertEqual(route["disposition"], "runtime_projection_route")
        self.assertFalse(route["double_entry_evidence"]["unified_operation_postings_observed"])
        self.assertFalse(self.matrix["coverage_complete"])
        self.assertEqual(self.matrix["playable_release_status"], "BLOCKED")

    def test_hidden_equipment_rejection_stays_on_existing_equip_boundary(self):
        routes = [r for r in self.inventory["writers"]
                  if r["path"] == "src/world/handler.c" and r["symbol"] == "equip_char"]
        self.assertEqual(len(routes), 1)
        candidates = [r for r in self.current if r["path"] == "src/world/handler.c"
                      and r["family"] == "item_publication"
                      and r["excerpt"] == "obj_to_char(obj, ch);"]
        self.assertTrue(candidates)
        source = (ROOT / "src/world/handler.c").read_text().splitlines()
        guarded = [r for r in candidates if any("item_restricted_for_player_pet(ch, obj)" in line
                   for line in source[max(0, r["line"] - 5):r["line"]])]
        self.assertEqual(len(guarded), 1)
        hit = guarded[0]
        self.assertEqual(self.owners[(hit["path"], hit["line"], hit["family"])], {routes[0]["id"]})


if __name__ == "__main__":
    unittest.main()
