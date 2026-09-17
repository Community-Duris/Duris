#!/usr/bin/env python3
"""Offline contracts for the #273 rested-bonus study evaluator."""
from __future__ import annotations

from copy import deepcopy
import json
from pathlib import Path
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts" / "telemetry" / "studies"))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from rested_bonus import StudyInputError, build_report, main  # noqa: E402
from telemetry_study_rested_fixtures import (  # noqa: E402
    copy_export,
    empty_export,
    qualified_export,
    small_export,
)


class RestedBonusStudyTest(unittest.TestCase):
    def test_qualified_fixture_reports_cohorts_generations_coverage_and_uncertainty(self):
        report = build_report(qualified_export())

        self.assertEqual(report["status"], "fixture_qualified")
        self.assertFalse(report["qualified_for_real_observation"])
        self.assertEqual(report["scope"]["config_generations"], ["cfg-v1", "cfg-v2"])
        self.assertEqual(report["coverage"]["config_generations_observed"], ["cfg-v1", "cfg-v2"])
        self.assertEqual(report["coverage"]["coverage_days"], 14.0)
        self.assertEqual({item["cohort"] for item in report["cohorts"]}, {"solo", "group"})
        self.assertEqual(len(report["cohorts"]), 8)
        self.assertTrue(report["sample_sufficiency"]["qualified"])
        self.assertTrue(all(cell["status"] == "qualified" for cell in report["results"]["cells"]))
        self.assertTrue(
            all(
                window["status"] == "qualified"
                for cell in report["results"]["cells"]
                for window in cell["return_windows"]
            )
        )
        self.assertTrue(
            all(
                cell["repeat_group_sensitivity"]["available"]
                for cell in report["results"]["cells"]
            )
        )
        self.assertTrue(report["uncertainty"]["association_not_causal"])
        self.assertFalse(report["uncertainty"]["causal_effect_proven"])
        self.assertFalse(report["future_change_protocol"]["automatic_balance_mutation"])
        self.assertFalse(report["future_change_protocol"]["production_activation"])

    def test_real_export_is_the_only_qualified_status(self):
        report = build_report(qualified_export(synthetic_fixture=False))

        self.assertEqual(report["status"], "qualified")
        self.assertTrue(report["qualified_for_real_observation"])
        self.assertFalse(report["provenance"]["synthetic_fixture"])

    def test_empty_and_small_samples_abstain(self):
        empty = build_report(empty_export())
        small = build_report(small_export())

        self.assertEqual(empty["status"], "abstain")
        self.assertIn("empty_sample", empty["abstention_reasons"])
        self.assertEqual(small["status"], "abstain")
        self.assertIn("small_cell_sample", small["abstention_reasons"])
        self.assertIn("small_active_exposure", small["abstention_reasons"])
        self.assertFalse(small["sample_sufficiency"]["qualified"])

    def test_unpublished_incomplete_and_quality_coverage_abstains(self):
        payload = copy_export()
        payload["scope"]["coverage"]["published"] = False
        payload["scope"]["coverage"]["complete"] = False
        payload["scope"]["coverage"]["quality_flags"] = ["late_input"]
        report = build_report(payload)

        self.assertEqual(report["status"], "abstain")
        self.assertIn("source_generation_not_published", report["abstention_reasons"])
        self.assertIn("coverage_incomplete", report["abstention_reasons"])
        self.assertIn("coverage_quality_flags_present", report["abstention_reasons"])

    def test_unqualified_or_contaminated_rows_are_excluded_without_raw_identity(self):
        payload = copy_export()
        payload["rows"][0]["mode_qualified"] = False
        payload["rows"][1]["quality_flags"] = ["coverage_gap"]
        report = build_report(payload)
        serialized = json.dumps(report, sort_keys=True)

        self.assertEqual(report["status"], "abstain")
        self.assertIn("invalid_or_unqualified_rows", report["abstention_reasons"])
        reasons = [reason for row in report["results"]["excluded_rows"] for reason in row["reasons"]]
        self.assertIn("mode_not_qualified", reasons)
        self.assertIn("row_quality_flags_present", reasons)
        self.assertNotIn("subject-00-00", serialized)
        self.assertNotIn("repeat-00", serialized)

    def test_forbidden_raw_identity_is_rejected(self):
        payload = copy_export()
        payload["rows"][0]["account_id"] = 123

        with self.assertRaisesRegex(StudyInputError, "forbidden raw field"):
            build_report(payload)

    def test_censoring_abstains_only_the_small_return_window(self):
        payload = copy_export()
        payload["rows"][0]["censored"] = True
        payload["rows"][0]["returned"] = False
        report = build_report(payload)
        first_cell = next(
            cell
            for cell in report["results"]["cells"]
            if cell["cohort"] == "solo"
            and cell["rested_tier"] == "unrested"
            and cell["config_generation"] == "cfg-v1"
        )
        under_one_hour = next(
            item for item in first_cell["return_windows"] if item["name"] == "under_1h"
        )

        self.assertEqual(report["status"], "fixture_qualified")
        self.assertEqual(under_one_hour["status"], "abstain")
        self.assertEqual(under_one_hour["observation_count"], 3)
        self.assertEqual(under_one_hour["censored_count"], 1)

    def test_row_order_does_not_change_report_or_repeat_group_sensitivity(self):
        first = build_report(copy_export())
        payload = copy_export()
        payload["rows"].reverse()
        second = build_report(payload)

        self.assertEqual(first, second)
        sensitivity = first["results"]["cells"][0]["repeat_group_sensitivity"]
        self.assertEqual(sensitivity["repeat_group_count"], 10)
        self.assertEqual(sensitivity["max_subjects_per_repeat_group"], 2)

    def test_cli_writes_the_same_report_as_the_library(self):
        payload = qualified_export()
        expected = build_report(payload)
        with tempfile.TemporaryDirectory() as directory:
            input_path = Path(directory) / "export.json"
            output_path = Path(directory) / "report.json"
            input_path.write_text(json.dumps(payload), encoding="utf-8")

            self.assertEqual(main(["--input", str(input_path), "--output", str(output_path)]), 0)
            actual = json.loads(output_path.read_text(encoding="utf-8"))
            self.assertEqual(actual, expected)

            second_path = Path(directory) / "report-second.json"
            self.assertEqual(main(["--input", str(input_path), "--output", str(second_path)]), 0)
            self.assertEqual(output_path.read_bytes(), second_path.read_bytes())


if __name__ == "__main__":
    unittest.main()
