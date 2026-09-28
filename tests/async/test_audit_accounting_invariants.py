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
        self.assertIn("replayed for reason", str(ctx.exception))

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


if __name__ == "__main__":
    unittest.main()
