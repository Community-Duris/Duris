#!/usr/bin/env python3
"""Offline contracts for the #274 deterministic shadow recommender."""
from __future__ import annotations

import json
from pathlib import Path
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts" / "telemetry" / "balance"))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from recommend import RecommendationInputError, build_recommendation, main  # noqa: E402
from telemetry_balance_shadow_fixtures import copy_export, shadow_export  # noqa: E402


class BalanceShadowRecommendationTest(unittest.TestCase):
    def test_identical_input_is_reproducible_and_bounded(self):
        first = build_recommendation(shadow_export())
        second = build_recommendation(shadow_export())

        self.assertEqual(first, second)
        self.assertEqual(first["status"], "recommend")
        self.assertEqual(first["target"]["parameter"], "payout.epic.zone.alignmentMod")
        self.assertEqual(first["target"]["current_value_milli"], 200)
        self.assertEqual(first["target"]["proposed_value_milli"], 250)
        self.assertEqual(first["input_reference"]["generation"], 42)
        self.assertEqual(first["input_reference"]["config_generation"], "cfg-v1")
        self.assertEqual(first["evidence"]["sample_count_raw"], 100)
        self.assertEqual(first["evidence"]["sample_count_capped"], 80)
        self.assertEqual(first["evidence"]["distinct_subject_count"], 80)
        self.assertEqual(first["evidence"]["distinct_repeat_group_count"], 20)
        self.assertTrue(first["evidence"]["repeat_group_cap_applied"])
        self.assertTrue(first["constraints"]["shadow_only"])
        self.assertFalse(first["constraints"]["can_apply"])
        self.assertFalse(first["constraints"]["gameplay_mutation"])
        self.assertFalse(first["constraints"]["automatic_balance_mutation"])

    def test_row_order_and_report_replay_do_not_change_recommendation(self):
        first = build_recommendation(shadow_export())
        payload = copy_export()
        payload["rows"].reverse()
        second = build_recommendation(payload)

        self.assertEqual(first, second)
        self.assertEqual(first["recommendation_id"], second["recommendation_id"])
        self.assertEqual(first["evidence"]["fingerprint"], second["evidence"]["fingerprint"])

    def test_sparse_and_low_distinct_group_inputs_abstain(self):
        payload = copy_export()
        payload["rows"] = payload["rows"][:4]
        for row in payload["rows"]:
            row["repeat_group_token"] = "one-repeat-group"
        result = build_recommendation(payload)

        self.assertEqual(result["status"], "abstain")
        self.assertIsNone(result["target"]["proposed_value_milli"])
        self.assertIn("sparse_sample", result["abstention_reasons"])
        self.assertIn("insufficient_distinct_repeat_groups", result["abstention_reasons"])
        self.assertFalse(result["constraints"]["can_apply"])

    def test_stale_incomplete_expired_and_quality_flagged_inputs_abstain(self):
        payload = copy_export()
        payload["as_of_utc"] = "2026-03-01T00:00:00Z"
        payload["report"]["coverage"]["complete"] = False
        payload["report"]["coverage"]["quality_flags"] = ["source_gap"]
        result = build_recommendation(payload)

        self.assertEqual(result["status"], "abstain")
        for reason in (
            "report_expired",
            "report_stale",
            "coverage_incomplete",
            "coverage_quality_flags_present",
        ):
            self.assertIn(reason, result["abstention_reasons"])

    def test_mismatched_and_duplicate_rows_are_excluded_and_abstain(self):
        payload = copy_export()
        payload["rows"][0]["source_generation"] = 41
        payload["rows"][1]["event_token"] = payload["rows"][2]["event_token"]
        result = build_recommendation(payload)

        self.assertEqual(result["status"], "abstain")
        self.assertIn("invalid_or_manipulated_rows", result["abstention_reasons"])
        reasons = [reason for row in result["evidence"]["excluded_rows"] for reason in row["reasons"]]
        self.assertIn("source_generation_mismatch", reasons)
        self.assertIn("duplicate_event_token", reasons)

    def test_target_bounds_hold_at_maximum(self):
        payload = copy_export()
        payload["target"]["current_value_milli"] = 1000
        result = build_recommendation(payload)

        self.assertEqual(result["status"], "hold")
        self.assertEqual(result["target"]["proposed_value_milli"], 1000)
        self.assertIn("target_bound_reached", result["decision"]["reason_codes"])

    def test_target_value_must_reference_the_report_config_generation(self):
        payload = copy_export()
        payload["target"]["config_generation"] = "cfg-v2"
        result = build_recommendation(payload)

        self.assertEqual(result["status"], "abstain")
        self.assertIn("target_config_generation_mismatch", result["abstention_reasons"])

    def test_cooldown_and_hysteresis_prevent_churn(self):
        cooldown_payload = copy_export()
        cooldown_payload["history"]["last_recommendation"] = {
            "status": "shadow_only",
            "target_parameter": "payout.epic.zone.alignmentMod",
            "proposed_value_milli": 250,
            "issued_at_utc": "2026-01-25T00:00:00Z",
        }
        cooldown = build_recommendation(cooldown_payload)
        self.assertEqual(cooldown["status"], "cooldown_hold")
        self.assertIn("cooldown_active", cooldown["decision"]["reason_codes"])

        hysteresis_payload = copy_export()
        hysteresis_payload["history"]["last_recommendation"] = {
            "status": "shadow_only",
            "target_parameter": "payout.epic.zone.alignmentMod",
            "proposed_value_milli": 250,
            "issued_at_utc": "2025-12-01T00:00:00Z",
        }
        hysteresis = build_recommendation(hysteresis_payload)
        self.assertEqual(hysteresis["status"], "hysteresis_hold")
        self.assertIn("hysteresis_suppressed", hysteresis["decision"]["reason_codes"])

    def test_applied_feedback_is_a_fail_closed_loop(self):
        payload = copy_export()
        payload["history"]["applied_mutations"] = 1
        result = build_recommendation(payload)

        self.assertEqual(result["status"], "abstain")
        self.assertIn("feedback_loop_detected", result["abstention_reasons"])

    def test_raw_identity_is_rejected(self):
        payload = copy_export()
        payload["rows"][0]["account_id"] = 7

        with self.assertRaisesRegex(RecommendationInputError, "forbidden raw field"):
            build_recommendation(payload)

    def test_cli_writes_a_deterministic_json_record(self):
        payload = shadow_export()
        expected = build_recommendation(payload)
        with tempfile.TemporaryDirectory() as directory:
            input_path = Path(directory) / "shadow.json"
            output_path = Path(directory) / "recommendation.json"
            second_path = Path(directory) / "recommendation-second.json"
            input_path.write_text(json.dumps(payload), encoding="utf-8")

            self.assertEqual(main(["--input", str(input_path), "--output", str(output_path)]), 0)
            self.assertEqual(json.loads(output_path.read_text(encoding="utf-8")), expected)
            self.assertEqual(main(["--input", str(input_path), "--output", str(second_path)]), 0)
            self.assertEqual(output_path.read_bytes(), second_path.read_bytes())


if __name__ == "__main__":
    unittest.main()
