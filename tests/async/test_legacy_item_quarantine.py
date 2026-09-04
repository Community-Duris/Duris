#!/usr/bin/env python3
"""Focused classification and UID-allocation tests for legacy item quarantine."""

from __future__ import annotations

import importlib.util
import os
import sys
import tempfile
import unittest
from dataclasses import replace
from pathlib import Path
from unittest import mock


ROOT = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location(
    "classify_legacy_item_quarantine",
    ROOT / "scripts/classify_legacy_item_quarantine.py")
assert SPEC is not None and SPEC.loader is not None
quarantine = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = quarantine
SPEC.loader.exec_module(quarantine)


def digest(value: int) -> str:
    """Return a deterministic test-only SHA-256-shaped reference."""
    return f"{value:064x}"


def row(index: int, parent: int | None = None, **changes) -> quarantine.EvidenceRow:
    """Build one valid evidence row with optional field replacements."""
    base = quarantine.EvidenceRow(
        digest(index), "player_items", index, 100 + index,
        digest(parent) if parent is not None else None, digest(900), True,
        1000 + index, "current", digest(500 + index), 1, 1, False,
        digest(800 + index))
    return replace(base, **changes)


def evidence(*rows: quarantine.EvidenceRow) -> quarantine.Evidence:
    """Build a test evidence packet with ordinary allocator floors."""
    return quarantine.Evidence(1000, 900, rows)


def decision(index: int, value: str) -> quarantine.Disposition:
    """Build one test operator disposition."""
    return quarantine.Disposition(digest(index), value, digest(800 + index))


