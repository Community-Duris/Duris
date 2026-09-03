#!/usr/bin/env python3
"""Regression coverage for exceptional account-parent merge verification."""

from __future__ import annotations

import importlib.util
import sys
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location(
    "verify_target_wins_account_merge",
    ROOT / "scripts/verify_target_wins_account_merge.py")
assert SPEC is not None and SPEC.loader is not None
merge = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = merge
SPEC.loader.exec_module(merge)


def digest(value: int) -> str:
    return f"{value:064x}"


def parent(reference: int, password: int, email: int, created: int) -> merge.Parent:
    return merge.Parent(digest(reference), digest(password), digest(email), digest(created))


def child(reference: int, parent_ref: int, action: str = "attach",
          target_ref: int | None = None) -> merge.Child:
    return merge.Child(
        digest(reference), digest(parent_ref), action,
        digest(target_ref) if target_ref is not None else None)


class TargetWinsAccountMergeTest(unittest.TestCase):
    def test_new_parent_accepts_attached_children(self):
        source = parent(1, 10, 11, 12)
        plan = merge.Plan(
            {source.account_ref: source}, {},
            (child(100, 1, target_ref=1),))
        result = merge.verify(plan, {})
        self.assertTrue(result.valid)
        self.assertEqual(result.summary()["new_parents"], 1)

    def test_byte_identical_existing_parent_is_safe(self):
        source = parent(1, 10, 11, 12)
        plan = merge.Plan(
            {source.account_ref: source}, {source.account_ref: source},
            (child(100, 1, target_ref=1),))
        result = merge.verify(plan, {})
        self.assertTrue(result.valid)
        self.assertEqual(result.summary()["identical_parents"], 1)

    def test_explicit_owner_evidence_allows_semantic_match(self):
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


if __name__ == "__main__":
    unittest.main()
