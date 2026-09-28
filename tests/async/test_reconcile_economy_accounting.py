#!/usr/bin/env python3
"""Corrupted disposable snapshots for the read-only economic reconciler."""

import copy
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
from reconcile_economy_accounting import MAX_INPUT_BYTES, MAX_ROWS, Reconciler, SnapshotError, view  # noqa: E402

LINEAGE = "11" * 16
EPOCH = "22" * 16
OP = "33" * 16
SOURCE = "44" * 48
LEGACY = "55" * 16


def key(kind, identity):
    return (bytes.fromhex(LINEAGE) + (1).to_bytes(2, "little") +
            kind.to_bytes(2, "little") + identity.to_bytes(8, "little") +
            (0).to_bytes(8, "little") + bytes(4)).hex()


WALLET = key(1, 7)
BANK = key(2, 9)
SINK = key(8, 9)


def clean_snapshot():
    return {
        "schema_version": 1, "lineage": LINEAGE, "epoch": EPOCH,
        "complete": True, "quiescent": True, "backend": "disposable",
        "native": {
            "holdings": [
                {"account_key": WALLET, "balance": [7, 0, 0, 0], "revision": 2,
                 "alias": None},
                {"account_key": BANK, "balance": [3, 0, 0, 0], "revision": 2},
            ],
            "items": [{"uid": 81, "revision": 2, "root": 81, "parent": None,
                       "owner": [1, 7, 0], "state": "live", "alias": None}],
        },
        "account_origins": [
            {"account_key": WALLET, "origin": "baseline", "balance": [10, 0, 0, 0], "revision": 1},
            {"account_key": BANK, "origin": "baseline", "balance": [0, 0, 0, 0], "revision": 1},
        ],
        "item_origins": [{"uid": 81, "origin": "baseline", "revision": 1, "root": 81,
                          "parent": None, "owner": [2, 8, 0], "state": "live"}],
        "operations": [{"operation_id": OP, "lineage": LINEAGE, "epoch": EPOCH,
                        "reason": 3, "outcome": "committed", "source_event": SOURCE,
                        "result_code": 0,
                        "account_count": 2, "posting_count": 2, "child_count": 0,
                        "item_event_count": 1, "realized_price_copper": 3,
                        "personal_alias": None}],
        "effects": [
            {"operation_id": OP, "account_index": 0, "account_key": WALLET,
             "before": [10, 0, 0, 0], "after": [7, 0, 0, 0],
             "before_revision": 1, "after_revision": 2},
            {"operation_id": OP, "account_index": 1, "account_key": BANK,
             "before": [0, 0, 0, 0], "after": [3, 0, 0, 0],
             "before_revision": 1, "after_revision": 2},
        ],
        "postings": [
            {"operation_id": OP, "line_index": 0, "account_index": 0,
             "child_index": 0, "delta": [-3, 0, 0, 0], "copper_value": -3},
            {"operation_id": OP, "line_index": 1, "account_index": 1,
             "child_index": 0, "delta": [3, 0, 0, 0], "copper_value": 3},
        ],
        "children": [],
        "item_references": [{"operation_id": OP, "event_index": 0, "uid": 81,
                             "before_revision": 1, "after_revision": 2,
                             "legacy_operation_id": LEGACY,
                             "legacy_event_index": 0, "child_index": 0}],
        "ownership_events": [{"operation_id": LEGACY, "event_index": 0, "uid": 81,
                              "before_revision": 1, "revision": 2, "root": 81,
                              "parent": None, "owner": [1, 7, 0], "state": "live",
                              "action": "move"}],
        "source_claims": [{"lineage": LINEAGE, "source_event": SOURCE,
                           "operation_id": OP}],
        "receipts": [{"operation_id": OP, "status": "committed", "result_code": 0}],
    }


def creation_snapshot():
    snapshot = clean_snapshot()
    snapshot["item_origins"][0].update(origin="creation", revision=0, root=81,
                                       parent=None, owner=[0, 0, 0], state="absent")
    snapshot["ownership_events"][0].update(before_revision=0, revision=1, action="create")
    snapshot["item_references"][0].update(before_revision=0, after_revision=1)
    snapshot["native"]["items"][0]["revision"] = 1
    return snapshot


