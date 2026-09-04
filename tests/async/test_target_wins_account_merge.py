#!/usr/bin/env python3
"""Regression coverage for exceptional account-parent merge verification."""

from __future__ import annotations

import importlib.util
import os
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock


ROOT = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location(
    "verify_target_wins_account_merge",
    ROOT / "scripts/verify_target_wins_account_merge.py")
assert SPEC is not None and SPEC.loader is not None
merge = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = merge
SPEC.loader.exec_module(merge)


def digest(value: int) -> str:
    """Return a deterministic opaque-reference fixture."""
    return f"{value:064x}"


def parent(reference: int, password: int, email: int, created: int) -> merge.Parent:
    """Build one parent fixture with deterministic fingerprints."""
    return merge.Parent(digest(reference), digest(password), digest(email), digest(created))


def child(reference: int, parent_ref: int, action: str = "attach",
          target_ref: int | None = None) -> merge.Child:
    """Build one child disposition fixture."""
    return merge.Child(
        digest(reference), digest(parent_ref), action,
        digest(target_ref) if target_ref is not None else None)


class TargetWinsAccountMergeTest(unittest.TestCase):
    """Exercise fail-closed exceptional account merge verification."""

    def test_new_parent_accepts_attached_children(self):
        """Permit children attached to a parent absent from the target."""
        source = parent(1, 10, 11, 12)
        plan = merge.Plan(
            {source.account_ref: source}, {},
            (child(100, 1, target_ref=1),))
        result = merge.verify(plan, {})
        self.assertTrue(result.valid)
        self.assertEqual(result.summary()["new_parents"], 1)

    def test_byte_identical_existing_parent_is_safe(self):
        """Permit attachment when all authentication metadata is identical."""
        source = parent(1, 10, 11, 12)
        plan = merge.Plan(
            {source.account_ref: source}, {source.account_ref: source},
            (child(100, 1, target_ref=1),))
        result = merge.verify(plan, {})
        self.assertTrue(result.valid)
        self.assertEqual(result.summary()["identical_parents"], 1)

    def test_explicit_owner_evidence_allows_semantic_match(self):
        """Permit a differing collision only with protected owner evidence."""
        source = parent(1, 10, 11, 12)
        target = parent(1, 20, 21, 22)
        decision = merge.Decision(source.account_ref, "same_owner", digest(50), None)
        plan = merge.Plan(
            {source.account_ref: source}, {target.account_ref: target},
            (child(100, 1, target_ref=1),))
        result = merge.verify(plan, {source.account_ref: decision})
        self.assertTrue(result.valid)
        self.assertEqual(result.summary()["approved_semantic_matches"], 1)

    def test_unverified_collision_blocks_otherwise_safe_children(self):
        """Block every child under a collision without an owner disposition."""
        source = parent(1, 10, 11, 12)
        target = parent(1, 20, 21, 22)
        plan = merge.Plan(
            {source.account_ref: source}, {target.account_ref: target},
            (child(100, 1, target_ref=1), child(101, 1, target_ref=1)))
        result = merge.verify(plan, {})
        self.assertFalse(result.valid)
        self.assertEqual(result.summary()["unverified_collisions"], 1)
        self.assertEqual(result.blocked_children, 2)

    def test_quarantine_and_remap_require_every_child_to_follow_decision(self):
        """Require all children to follow the exact approved disposition."""
        source = parent(1, 10, 11, 12)
        target = parent(1, 20, 21, 22)
        remap_target = parent(2, 30, 31, 32)
        quarantine = merge.Decision(source.account_ref, "quarantine", digest(50), None)
        quarantined = merge.Plan(
            {source.account_ref: source},
            {target.account_ref: target, remap_target.account_ref: remap_target},
            (child(100, 1, "quarantine"),))
        self.assertTrue(merge.verify(
            quarantined, {source.account_ref: quarantine}).valid)

        remap = merge.Decision(source.account_ref, "remap", digest(51), digest(2))
        remapped = merge.Plan(
            quarantined.source, quarantined.target,
            (child(100, 1, "remap", 2),))
        self.assertTrue(merge.verify(remapped, {source.account_ref: remap}).valid)
        mixed = merge.Plan(
            quarantined.source, quarantined.target,
            (child(100, 1, "remap", 2), child(101, 1, target_ref=1)))
        self.assertFalse(merge.verify(mixed, {source.account_ref: remap}).valid)

    def test_remap_target_must_not_be_another_source_parent(self):
        """Reject remapping into any other source/target collision."""
        source = parent(1, 10, 11, 12)
        target = parent(1, 20, 21, 22)
        other_source = parent(2, 30, 31, 32)
        other_target = parent(2, 40, 41, 42)
        decision = merge.Decision(source.account_ref, "remap", digest(50), digest(2))
        plan = merge.Plan(
            {source.account_ref: source, other_source.account_ref: other_source},
            {target.account_ref: target, other_target.account_ref: other_target},
            (child(100, 1, "remap", 2), child(101, 2, "quarantine")),
        )
        other_decision = merge.Decision(
            other_source.account_ref, "quarantine", digest(51), None)
        self.assertFalse(merge.verify(plan, {
            source.account_ref: decision,
            other_source.account_ref: other_decision,
        }).valid)

    def test_rejected_artifact_closes_its_file_descriptor(self):
        """Close protected artifact descriptors even when mode validation fails."""
        with tempfile.TemporaryDirectory() as temporary:
            artifact = Path(temporary) / "plan.json"
            artifact.write_text("{}", encoding="utf-8")
            os.chmod(artifact, 0o644)  # noqa: S103 - exercise unsafe-file rejection
            descriptor = os.open(artifact, os.O_RDONLY)
            with mock.patch.object(merge.os, "open", return_value=descriptor):
                with self.assertRaisesRegex(
                        merge.MergeVerificationError, "owner-only regular file"):
                    merge._read_protected_json(artifact.resolve(), "merge plan")
            with self.assertRaises(OSError):
                os.fstat(descriptor)


if __name__ == "__main__":
    unittest.main()
