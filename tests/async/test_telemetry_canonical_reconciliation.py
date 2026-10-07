#!/usr/bin/env python3
"""Bounded discovery/progress contracts; fixtures are not native commit proof."""
from pathlib import Path
from dataclasses import replace
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
from scripts.telemetry import canonical_reward_contract as contract
from scripts.telemetry.canonical_reward_source import CanonicalRewardSnapshot, replay_retained_bank_cut
from scripts.telemetry.canonical_reward_reconciliation import SweepPosition, AttemptStatus, make_step
from test_telemetry_reward_projection import bank_evidence, operation, _SnapshotConnection, _FakeFactory


def page(numbers, *, after=contract.ZERO_ID, through=None, page_size=1, mutate=False):
    connection = _SnapshotConnection([bank_evidence(operation_number=number) for number in numbers])
    if mutate:
        connection.tables["economic_accounting_coin_posting"][0]["copper_value"] += 1
    return CanonicalRewardSnapshot(_FakeFactory(connection), page_size=page_size).capture_bank_partition(
        0, after, through=through, preserve_refusals=True)


class CanonicalReconciliationTest(unittest.TestCase):
    def test_frozen_upper_bounds_a_pass_while_new_higher_keys_arrive(self):
        first = page([111, 112])
        position = SweepPosition(operation(55), 0, 0)
        step = make_step(position, page=first)
        self.assertEqual(step.cursor_after, operation(111))
        self.assertEqual(step.upper_after, operation(112))
        self.assertEqual(step.completed_passes, 0)
        self.assertEqual(step.next_bucket, 1)
        resumed = SweepPosition(operation(55), 256, 0, step.cursor_after, step.upper_after)
        last = page([110, 111, 112, 113], after=resumed.cursor, through=resumed.pass_upper)
        self.assertEqual(last.cut.selected_operations, (operation(112),))
        self.assertTrue(last.wrapped)
        completed = make_step(resumed, page=last)
        self.assertEqual(completed.cursor_after, contract.ZERO_ID)
        self.assertIsNone(completed.upper_after)
        self.assertEqual(completed.completed_passes, 1)
        revisited = page([110, 111, 112, 113])
        self.assertEqual(revisited.cut.selected_operations, (operation(110),))
        self.assertTrue(revisited.future_commits_provisional)

    def test_discovery_request_is_bound_to_the_exact_retained_cut_digest(self):
        captured = page([111, 112]).cut
        restored = replay_retained_bank_cut(captured.captured_utc_usec, captured.selected_operations,
            captured.sources, captured.source_digest, discovery=captured.discovery)
        self.assertEqual(restored.discovery, captured.discovery)
        self.assertEqual(restored.events, captured.events)
        for changed in (None, replace(captured.discovery, page_limit=2),
                        replace(captured.discovery, through=operation(112))):
            with self.subTest(discovery=changed), self.assertRaises(contract.EvidenceError):
                replay_retained_bank_cut(captured.captured_utc_usec, captured.selected_operations,
                    captured.sources, captured.source_digest, discovery=changed)

    def test_failure_rotates_the_fair_slot_without_advancing_source_cursor(self):
        prefix = 255 << 120
        position = SweepPosition(operation(55), 73, 255, operation(prefix + 111), operation(prefix + 112), 4, 2)
        for status in tuple(AttemptStatus)[2:]:
            step = make_step(position, failure=status, attempted_utc_usec=1_000_000)
            self.assertEqual(step.cursor_after, position.cursor)
            self.assertEqual(step.upper_after, position.pass_upper)
            self.assertEqual(step.completed_passes, 4)
            self.assertEqual(step.failures, 3)
            self.assertEqual(step.next_bucket, 0)
            self.assertIsNone(step.cut_id)
            payload = contract.decode_source_payload(step.payload)
            self.assertEqual(payload["future_commits_provisional"], 1)
            self.assertIsNone(payload["selected_count"])

    def test_quarantined_page_records_unknowns_and_never_acquires_an_amount(self):
        captured = page([111], mutate=True)
        step = make_step(SweepPosition(operation(55), 0, 0), page=captured)
        self.assertEqual(step.status, AttemptStatus.QUARANTINED)
        self.assertEqual(contract.decode_source_payload(step.payload)["unknown_count"], 1)
        self.assertEqual(contract.earned_totals(captured.cut.events), {})

    def test_forged_cursor_limit_upper_or_scope_is_refused(self):
        original = page([111, 112])
        for changed in (replace(original, cursor_after=operation(112)), replace(original, page_limit=2),
                        replace(original, pass_upper=operation(113)), replace(original, bucket=1),
                        replace(original, wrapped=True)):
            with self.subTest(page=changed.bucket), self.assertRaises(contract.EvidenceError):
                make_step(SweepPosition(operation(55), 0, 0), page=changed)

    def test_a_resumed_pass_cannot_change_its_frozen_upper(self):
        position = SweepPosition(operation(55), 256, 0, operation(111), operation(112))
        changed = page([111, 112, 113], after=position.cursor)
        with self.assertRaises(contract.EvidenceError):
            make_step(position, page=changed)

    def test_counter_overflow_and_uncaptured_success_are_refused(self):
        with self.assertRaises(contract.EvidenceError):
            make_step(SweepPosition(operation(55), (1 << 64) - 1, 0),
                      failure=AttemptStatus.SOURCE_UNAVAILABLE, attempted_utc_usec=1)
        with self.assertRaises(contract.EvidenceError):
            make_step(SweepPosition(operation(55), 0, 0), failure=AttemptStatus.CAPTURED, attempted_utc_usec=1)


if __name__ == "__main__":
    unittest.main()