class LegacyItemQuarantineTest(unittest.TestCase):
    """Exercise fail-closed quarantine classification and recovery planning."""

    def test_overlapping_reasons_have_one_primary_and_child_impact(self):
        """Retain overlapping reasons while counting one primary class."""
        parent = row(
            1, prototype_state="artifact", owner_proven=False,
            metadata_candidates=0, uid_candidates=2)
        child = row(2, 1)
        result = quarantine.classify(evidence(parent, child))
        by_ref = {item.row.row_ref: item for item in result}
        self.assertEqual(by_ref[digest(1)].primary, "artifact")
        self.assertGreater(len(by_ref[digest(1)].reasons), 1)
        self.assertEqual(by_ref[digest(2)].primary, "dependent_ancestry")
        summary = quarantine.classification_summary(result)
        self.assertEqual(summary["rows"], 2)
        self.assertEqual(summary["dependent_children"], 1)

    def test_complete_proven_acyclic_chain_is_recoverable(self):
        """Recover an independently proven complete ancestry chain unchanged."""
        source = evidence(row(1), row(2, 1))
        classified = quarantine.classify(source)
        self.assertTrue(all(item.primary == "recoverable" for item in classified))
        recovery, next_uid, missing = quarantine.plan_recovery(source, classified, {})
        self.assertEqual((len(recovery), next_uid, missing), (2, 1000, 0))
        child = next(item for item in recovery if item.row.row_ref == digest(2))
        self.assertEqual((child.new_parent_uid, child.new_root_uid), (101, 101))

    def test_uid_collision_requires_evidence_and_allocates_above_every_floor(self):
        """Remap only a proven collision and keep the allocator representable."""
        collision = row(1, item_uid=5000, uid_candidates=2)
        source = quarantine.Evidence(4000, 4500, (collision,))
        classified = quarantine.classify(source)
        recovery, _, missing = quarantine.plan_recovery(source, classified, {})
        self.assertEqual((recovery, missing), ([], 1))
        recovery, next_uid, missing = quarantine.plan_recovery(
            source, classified, {digest(1): decision(1, "recover_new_uid")})
        self.assertEqual((recovery[0].new_uid, next_uid, missing), (5001, 5002, 0))

        absent_uid = evidence(row(2, uid_candidates=0))
        classified = quarantine.classify(absent_uid)
        with self.assertRaisesRegex(quarantine.QuarantineError, "invalid recovery"):
            quarantine.plan_recovery(
                absent_uid, classified,
                {digest(2): decision(2, "recover_new_uid")})

        exhausted = quarantine.Evidence(
            quarantine.MAX_UID, quarantine.MAX_UID,
            (row(3, item_uid=quarantine.MAX_UID, uid_candidates=2),))
        classified = quarantine.classify(exhausted)
        with self.assertRaisesRegex(quarantine.QuarantineError, "overflowed"):
            quarantine.plan_recovery(
                exhausted, classified,
                {digest(3): decision(3, "recover_new_uid")})

    def test_descendant_recovery_requires_every_ancestor_and_rewrites_chain(self):
        """Recover a sound descendant only through its approved remapped parent."""
        parent = row(1, item_uid=5000, uid_candidates=2)
        child = row(2, 1)
        source = quarantine.Evidence(4000, 4500, (parent, child))
        classified = quarantine.classify(source)
        dispositions = {
            digest(1): decision(1, "recover_new_uid"),
            digest(2): decision(2, "recover_descendant"),
        }
        recovery, next_uid, missing = quarantine.plan_recovery(
            source, classified, dispositions)
        by_ref = {item.row.row_ref: item for item in recovery}
        self.assertEqual((len(recovery), next_uid, missing), (2, 5002, 0))
        self.assertEqual(by_ref[digest(1)].new_uid, 5001)
        self.assertEqual(
            (by_ref[digest(2)].new_uid, by_ref[digest(2)].new_parent_uid,
             by_ref[digest(2)].new_root_uid),
            (102, 5001, 5001))

        dispositions[digest(1)] = decision(1, "hold")
        with self.assertRaisesRegex(quarantine.QuarantineError, "every ancestor"):
            quarantine.plan_recovery(source, classified, dispositions)

    def test_replacement_uid_order_is_stable_across_evidence_order(self):
        """Allocate replacement UIDs by stable source identity, not set order."""
        first = row(1, item_uid=5000, uid_candidates=2)
        second = row(2, item_uid=6000, uid_candidates=2)
        dispositions = {
            digest(1): decision(1, "recover_new_uid"),
            digest(2): decision(2, "recover_new_uid"),
        }
        assignments = []
        for source in (
                quarantine.Evidence(4000, 4500, (first, second)),
                quarantine.Evidence(4000, 4500, (second, first))):
            recovery, next_uid, missing = quarantine.plan_recovery(
                source, quarantine.classify(source), dispositions)
            assignments.append(
                ([(item.row.source_row_id, item.new_uid) for item in recovery],
                 next_uid, missing))
        self.assertEqual(assignments[0], assignments[1])
        self.assertEqual(assignments[0], ([(1, 6001), (2, 6002)], 6003, 0))

    def test_risky_classes_require_explicit_non_recovery_disposition(self):
        """Require a hold or discard for every unsafe non-recoverable row."""
        rows = (
            row(1, owner_proven=False),
            row(2, metadata_candidates=0),
            row(3, parent=99),
            row(4, 5), row(5, 4),
            replace(row(6, 1), owner_ref=digest(901)),
        )
        source = evidence(*rows)
        classified = quarantine.classify(source)
        _, _, missing = quarantine.plan_recovery(source, classified, {})
        self.assertEqual(missing, len(rows))
        holds = {item.row.row_ref: decision(item.row.source_row_id, "hold")
                 for item in classified}
        recovery, _, missing = quarantine.plan_recovery(source, classified, holds)
        self.assertEqual((recovery, missing), ([], 0))

    def test_parent_held_back_prevents_dependent_recovery(self):
        """Keep dependent rows out when their rejected parent remains held."""
        parent = row(1, uid_candidates=2)
        child = row(2, 1)
        source = evidence(parent, child)
        classified = quarantine.classify(source)
        recovery, _, missing = quarantine.plan_recovery(
            source, classified, {digest(1): decision(1, "hold")})
        self.assertEqual((recovery, missing), ([], 1))
        dispositions = {
            digest(1): decision(1, "hold"),
            digest(2): decision(2, "hold"),
        }
        recovery, _, missing = quarantine.plan_recovery(
            source, classified, dispositions)
        self.assertEqual((recovery, missing), ([], 0))

    def test_planner_rejects_incomplete_or_modified_classifications(self):
        """Bind planning to the complete classifications derived from evidence."""
        source = evidence(row(1), row(2, 1))
        classified = quarantine.classify(source)
        with self.assertRaisesRegex(quarantine.QuarantineError, "exactly match"):
            quarantine.plan_recovery(source, classified[:-1], {})
        forged = [replace(item, primary="recoverable", reasons=frozenset())
                  for item in quarantine.classify(evidence(row(3, owner_proven=False)))]
        with self.assertRaisesRegex(quarantine.QuarantineError, "exactly match"):
            quarantine.plan_recovery(
                evidence(row(3, owner_proven=False)), forged, {})

    def test_planner_rejects_stale_or_mismatched_disposition_evidence(self):
        """Reject an operator decision not bound to the current evidence row."""
        source = evidence(row(1, uid_candidates=2))
        classified = quarantine.classify(source)
        stale = replace(decision(1, "recover_new_uid"), evidence_ref=digest(999))
        with self.assertRaisesRegex(quarantine.QuarantineError, "stale or mismatched"):
            quarantine.plan_recovery(source, classified, {digest(1): stale})

    def test_cli_rejects_incomplete_evidence_before_disposition_or_planning(self):
        """Require the complete 38,257-row retained set at the CLI boundary."""
        arguments = quarantine.argparse.Namespace(
            evidence=Path("/unused/evidence.json"), dispositions=None,
            recovery_plan=None)
        with mock.patch.object(quarantine, "parse_arguments", return_value=arguments), \
                mock.patch.object(
                    quarantine, "read_evidence",
                    return_value=(evidence(row(1)), digest(700))), \
                mock.patch.object(quarantine, "read_dispositions") as read_dispositions, \
                mock.patch.object(quarantine, "plan_recovery") as plan_recovery, \
                mock.patch("builtins.print"):
            self.assertEqual(quarantine.main(), 2)
        read_dispositions.assert_not_called()
        plan_recovery.assert_not_called()

    def test_rejected_private_artifact_closes_its_descriptor(self):
        """Close an opened artifact even when its owner-only mode is rejected."""
        with tempfile.TemporaryDirectory() as temporary:
            artifact = Path(temporary).resolve() / "evidence.json"
            artifact.write_text("{}", encoding="utf-8")
            os.chmod(artifact, 0o644)  # noqa: S103 - deliberately unsafe fixture.
            descriptor = os.open(artifact, os.O_RDONLY)
            with mock.patch.object(quarantine.os, "open", return_value=descriptor):
                with self.assertRaisesRegex(
                        quarantine.QuarantineError, "owner-only regular file"):
                    quarantine._read_json(artifact, "quarantine evidence")
            with self.assertRaises(OSError):
                os.fstat(descriptor)

    def test_recovery_plan_is_owner_only_and_binds_input_digests(self):
        """Write a new owner-only plan bound to both protected input digests."""
        source = evidence(row(1))
        classified = quarantine.classify(source)
        recovery, next_uid, missing = quarantine.plan_recovery(source, classified, {})
        self.assertEqual(missing, 0)
        with tempfile.TemporaryDirectory() as temporary:
            os.chmod(temporary, 0o700)
            plan = Path(temporary).resolve() / "recovery.json"
            digest_value = quarantine.write_plan(
                plan, digest(700), digest(701), recovery, next_uid)
            self.assertRegex(digest_value, r"^[0-9a-f]{64}$")
            self.assertEqual(os.stat(plan).st_mode & 0o777, 0o600)
            payload = plan.read_text(encoding="utf-8")
            self.assertIn(digest(700), payload)
            self.assertIn(digest(701), payload)


if __name__ == "__main__":
    unittest.main()
