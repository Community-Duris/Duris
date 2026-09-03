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


ROOT = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location(
    "classify_legacy_item_quarantine",
    ROOT / "scripts/classify_legacy_item_quarantine.py")
assert SPEC is not None and SPEC.loader is not None
quarantine = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = quarantine
SPEC.loader.exec_module(quarantine)


def digest(value: int) -> str:
    return f"{value:064x}"


def row(index: int, parent: int | None = None, **changes) -> quarantine.EvidenceRow:
    base = quarantine.EvidenceRow(
        digest(index), "player_items", index, 100 + index,
        digest(parent) if parent is not None else None, digest(900), True,
        1000 + index, "current", digest(500 + index), 1, 1, False)
    return replace(base, **changes)


def evidence(*rows: quarantine.EvidenceRow) -> quarantine.Evidence:
    return quarantine.Evidence(1000, 900, rows)


def decision(index: int, value: str) -> quarantine.Disposition:
    return quarantine.Disposition(digest(index), value, digest(800 + index))


class LegacyItemQuarantineTest(unittest.TestCase):
    def test_overlapping_reasons_have_one_primary_and_child_impact(self):
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
        source = evidence(row(1), row(2, 1))
        classified = quarantine.classify(source)
        self.assertTrue(all(item.primary == "recoverable" for item in classified))
        recovery, next_uid, missing = quarantine.plan_recovery(source, classified, {})
        self.assertEqual((len(recovery), next_uid, missing), (2, 1000, 0))
        child = next(item for item in recovery if item.row.row_ref == digest(2))
        self.assertEqual((child.new_parent_uid, child.new_root_uid), (101, 101))

    def test_uid_collision_requires_evidence_and_allocates_above_every_floor(self):
        collision = row(1, item_uid=5000, uid_candidates=2)
        source = quarantine.Evidence(4000, 4500, (collision,))
        classified = quarantine.classify(source)
        recovery, _, missing = quarantine.plan_recovery(source, classified, {})
        self.assertEqual((recovery, missing), ([], 1))
        recovery, next_uid, missing = quarantine.plan_recovery(
            source, classified, {digest(1): decision(1, "recover_new_uid")})
        self.assertEqual((recovery[0].new_uid, next_uid, missing), (5001, 5002, 0))

    def test_risky_classes_require_explicit_non_recovery_disposition(self):
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

    def test_recovery_plan_is_owner_only_and_binds_input_digests(self):
        source = evidence(row(1))
        classified = quarantine.classify(source)
        recovery, next_uid, missing = quarantine.plan_recovery(source, classified, {})
        self.assertEqual(missing, 0)
        with tempfile.TemporaryDirectory() as temporary:
            os.chmod(temporary, 0o700)
            plan = Path(temporary) / "recovery.json"
            digest_value = quarantine.write_plan(
                plan, digest(700), digest(701), recovery, next_uid)
            self.assertRegex(digest_value, r"^[0-9a-f]{64}$")
            self.assertEqual(os.stat(plan).st_mode & 0o777, 0o600)
            payload = plan.read_text()
            self.assertIn(digest(700), payload)
            self.assertIn(digest(701), payload)


if __name__ == "__main__":
    unittest.main()
