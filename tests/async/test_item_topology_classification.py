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
from unittest import mock


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
    """Build one internally consistent payload/custody topology fixture."""
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
    """Exercise topology classification precedence and protected evidence."""

    def categories(self, rows, maximum_depth=32):
        """Return category names for compact fixture assertions."""
        return [finding.category for finding in topology.classify(rows, maximum_depth)]

    def test_acyclic_parent_and_root_drift_is_repairable(self):
        """Classify consistent acyclic projection lag as repairable."""
        rows = [row(1), row(2, 1, 1, current_parent_uid=None, current_root_uid=2)]
        self.assertEqual(self.categories(rows), ["repairable_projection_lag"])

    def test_missing_and_foreign_parent_are_refused(self):
        """Refuse payload-parent loss and foreign/inactive custody."""
        missing_payload = row(2, broken_parent=True)
        self.assertEqual(self.categories([missing_payload]), ["missing_payload_parent"])
        missing_current = row(2, 1, 1, payload_parent_state=None)
        self.assertIn("missing_current_parent", self.categories([row(1), missing_current]))
        foreign = row(2, 1, 1, payload_parent_owner_id=8)
        self.assertIn("foreign_or_inactive_parent", self.categories([row(1), foreign]))

    def test_cycle_and_depth_failure_are_distinct(self):
        """Distinguish cyclic ancestry from excessive acyclic depth."""
        cycle = [row(1, 2, 1), row(2, 1, 1)]
        self.assertEqual(self.categories(cycle), ["cycle", "cycle"])
        deep = [row(1), row(2, 1, 1), row(3, 2, 1), row(4, 3, 1)]
        self.assertIn("depth_exceeded", self.categories(deep, maximum_depth=3))

    def test_owner_disagreement_prevents_repair(self):
        """Refuse repair when authoritative and payload owners disagree."""
        disagreement = row(2, current_owner_id=8)
        self.assertEqual(self.categories([disagreement]), ["owner_disagreement"])

    def test_quarantine_and_inactive_state_are_expected_transitions(self):
        """Treat only otherwise-consistent lifecycle states as expected."""
        findings = topology.classify([
            row(1, quarantined=True), row(2, state=2),
        ])
        self.assertTrue(all(finding.expected for finding in findings))
        self.assertEqual(
            {finding.category for finding in findings},
            {"expected_quarantine", "expected_inactive_state"})

    def test_corruption_precedes_quarantine_and_inactive_state(self):
        """Never let lifecycle context hide authoritative owner corruption."""
        findings = topology.classify([
            row(1, quarantined=True, current_owner_id=8),
            row(2, state=2, current_owner_id=8),
        ])
        self.assertTrue(all(not finding.expected for finding in findings))
        self.assertEqual(
            {finding.category for finding in findings}, {"owner_disagreement"})

        graph_findings = topology.classify([
            row(3, 4, 3, quarantined=True), row(4, 3, 3),
        ])
        self.assertTrue(all(not finding.expected for finding in graph_findings))
        self.assertEqual(
            {finding.category for finding in graph_findings}, {"cycle"})

        parent_findings = topology.classify([
            row(5),
            row(6, 5, 5, state=2, payload_parent_state=None),
            row(7, 5, 5, quarantined=True, payload_parent_owner_id=8),
        ])
        self.assertTrue(all(not finding.expected for finding in parent_findings))
        self.assertEqual(
            {finding.category for finding in parent_findings},
            {"missing_current_parent", "foreign_or_inactive_parent"},
        )

    def test_duplicate_identical_custody_rows_are_corruption(self):
        """Report duplicate custody sources even when their payloads agree."""
        duplicate = row(1)
        self.assertEqual(
            self.categories([duplicate, replace(duplicate, source_row_id=2)]),
            ["duplicate_custody_rows"],
        )
        self.assertEqual(
            self.categories([
                duplicate,
                replace(duplicate, source_row_id=2, payload_owner_id=8),
            ]),
            ["ambiguous_payload"],
        )
        self.assertEqual(
            self.categories([
                duplicate,
                replace(duplicate, source_row_id=2, broken_parent=True),
            ]),
            ["missing_payload_parent"],
        )

    def test_exact_rows_are_written_only_to_owner_only_artifact(self):
        """Write protected row evidence with a stable digest and safe mode."""
        findings = topology.classify([
            row(2, 1, 1, current_parent_uid=None, current_root_uid=2), row(1),
        ])
        with tempfile.TemporaryDirectory() as temporary:
            os.chmod(temporary, 0o700)
            artifact = Path(temporary).resolve() / "classification.tsv"
            digest = topology.write_artifact(artifact, "duris_test", findings)
            self.assertRegex(digest, r"^[0-9a-f]{64}$")
            self.assertEqual(os.stat(artifact).st_mode & 0o777, 0o600)
            self.assertIn(
                "repairable_projection_lag", artifact.read_text(encoding="utf-8"))

    def test_clean_main_writes_header_only_artifact_and_succeeds(self):
        """Preserve clean post-repair evidence without returning blocked."""
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary).resolve()
            os.chmod(directory, 0o700)
            artifact = directory / "classification.tsv"
            arguments = topology.argparse.Namespace(
                env_file=directory / "env", artifact=artifact)
            with mock.patch.object(topology, "parse_arguments", return_value=arguments), \
                    mock.patch.object(
                        topology, "read_env_file", return_value={"DB_NAME": "duris_test"}), \
                    mock.patch.object(topology, "active_connections", return_value=0), \
                    mock.patch.object(topology, "load_rows", return_value=[]), \
                    mock.patch("builtins.print"):
                self.assertEqual(topology.main(), 0)
            self.assertEqual(
                artifact.read_text(encoding="utf-8").splitlines(),
                [topology.ARTIFACT_HEADER, "# database=duris_test",
                 topology.ARTIFACT_COLUMNS],
            )


if __name__ == "__main__":
    unittest.main()
