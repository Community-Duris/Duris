#!/usr/bin/env python3
"""Regression and invariant tests for audit_accounting_invariants.py."""

import copy
import json
from pathlib import Path
import subprocess
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
DOCS = ROOT / "docs/persistence/economy_accounting"

sys.path.insert(0, str(ROOT / "scripts"))
from audit_accounting_invariants import AccountingInvariantAuditor, AuditError


class TestAccountingInvariants(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.registry = json.loads((DOCS / "registry.json").read_text(encoding="utf-8"))
        cls.golden = json.loads((DOCS / "golden.json").read_text(encoding="utf-8"))
        cls.auditor = AccountingInvariantAuditor(cls.registry)

    def test_golden_fixtures_pass_cleanly(self):
        fixtures = self.golden.get("fixtures", [])
        self.assertGreater(len(fixtures), 0)
        for fix in fixtures:
            stats = self.auditor.audit_fixture(fix, fix.get("id", "test"))
            self.assertGreaterEqual(stats["operations_checked"], 0)

    def test_cli_execution_succeeds(self):
        res = subprocess.run(
            [sys.executable, str(ROOT / "scripts/audit_accounting_invariants.py"), "-v"],
            capture_output=True,
            text=True,
            check=False,
        )
        self.assertEqual(res.returncode, 0, f"CLI failed: {res.stderr}")
        self.assertIn("Audit PASSED", res.stdout)

    def test_detects_non_zero_sum_violation(self):
        # Take the wallet_bank fixture and corrupt one posting so it does not sum to zero
        fix = copy.deepcopy(self.golden["fixtures"][0])
        self.assertEqual(fix["id"], "wallet_bank")
        fix["operations"][0]["postings"][0]["delta"][3] -= 5  # unbalance copper
        with self.assertRaises(AuditError) as ctx:
            self.auditor.audit_fixture(fix, "corrupted_zero_sum")
        self.assertIn("Double-entry violation", str(ctx.exception))

    def test_detects_replayed_source_event(self):
        # Find reward fixture which requires a source event
        reward_fix = next(f for f in self.golden["fixtures"] if f["id"] == "reward")
        fix = copy.deepcopy(reward_fix)
        # Duplicate the operation with different operation_id but same source_event
        op2 = copy.deepcopy(fix["operations"][0])
        op2["operation_id"] = "ffffffffffffffffffffffffffffffff"
        fix["operations"].append(op2)
        with self.assertRaises(AuditError) as ctx:
            self.auditor.audit_fixture(fix, "replayed_source_event")
        self.assertIn(f"Source event {op2['source_event']} replayed in op "
                      f"{op2['operation_id']}", str(ctx.exception))

    def test_detects_conflicting_duplicate_operation(self):
        fix = copy.deepcopy(self.golden["fixtures"][0])
        op2 = copy.deepcopy(fix["operations"][0])
        op2["actor"] = "different_actor"
        fix["operations"].append(op2)
        with self.assertRaises(AuditError) as ctx:
            self.auditor.audit_fixture(fix, "conflicting_dup_op")
        self.assertIn("conflicting payload", str(ctx.exception))

    def test_detects_cyclic_custody(self):
        fix = {
            "holdings": {},
            "custody": {
                "100": {"parent": 200, "root": 200},
                "200": {"parent": 100, "root": 100},
            },
            "operations": [],
        }
        with self.assertRaises(AuditError) as ctx:
            self.auditor.audit_fixture(fix, "cyclic_custody")
        self.assertIn("Cyclic container hierarchy", str(ctx.exception))

    def test_detects_self_referential_child(self):
        fix = copy.deepcopy(self.golden["fixtures"][0])
        op = fix["operations"][0]
        op["children"] = [{"operation_id": op["operation_id"], "parent_id": op["operation_id"]}]
        with self.assertRaises(AuditError) as ctx:
            self.auditor.audit_fixture(fix, "self_child")
        self.assertIn("Self-referential child operation", str(ctx.exception))

    def test_detects_negative_opening_balance(self):
        fix = copy.deepcopy(self.golden["fixtures"][0])
        fix["holdings"]["wallet"]["balance"] = [0, 0, -1, 0]
        with self.assertRaises(AuditError) as ctx:
            self.auditor.audit_fixture(fix, "negative_balance")
        self.assertIn("Negative opening balance", str(ctx.exception))

    def test_detects_unauthorized_account_kind(self):
        fix = copy.deepcopy(self.golden["fixtures"][0])
        # Change bank to sink for bank_transfer which only allows ordinary currency kinds
        fix["holdings"]["bank"]["kind"] = "sink"
        with self.assertRaises(AuditError) as ctx:
            self.auditor.audit_fixture(fix, "unauthorized_kind")
        self.assertIn("not authorized for reason", str(ctx.exception))


    def test_multi_operation_accounting_fixture(self):
        """Audit one synthetic fixture with sequential postings across five domains:
        1. Quest reward (issuance -> wallet)
        2. Shop trade (wallet -> merchant, custody change)
        3. Auction bid & escrow (wallet -> auction_escrow)
        4. Outbid refund (auction_escrow -> wallet)
        5. Bank deposit (wallet -> bank)
        """
        journey_fixture = {
            "id": "e2e_qualification_journey",
            "lineage": "33333333333333333333333333333333",
            "epoch": "44444444444444444444444444444444",
            "holdings": {
                "player_wallet": {
                    "kind": "wallet",
                    "identity": 1001,
                    "balance": [0, 0, 0, 0],
                },
                "player_bank": {
                    "kind": "bank",
                    "identity": 1001,
                    "balance": [0, 0, 0, 0],
                },
                "auction_vault": {
                    "kind": "auction_escrow",
                    "identity": 5001,
                    "balance": [0, 0, 0, 0],
                },
                "reward_issuance": {
                    "kind": "issuance",
                    "identity": 9001,
                    "balance": [0, 0, 0, 0],
                },
                "shop_counterparty": {
                    "kind": "wallet",
                    "identity": 2002,
                    "balance": [0, 0, 0, 0],
                },
            },
            "custody": {
                "99999": {
                    "kind": "player",
                    "identity": 1001,
                    "parent": 0,
                    "root": 99999,
                }
            },
            "operations": [
                # Step 1: Quest reward gives 5 gold (500 copper) to player wallet
                {
                    "operation_id": "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
                    "reason": "quest_reward",
                    "actor": "domain",
                    "source_event": "e0000000000000000000000000000001",
                    "postings": [
                        {"account": "reward_issuance", "delta": [0, 0, -5, 0]},
                        {"account": "player_wallet", "delta": [0, 0, 5, 0]},
                    ],
                    "items": [],
                    "children": [],
                },
                # Step 2: Shop purchase of item 99999 for 2 gold (200 copper)
                {
                    "operation_id": "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb",
                    "reason": "shop_buy",
                    "actor": "domain",
                    "source_event": "e0000000000000000000000000000002",
                    "postings": [
                        {"account": "player_wallet", "delta": [0, 0, -2, 0]},
                        {"account": "shop_counterparty", "delta": [0, 0, 2, 0]},
                    ],
                    "items": [
                        {
                            "event_index": 0,
                            "operation_id": "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb",
                            "uid": 99999,
                            "action": "transfer",
                            "before": {
                                "kind": "player",
                                "identity": 1001,
                                "parent": 0,
                                "root": 99999,
                            },
                            "after": {
                                "kind": "shopkeeper",
                                "identity": 2002,
                                "parent": 0,
                                "root": 99999,
                            },
                        }
                    ],
                    "children": [],
                },
                # Step 3: Auction bid of 2 gold from player wallet into auction escrow
                {
                    "operation_id": "cccccccccccccccccccccccccccccccc",
                    "reason": "auction_bid",
                    "actor": "domain",
                    "source_event": "e0000000000000000000000000000003",
                    "postings": [
                        {"account": "player_wallet", "delta": [0, 0, -2, 0]},
                        {"account": "auction_vault", "delta": [0, 0, 2, 0]},
                    ],
                    "items": [],
                    "children": [],
                },
                # Step 4: Outbid refund returned from auction escrow to player wallet
                {
                    "operation_id": "dddddddddddddddddddddddddddddddd",
                    "reason": "auction_outbid",
                    "actor": "domain",
                    "source_event": "e0000000000000000000000000000004",
                    "postings": [
                        {"account": "auction_vault", "delta": [0, 0, -2, 0]},
                        {"account": "player_wallet", "delta": [0, 0, 2, 0]},
                    ],
                    "items": [],
                    "children": [],
                },
                # Step 5: Bank deposit of remaining 3 gold into player bank
                {
                    "operation_id": "eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee",
                    "reason": "bank_transfer",
                    "actor": "domain",
                    "source_event": None,
                    "postings": [
                        {"account": "player_wallet", "delta": [0, 0, -3, 0]},
                        {"account": "player_bank", "delta": [0, 0, 3, 0]},
                    ],
                    "items": [],
                    "children": [],
                },
            ],
        }

        # Validate that the entire synthetic journey preserves all double-entry and anti-duplication invariants
        stats = self.auditor.audit_fixture(journey_fixture, "e2e_qualification_journey")
        self.assertEqual(stats["operations_checked"], 5)
        self.assertEqual(stats["zero_sum_verified"], 5)
        self.assertEqual(stats["source_events_verified"], 4)
        self.assertEqual(stats["items_checked"], 1)

if __name__ == "__main__":
    unittest.main()
