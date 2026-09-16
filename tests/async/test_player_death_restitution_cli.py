#!/usr/bin/env python3
"""Focused regression tests for the issue-331 restitution CLI."""

from __future__ import annotations

import importlib.util
import json
import os
from pathlib import Path
import tempfile
import unittest
from unittest import mock


ROOT = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location(
    "player_death_restitution", ROOT / "scripts" / "player_death_restitution.py"
)
assert SPEC is not None and SPEC.loader is not None
cli = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(cli)


class ScalarDB:
    def __init__(self, values: list[str]) -> None:
        self.database = "duris_issue_331_test"
        self.values = iter(values)
        self.queries: list[str] = []

    def scalar(self, query: str) -> str:
        self.queries.append(query)
        return next(self.values)



def payload_item(uid: int, parent_index: int, vnum: int = 100) -> dict[str, object]:
    return {
        "object_uid": uid,
        "parent_index": parent_index,
        "vnum": vnum,
        "item_payload_hex": "00",
    }


class RestitutionCliTests(unittest.TestCase):
    def test_legacy_quiescence_file_is_not_an_attestation(self) -> None:
        db = ScalarDB([])
        with tempfile.TemporaryDirectory() as directory:
            proof = Path(directory) / "proof"
            proof.write_text(
                "\n".join(
                    [
                        "format=duris-death-restitution-quiescence-v1",
                        f"database={db.database}",
                        "server_state=stopped",
                        "writers=drained",
                        "ownership_work=drained",
                        "currency_work=drained",
                        "expires_at=2099-01-01T00:00:00+00:00",
                    ]
                )
                + "\n",
                encoding="utf-8",
            )
            os.chmod(proof, 0o600)
            with self.assertRaises(cli.ToolError):
                cli.check_quiescence(db, proof)
            self.assertEqual(db.queries, [])

    @mock.patch.object(cli, "check_process_inventory")
    def test_v3_context_checks_process_and_database_state(self, process_inventory: mock.Mock) -> None:
        # Format/context test only. Real runtime exclusion is exercised by the
        # native factory/pool and guarded disposable-DB integration tests.
        db = ScalarDB(["1", "0", "0", "0", "0", "0", "0"])
        with tempfile.TemporaryDirectory() as directory:
            proof = Path(directory) / "proof"
            proof.write_text("\n".join([
                f"format={cli.QUIESCENCE_PROOF_FORMAT}",
                f"database={db.database}",
                f"boundary={cli.QUIESCENCE_BOUNDARY}",
                f"guard={cli.RUNTIME_EXCLUSION_LOCK_PREFIX}",
                "expires_at=2099-01-01T00:00:00+00:00",
            ]) + "\n", encoding="utf-8")
            os.chmod(proof, 0o600)
            cli.check_quiescence(db, proof)
        process_inventory.assert_called_once_with()
        self.assertEqual(len(db.queries), 7)

    def test_quiescence_rejects_transient_writer_sample(self) -> None:
        db = ScalarDB(["1", "0", "1"])
        with self.assertRaisesRegex(cli.ToolError, "active database writers"):
            cli.check_database_quiescence(db)
        self.assertEqual(len(db.queries), 3)

    @mock.patch.object(cli, "ensure_codec", return_value=Path("/unused-codec"))
    @mock.patch.object(cli.subprocess, "run")
    def test_normalized_native_codec_death_versions(self, run: mock.Mock, _codec: mock.Mock) -> None:
        # This tests the Python acceptance boundary, not native byte decoding.
        # Native wire-2 decoding is separately exercised on retained evidence.
        for wire in (2, 4, 6):
            with self.subTest(wire=wire):
                run.return_value = mock.Mock(returncode=0, stdout=json.dumps({
                    "wire_version": wire, "schema_version": 6,
                }).encode())
                self.assertEqual(cli.decode_payload(b"fixture")["wire_version"], wire)
        for wire, schema in ((1, 5), (3, 5), (5, 5), (2, 5), (99, 6)):
            with self.subTest(wire=wire, schema=schema):
                run.return_value = mock.Mock(returncode=0, stdout=json.dumps({
                    "wire_version": wire, "schema_version": schema,
                }).encode())
                with self.assertRaises(cli.ToolError):
                    cli.decode_payload(b"fixture")

    def test_normalized_related_payload_is_reused_and_deduplicated(self) -> None:
        selected = [payload_item(900, -1)]
        earlier = payload_item(100, 0)
        normalized_earlier, _, _, _, _ = cli.normalize_payload_items([selected[0], earlier])
        earlier = normalized_earlier[0]
        inspection = {
            "decoded": {"death": {"corpse": selected}},
            "related_deaths": [
                {"payload_items": [earlier], "payload_order": ["100"]},
                {"payload_items": [dict(earlier)], "payload_order": ["100"]},
            ],
        }
        items, parents, roots, order, conflicts = cli.payload_evidence_maps(inspection)
        self.assertEqual(order, ["100"])
        self.assertEqual(conflicts, set())
        self.assertEqual(items["100"]["parent_item_uid"], 0)
        self.assertEqual(items["100"]["root_item_uid"], 100)
        self.assertEqual(parents["100"], 0)
        self.assertEqual(roots["100"], 100)

    def test_conflicting_normalized_related_payload_refuses_uid(self) -> None:
        selected = [payload_item(900, -1)]
        first = payload_item(100, 0, 100)
        second = payload_item(100, 0, 101)
        first_normalized, _, _, _, _ = cli.normalize_payload_items([selected[0], first])
        second_normalized, _, _, _, _ = cli.normalize_payload_items([selected[0], second])
        first = first_normalized[0]
        second = second_normalized[0]
        inspection = {
            "decoded": {"death": {"corpse": selected}},
            "related_deaths": [
                {"payload_items": [first], "payload_order": ["100"]},
                {"payload_items": [second], "payload_order": ["100"]},
            ],
        }
        _, _, _, _, conflicts = cli.payload_evidence_maps(inspection)
        self.assertEqual(conflicts, {"100"})

    def test_identity_mismatched_related_payload_marks_all_seen_uids_conflicting(self) -> None:
        inspection = {
            "decoded": {"death": {"corpse": [payload_item(900, -1), payload_item(100, 0)]}},
            "related_deaths": [
                {
                    "identity_error": "payload identity does not match SQL row",
                    "payload_uids": [100],
                }
            ],
        }
        _, _, _, _, conflicts = cli.payload_evidence_maps(inspection)
        self.assertEqual(conflicts, {"100"})

    def test_artifact_location_is_fenced_and_unique_gloves_are_not_artifacts(self) -> None:
        item = {"object_uid": 1004, "vnum": 104}
        current = {"item_revision": 11}
        domain = {
            "vnum": 104,
            "owned": 1,
            "loc_type": 5,
            "location": 999,
            "timer_epoch": 123456,
            "artifact_type": 2,
            "bind_owner_pid": 42,
            "bind_timer_epoch": 654321,
            "item_uid": 1004,
            "item_revision": 11,
            "revision": 4,
        }
        artifacts = {"domain": {"104": domain}, "bind": {}, "mortal": {}, "god": {}}
        classification, _, _, _ = cli.artifact_authority_fence(item, 42, artifacts, current)
        self.assertEqual(classification, "artifact_legacy_conflict")
        unique = {
            "object_uid": 1003,
            "vnum": 103,
            "type": 4,
            "extra_flags": 0,
            "name_hex": b"unique gloves".hex(),
        }
        self.assertEqual(cli.item_kind(unique, {}), "unique")

    def test_artifact_timing_compensation_is_scoped_to_explicit_uids(self) -> None:
        parsed = cli.parse_artifact_timing_compensation_specs([
            "1004=456:ticket-1004",
            "1005=789:ticket-1005",
        ])
        self.assertEqual(parsed, {
            "1004": {"seconds": 456, "approval": "ticket-1004"},
            "1005": {"seconds": 789, "approval": "ticket-1005"},
        })
        with self.assertRaises(cli.ToolError):
            cli.parse_artifact_timing_compensation_specs(["1004=456:ticket", "1004=789:other"])
        with self.assertRaises(cli.ToolError):
            cli.parse_artifact_timing_compensation_specs(["456:global-approval"])

    def test_timing_compensation_plan_map_must_match_eligible_artifacts(self) -> None:
        plan = {
            "artifact_timing_compensations": {
                "1004": {"seconds": 456, "approval": "ticket-1004"},
            },
            "artifact_timing_compensation_count": 1,
            "items": [{
                "item_uid": 1004,
                "eligible": True,
                "kind": "artifact",
                "artifact_timing": {
                    "basis": "approved_compensation",
                    "usable_lifetime_seconds": 456,
                    "compensation_reference": "ticket-1004",
                },
            }],
        }
        cli.validate_artifact_timing_compensations(plan)
        plan["artifact_timing_compensations"] = {
            "1004": {"seconds": 456, "approval": "ticket-1004"},
            "1005": {"seconds": 789, "approval": "ticket-1005"},
        }
        plan["artifact_timing_compensation_count"] = 2
        with self.assertRaisesRegex(cli.ToolError, "UID map"):
            cli.validate_artifact_timing_compensations(plan)

    def test_apply_sql_uses_only_dedicated_restitution_audit(self) -> None:
        metadata = {
            "object_uid": 100,
            "vnum": 100,
            "type": 1,
            "string_mask": 0,
            "values": [0] * 8,
            "bitvectors": [0] * 5,
            "weight": 1,
            "cost": 1,
            "timers": [0] * 6,
            "extra_flags": 0,
            "wear_flags": 0,
            "material": 1,
            "condition": 1,
            "affects": [],
            "extra_descriptions": [],
            "item_payload_hex": "00",
        }
        row = {
            "item_uid": 100,
            "eligible": True,
            "classification": "recoverable_exact",
            "disposition": 1,
            "kind": "normal",
            "vnum": 100,
            "metadata": metadata,
            "metadata_digest": "11" * 32,
            "metadata_payload_hex": "00",
            "source_root_item_uid": 100,
            "source_parent_item_uid": 0,
            "source_item_revision": 11,
            "delivered_root_item_uid": 100,
            "delivered_parent_item_uid": 0,
            "expected_current": {
                "root_item_uid": 100,
                "parent_item_uid": 0,
                "item_revision": 11,
                "owner_revision": 6,
                "state": 3,
            },
            "note": "test",
        }
        plan = {
            "source": {"pid": 42, "death_revision": 7, "operation_id_hex": "aa" * 16},
            "recipient_pid": 42,
            "restitution_id_hex": "bb" * 16,
            "evidence_digest": "cc" * 32,
            "plan_digest": "dd" * 32,
            "payload_digest": "ee" * 32,
            "items": [row],
        }
        sql = cli.build_apply_sql(plan, "test", "focused-test")
        self.assertNotIn("critical_operation_inbox", sql)
        self.assertNotIn("item_ownership_ledger", sql)
        self.assertIn("player_death_restitution_receipt", sql)
        self.assertIn("@database_quiescent", sql)
        self.assertIn("FOR UPDATE", sql)
        self.assertIn("@owner_rows_updated=1", sql)
        self.assertIn("@restitution_decision", sql)


if __name__ == "__main__":
    unittest.main()
