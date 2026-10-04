#!/usr/bin/env python3
"""Control detail must coexist with sealed reports in the ingestion stream."""
from __future__ import annotations

from copy import deepcopy
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
from scripts.telemetry import control_contract as controls
from scripts.telemetry.rollup_engine import build_page_contributions, SemanticError, BoundsExceeded
from scripts.telemetry.rollup_definitions import RollupTarget
import test_telemetry_battle_contribution_contract as control_fixtures
from test_telemetry_observations import ownership


class ControlMixedStreamTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        control_fixtures.ControlContractTests.setUpClass()
        value = control_fixtures.ControlContractTests.value
        cls.raw = dict(value, ingest_id=2, schema_version=1,
            record_kind=13, boot_id=value["ctl_boot_id"],
            process_id=value["ctl_process_id"], record_seq=2,
            occurrence_utc_usec=value["ctl_decision_utc_usec"])
        controls.validate_raw_observation(cls.raw)

    @classmethod
    def tearDownClass(cls):
        control_fixtures.ControlContractTests.tearDownClass()

    def target(self, definition):
        return RollupTarget(definition, 99, self.raw["ctl_environment_id"], self.raw["ctl_season_id"])

    def test_all_sealed_definitions_preserve_values_and_advance_over_control(self):
        row = ownership(ingest_id=1, record_seq=1, environment_id=self.raw["ctl_environment_id"],
                        season_id=self.raw["ctl_season_id"])
        original = deepcopy(self.raw)
        for definition in (1, 2, 3, 5, 6):
            with self.subTest(definition=definition):
                before = build_page_contributions([row], self.target(definition))
                after = build_page_contributions([row, self.raw], self.target(definition))
                self.assertEqual((before.cursor, after.cursor), (1, 2))
                self.assertEqual(after.fetched_rows, 2)
                for field in ("sessions", "player_days", "cohorts", "members", "identity_inputs",
                    "battle_inputs", "state_quality_flags", "coverage_start_utc_usec", "coverage_end_utc_usec",
                    "output_fanout"):
                    self.assertEqual(getattr(after, field), getattr(before, field), field)
                self.assertEqual(after.observations.output_fanout, before.observations.output_fanout)
                self.assertGreater(after.estimated_bytes, before.estimated_bytes)
        self.assertEqual(self.raw, original)

    def test_malformed_control_and_unknown_future_kind_refuse_even_unselected_rows(self):
        for update in ({"ctl_definition_version": 2}, {"schema_version": 2}, {"record_kind": 14},
                       {"pid": 0}, {"ctl_configured_ticks": None}, {"ctl_process_id": 0}):
            for definition in (1, 2, 3, 5, 6):
                with self.subTest(update=update, definition=definition), self.assertRaises(SemanticError):
                    build_page_contributions([dict(self.raw, **update)], self.target(definition))

    def test_unselected_control_is_still_bounded_and_replay_checked(self):
        first = dict(self.raw, ingest_id=1)
        before = build_page_contributions([first], self.target(5))
        second = dict(self.raw, record_seq=3)
        with self.assertRaises(BoundsExceeded):
            build_page_contributions([first, second], self.target(5), max_page_bytes=before.estimated_bytes)
        with self.assertRaisesRegex(SemanticError, "duplicate replay key"):
            build_page_contributions([first, self.raw], self.target(5))

    def test_control_reports_require_a_separate_supported_definition(self):
        page = build_page_contributions([self.raw], self.target(7))
        self.assertEqual(page.cursor, 2)
        self.assertEqual(len(page.battle_inputs), 1)
        for definition in (5, 6):
            self.assertEqual(build_page_contributions([self.raw], self.target(definition)).battle_inputs, [])


if __name__ == "__main__":
    unittest.main(verbosity=2)
