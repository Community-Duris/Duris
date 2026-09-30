#!/usr/bin/env python3
"""Focused protected-case classification for anomalous custody history."""

from __future__ import annotations

import os
import stat
import sys
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
import classify_item_custody_history as history  # noqa: E402


def current(uid: int = 101, *, revision: int = 2,
            opening: int | None = None, events: int | None = 1) -> history.Row:
    return history.Row((uid, uid, None, 1, 7, 0, revision, 500, 3,
                        opening, events, 1 if events else None, events))


def death(uid: int, root: int, revision: int, *, operation: str | None = "A"):
    return (7, 9, uid, root, 0, revision, 500, 1, 1, 7, 0, 1,
            operation * 32 if operation else None,
            operation * 64 if operation else None)


def snapshot(*rows: history.Row, deaths=()) -> history.Snapshot:
    return history.Snapshot(tuple(rows), tuple(deaths))


class CustodyHistoryClassificationTest(unittest.TestCase):
    def test_exact_root_and_unwitnessed_cases_stay_distinct(self):
        evidence = snapshot(current(101), current(102), current(103),
                            deaths=(death(101, 101, 1), death(999, 102, 0)))
        cases = history.classify(evidence)
        self.assertEqual([case.category for case in cases], [
            "revision_mismatch_exact_death_witness",
            "revision_mismatch_root_death_witness",
            "revision_mismatch_unwitnessed",
        ])
        self.assertIn("cases=3 death_evidence_rows=2", history.summary(evidence, cases))
        self.assertNotIn("101", history.summary(evidence, cases))

    def test_death_rows_are_stored_once_even_when_many_items_share_root(self):
        evidence = snapshot(current(101), current(102),
                            deaths=(death(101, 101, 1), death(888, 101, 0)))
        cases = history.classify(evidence)
        self.assertEqual(len(cases), 2)
        self.assertEqual(len(evidence.death_rows), 2)
        self.assertEqual(cases[0].category, "revision_mismatch_exact_death_witness")
        self.assertEqual(cases[1].category, "revision_mismatch_unwitnessed")
        self.assertEqual(cases[0].evidence_id("1" * 64),
                         history.classify(evidence)[0].evidence_id("1" * 64))
        self.assertNotEqual(cases[0].evidence_id("1" * 64),
                            cases[0].evidence_id("2" * 64))

    def test_missing_origin_is_held_even_at_revision_zero(self):
        self.assertEqual(history.classify(snapshot(current(revision=0, events=None)))[0]
                         .category, "missing_origin")
        with self.assertRaises(history.HistoryError):
            history.classify(snapshot(current(revision=1, events=1)))

    def test_queries_retain_current_rows_and_death_evidence_separately(self):
        statement = history.classification_sql()
        self.assertIn("FROM item_current_owner c", statement)
        self.assertNotIn("player_death_custody", statement)
        witness = history.witness_sql([7])
        self.assertIn("FROM player_death_custody d", witness)
        self.assertIn("LEFT JOIN player_death_disposition p", witness)
        self.assertIn("b.item_uid IS NULL AND l.item_uid IS NULL", statement)
        self.assertIn("LIMIT 100001", statement)
        current_line = "\t".join("NULL" if value is None else str(value)
                                 for value in current().values)
        death_line = "\t".join(str(value) for value in death(101, 101, 1))
        loaded = history.load_rows(lambda source: current_line if
                                   "FROM item_current_owner c" in source else death_line)
        self.assertEqual(loaded.current_rows, (current(),))
        self.assertEqual(loaded.death_rows, (death(101, 101, 1),))

    def test_missing_disposition_is_retained_without_becoming_a_witness(self):
        evidence = snapshot(current(), deaths=(death(101, 101, 1, operation=None),))
        self.assertEqual(history.classify(evidence)[0].category,
                         "revision_mismatch_unwitnessed")
        self.assertEqual(len(evidence.death_rows), 1)

    def test_normalized_artifact_serializes_each_death_source_once(self):
        evidence = snapshot(current(101), current(102),
                            deaths=(death(101, 101, 1), death(888, 101, 0)))
        cases = history.classify(evidence)
        payload = history.render_artifact("clone", "1" * 64, evidence, cases)
        lines = payload.decode().splitlines()
        self.assertEqual(lines[0], history.ARTIFACT_HEADER)
        self.assertEqual(len([line for line in lines if line.startswith("#")]), 4)
        self.assertEqual(len(lines), 10)
        self.assertIn(cases[0].evidence_id("1" * 64), payload.decode())
        self.assertEqual(payload, history.render_artifact(
            "clone", "1" * 64, evidence, cases))

    def test_duplicate_current_uid_and_malformed_source_fail_closed(self):
        with self.assertRaises(history.HistoryError):
            history.classify(snapshot(current(), current(revision=3)))
        with self.assertRaises(history.HistoryError):
            history.load_rows(lambda _statement: "101\tbad")

    @unittest.skipUnless(hasattr(os, "getuid"), "Unix owner-only permissions required")
    def test_protected_artifact_contains_all_evidence_and_no_overwrite(self):
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary) / "protected"
            directory.mkdir(mode=0o700)
            artifact = directory / "cases.tsv"
            evidence = snapshot(current(), deaths=(death(101, 101, 1),))
            cases = history.classify(evidence)
            digest = history.write_artifact(artifact, "clone", "1" * 64,
                                            evidence, cases)
            self.assertEqual(len(digest), 64)
            self.assertEqual(stat.S_IMODE(artifact.stat().st_mode), 0o600)
            self.assertIn(cases[0].evidence_id("1" * 64), artifact.read_text())
            self.assertIn("# death_custody_evidence", artifact.read_text())
            with self.assertRaises(history.HistoryError):
                history.write_artifact(artifact, "clone", "1" * 64,
                                       evidence, cases)


if __name__ == "__main__":
    unittest.main()
