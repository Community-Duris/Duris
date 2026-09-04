#!/usr/bin/env python3
"""Focused tests for semantic legacy membership reconciliation."""

from __future__ import annotations

import hashlib
import importlib.util
import json
import os
import subprocess
import sys
import tempfile
import unittest
from dataclasses import replace
from pathlib import Path
from unittest import mock

from _paths import ROOT

SPEC = importlib.util.spec_from_file_location(
    "reconcile_legacy_membership",
    ROOT / "scripts/reconcile_legacy_membership.py")
assert SPEC is not None and SPEC.loader is not None
reconcile = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = reconcile
SPEC.loader.exec_module(reconcile)


def digest(value: int) -> str:
    """Return a deterministic test-only SHA-256-shaped reference."""
    return f"{value:064x}"


def effects(**changes: str) -> dict[str, str]:
    """Build a complete downstream-effect review for one test row."""
    value = {name: "not_applicable" for name in reconcile.EFFECT_NAMES}
    value.update({
        "membership": "restore",
        "profile_display": "restore",
        "administrative_permissions": "preserve",
    })
    value.update(changes)
    return value


def definitions() -> dict[str, reconcile.Definition]:
    """Build current guild, association, rank, and obsolete definitions."""
    member = reconcile.Rank(digest(31), 2, False)
    leader = reconcile.Rank(digest(32), 6, True)
    guild_one = reconcile.Definition(
        digest(11), "guild", 1, True,
        {member.rank_ref: member, leader.rank_ref: leader})
    guild_two = reconcile.Definition(
        digest(12), "guild", 99, True, {member.rank_ref: member})
    association = reconcile.Definition(digest(21), "association", 8, True, {})
    obsolete = reconcile.Definition(digest(22), "association", 9, False, {})
    return {
        value.definition_ref: value
        for value in (guild_one, guild_two, association, obsolete)
    }


def row(index: int, *, domain: str = "guild", target: int = 11,
        **changes) -> reconcile.EvidenceRow:
    """Build one semantically matched reset row with optional replacements."""
    target_ref = digest(target)
    base = reconcile.EvidenceRow(
        digest(100 + index), digest(200 + index), domain, 1,
        digest(11) if domain == "guild" else digest(21), "current",
        digest(300 + index), frozenset({target_ref}), frozenset({target_ref}),
        digest(31) if domain == "guild" else None,
        effects(rank="restore") if domain == "guild" else effects(),
        (), digest(700 + index))
    return replace(base, **changes)


def evidence(*rows: reconcile.EvidenceRow) -> reconcile.Evidence:
    """Build one protected in-memory evidence packet."""
    return reconcile.Evidence(
        digest(1), digest(2), digest(3), digest(4), digest(5),
        definitions(), rows)


def disposition(item: reconcile.EvidenceRow, value: str) -> reconcile.Disposition:
    """Build one permanent decision bound to its current evidence row."""
    return reconcile.Disposition(item.row_ref, value, item.evidence_ref)


def issue_payload() -> dict:
    """Build the exact 176-association/190-guild CLI evidence fixture."""
    current_definitions = [
        {
            "definition_ref": digest(11), "domain": "guild",
            "numeric_id": 1, "active": True,
            "ranks": [{
                "rank_ref": digest(31), "rank_index": 2,
                "administrative": False,
            }],
        },
        {
            "definition_ref": digest(21), "domain": "association",
            "numeric_id": 8, "active": True, "ranks": [],
        },
    ]
    rows = []
    for index in range(176):
        rows.append({
            "row_ref": digest(1000 + index),
            "player_ref": digest(2000 + index),
            "domain": "association", "legacy_numeric_id": 21,
            "numeric_definition_ref": digest(21),
            "legacy_definition_state": "current",
            "legacy_name_ref": digest(3000 + index),
            "canonical_name_matches": [digest(21)],
            "membership_history_matches": [digest(21)],
            "requested_rank_ref": None, "effects": effects(),
            "current_authority": [], "evidence_ref": digest(7000 + index),
        })
    for index in range(190):
        rows.append({
            "row_ref": digest(4000 + index),
            "player_ref": digest(5000 + index),
            "domain": "guild", "legacy_numeric_id": 11,
            "numeric_definition_ref": digest(11),
            "legacy_definition_state": "current",
            "legacy_name_ref": digest(6000 + index),
            "canonical_name_matches": [digest(11)],
            "membership_history_matches": [digest(11)],
            "requested_rank_ref": digest(31),
            "effects": effects(rank="restore"),
            "current_authority": [], "evidence_ref": digest(8000 + index),
        })
    return {
        "version": 1, "source_stage_fingerprint": digest(1),
        "definitions_fingerprint": digest(2),
        "authority_snapshot_fingerprint": digest(3),
        "unchanged_target_fingerprint": digest(4),
        "cross_repo_contract_ref": digest(5),
        "current_definitions": current_definitions, "rows": rows,
    }


