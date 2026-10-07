#!/usr/bin/env python3
"""Private prefix coverage semantics; actual SQL gap proof uses the native gate."""
from pathlib import Path
from unittest.mock import patch
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
from scripts.telemetry import canonical_reward_contract as contract
from scripts.telemetry.canonical_reward_reconciliation import (
    SweepPosition, AttemptStatus, make_step, reconcile_once, RetentionGap,
)
from scripts.telemetry.canonical_reward_health import (prefix_health, SweepHealth, replay_health,
    public_health_rows, decode_public_health, health_summary)
from scripts.telemetry.reward_projection import ProjectionBoundsExceeded
from test_telemetry_canonical_reconciliation import page
from test_telemetry_reward_projection import operation


def checkpoint():
    captured = page([111, 112])
    step = make_step(SweepPosition(operation(55), 0, 0), page=captured)
    return dict(scan_id=operation(55), bucket=0, cursor_operation=step.cursor_after,
        pass_upper=step.upper_after, completed_passes=step.completed_passes, failures=step.failures,
        last_revision=step.revision, last_cut=step.cut_id, status=int(step.status),
        attempted_utc_usec=step.attempt_utc_usec, payload=step.payload, payload_digest=step.payload_digest,
        definition_version=2, sealed=1, captured_utc_usec=captured.cut.captured_utc_usec, discovery_bucket=0,
        selected_count=len(captured.cut.selected_operations), source_count=len(captured.cut.sources),
        reserved_bytes=captured.cut.reserved_bytes, actual_selected=len(captured.cut.selected_operations),
        actual_sources=len(captured.cut.sources))


def health_payloads(*, gap=False, snapshot=2_000_000):
    state = dict(scan_id=operation(55), revision=1, next_bucket=1, definition_version=2,
                 provisional=1, read_utc_usec=snapshot)
    first = checkpoint()
    if gap:
        first["actual_sources"] -= 1
    rows = [first]
    for bucket in range(1, 256):
        rows.append(dict(scan_id=state["scan_id"], bucket=bucket, cursor_operation=contract.ZERO_ID,
            pass_upper=None, completed_passes=0, failures=0, last_revision=0, last_cut=None))
    return tuple(contract.encode_source_payload(row) for row in [state, *rows])


