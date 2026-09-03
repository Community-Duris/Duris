#!/usr/bin/env python3
"""Focused root-cause fixtures for item topology classification."""

from __future__ import annotations

import importlib.util
import os
import sys
import tempfile
import unittest
from dataclasses import replace
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
SPEC = importlib.util.spec_from_file_location(
    "classify_item_topology", ROOT / "scripts/classify_item_topology.py")
assert SPEC is not None and SPEC.loader is not None
topology = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = topology
SPEC.loader.exec_module(topology)


def row(uid: int, parent: int | None = None, root: int | None = None,
        **changes) -> topology.Row:
    base = topology.Row(
        "player_items", uid, uid, parent, 1, 7, 0, 100 + uid,
        current_root_uid=root if root is not None else uid,
        current_parent_uid=parent, current_owner_type=1, current_owner_id=7,
        current_owner_context_id=0, item_revision=2, current_vnum=100 + uid,
        state=1, expected_item_revision=2,
        payload_parent_state=1 if parent is not None else None,
        payload_parent_owner_type=1 if parent is not None else None,
        payload_parent_owner_id=7 if parent is not None else None,
        payload_parent_owner_context_id=0 if parent is not None else None,
    )
    return replace(base, **changes)


class ItemTopologyClassificationTest(unittest.TestCase):
    def categories(self, rows, maximum_depth=32):
        return [finding.category for finding in topology.classify(rows, maximum_depth)]

    def test_acyclic_parent_and_root_drift_is_repairable(self):
        rows = [row(1), row(2, 1, 1, current_parent_uid=None, current_root_uid=2)]
        self.assertEqual(self.categories(rows), ["repairable_projection_lag"])

    def test_missing_and_foreign_parent_are_refused(self):
        missing_payload = row(2, broken_parent=True)
        self.assertEqual(self.categories([missing_payload]), ["missing_payload_parent"])
        missing_current = row(2, 1, 1, payload_parent_state=None)
        self.assertIn("missing_current_parent", self.categories([row(1), missing_current]))
        foreign = row(2, 1, 1, payload_parent_owner_id=8)
        self.assertIn("foreign_or_inactive_parent", self.categories([row(1), foreign]))

    def test_cycle_and_depth_failure_are_distinct(self):
        cycle = [row(1, 2, 1), row(2, 1, 1)]
        self.assertEqual(self.categories(cycle), ["cycle", "cycle"])
        deep = [row(1), row(2, 1, 1), row(3, 2, 1), row(4, 3, 1)]
        self.assertIn("depth_exceeded", self.categories(deep, maximum_depth=3))

    def test_owner_disagreement_prevents_repair(self):
        disagreement = row(2, current_owner_id=8)
        self.assertEqual(self.categories([disagreement]), ["owner_disagreement"])

    def test_quarantine_and_inactive_state_are_expected_transitions(self):
        findings = topology.classify([
            row(1, quarantined=True), row(2, state=2),
        ])
        self.assertTrue(all(finding.expected for finding in findings))
        self.assertEqual(
            {finding.category for finding in findings},
            {"expected_quarantine", "expected_inactive_state"})

    def test_exact_rows_are_written_only_to_owner_only_artifact(self):
        findings = topology.classify([
            row(2, 1, 1, current_parent_uid=None, current_root_uid=2), row(1),
        ])
        with tempfile.TemporaryDirectory() as temporary:
            os.chmod(temporary, 0o700)
            artifact = Path(temporary) / "classification.tsv"
            digest = topology.write_artifact(artifact, "duris_test", findings)
            self.assertRegex(digest, r"^[0-9a-f]{64}$")
            self.assertEqual(os.stat(artifact).st_mode & 0o777, 0o600)
            self.assertIn("repairable_projection_lag", artifact.read_text())


if __name__ == "__main__":
    unittest.main()
