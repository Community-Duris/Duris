#!/usr/bin/env python3
"""Focused regression tests for the issue-331 restitution CLI."""

from __future__ import annotations

import importlib.util
import json
import hashlib
import os
from pathlib import Path
import subprocess
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
    def test_native_raw_wire_and_normalized_death_schema_contract(
        self, run: mock.Mock, _codec: mock.Mock
    ) -> None:
        # This tests the Python acceptance boundary, not native byte decoding.
        # The native codec bridge emits both values: historical raw wires 2/4/6
        # and the current writer's raw wire 8 all normalize to death schema 8.
        self.assertEqual(cli.DEATH_NORMALIZED_SCHEMA_VERSION, 8)
        self.assertEqual(cli.DEATH_SCHEMA_VERSION, 8)
        for wire in (2, 4, 6, 8):
            with self.subTest(wire=wire):
                run.return_value = mock.Mock(returncode=0, stdout=json.dumps({
                    "wire_version": wire, "schema_version": 8,
                }).encode())
                decoded = cli.decode_payload(b"fixture")
                self.assertEqual(decoded["wire_version"], wire)
                self.assertEqual(decoded["schema_version"], 8)
        for wire, schema in (
            (1, 8), (3, 8), (5, 8), (7, 8), (99, 8),
            (2, 7), (6, 7), (8, 7),
        ):
            with self.subTest(wire=wire, schema=schema):
                run.return_value = mock.Mock(returncode=0, stdout=json.dumps({
                    "wire_version": wire, "schema_version": schema,
                }).encode())
                with self.assertRaisesRegex(cli.ToolError, "raw-wire.*schema-8"):
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

    def test_current_native_writer_reaches_python_inspection_gate(self) -> None:
        # Compile the tracked native fixture and the actual bridge in a private
        # temporary directory.  This catches the production mismatch where the
        # bridge returned raw wire 8 / normalized schema 8 but Python still
        # required the retired schema-6 label.
        fixture_source = ROOT / "tests" / "async" / "player_death_restitution_fixture.cpp"
        codec_source = ROOT / "src" / "player" / "player_snapshot_codec.c"
        bridge_source = ROOT / "scripts" / "player_death_restitution_codec.cpp"
        with tempfile.TemporaryDirectory(prefix="duris-504-codec-") as directory:
            fixture = Path(directory) / "fixture"
            bridge = Path(directory) / "bridge"
            for output, sources in (
                (fixture, (fixture_source, codec_source)),
                (bridge, (bridge_source, codec_source)),
            ):
                subprocess.run(
                    ["g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                     "-Isrc", "-I.", *(str(source) for source in sources), "-o", str(output)],
                    cwd=ROOT, check=True, capture_output=True, text=True,
                )
            fixture_output = subprocess.run(
                [str(fixture)], capture_output=True, check=True,
            ).stdout.decode("ascii")
            payloads = [bytes.fromhex(line) for line in fixture_output.splitlines()]
            self.assertEqual(len(payloads), 2)
            with mock.patch.object(cli, "ensure_codec", return_value=bridge):
                for payload in payloads:
                    decoded = cli.decode_payload(payload)
                    self.assertEqual(int.from_bytes(payload[:4], "little"), 8)
                    self.assertEqual(decoded["wire_version"], 8)
                    self.assertEqual(decoded["schema_version"], 8)
                    self.assertIsInstance(decoded["death"], dict)
            for bad_wire in (7, 99):
                corrupted = bytearray(payloads[0])
                corrupted[:4] = bad_wire.to_bytes(4, "little")
                rejected = subprocess.run(
                    [str(bridge), "decode-death"], input=bytes(corrupted),
                    capture_output=True, check=False,
                )
                self.assertNotEqual(rejected.returncode, 0)

    def test_native_export_is_not_gated_by_offline_sql_boundary(self) -> None:
        args = cli.parser().parse_args([
            "export", "--plan", "plan.json", "--inspect", "inspect.json",
            "--artifact", "staff.json", "--approve", "--actor", "staff",
            "--reason", "death-evidence",
        ])
        self.assertEqual(args.command, "export")
        self.assertFalse(hasattr(args, "offline_proof"))
        self.assertFalse(hasattr(args, "maintenance_kind"))

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

    def test_strict_native_comparison_rejects_changed_or_truncated_ist1(self) -> None:
        metadata = {
            "object_uid": 100,
            "vnum": 677,
            "type": 5,
            "string_mask": 0,
            "name_hex": "",
            "short_description_hex": "",
            "description_hex": "",
            "action_description_hex": "",
            "values": [0] * 8,
            "bitvectors": [0] * 5,
            "weight": 1,
            "cost": 1,
            "timers": [0],
            "extra_flags": 0,
            "wear_flags": 0,
            "material": 1,
            "condition": 100,
            "affects": [],
            "extra_descriptions": [],
            "item_payload_hex": "00",
        }
        native = cli._native_item_state_payload(metadata, 100, 0)
        self.assertEqual(native[:6], b"IST1\x01\x00")
        metadata_digest = hashlib.sha256(b"\x00").hexdigest()
        row = {
            "item_uid": 100,
            "eligible": True,
            "kind": "normal",
            "vnum": 677,
            "metadata": metadata,
            "metadata_digest": metadata_digest,
            "metadata_payload_hex": "00",
            "source_root_item_uid": 100,
            "source_parent_item_uid": 0,
            "source_item_revision": 11,
            "delivered_root_item_uid": 100,
            "delivered_parent_item_uid": 0,
            "equipment_slot": 0,
        }
        plan = {
            "source": {"pid": 42, "death_revision": 7, "operation_id_hex": "aa" * 16},
            "recipient_pid": 42,
            "restitution_id_hex": "bb" * 16,
            "evidence_digest": "cc" * 32,
            "plan_digest": "dd" * 32,
            "items": [row],
            "recipient_existing_uids": [],
        }
        receipt = {
            "source_pid": 42, "death_revision": 7, "recipient_pid": 42,
            "death_operation_id_hex": "aa" * 16, "evidence_digest": "cc" * 32,
            "plan_digest": "dd" * 32, "candidate_count": 1,
            "delivered_count": 1, "status": cli.RECEIPT_APPLIED,
        }
        delivery = [{
            "item_uid": 100, "metadata_digest": metadata_digest,
            "original_payload_hex": "00", "delivered_item_id": 900,
        }]
        player = {
            "id": 900, "pid": 42, "vnum": 677, "equip_slot": 0,
            "container_id": 0, "quantity": 1, "weight": 1, "cost": 1,
            "timer": 0, "extra_flags": 0, "wear_flags": 0, "type": 5,
            "values": [0] * 8, "name_hex": None, "short_description_hex": None,
            "description_hex": None, "action_description_hex": None,
            "bitvectors": [0] * 5, "object_uid": 100, "condition": 100,
        }
        owner = {
            "item_uid": 100, "root_item_uid": 100, "parent_item_uid": 0,
            "owner_type": cli.OWNER_PLAYER, "owner_id": 42,
            "owner_context_id": 0, "item_revision": 11, "vnum": 677,
            "state": cli.STATE_ACTIVE,
        }
        db = mock.Mock()
        db.policy = None
        db.scalar.return_value = "0"
        common = {
            "fetch_receipt": mock.Mock(return_value=receipt),
            "fetch_delivery_rows": mock.Mock(return_value=delivery),
            "fetch_restitution_item_rows": mock.Mock(return_value={}),
            "fetch_player_rows": mock.Mock(return_value={"100": player}),
            "fetch_current_owners": mock.Mock(return_value={"100": owner}),
            "fetch_item_metadata": mock.Mock(return_value=({}, {})),
            "fetch_runtime_state": mock.Mock(return_value={"100": native.hex()}),
            "fetch_recipient_uids": mock.Mock(return_value=[100]),
        }
        with mock.patch.multiple(cli, **common):
            ok, _, failures = cli.verify_plan(
                db, plan, policy=None,
            )
            self.assertTrue(ok, failures)
            changed = bytearray(native)
            changed[24] ^= 1
            for label, payload in (("changed-field", bytes(changed)), ("truncated", native[:-1])):
                with self.subTest(label=label):
                    self.assertEqual(payload[:6], native[:6])
                    with mock.patch.object(cli, "fetch_runtime_state", return_value={"100": payload.hex()}):
                        ok, _, failures = cli.verify_plan(db, plan, policy=None)
                    self.assertFalse(ok)
                    self.assertIn("exact runtime metadata payload differs", failures)

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
        # MySQL 8 cannot reopen one TEMPORARY table through two aliases in a
        # statement. Keep the full parent authority fence using a separate copy.
        self.assertIn("CREATE TEMPORARY TABLE restitution_apply_parents LIKE restitution_apply_items;", sql)
        self.assertIn("INSERT INTO restitution_apply_parents SELECT * FROM restitution_apply_items;", sql)
        parent_gate = next(line for line in sql.splitlines() if line.startswith("SET @parent_ok="))
        self.assertEqual(parent_gate.count("restitution_apply_items"), 1)
        self.assertIn("JOIN restitution_apply_parents pp", parent_gate)
        self.assertIn("parent.item_revision=pp.expected_item_revision", parent_gate)
        self.assertIn("pp.expected_state=3", parent_gate)
        self.assertIn("@restitution_decision", sql)
        epoch_marker = "SET @restitution_delivery_epoch=FLOOR(UNIX_TIMESTAMP(CURRENT_TIMESTAMP(6)));"
        self.assertEqual(sql.count(epoch_marker), 1)
        epoch_position = sql.index(epoch_marker)
        self.assertLess(sql.rfind("FOR UPDATE"), epoch_position)
        self.assertLess(epoch_position, sql.index("INSERT INTO player_death_restitution_receipt"))


if __name__ == "__main__":
    unittest.main()
