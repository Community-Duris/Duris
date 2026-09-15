#!/usr/bin/env python3
"""Failing-first regressions for the independently reviewed #268 semantics."""
from __future__ import annotations

from datetime import date
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts" / "telemetry"))

from rollup_definitions import (  # noqa: E402
    ROLLUP_QUALITY_CONTEXT_UNAVAILABLE,
    ROLLUP_QUALITY_LATE_INPUT,
    ROLLUP_QUALITY_SESSION_GAP,
    ROLLUP_QUALITY_UTC_MISMATCH,
    RollupTarget,
    report_catalog,
)
from rollup_engine import (  # noqa: E402
    SemanticError,
    build_page_contributions,
    membership_contribution_from_row,
)


class ReviewSemanticsTest(unittest.TestCase):
    target = RollupTarget(1, 1, 8, 7)

    @staticmethod
    def interval(**updates):
        row = {
            "ingest_id": 1,
            "boot_id": 100,
            "process_id": 200,
            "record_seq": 1,
            "schema_version": 1,
            "record_kind": 1,
            "occurrence_utc_usec": 100,
            "ingested_utc_usec": 100,
            "environment_id": 8,
            "season_id": 7,
            "session_boot_id": 300,
            "session_process_id": 400,
            "session_seq": 1,
            "subject_id": 9001,
            "pid": 42,
            "start_monotonic_usec": 0,
            "end_monotonic_usec": 100,
            "start_utc_usec": 0,
            "end_utc_usec": 100,
            "duration_usec": 100,
            "category": 2,
            "context": 2,
            "context_quality": 1,
            "level_band": 5,
            "class_id": 6,
            "race_id": 7,
            "faction_id": 8,
            "zone_vnum": 123,
            "group_size": 1,
            "config_id": 99,
            "classifier_version": 1,
            "policy_version": 1,
            "quality_flags": 0,
        }
        row.update(updates)
        return row

    @staticmethod
    def member_row(**updates):
        row = {
            "utc_day": date(1970, 1, 1),
            "level_band": 5,
            "class_id": 6,
            "race_id": 7,
            "faction_id": 8,
            "zone_vnum": 123,
            "config_id": 99,
            "category": 2,
            "membership_kind": 1,
            "subject_id": 9001,
            "session_boot_id": 0,
            "session_process_id": 0,
            "session_seq": 0,
            "duration_usec": 10,
            "attributable_usec": 10,
            "observed_intervals": 1,
            "quality_flags": 0,
            "input_watermark": 1,
        }
        row.update(updates)
        return row

    def test_partial_and_unavailable_context_keep_captured_dimensions(self):
        for quality in (2, 4):
            with self.subTest(context_quality=quality):
                row = self.interval(context_quality=quality, occurrence_utc_usec=100)
                contribution = build_page_contributions([row], self.target)
                cohort = next(iter(contribution.cohorts.values()))
                self.assertEqual(
                    (cohort.level_band, cohort.class_id, cohort.race_id, cohort.faction_id, cohort.zone_vnum),
                    (5, 6, 7, 8, 123),
                )
                self.assertEqual(cohort.attributable_usec, 0)
                self.assertTrue(cohort.quality_flags & ROLLUP_QUALITY_CONTEXT_UNAVAILABLE)
                member = next(iter(contribution.members.values()))
                self.assertEqual(member.level_band, 5)
                self.assertEqual(member.zone_vnum, 123)
                self.assertEqual(member.attributable_usec, 0)
                self.assertEqual(row["duration_usec"], 100)

    def test_derived_late_uses_header_occurrence_and_reaches_all_affected_rows(self):
        first = self.interval()
        second = self.interval(
            ingest_id=2,
            record_seq=2,
            session_seq=2,
            subject_id=9002,
            pid=43,
            occurrence_utc_usec=50,
            start_utc_usec=200,
            end_utc_usec=300,
            start_monotonic_usec=200,
            end_monotonic_usec=300,
        )
        contribution = build_page_contributions([first, second], self.target)
        self.assertTrue(contribution.state_quality_flags & ROLLUP_QUALITY_LATE_INPUT)
        sessions = {delta.session_seq: delta for delta in contribution.sessions.values()}
        self.assertFalse(sessions[1].quality_flags & ROLLUP_QUALITY_LATE_INPUT)
        self.assertTrue(sessions[2].quality_flags & ROLLUP_QUALITY_LATE_INPUT)
        self.assertTrue(
            any(
                delta.subject_id == 9002 and delta.quality_flags & ROLLUP_QUALITY_LATE_INPUT
                for delta in contribution.player_days.values()
            )
        )
        self.assertTrue(all(delta.quality_flags & ROLLUP_QUALITY_LATE_INPUT for delta in contribution.cohorts.values()))
        self.assertTrue(
            any(
                delta.subject_id == 9002 and delta.quality_flags & ROLLUP_QUALITY_LATE_INPUT
                for delta in contribution.members.values()
            )
        )

    def test_identical_concurrent_intervals_are_not_derived_late(self):
        first = self.interval()
        second = self.interval(
            ingest_id=2,
            record_seq=2,
            session_seq=2,
            subject_id=9002,
            pid=43,
        )
        contribution = build_page_contributions([first, second], self.target)
        self.assertFalse(contribution.state_quality_flags & ROLLUP_QUALITY_LATE_INPUT)
        self.assertTrue(all(not delta.quality_flags & ROLLUP_QUALITY_LATE_INPUT for delta in contribution.sessions.values()))
        self.assertTrue(all(not delta.quality_flags & ROLLUP_QUALITY_LATE_INPUT for delta in contribution.player_days.values()))
        self.assertTrue(all(not delta.quality_flags & ROLLUP_QUALITY_LATE_INPUT for delta in contribution.cohorts.values()))
        self.assertTrue(all(not delta.quality_flags & ROLLUP_QUALITY_LATE_INPUT for delta in contribution.members.values()))

    def test_raw_late_bit_is_promoted_to_rollup_late_on_all_affected_rows(self):
        row = self.interval(quality_flags=1 << 8)
        contribution = build_page_contributions([row], self.target)
        self.assertTrue(contribution.state_quality_flags & ROLLUP_QUALITY_LATE_INPUT)
        self.assertTrue(all(delta.quality_flags & ROLLUP_QUALITY_LATE_INPUT for delta in contribution.sessions.values()))
        self.assertTrue(all(delta.quality_flags & ROLLUP_QUALITY_LATE_INPUT for delta in contribution.player_days.values()))
        self.assertTrue(all(delta.quality_flags & ROLLUP_QUALITY_LATE_INPUT for delta in contribution.cohorts.values()))
        self.assertTrue(all(delta.quality_flags & ROLLUP_QUALITY_LATE_INPUT for delta in contribution.members.values()))

    def test_raw_late_bit_is_promoted_for_control_and_gap_rows(self):
        cases = (
            (
                "lifecycle",
                self.interval(record_kind=2, lifecycle=1, end_reason=0, quality_flags=1 << 8),
                True,
            ),
            (
                "checkpoint",
                self.interval(
                    record_kind=3,
                    checkpoint_revision=1,
                    connected_usec=100,
                    active_usec=100,
                    idle_usec=0,
                    unknown_usec=0,
                    resident_usec=100,
                    linkdead_usec=0,
                    quality_flags=1 << 8,
                ),
                True,
            ),
            ("session_gap", self.interval(record_kind=4, gap_reason=1, quality_flags=1 << 8), True),
            (
                "process_gap",
                self.interval(
                    record_kind=4,
                    gap_reason=1,
                    session_boot_id=0,
                    session_process_id=0,
                    session_seq=0,
                    quality_flags=1 << 8,
                ),
                False,
            ),
        )
        for label, row, has_session in cases:
            with self.subTest(kind=label):
                contribution = build_page_contributions([row], self.target)
                self.assertTrue(contribution.state_quality_flags & ROLLUP_QUALITY_LATE_INPUT)
                self.assertEqual(bool(contribution.sessions), has_session)
                self.assertTrue(
                    all(
                        delta.quality_flags & ROLLUP_QUALITY_LATE_INPUT
                        for delta in contribution.sessions.values()
                    )
                )

    def test_invalid_utc_row_does_not_advance_derived_late_boundary(self):
        rows = [
            self.interval(
                ingest_id=1,
                record_seq=1,
                session_seq=1,
                occurrence_utc_usec=100,
                start_utc_usec=0,
                end_utc_usec=100,
                start_monotonic_usec=0,
                end_monotonic_usec=100,
            ),
            self.interval(
                ingest_id=2,
                record_seq=2,
                session_seq=2,
                occurrence_utc_usec=900,
                start_utc_usec=200,
                end_utc_usec=1000,
                start_monotonic_usec=200,
                end_monotonic_usec=300,
                duration_usec=100,
            ),
            self.interval(
                ingest_id=3,
                record_seq=3,
                session_seq=3,
                occurrence_utc_usec=550,
                start_utc_usec=500,
                end_utc_usec=600,
                start_monotonic_usec=400,
                end_monotonic_usec=500,
            ),
        ]
        contribution = build_page_contributions(rows, self.target)
        sessions = {key[2]: delta for key, delta in contribution.sessions.items()}
        self.assertTrue(contribution.state_quality_flags & ROLLUP_QUALITY_UTC_MISMATCH)
        self.assertFalse(sessions[3].quality_flags & ROLLUP_QUALITY_LATE_INPUT)
        self.assertEqual(contribution.coverage_end_utc_usec, 600)

    def test_session_gap_sets_quality_without_fabricating_duration(self):
        row = self.interval(
            record_kind=4,
            occurrence_utc_usec=150,
            quality_flags=0,
            gap_reason=1,
        )
        contribution = build_page_contributions([row], self.target)
        session = next(iter(contribution.sessions.values()))
        self.assertTrue(session.quality_flags & ROLLUP_QUALITY_SESSION_GAP)
        self.assertTrue(contribution.state_quality_flags & ROLLUP_QUALITY_SESSION_GAP)
        self.assertEqual(session.observed_intervals, 0)
        self.assertEqual(sum(session.covered.values()), 0)
        self.assertEqual(contribution.player_days, {})
        self.assertEqual(contribution.cohorts, {})
        self.assertEqual(contribution.members, {})

    def test_membership_reader_rejects_invalid_kind_and_identity_shapes(self):
        invalid = (
            {"membership_kind": 3},
            {"membership_kind": 1, "session_boot_id": 1},
            {"membership_kind": 1, "session_process_id": 1},
            {"membership_kind": 1, "session_seq": 1},
            {"membership_kind": 2, "session_boot_id": 0},
            {"membership_kind": 2, "session_process_id": 0},
            {"membership_kind": 2, "session_seq": 0},
            {"subject_id": 0},
        )
        for changes in invalid:
            with self.subTest(changes=changes):
                with self.assertRaises(SemanticError):
                    membership_contribution_from_row(self.target, self.member_row(**changes))
        for kind, ids in ((1, (0, 0, 0)), (2, (11, 12, 13))):
            result = membership_contribution_from_row(
                self.target,
                self.member_row(membership_kind=kind, session_boot_id=ids[0], session_process_id=ids[1], session_seq=ids[2]),
            )
            self.assertEqual(result.membership_kind, kind)
            self.assertEqual((result.session_boot_id, result.session_process_id, result.session_seq), ids)

    def test_catalog_exposes_machine_readable_rate_metadata(self):
        catalog = report_catalog()
        self.assertEqual({item["name"] for item in catalog}, {"session_playtime", "cohort_activity"})
        for item in catalog:
            with self.subTest(name=item["name"]):
                self.assertTrue(item["rate_numerator_metrics"])
                self.assertTrue(item["rate_denominator_metrics"])
                self.assertEqual(item["rate_unit"], "fraction")
                self.assertEqual(item["zero_denominator_policy"], "null")
                self.assertEqual(item["missing_input_policy"], "null")
                self.assertIsInstance(item["denominator"], str)
                self.assertEqual(item["rate"]["zero_denominator_policy"], "null")

    def test_sql_null_interval_dimensions_fail_closed(self):
        row = self.interval(level_band=None, context_quality=2)
        with self.assertRaises(SemanticError):
            build_page_contributions([row], self.target)


if __name__ == "__main__":
    unittest.main(verbosity=2)