class CanonicalHealthTest(unittest.TestCase):
    def test_retained_evidence_replays_and_projects_gap_without_private_step_bytes(self):
        retained = replay_health(health_payloads(gap=True), snapshot=2_000_000)
        public = decode_public_health(public_health_rows(retained), snapshot=2_000_000)
        summary = health_summary(public)
        self.assertEqual(summary["sweep_visited_prefixes"], 1)
        self.assertEqual(summary["sweep_unobserved_prefixes"], 255)
        self.assertEqual(summary["sweep_retained_metadata_gaps"], 1)
        self.assertIsNone(public["prefixes"][0]["last_capture_utc_usec"])
        self.assertNotIn("payload", public["prefixes"][0])
        self.assertNotIn("last_cut", public["prefixes"][0])
        self.assertNotIn("payload_digest", public["prefixes"][0])

    def test_health_refuses_partial_inventory_foreign_snapshot_and_byte_overrun(self):
        payloads = health_payloads()
        for rows, snapshot in ((payloads[:-1], 2_000_000), (payloads, 2_000_001),
                               ((payloads[0], payloads[2], payloads[1], *payloads[3:]), 2_000_000)):
            with self.subTest(snapshot=snapshot), self.assertRaises(contract.EvidenceError):
                replay_health(rows, snapshot=snapshot)
        reservation = replay_health(payloads, snapshot=2_000_000).health.reserved_bytes
        with self.assertRaises(ProjectionBoundsExceeded):
            replay_health(payloads, snapshot=2_000_000, byte_limit=reservation - 1)

    def test_public_projection_refuses_private_columns_and_hides_no_unvisited_prefix(self):
        payloads = public_health_rows(replay_health(health_payloads(), snapshot=2_000_000))
        row = contract.decode_source_payload(payloads[2])
        for change in (dict(payload=b"private"), dict(last_status=1, last_attempt_utc_usec=1),
                       dict(last_capture_utc_usec=1), dict(retained_metadata_gap=1)):
            changed = (*payloads[:2], contract.encode_source_payload(dict(row, **change)), *payloads[3:])
            with self.subTest(change=change), self.assertRaises(contract.EvidenceError):
                decode_public_health(changed, snapshot=2_000_000)

    def test_inventory_cannot_relabel_a_visited_prefix_as_unobserved(self):
        payloads = health_payloads()
        first = contract.decode_source_payload(payloads[1])
        first.update(cursor_operation=contract.ZERO_ID, pass_upper=None, completed_passes=0,
                     failures=0, last_revision=0, last_cut=None)
        with self.assertRaises(contract.EvidenceError):
            replay_health((payloads[0], contract.encode_source_payload(first), *payloads[2:]), snapshot=2_000_000)

    def test_success_exposes_exact_point_labels_without_inventing_backlog_or_retention(self):
        row = checkpoint()
        prefix = prefix_health(row, revision=1)
        health = SweepHealth(operation(55), 1, 1, 100, (prefix,), 12288, 2)
        self.assertEqual(health.visited_prefixes, 1)
        self.assertEqual(health.active_passes, 1)
        self.assertEqual(health.latest_failed_prefixes, 0)
        self.assertEqual(health.retained_metadata_gaps, 0)
        self.assertEqual(prefix.last_capture_utc_usec, row["captured_utc_usec"])
        self.assertIsNone(health.known_source_backlog)
        self.assertIsNone(health.source_retention_floor)
        self.assertFalse(health.retention_acknowledged)
        self.assertTrue(health.future_commits_provisional)

    def test_missing_source_counts_make_capture_freshness_unknown(self):
        row = checkpoint()
        row["actual_sources"] -= 1
        prefix = prefix_health(row, revision=1)
        self.assertTrue(prefix.retained_metadata_gap)
        self.assertIsNone(prefix.last_capture_utc_usec)
        self.assertEqual(prefix.last_status, AttemptStatus.CAPTURED)

    def test_missing_or_changed_step_remains_visible_without_a_trusted_attempt_date(self):
        for change in (dict(payload=None, payload_digest=None), dict(payload=b"{}"), dict(status=2),
                       dict(last_cut=b"\x09" * 32)):
            row = checkpoint()
            row.update(change)
            with self.subTest(change=change):
                prefix = prefix_health(row, revision=1)
                self.assertTrue(prefix.retained_metadata_gap)
                self.assertIsNone(prefix.last_status)
                self.assertIsNone(prefix.last_attempt_utc_usec)

    def test_failed_attempt_preserves_prior_capture_label_and_failure_status(self):
        row = checkpoint()
        position = SweepPosition(operation(55), 256, 0, row["cursor_operation"], row["pass_upper"])
        step = make_step(position, failure=AttemptStatus.SOURCE_UNAVAILABLE, attempted_utc_usec=2_000_000)
        row.update(last_revision=step.revision, status=int(step.status), attempted_utc_usec=step.attempt_utc_usec,
                   payload=step.payload, payload_digest=step.payload_digest, failures=1)
        prefix = prefix_health(row, revision=257)
        self.assertEqual(prefix.last_status, AttemptStatus.SOURCE_UNAVAILABLE)
        self.assertEqual(prefix.last_capture_utc_usec, row["captured_utc_usec"])
        self.assertFalse(prefix.retained_metadata_gap)

    def test_gap_status_remains_visible_after_metadata_restoration(self):
        row = checkpoint()
        position = SweepPosition(operation(55), 256, 0, row["cursor_operation"], row["pass_upper"])
        step = make_step(position, failure=AttemptStatus.RETENTION_GAP, attempted_utc_usec=2_000_000)
        row.update(last_revision=step.revision, status=int(step.status), attempted_utc_usec=step.attempt_utc_usec,
                   payload=step.payload, payload_digest=step.payload_digest, failures=1)
        self.assertTrue(prefix_health(row, revision=257).retained_metadata_gap)

    def test_never_visited_prefix_is_unobserved_not_empty_history(self):
        row = dict(bucket=5, cursor_operation=contract.ZERO_ID, pass_upper=None, completed_passes=0,
                   failures=0, last_revision=0, last_cut=None)
        prefix = prefix_health(row, revision=0)
        self.assertIsNone(prefix.last_status)
        self.assertIsNone(prefix.last_capture_utc_usec)
        self.assertIsNone(prefix.last_attempt_utc_usec)
        self.assertEqual(prefix.completed_passes, 0)

    def test_foreign_cursor_and_future_checkpoint_revision_are_refused(self):
        for change in (dict(cursor_operation=operation(1 << 120)), dict(last_revision=2)):
            row = checkpoint()
            row.update(change)
            with self.subTest(change=change), self.assertRaises(contract.EvidenceError):
                prefix_health(row, revision=1)

    def test_retention_gap_prevents_native_capture_and_durably_rotates_one_slot(self):
        position = SweepPosition(operation(55), 256, 0, operation(111), operation(112), 1, 0)
        advanced = []

        class GapJournal:
            def __init__(self, *args, **kwargs):
                self.time_limit_s = kwargs["time_limit_s"]

            def position(self, scan_id):
                return position

            def verify_progress_evidence(self, current):
                self.assert_position = current
                raise RetentionGap("canonical_sweep_retention_gap")

            def advance(self, step):
                advanced.append(step)

        with patch("scripts.telemetry.canonical_reward_reconciliation.CanonicalRewardJournal", GapJournal), \
             patch("scripts.telemetry.canonical_reward_reconciliation.CanonicalRewardSnapshot") as source:
            step = reconcile_once(position.scan_id, object(), object(), attempted_utc_usec=2_000_000)
        source.assert_not_called()
        self.assertEqual(advanced, [step])
        self.assertEqual(step.status, AttemptStatus.RETENTION_GAP)
        self.assertEqual(step.cursor_after, position.cursor)
        self.assertEqual(step.upper_after, position.pass_upper)
        self.assertEqual(step.completed_passes, position.completed_passes)
        self.assertEqual(step.failures, position.failures + 1)
        self.assertEqual(step.next_bucket, 1)


if __name__ == "__main__":
    unittest.main()
