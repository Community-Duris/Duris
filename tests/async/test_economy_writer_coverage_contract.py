#!/usr/bin/env python3
"""Negative activation contract for a real, currently unguarded economy writer."""
from __future__ import annotations

import json
from pathlib import Path
import re
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
import generate_economy_writer_coverage as coverage  # noqa: E402

MATRIX = ROOT / "docs/persistence/economy_accounting/writer_coverage_matrix.json"


class SplitEconomyActivationContract(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.artifact = json.loads(MATRIX.read_text(encoding="utf-8"))
        cls.routes = {route["id"]: route for route in cls.artifact["routes"]}

    def test_machine_counts_reconcile_to_route_rows(self) -> None:
        routes = self.artifact["routes"]
        counts = self.artifact["counts"]
        self.assertEqual(len(routes), counts["matrix_rows"])
        self.assertEqual(len({route["id"] for route in routes}), len(routes))
        self.assertEqual(sum(route["current_critical_command_schema"]["current_schema"] == 1 for route in routes),
                         counts["routes_with_current_schema_1_path"])
        self.assertEqual(sum(route["current_critical_command_schema"]["schema_2_gameplay_producer_connected"] for route in routes),
                         counts["routes_with_schema_2_gameplay_producer"])
        self.assertEqual(sum(route["double_entry_evidence"]["unified_operation_postings_observed"] for route in routes),
                         counts["routes_with_unified_double_entry_evidence"])
        self.assertEqual(sum(route["counts_as_runtime_writer"] for route in routes),
                         counts["runtime_mutation_and_projection_routes"])
        self.assertEqual(sum(route["counts_as_operational_writer"] for route in routes),
                         counts["offline_operational_writer_routes"])
        self.assertEqual(sum(route["disposition"] == "dormant_writer_candidate" for route in routes),
                         counts["dormant_writer_candidates"])
        self.assertEqual(sum(route["disposition"] == "non_writer_candidate" for route in routes),
                         counts["non_writer_or_out_of_scope_candidates"])

    def test_do_split_is_not_permitted_after_activation(self) -> None:
        route = self.routes["currency.split"]
        self.assertEqual(route["source"]["file"], "src/cmd/actoth.c")
        self.assertEqual(route["source"]["function"], "do_split")
        self.assertEqual(route["source"]["definition_lines"], [6113])
        self.assertEqual(route["current_critical_command_schema"]["current_schema"], 1)
        self.assertEqual(route["current_critical_command_schema"]["route_mode"],
                         "multiple_schema_1_currency_legs_not_atomic_root")
        self.assertTrue(route["blocking_policy_after_activation"]["must_block_on_activation"])
        self.assertEqual(route["blocking_policy_after_activation"]["decision"],
                         "block_until_typed_schema2_accounting")

        raw = (ROOT / route["source"]["file"]).read_text(encoding="utf-8")
        code = coverage.mask_cpp(raw)
        declaration = re.search(r"\bvoid\s+do_split\s*\(", code)
        self.assertIsNotNone(declaration)
        assert declaration is not None
        body_start = code.find("{", declaration.end())
        self.assertGreaterEqual(body_start, 0)
        depth = 0
        body_end = -1
        for index in range(body_start, len(code)):
            if code[index] == "{":
                depth += 1
            elif code[index] == "}":
                depth -= 1
                if depth == 0:
                    body_end = index
                    break
        self.assertGreater(body_end, body_start)
        body = code[body_start:body_end + 1]
        credit = body.find("ADD_MONEY(gl->ch")
        debit = body.find("SUB_MONEY(ch")
        self.assertGreaterEqual(credit, 0, "expected current recipient wallet mutation leg")
        self.assertGreater(debit, credit, "sender debit currently follows recipient credits")
        self.assertNotRegex(body, r"(?:economic_accounting|critical_command_accounting_schema|schema_2)")
        self.assertIn("no atomic root command", route["current_critical_command_schema"]["interpretation"])

    def test_unmatched_lexical_sites_keep_matrix_incomplete(self) -> None:
        self.assertFalse(self.artifact["coverage_complete"])
        self.assertGreater(self.artifact["lexical_census"]["unmapped_current_unique_sites"], 0)
        self.assertEqual(self.artifact["playable_release_status"], "BLOCKED")


if __name__ == "__main__":
    unittest.main(verbosity=2)
