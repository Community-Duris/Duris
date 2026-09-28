#!/usr/bin/env python3
"""Comprehensive multi-domain double-entry accounting integration and qualification suite.

Covers cross-domain verification across:
- Item Reference dual-backend models and schema contracts
- Flatfile 66-byte sharded item custody accounting reference storage
- Item transfer accounting context propagation (root_operation_id, child_index, line_index_base)
- Multi-domain custody transitions: Corpse Lifecycle, Shop Trade, Auction, Restitution, Collector
- Coin transfer double-entry composite evidence recording
- Bank balances live predicate checks across MariaDB and Flatfile
- Complete anti-duplication, conservation, and custody invariant verification
"""

import copy
import json
import os
from pathlib import Path
import subprocess
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
DOCS = ROOT / "docs/persistence/economy_accounting"

sys.path.insert(0, str(ROOT / "scripts"))
from audit_accounting_invariants import AccountingInvariantAuditor, AuditError


class TestDoubleEntryAccountingQualification(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.registry = json.loads((DOCS / "registry.json").read_text(encoding="utf-8"))
        cls.golden = json.loads((DOCS / "golden.json").read_text(encoding="utf-8"))
        cls.auditor = AccountingInvariantAuditor(cls.registry)

    def test_economy_census_and_contracts_pass(self):
        result = subprocess.run(
            [sys.executable, str(ROOT / "scripts/validate_economy_accounting.py")],
            cwd=ROOT,
            capture_output=True,
            text=True,
            check=False,
        )
        self.assertEqual(result.returncode, 0, f"Economy validation failed:\n{result.stderr}\n{result.stdout}")
        self.assertIn("accounting contracts:", result.stdout)

    def test_all_golden_fixtures_satisfy_anti_duplication_and_conservation(self):
        fixtures = self.golden.get("fixtures", [])
        self.assertGreater(len(fixtures), 0)
        total_ops = 0
        total_postings = 0
        for fix in fixtures:
            stats = self.auditor.audit_fixture(fix, fix.get("id", "fixture"))
            total_ops += stats["operations_checked"]
            total_postings += stats["postings_checked"]
        self.assertGreaterEqual(total_ops, 20)
        self.assertGreaterEqual(total_postings, 20)

    def test_simulated_multi_domain_end_to_end_journey(self):
        """Simulate an end-to-end player journey crossing:
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

    def test_domain_subsystem_regression_suites(self):
        """Execute each domain-specific regression test in tests/async/."""
        tests_to_run = [
            "test_audit_accounting_invariants.py",
            "test_economic_accounting_item_reference.py",
            "test_item_transfer_accounting_context.py",
            "test_flatfile_item_accounting_reference.py",
            "test_coin_transfer_accounting.py",
            "test_corpse_lifecycle_accounting_context.py",
            "test_universal_item_transfer_accounting.py",
            "test_collector_accounting_context.py",
            "test_shop_trade_accounting_context.py",
            "test_auction_accounting_context.py",
            "test_player_death_restitution_accounting.py",
            "test_coin_transfer_item_accounting.py",
            "test_artifact_guild_repository.py",
            "test_zone_touch_repository.py",
            "test_account_bank_dual_backend_baseline.py",
        ]

        for tname in tests_to_run:
            tpath = ROOT / "tests/async" / tname
            if not tpath.exists():
                continue
            with self.subTest(test=tname):
                res = subprocess.run(
                    [sys.executable, str(tpath)],
                    cwd=ROOT,
                    capture_output=True,
                    text=True,
                    check=False,
                    env=dict(os.environ, CXX=os.environ.get("CXX", "g++-12")),
                )
                self.assertEqual(
                    res.returncode, 0,
                    f"Subsystem test {tname} failed:\nSTDOUT:\n{res.stdout}\nSTDERR:\n{res.stderr}"
                )


if __name__ == "__main__":
    unittest.main()