class LegacyMembershipReconciliationTest(unittest.TestCase):
    """Exercise semantic classification and fail-closed protected planning."""

    def test_same_number_different_guild_uses_semantics_not_numeric_id(self):
        """Ignore a colliding legacy number when independent semantics differ."""
        # The legacy number points at guild 11, while name and history prove
        # guild 12 (whose current numeric id is 99). Numeric equality must lose.
        item = row(1, target=12, numeric_definition_ref=digest(11))
        classified = reconcile.classify(evidence(item))
        self.assertEqual(classified[0].category, "uniquely_mappable")
        self.assertEqual(classified[0].target_ref, digest(12))
        actions, missing = reconcile.build_plan(
            evidence(item), classified, {})
        self.assertEqual(missing, 0)
        self.assertEqual(actions[0]["target_numeric_id"], 99)
        self.assertFalse(actions[0]["legacy_numeric_id_used_as_evidence"])

    def test_obsolete_definition_is_not_restored(self):
        """Keep an obsolete definition reset until permanent disposition."""
        item = row(
            2, domain="association", target=22,
            legacy_definition_state="obsolete",
            numeric_definition_ref=digest(22), requested_rank_ref=None)
        classified = reconcile.classify(evidence(item))
        self.assertEqual(classified[0].category, "obsolete")
        actions, missing = reconcile.build_plan(evidence(item), classified, {})
        self.assertEqual((actions, missing), ([], 1))

    def test_valid_semantic_association_match_is_planned(self):
        """Plan a uniquely proven association without changing prestige."""
        item = row(3, domain="association", target=21, requested_rank_ref=None)
        classified = reconcile.classify(evidence(item))
        self.assertEqual(classified[0].category, "uniquely_mappable")
        actions, missing = reconcile.build_plan(evidence(item), classified, {})
        self.assertEqual((actions[0]["action"], missing),
                         ("set_player_association", 0))
        self.assertEqual(actions[0]["prestige_action"], "unchanged")

    def test_existing_target_membership_and_rank_remain_authoritative(self):
        """Preserve an existing target membership and its current rank."""
        item = row(
            4, current_authority=(reconcile.Membership(digest(11), digest(32)),),
            effects=effects(membership="preserve", rank="preserve"))
        classified = reconcile.classify(evidence(item))
        actions, missing = reconcile.build_plan(evidence(item), classified, {})
        self.assertEqual((actions[0]["action"], missing),
                         ("keep_current_authority", 0))
        self.assertIsNone(actions[0]["target_rank_index"])

    def test_conflict_and_admin_restore_require_permanent_disposition(self):
        """Require evidence-bound dispositions for conflicts and admin ranks."""
        conflict = row(
            5, current_authority=(reconcile.Membership(digest(12), digest(31)),))
        admin = row(6, requested_rank_ref=digest(32))
        classified = reconcile.classify(evidence(conflict, admin))
        self.assertEqual(
            {item.category for item in classified},
            {"conflicted", "insufficient_evidence"})
        actions, missing = reconcile.build_plan(
            evidence(conflict, admin), classified, {})
        self.assertEqual((actions, missing), ([], 2))
        dispositions = {
            conflict.row_ref: disposition(conflict, "leave_unrestored"),
            admin.row_ref: disposition(admin, "player_support"),
        }
        actions, missing = reconcile.build_plan(
            evidence(conflict, admin), classified, dispositions)
        self.assertEqual((actions, missing), ([], 0))

    def test_repeat_planning_is_byte_stable(self):
        """Emit byte-identical plans from identical protected inputs."""
        item = row(7)
        source = evidence(item)
        classified = reconcile.classify(source)
        actions, missing = reconcile.build_plan(source, classified, {})
        self.assertEqual(missing, 0)
        with tempfile.TemporaryDirectory() as temporary:
            first = Path(temporary).resolve() / "first.json"
            second = Path(temporary).resolve() / "second.json"
            one = reconcile.write_plan(
                first, source, digest(501), None, classified, {}, actions)
            two = reconcile.write_plan(
                second, source, digest(501), None, classified, {}, actions)
            self.assertEqual(one, two)
            self.assertEqual(first.read_bytes(), second.read_bytes())
            self.assertEqual(os.stat(first).st_mode & 0o777, 0o600)
            self.assertEqual(hashlib.sha256(first.read_bytes()).hexdigest(), one)

    def test_planner_rejects_forged_classification_and_stale_disposition(self):
        """Bind plans to exact classifications and current row evidence."""
        unsafe = row(8, domain="association", target=22,
                     legacy_definition_state="obsolete",
                     numeric_definition_ref=digest(22), requested_rank_ref=None)
        source = evidence(unsafe)
        classified = reconcile.classify(source)
        forged = [replace(classified[0], category="uniquely_mappable",
                          target_ref=digest(21))]
        with self.assertRaisesRegex(reconcile.ReconciliationError, "exactly match"):
            reconcile.build_plan(source, forged, {})
        stale = replace(
            disposition(unsafe, "leave_unrestored"), evidence_ref=digest(999))
        with self.assertRaisesRegex(
                reconcile.ReconciliationError, "stale or mismatched"):
            reconcile.build_plan(source, classified, {unsafe.row_ref: stale})

    def test_rejected_private_artifact_closes_its_descriptor(self):
        """Close an opened artifact when its owner-only mode is rejected."""
        with tempfile.TemporaryDirectory() as temporary:
            artifact = Path(temporary).resolve() / "evidence.json"
            artifact.write_text("{}", encoding="utf-8")
            os.chmod(artifact, 0o644)  # noqa: S103 - deliberately unsafe fixture.
            descriptor = os.open(artifact, os.O_RDONLY)
            with mock.patch.object(reconcile.os, "open", return_value=descriptor):
                with self.assertRaisesRegex(
                        reconcile.ReconciliationError, "owner-only regular file"):
                    reconcile._read_protected_json(artifact, "reconciliation evidence")
            with self.assertRaises(OSError):
                os.fstat(descriptor)

    def test_cli_enforces_complete_issue_counts_and_writes_protected_plan(self):
        """Accept the complete issue counts and write a protected plan."""
        payload = issue_payload()
        with tempfile.TemporaryDirectory() as temporary:
            evidence_path = Path(temporary).resolve() / "evidence.json"
            plan_path = Path(temporary).resolve() / "plan.json"
            evidence_path.write_text(json.dumps(payload), encoding="utf-8")
            os.chmod(evidence_path, 0o600)
            completed = subprocess.run(
                [sys.executable, str(ROOT / "scripts/reconcile_legacy_membership.py"),
                 "--evidence", str(evidence_path.resolve()),
                 "--plan-output", str(plan_path.resolve())],
                check=False, capture_output=True, text=True)
            self.assertEqual(completed.returncode, 0, completed.stderr)
            report = json.loads(completed.stdout)
            self.assertEqual(report["status"], "ready")
            self.assertEqual(report["domains"]["association"]["rows"], 176)
            self.assertEqual(report["domains"]["guild"]["rows"], 190)
            self.assertEqual(os.stat(plan_path).st_mode & 0o777, 0o600)

    def test_cli_blocks_short_counts_and_writes_no_plan(self):
        """Report a short evidence set as blocked and refuse plan output."""
        payload = issue_payload()
        payload["rows"].pop()
        with tempfile.TemporaryDirectory() as temporary:
            evidence_path = Path(temporary).resolve() / "evidence.json"
            plan_path = Path(temporary).resolve() / "plan.json"
            evidence_path.write_text(json.dumps(payload), encoding="utf-8")
            os.chmod(evidence_path, 0o600)
            command = [
                sys.executable,
                str(ROOT / "scripts/reconcile_legacy_membership.py"),
                "--evidence", str(evidence_path),
            ]
            report_run = subprocess.run(
                command, check=False, capture_output=True, text=True)
            self.assertEqual(report_run.returncode, 2, report_run.stderr)
            self.assertEqual(json.loads(report_run.stdout)["status"], "blocked")

            plan_run = subprocess.run(
                [*command, "--plan-output", str(plan_path)],
                check=False, capture_output=True, text=True)
            self.assertEqual(plan_run.returncode, 2)
            self.assertIn("blocked by counts", plan_run.stderr)
            self.assertFalse(plan_path.exists())


if __name__ == "__main__":
    unittest.main()
