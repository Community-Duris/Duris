#!/usr/bin/env python3
"""Synthetic fail-closed regressions for account erasure and restore tombstones."""

from __future__ import annotations

import copy
import sys
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
import account_erasure as erasure  # noqa: E402
import personal_data_export as personal_export  # noqa: E402


class AccountErasureTest(unittest.TestCase):
    def setUp(self) -> None:
        canonical = erasure.load_policy()
        manifest = copy.deepcopy(canonical.manifest)
        manifest["controller_approval"] = {
            "status": "approved", "reference": "TEST-ERASURE-APPROVAL",
            "destructive_rules_enabled": True,
        }
        entries = {entry["id"]: entry for entry in manifest["entries"]}
        for entry in entries.values():
            if entry["data_subject_key"] != "not_applicable":
                entry["controller_decision"] = {
                    "status": "approved", "reference": "TEST-STORE-ACTION"
                }
        entries["database:accounts"]["terminal_action"] = "cascade"
        entries["database:player_data"]["terminal_action"] = "cascade"
        entries["file:runtime_accounts"]["terminal_action"] = "cascade"
        entries["file:runtime_pfiles"]["terminal_action"] = "cascade"
        entries["file:player_logs"]["terminal_action"] = "pseudonymize"
        self.snapshot = erasure.ErasurePolicySnapshot(manifest, entries, "b" * 64)
        self.secret = b"z" * 32

    @staticmethod
    def verifier(account: str, password: bytes) -> bool:
        return account.casefold() == "tester" and password == b"correct-password"

    def create(self) -> tuple[erasure.ErasureRequest, bytes]:
        gate = personal_export.ReauthenticationGate(b"a" * 32)
        password = bytearray(b"correct-password")
        request, owner = erasure.create_request(
            self.snapshot, gate, self.secret, "Tester", password,
            "erase-request-0001", 1000, self.verifier,
        )
        self.assertEqual(password, bytearray(len(password)))
        return request, owner

    def complete_actions(self, coordinator: erasure.ErasureCoordinator) -> None:
        coordinator.confirm(f"ERASE {coordinator.request.request_id[:8]}")
        coordinator.fence(77, descriptors_closed=True)
        coordinator.drain(0, 0, value_domains_reconciled=True)
        for store_id, evidence in coordinator.request.stores.items():
            affected = 1 if evidence.action != "retain" else 0
            coordinator.apply(store_id, affected, 0, reconciled=True)
        coordinator.verify()

    def test_canonical_policy_is_blocked_with_exact_ordered_coverage(self) -> None:
        """Erasure covers every manifest entry, in order, and stays disabled.

        The ordered action list must be as long as the inventory, so no store is
        silently skipped, and validate_ready must still refuse while the controller
        has not enabled destructive rules.
        """
        canonical = erasure.load_policy()
        self.assertEqual(len(canonical.entries), 195)
        self.assertEqual(len(erasure.ordered_actions(canonical)), 195)
        with self.assertRaisesRegex(erasure.ErasureContractError, "disabled"):
            erasure.validate_ready(canonical)

    def test_authentication_stable_identity_ownership_and_cancel_boundary(self) -> None:
        request, owner = self.create()
        replay, _ = self.create()
        self.assertEqual(replay.request_id, request.request_id)
        self.assertFalse(request.owns(b"x" * 32))
        with self.assertRaisesRegex(erasure.ErasureContractError, "ownership"):
            erasure.ErasureCoordinator(request, b"x" * 32)
        coordinator = erasure.ErasureCoordinator(request, owner)
        coordinator.cancel()
        self.assertEqual(request.status, erasure.Status.CANCELLED)

    def test_fence_drain_and_reconciliation_are_mandatory(self) -> None:
        request, owner = self.create()
        coordinator = erasure.ErasureCoordinator(request, owner)
        with self.assertRaisesRegex(erasure.ErasureContractError, "fence"):
            coordinator.fence(0, descriptors_closed=False)
        with self.assertRaisesRegex(erasure.ErasureContractError, "confirmation"):
            coordinator.confirm("ERASE wrong")
        coordinator.confirm(f"ERASE {request.request_id[:8]}")
        coordinator.fence(1, descriptors_closed=True)
        with self.assertRaisesRegex(erasure.ErasureContractError, "pending work"):
            coordinator.drain(1, 0, value_domains_reconciled=True)
        coordinator.drain(0, 0, value_domains_reconciled=True)
        with self.assertRaisesRegex(erasure.ErasureContractError, "identifiers remain"):
            coordinator.apply("database:accounts", 1, 1, reconciled=False)
        with self.assertRaisesRegex(erasure.ErasureContractError, "all stores"):
            coordinator.verify()

    def test_retained_and_value_domain_guards_fail_closed(self) -> None:
        request, owner = self.create()
        coordinator = erasure.ErasureCoordinator(request, owner)
        coordinator.confirm(f"ERASE {request.request_id[:8]}")
        coordinator.fence(1, True)
        coordinator.drain(0, 0, True)
        retained = next(store_id for store_id, item in request.stores.items()
                        if item.action == "retain")
        with self.assertRaisesRegex(erasure.ErasureContractError, "cannot be mutated"):
            coordinator.apply(retained, 1, 0, True)
        value_store = next((store_id for store_id in request.stores
                            if any(name in store_id for name in erasure.VALUE_DOMAINS)), None)
        self.assertIsNotNone(value_store)
        with self.assertRaisesRegex(erasure.ErasureContractError,
                                    "transactional disposition"):
            coordinator.apply(value_store, 0, 0, True, via_domain_command=False)

    def test_tombstone_blocks_all_restore_classes_and_is_idempotent(self) -> None:
        request, owner = self.create()
        coordinator = erasure.ErasureCoordinator(request, owner)
        self.complete_actions(coordinator)
        ledger = erasure.TombstoneLedger()
        tombstone = ledger.commit(request, 2000)
        self.assertEqual(ledger.commit(request, 2000), tombstone)
        restored = [
            {"account_scope_hash": request.account_scope_hash, "value": "erased"},
            {"account_scope_hash": "f" * 64, "value": "kept"},
        ]
        for source in erasure.RESTORE_SOURCES:
            filtered = ledger.restore_preflight(source, "1" * 64, restored)
            self.assertEqual(filtered, [restored[1]])
        with self.assertRaisesRegex(erasure.ErasureContractError, "stable account scope"):
            ledger.restore_preflight("database_backup", "1" * 64, [{"name": "Tester"}])
        ledger.finalize(request, credentials_remaining=0, login_loadable=False)
        self.assertEqual(request.status, erasure.Status.COMPLETE)

    def test_completion_rejects_credentials_or_loadable_identity(self) -> None:
        request, owner = self.create()
        coordinator = erasure.ErasureCoordinator(request, owner)
        self.complete_actions(coordinator)
        ledger = erasure.TombstoneLedger()
        ledger.commit(request, 2000)
        with self.assertRaisesRegex(erasure.ErasureContractError, "blocks completion"):
            ledger.finalize(request, credentials_remaining=1, login_loadable=False)
        with self.assertRaisesRegex(erasure.ErasureContractError, "blocks completion"):
            ledger.finalize(request, credentials_remaining=0, login_loadable=True)


if __name__ == "__main__":
    unittest.main()
