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

    def test_loopback_transport_uses_client_compatible_ssl_option(self):
        """Delegate local SSL option selection to the shared client probe."""
        config = {"DB_HOST": "127.0.0.1", "DB_PORT": "3306"}
        with mock.patch.object(
                topology, "preferred_mysql_ssl_arguments",
                return_value=("--ssl-mode=PREFERRED",)):
            arguments = topology.connection_arguments(config)
        self.assertIn("--ssl-mode=PREFERRED", arguments)
        self.assertNotIn("--skip-ssl", arguments)

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

    def test_post_baseline_revision_uses_creation_and_transfer_events(self):
        """A UID created after baseline still needs a contiguous event history."""
        statement = topology.classification_sql()
        self.assertIn("baseline.item_uid IS NULL AND ledger.item_uid IS NULL", statement)
        self.assertIn("COALESCE(baseline.opening_item_revision,0)+"
                      "COALESCE(ledger.event_count,0)", statement)
        post_baseline = row(9, item_revision=2, expected_item_revision=3)
        self.assertEqual(self.categories([post_baseline]),
                         ["item_revision_mismatch"])

    def test_orphaned_corpse_owner_remains_in_candidate_population(self):
        """Retain and classify a corpse item whose player owner is absent."""
        output = "\t".join((
            "corpse_items", "1", "1", "NULL", "4", "NULL", "0", "101",
            "0", "0", "1", "NULL", "4", "99", "0", "2", "101", "1",
            "2", "NULL", "NULL", "NULL", "NULL",
        ))

        def query(statement):
            """Return one orphaned corpse row from the expected left join."""
            self.assertIn("LEFT JOIN player_data", statement)
            return output

        rows = topology.load_rows(query)
        self.assertIsNone(rows[0].payload_owner_id)
        self.assertEqual(self.categories(rows), ["missing_payload_owner"])

    def test_pet_payload_uses_pet_uid_and_player_context(self):
        """Keep pet payloads in the same repair inventory as player items."""
        statement = topology.classification_sql()
        self.assertIn("FROM player_pet_items i JOIN player_pets pp", statement)
        self.assertIn("CAST(pp.pet_uid AS UNSIGNED),CAST(pp.owner_pid AS UNSIGNED)",
                      statement)
        pet = row(8, source_table="player_pet_items", payload_owner_type=11,
                  payload_owner_id=80, payload_owner_context_id=7,
                  current_owner_type=11, current_owner_id=80,
                  current_owner_context_id=7)
        self.assertEqual(self.categories([pet]), [])
        self.assertEqual(self.categories([replace(pet, current_owner_id=None)]),
                         ["owner_disagreement"])

    def test_shopkeeper_and_siege_payloads_are_included(self):
        """Use the runtime shop ID offset and room custody for legacy stores."""
        statement = topology.classification_sql()
        self.assertIn("FROM shopkeeper_items i LEFT JOIN shopkeepers s", statement)
        self.assertIn("CAST(s.shop_id AS UNSIGNED)+1", statement)
        self.assertIn("FROM siege_items i LEFT JOIN siege_items p", statement)
        self.assertNotIn("WHERE i.obj_uid>0", statement)
        shop = row(8, source_table="shopkeeper_items", payload_owner_type=9,
                   payload_owner_id=42, current_owner_type=9, current_owner_id=42)
        siege = row(9, source_table="siege_items", payload_owner_type=3,
                    payload_owner_id=3001, current_owner_type=3,
                    current_owner_id=3001)
        self.assertEqual(self.categories([shop, siege]), [])

    def test_null_and_zero_uids_remain_distinct_protected_cases(self):
        """Do not discard unidentified physical payload rows from the inventory."""
        null_uid = replace(row(1), source_row_id=30, item_uid=None)
        zero_uid = replace(row(2), source_row_id=31, item_uid=0)
        findings = topology.classify([null_uid, zero_uid])
        self.assertEqual([finding.category for finding in findings],
                         ["missing_item_uid", "missing_item_uid"])
        self.assertIn("\tNULL\t", findings[0].artifact_row())
        self.assertIn("\t0\t", findings[1].artifact_row())
        self.assertIn("null_uids=1 zero_uids=1", topology.summary(
            [null_uid, zero_uid], findings))
        sample = "\t".join((
            "siege_items", "31", "NULL", "NULL", "3", "3001", "0", "101",
            "0", "0", "NULL", "NULL", "NULL", "NULL", "NULL", "NULL",
            "NULL", "NULL", "NULL", "NULL", "NULL", "NULL", "NULL",
        ))
        parsed = topology.load_rows(lambda _statement: sample)
        self.assertIsNone(parsed[0].item_uid)
        self.assertEqual(self.categories(parsed), ["missing_item_uid"])

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

    def test_duplicate_uid_preserves_every_physical_payload_row(self):
        """A repair case must include both source IDs, including conflicts."""
        duplicate = row(1)
        for second, category in (
                (replace(duplicate, source_row_id=2), "duplicate_payload_uid"),
                (replace(duplicate, source_row_id=2, payload_owner_id=8),
                 "ambiguous_payload"),
                (replace(duplicate, source_row_id=2, broken_parent=True),
                 "missing_payload_parent")):
            rows = [duplicate, second]
            findings = topology.classify(rows)
            self.assertEqual([finding.category for finding in findings],
                             [category, category])
            self.assertEqual([finding.row.source_row_id for finding in findings],
                             [1, 2])
            self.assertEqual(len({finding.artifact_row() for finding in findings}), 2)
            self.assertIn("duplicate_uid_rows=2", topology.summary(rows, findings))

    @unittest.skipUnless(hasattr(os, "getuid"), "Unix owner-only permissions required")
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

    @unittest.skipUnless(hasattr(os, "getuid"), "Unix owner-only permissions required")
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