class ReconciliationTests(unittest.TestCase):
    def codes(self, snapshot):
        before = copy.deepcopy(snapshot)
        report = Reconciler().audit(snapshot)
        self.assertEqual(snapshot, before, "audit must leave its input untouched")
        self.assertEqual(sum(report["exception_counts"].values()), report["exception_count"])
        return set(report["exception_counts"])

    def test_clean_snapshot_and_erased_alias(self):
        snapshot = clean_snapshot()
        self.assertEqual(self.codes(snapshot), set())
        report = Reconciler().audit(snapshot)
        for name in ("holdings", "provenance", "supply", "prices", "routes"):
            data = view(snapshot, report, name, 3, uid=81)
            self.assertNotIn("alias", json.dumps(data))
            self.assertLessEqual(len(data["rows"]), 3)

    def test_missing_posting(self):
        snapshot = clean_snapshot()
        snapshot["postings"].pop()
        self.assertIn("unbalanced_root", self.codes(snapshot))
        self.assertIn("evidence_count_mismatch", self.codes(snapshot))

    def test_extra_coin_effect(self):
        snapshot = clean_snapshot()
        snapshot["effects"][1]["after"][0] = 4
        self.assertIn("account_effect_posting_mismatch", self.codes(snapshot))

    def test_duplicate_account_effect_and_foreign_epoch(self):
        snapshot = clean_snapshot()
        duplicate = copy.deepcopy(snapshot["effects"][0])
        duplicate["account_index"] = 2
        snapshot["effects"].append(duplicate)
        snapshot["operations"][0]["account_count"] = 3
        self.assertIn("duplicate_account_effect", self.codes(snapshot))
        snapshot = clean_snapshot()
        snapshot["operations"][0]["epoch"] = "66" * 16
        self.assertIn("foreign_epoch", self.codes(snapshot))

    def test_duplicate_uid_and_source_event(self):
        snapshot = creation_snapshot()
        self.assertEqual(self.codes(snapshot), set())
        second = copy.deepcopy(snapshot["ownership_events"][0])
        second.update(operation_id="66" * 16, event_index=1, before_revision=1, revision=2)
        snapshot["ownership_events"].append(second)
        self.assertIn("duplicate_uid", self.codes(snapshot))

        snapshot = clean_snapshot()
        second_op = copy.deepcopy(snapshot["operations"][0])
        second_op.update(operation_id="66" * 16, account_count=0, posting_count=0,
                         item_event_count=0, realized_price_copper=None)
        snapshot["operations"].append(second_op)
        self.assertIn("duplicate_source_event", self.codes(snapshot))

    def test_orphan_reference_and_missing_event(self):
        snapshot = clean_snapshot()
        snapshot["item_references"][0]["legacy_event_index"] = 8
        codes = self.codes(snapshot)
        self.assertIn("orphan_item_reference", codes)
        self.assertIn("missing_item_reference", codes)

        snapshot = clean_snapshot()
        snapshot["item_references"][0]["before_revision"] = 99
        self.assertIn("orphan_item_reference", self.codes(snapshot))

    def test_stale_native_and_unknown_openings(self):
        snapshot = clean_snapshot()
        snapshot["native"]["holdings"][0]["balance"][0] = 8
        self.assertIn("stale_native_balance", self.codes(snapshot))
        snapshot = clean_snapshot()
        snapshot["account_origins"].pop()
        self.assertIn("unknown_opening", self.codes(snapshot))
        snapshot = clean_snapshot()
        snapshot["item_origins"].pop()
        self.assertIn("unknown_legacy_origin", self.codes(snapshot))

    def test_native_mapping_census_reports_unmapped_and_dangling_rows(self):
        snapshot = clean_snapshot()
        snapshot["backend"] = "sql_partial"
        snapshot["complete"] = False
        snapshot["native_mapping_coverage"] = {
            "wallet_rows": 3, "bank_rows": 1,
            "unmapped_wallet_rows": 2, "unmapped_bank_rows": 0,
            "multiply_mapped_wallet_rows": 1, "multiply_mapped_bank_rows": 0,
            "dangling_wallet_mappings": 0, "dangling_bank_mappings": 1,
            "invalid_wallet_rows": 0, "invalid_bank_rows": 0,
        }
        report = Reconciler(1).audit(snapshot)
        self.assertEqual(report["exception_counts"]["unmapped_native_wallet"], 2)
        self.assertEqual(report["exception_counts"]["multiply_mapped_native_wallet"], 1)
        self.assertEqual(report["exception_counts"]["dangling_bank_mapping"], 1)
        self.assertEqual(len(report["exceptions"]), 1)
        self.assertTrue(report["truncated"])
        snapshot["native_mapping_coverage"]["wallet_rows"] = True
        with self.assertRaisesRegex(SnapshotError, "coverage count"):
            Reconciler().audit(snapshot)

    def test_lineage_source_claim_scope(self):
        snapshot = clean_snapshot()
        snapshot["backend"] = "sql_partial"
        snapshot["complete"] = False
        prior = {"lineage": LINEAGE, "source_event": "66" * 48,
                 "operation_id": "77" * 16, "operation_lineage": LINEAGE,
                 "operation_epoch": "88" * 16,
                 "operation_source_event": "66" * 48,
                 "operation_outcome": "committed"}
        snapshot["source_claims"].append(prior)
        snapshot["source_claim_coverage"] = {
            "source_operations": 2, "missing_claim_operations": 0,
            "duplicate_source_values": 0}
        self.assertNotIn("orphan_source_claim", self.codes(snapshot))
        snapshot["source_claim_coverage"].update(
            missing_claim_operations=1, duplicate_source_values=1)
        codes = self.codes(snapshot)
        self.assertIn("lineage_missing_source_claim", codes)
        self.assertIn("lineage_duplicate_source_event", codes)
        prior["operation_source_event"] = "99" * 48
        self.assertIn("orphan_source_claim", self.codes(snapshot))
        prior["operation_source_event"] = prior["source_event"]
        prior["operation_outcome"] = "rejected"
        self.assertIn("orphan_source_claim", self.codes(snapshot))
        snapshot["source_claim_coverage"]["source_operations"] = True
        with self.assertRaisesRegex(SnapshotError, "source claim coverage"):
            Reconciler().audit(snapshot)

    def test_evidence_loss_and_unlinked_child(self):
        snapshot = clean_snapshot()
        snapshot["complete"] = False
        self.assertIn("evidence_loss", self.codes(snapshot))
        snapshot = clean_snapshot()
        snapshot["children"] = [{"operation_id": OP, "child_index": 1,
                                 "parent_index": 0, "child_operation_id": "77" * 16}]
        snapshot["operations"][0]["child_count"] = 1
        self.assertIn("unlinked_child", self.codes(snapshot))

    def test_missing_receipt_and_orphan_parent(self):
        snapshot = clean_snapshot()
        snapshot["receipts"].clear()
        self.assertIn("missing_receipt", self.codes(snapshot))
        snapshot = clean_snapshot()
        snapshot["native"]["items"][0]["parent"] = 999
        self.assertIn("orphan_item_parent", self.codes(snapshot))

    def test_rejected_receipt_and_refund_sink_sign(self):
        snapshot = clean_snapshot()
        rejected = copy.deepcopy(snapshot["operations"][0])
        rejected.update(operation_id="66" * 16, outcome="rejected", result_code=9,
                        source_event=None, account_count=0, posting_count=0,
                        item_event_count=0, realized_price_copper=None)
        snapshot["operations"].append(rejected)
        snapshot["receipts"].append({"operation_id": rejected["operation_id"],
                                     "status": "rejected", "result_code": 9})
        self.assertEqual(self.codes(snapshot), set())

        snapshot["operations"][0].update(reason=20, original_operation_id=rejected["operation_id"])
        snapshot["account_origins"] = [snapshot["account_origins"][0]]
        snapshot["account_origins"][0]["balance"] = [7, 0, 0, 0]
        snapshot["native"]["holdings"] = [snapshot["native"]["holdings"][0]]
        snapshot["native"]["holdings"][0]["balance"] = [10, 0, 0, 0]
        snapshot["effects"][0]["before"] = [7, 0, 0, 0]
        snapshot["effects"][0]["after"] = [10, 0, 0, 0]
        snapshot["effects"][1]["account_key"] = SINK
        snapshot["effects"][1]["after"] = [-3, 0, 0, 0]
        snapshot["postings"][0]["delta"] = [3, 0, 0, 0]
        snapshot["postings"][0]["copper_value"] = 3
        snapshot["postings"][1]["delta"] = [-3, 0, 0, 0]
        snapshot["postings"][1]["copper_value"] = -3
        self.assertEqual(self.codes(snapshot), set())
        snapshot["postings"][1]["delta"] = [3, 0, 0, 0]
        snapshot["postings"][1]["copper_value"] = 3
        self.assertIn("invalid_system_posting_sign", self.codes(snapshot))

    def test_realized_prices_require_a_committed_root_and_valid_copper(self):
        snapshot = clean_snapshot()
        self.assertEqual(view(snapshot, Reconciler().audit(snapshot), "prices", 10)["count"], 1)
        snapshot["operations"][0].update(outcome="rejected", result_code=9)
        snapshot["receipts"][0].update(status="rejected", result_code=9)
        self.assertIn("rejected_realized_price", self.codes(snapshot))
        self.assertEqual(view(snapshot, Reconciler().audit(snapshot), "prices", 10)["count"], 0)
        snapshot = clean_snapshot()
        snapshot["operations"][0]["realized_price_copper"] = True
        self.assertIn("invalid_realized_price", self.codes(snapshot))
        snapshot["operations"][0]["realized_price_copper"] = 2**63
        self.assertIn("invalid_realized_price", self.codes(snapshot))
        self.assertEqual(view(snapshot, Reconciler().audit(snapshot), "prices", 10)["count"], 0)

    def test_missing_creation_event(self):
        snapshot = creation_snapshot()
        snapshot["operations"][0]["item_event_count"] = 0
        snapshot["item_references"].clear()
        snapshot["ownership_events"].clear()
        self.assertIn("missing_item_creation", self.codes(snapshot))

        snapshot = creation_snapshot()
        snapshot["item_origins"][0]["state"] = "live"
        self.assertIn("invalid_item_creation_origin", self.codes(snapshot))

    def test_nonzero_account_creation_origin(self):
        snapshot = clean_snapshot()
        snapshot["account_origins"][0]["origin"] = "creation"
        self.assertIn("invalid_account_creation_origin", self.codes(snapshot))

    def test_retired_account_has_zero_terminal_balance_and_no_native_row(self):
        snapshot = clean_snapshot()
        second_id = "66" * 16
        second = copy.deepcopy(snapshot["operations"][0])
        second.update(operation_id=second_id, reason=1, source_event=None,
                      item_event_count=0, realized_price_copper=None)
        snapshot["operations"].append(second)
        snapshot["receipts"].append({"operation_id": second_id, "status": "committed",
                                     "result_code": 0})
        wallet = copy.deepcopy(snapshot["effects"][0])
        wallet.update(operation_id=second_id, before=[7, 0, 0, 0], after=[10, 0, 0, 0],
                      before_revision=2, after_revision=3)
        bank = copy.deepcopy(snapshot["effects"][1])
        bank.update(operation_id=second_id, before=[3, 0, 0, 0], after=[0, 0, 0, 0],
                    before_revision=2, after_revision=3)
        snapshot["effects"].extend([wallet, bank])
        for line_index, account_index, amount in ((0, 0, 3), (1, 1, -3)):
            snapshot["postings"].append({"operation_id": second_id,
                                         "line_index": line_index, "account_index": account_index,
                                         "child_index": 0, "delta": [amount, 0, 0, 0],
                                         "copper_value": amount})
        snapshot["account_origins"][1]["retired_by"] = second_id
        snapshot["native"]["holdings"] = [snapshot["native"]["holdings"][0]]
        snapshot["native"]["holdings"][0].update(balance=[10, 0, 0, 0], revision=3)
        self.assertEqual(self.codes(snapshot), set())
        snapshot["native"]["holdings"].append({"account_key": BANK,
                                                 "balance": [0, 0, 0, 0], "revision": 3})
        self.assertIn("retired_native_holding", self.codes(snapshot))

    def test_duplicate_item_revision(self):
        snapshot = clean_snapshot()
        duplicate = copy.deepcopy(snapshot["ownership_events"][0])
        duplicate.update(operation_id="66" * 16, event_index=1)
        snapshot["ownership_events"].append(duplicate)
        self.assertIn("duplicate_uid_revision", self.codes(snapshot))

    def test_cli_bounded_exception_result(self):
        snapshot = clean_snapshot()
        snapshot["postings"].pop()
        self.assertEqual(Reconciler(0).audit(snapshot)["exception_counts"],
                         Reconciler(1).audit(snapshot)["exception_counts"])
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "snapshot.json"
            path.write_text(json.dumps(snapshot), encoding="utf-8")
            command = [sys.executable, str(ROOT / "scripts/reconcile_economy_accounting.py"),
                       str(path), "--limit", "1"]
            result = subprocess.run(command, capture_output=True, text=True, check=False)
            self.assertEqual(result.returncode, 1, result.stderr)
            report = json.loads(result.stdout)
            self.assertEqual(len(report["exceptions"]), 1)
            self.assertTrue(report["truncated"])

    def test_oversized_snapshot_and_collection_refuse(self):
        snapshot = clean_snapshot()
        snapshot["receipts"] = snapshot["receipts"] * (MAX_ROWS + 1)
        with self.assertRaisesRegex(SnapshotError, "oversized receipts"):
            Reconciler().audit(snapshot)
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "oversized.json"
            with path.open("wb") as stream:
                stream.truncate(MAX_INPUT_BYTES + 1)
            command = [sys.executable, str(ROOT / "scripts/reconcile_economy_accounting.py"), str(path)]
            result = subprocess.run(command, capture_output=True, text=True, check=False)
            self.assertEqual(result.returncode, 2, result.stderr)
            self.assertIn("limit exceeded", result.stderr)


if __name__ == "__main__":
    unittest.main()
