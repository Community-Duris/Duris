#!/usr/bin/env python3
"""Focused synthetic regressions for the personal-data export contract."""

from __future__ import annotations

import copy
import json
import os
import sys
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
import personal_data_export as export  # noqa: E402


class PersonalDataExportTest(unittest.TestCase):
    def setUp(self) -> None:
        canonical = export.load_policy()
        manifest = copy.deepcopy(canonical.manifest)
        manifest["export_policy"].update({
            "status": "approved",
            "reference": "TEST-SHARED-DISCLOSURE-APPROVAL",
            "shared_disclosure_enabled": True,
        })
        entries = {entry["id"]: entry for entry in manifest["entries"]}
        for entry in entries.values():
            entry["export_rule"] = {
                "disposition": "exclude",
                "subject_route": "not_required",
                "decision": {"status": "approved", "reference": "TEST-EXCLUDE"},
                "excluded_fields": entry["export_rule"]["excluded_fields"],
                "shared_fields": [],
            }
        entries["database:accounts"]["export_rule"] = {
            "disposition": "include",
            "subject_route": "direct_account",
            "decision": {"status": "approved", "reference": "TEST-INCLUDE"},
            "excluded_fields": ["confirmation_code", "password"],
            "shared_fields": [],
        }
        entries["database:offline_messages"]["export_rule"] = {
            "disposition": "shared_redacted",
            "subject_route": "shared_subject",
            "decision": {"status": "approved", "reference": "TEST-SHARED"},
            "excluded_fields": ["body"],
            "shared_fields": ["direction", "message_id", "occurred_at"],
        }
        self.snapshot = export.ExportPolicySnapshot(manifest, entries, "a" * 64)
        self.gate = export.ReauthenticationGate(b"t" * 32)

    @staticmethod
    def verifier(account: str, password: bytes) -> bool:
        return account.casefold() == "tester" and password == b"correct-password"

    def create(self, now: int = 1000) -> tuple[export.ExportRequest, bytes]:
        password = bytearray(b"correct-password")
        request, token = export.create_request(
            self.snapshot, self.gate, "Tester", password, "request-key-0001",
            now, self.verifier,
        )
        self.assertEqual(password, bytearray(len(password)))
        return request, token

    def complete(self, request: export.ExportRequest, token: bytes) -> bytes:
        for store_id in sorted(self.snapshot.entries):
            if store_id == "database:accounts":
                records = [{"account_name": "Tester", "email": "t@example.test",
                            "password": "must-not-leak", "confirmation_code": "no"}]
            elif store_id == "database:offline_messages":
                records = [{"message_id": 7, "direction": "received",
                            "occurred_at": "2026-08-27T00:00:00Z", "body": "private",
                            "other_account": "unrelated"}]
            else:
                records = []
            export.add_section_batch(
                self.snapshot, request, token, store_id, records,
                request.snapshot_id, True,
            )
        return export.build_bundle(self.snapshot, request, token)

    def test_canonical_policy_is_blocked_and_exactly_covered(self) -> None:
        canonical = export.load_policy()
        self.assertEqual(len(canonical.entries), 195)
        with self.assertRaisesRegex(export.ExportContractError, "not approved"):
            export.validate_export_ready(canonical)
        self.assertEqual(
            set(canonical.entries), {entry["id"] for entry in canonical.manifest["entries"]}
        )

    def test_reauthentication_ownership_rate_and_cooldown(self) -> None:
        password = bytearray(b"wrong")
        with self.assertRaisesRegex(export.ExportContractError, "failed"):
            export.create_request(
                self.snapshot, self.gate, "Tester", password, "request-key-0001",
                1000, self.verifier,
            )
        self.assertEqual(password, bytearray(len(password)))
        request, token = self.create(1001)
        self.assertFalse(request.owns(b"x" * 32))
        with self.assertRaisesRegex(export.ExportContractError, "cooldown"):
            export.create_request(
                self.snapshot, self.gate, "Tester", bytearray(b"correct-password"),
                "request-key-0002", 1002, self.verifier,
            )
        first_id = request.request_id
        fresh_gate = export.ReauthenticationGate(b"t" * 32)
        replay, _ = export.create_request(
            self.snapshot, fresh_gate, "tester", bytearray(b"correct-password"),
            "request-key-0001", 2000, self.verifier,
        )
        self.assertEqual(replay.request_id, first_id)
        self.assertTrue(request.owns(token))

    def test_bundle_is_deterministic_filtered_and_tamper_evident(self) -> None:
        request, token = self.create()
        payload = self.complete(request, token)
        bundle = export.verify_bundle(payload)
        self.assertEqual(bundle["section_count"], 195)
        self.assertEqual(bundle["record_count"], 2)
        accounts = next(section for section in bundle["sections"]
                        if section["store_id"] == "database:accounts")
        self.assertNotIn("password", accounts["records"][0])
        shared = next(section for section in bundle["sections"]
                      if section["store_id"] == "database:offline_messages")
        self.assertEqual(set(shared["records"][0]),
                         {"direction", "message_id", "occurred_at"})
        tampered = json.loads(payload)
        tampered["record_count"] = 99
        with self.assertRaisesRegex(export.ExportContractError, "checksum"):
            export.verify_bundle(export.canonical_json(tampered))
        duplicate = payload[:-1] + b',"package_checksum":"duplicate"}'
        with self.assertRaisesRegex(export.ExportContractError, "duplicate bundle key"):
            export.verify_bundle(duplicate)
        malformed = json.loads(payload)
        malformed["sections"][0].pop("records")
        unsigned = dict(malformed)
        unsigned.pop("package_checksum")
        malformed["package_checksum"] = export.digest_hex(export.canonical_json(unsigned))
        with self.assertRaisesRegex(export.ExportContractError, "section shape"):
            export.verify_bundle(export.canonical_json(malformed))

    def test_snapshot_bounds_and_non_string_keys_fail_closed(self) -> None:
        request, token = self.create()
        with self.assertRaisesRegex(export.ExportContractError, "snapshot"):
            export.add_section_batch(
                self.snapshot, request, token, "database:accounts", [], "changed", True
            )
        request, token = self.create(5000)
        oversized = [{"account_name": str(index)} for index in range(257)]
        with self.assertRaisesRegex(export.ExportContractError, "row batch"):
            export.add_section_batch(
                self.snapshot, request, token, "database:accounts", oversized,
                request.snapshot_id, True,
            )
        with self.assertRaisesRegex(export.ExportContractError, "keys must be strings"):
            export.filter_record({1: "invalid"},
                                 self.snapshot.entries["database:accounts"]["export_rule"])

    def test_private_atomic_one_time_spool_cancel_and_expiry(self) -> None:
        request, token = self.create()
        payload = self.complete(request, token)
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary) / "exports"
            spool = export.ExportSpool(root)
            self.assertEqual(os.stat(root).st_mode & 0o777, 0o700)
            spool.publish(request, token, payload, 1001)
            spool.publish(request, token, payload, 1001)
            assert request.package_path is not None
            self.assertEqual(os.stat(request.package_path).st_mode & 0o777, 0o600)
            with self.assertRaisesRegex(export.ExportContractError, "ownership"):
                spool.retrieve_once(request, b"x" * 32, 1002)
            self.assertEqual(spool.retrieve_once(request, token, 1002), payload)
            with self.assertRaisesRegex(export.ExportContractError, "not available"):
                spool.retrieve_once(request, token, 1003)

            cancelled, cancelled_token = self.create(5000)
            cancelled_payload = self.complete(cancelled, cancelled_token)
            spool.publish(cancelled, cancelled_token, cancelled_payload, 5001)
            cancelled_path = cancelled.package_path
            spool.cancel(cancelled, cancelled_token)
            self.assertFalse(cancelled_path.exists())
            self.assertFalse(cancelled.sections)

            expired, expired_token = self.create(9000)
            expired_payload = self.complete(expired, expired_token)
            spool.publish(expired, expired_token, expired_payload, 9001)
            self.assertTrue(spool.expire(expired, expired.expires_at))
            self.assertEqual(expired.status, export.RequestStatus.EXPIRED)

    def test_symlink_spool_and_redacted_status(self) -> None:
        request, _ = self.create()
        status = request.redacted_status()
        self.assertNotIn("account_scope_hash", status)
        self.assertNotIn("owner_token_hash", status)
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            real = directory / "real"
            real.mkdir()
            link = directory / "link"
            link.symlink_to(real, target_is_directory=True)
            with self.assertRaisesRegex(export.ExportContractError, "real directory"):
                export.ExportSpool(link)


if __name__ == "__main__":
    unittest.main()
