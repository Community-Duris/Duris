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
from reconcile_economy_accounting import (MAX_INPUT_BYTES, MAX_ROWS, NATIVE_COVERAGE_EXCEPTIONS,
                                          Reconciler, SnapshotError, view)  # noqa: E402

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
        "source_event_policy_coverage": {
            "required_committed_operations": 0, "missing_required_source_events": 0},
        "native": {
            "holdings": [
                {"account_key": WALLET, "balance": [7, 0, 0, 0], "revision": 2,
                 "alias": None},
                {"account_key": BANK, "balance": [3, 0, 0, 0], "revision": 2},
            ],
            "items": [{"uid": 81, "revision": 2, "root": 81, "parent": None,
                       "owner": [1, 7, 0], "state": "live", "alias": None}],
            "retired_mappings": [],
            "retirement_roots": [],
            "retirement_coverage": {"rows": 0, "current_epoch_rows": 0,
                                    "matched_opening_origins": 0,
                                    "unmatched_current_epoch_rows": 0, "root_rows": 0},
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
                        "item_event_count": 1, "realized_price_copper": None,
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
        "receipts": [{"operation_id": OP, "status": 1, "result_code": 0,
                       "failure_stage": 0, "committed_at_present": True}],
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
        coverage = {f"{kind}_rows": 0 for kind in (
            "wallet", "bank", "pile", "auction_escrow", "pending_claim", "treasury")}
        for field in NATIVE_COVERAGE_EXCEPTIONS:
            coverage[field] = 0
        coverage.update(wallet_rows=3, bank_rows=1,
                        unmapped_wallet_rows=2, multiply_mapped_wallet_rows=1,
                        dangling_bank_mappings=1, invalid_pile_mappings=1,
                        invalid_unknown_kind_mappings=1)
        snapshot["native_mapping_coverage"] = coverage
        report = Reconciler(1).audit(snapshot)
        self.assertEqual(report["exception_counts"]["unmapped_native_wallet"], 2)
        self.assertEqual(report["exception_counts"]["multiply_mapped_native_wallet"], 1)
        self.assertEqual(report["exception_counts"]["dangling_bank_mapping"], 1)
        self.assertEqual(report["exception_counts"]["invalid_native_pile_mapping"], 1)
        self.assertEqual(report["exception_counts"]["invalid_native_mapping_kind"], 1)
        self.assertEqual(len(report["exceptions"]), 1)
        self.assertTrue(report["truncated"])
        snapshot["native_mapping_coverage"]["wallet_rows"] = True
        with self.assertRaisesRegex(SnapshotError, "coverage count"):
            Reconciler().audit(snapshot)

    def test_duplicate_pending_claim_source_is_reported(self):
        snapshot = clean_snapshot()
        source = {"source_operation_id": "66" * 16, "source_slot": 2,
                  "account_key": key(5, 13), "beneficiary_pid": 7, "amount": 250,
                  "mapping_native_id": 7, "mapping_active_native_id": 7,
                  "mapping_valid": True, "source_root_valid": True,
                  "source_inbox_receipt": {"status": 1, "result_code": 0,
                                            "failure_stage": 0,
                                            "committed_at_present": True},
                  "claim_operation_id": None, "consumer_root_valid": None,
                  "consumer_inbox_receipt": None}
        snapshot["native"]["pending_claim_sources"] = [copy.deepcopy(source), source]
        snapshot["native"]["pending_claim_source_coverage"] = {
            "rows": 2, "open_rows": 2, "consumed_rows": 0,
            "invalid_account_mappings": 0, "invalid_source_roots": 0,
            "invalid_consumer_roots": 0}
        self.assertIn("duplicate_pending_claim_source", self.codes(snapshot))
        for row in snapshot["native"]["pending_claim_sources"]:
            row["mapping_native_id"] = 8
            row["mapping_active_native_id"] = 8
            row["mapping_valid"] = False
        snapshot["native"]["pending_claim_source_coverage"][
            "invalid_account_mappings"] = 2
        self.assertIn("invalid_pending_claim_source_mapping", self.codes(snapshot))

    def test_pending_claim_source_requires_durable_inbox_receipt(self):
        snapshot = clean_snapshot()
        snapshot["native"]["pending_claim_sources"] = [{
            "source_operation_id": "66" * 16, "source_slot": 1,
            "account_key": key(5, 13), "beneficiary_pid": 7, "amount": 250,
            "mapping_native_id": 7, "mapping_active_native_id": 7,
            "mapping_valid": True, "source_root_valid": False,
            "source_inbox_receipt": {"status": 1, "result_code": 0,
                                      "failure_stage": 0,
                                      "committed_at_present": False},
            "claim_operation_id": None, "consumer_root_valid": None,
            "consumer_inbox_receipt": None}]
        snapshot["native"]["pending_claim_source_coverage"] = {
            "rows": 1, "open_rows": 1, "consumed_rows": 0,
            "invalid_account_mappings": 0, "invalid_source_roots": 1,
            "invalid_consumer_roots": 0}
        self.assertIn("invalid_pending_claim_source_root", self.codes(snapshot))

    def test_lineage_uid_reference_root_requires_durable_inbox_receipt(self):
        native = {
            "lineage_uid_references": [],
            "lineage_uid_reference_roots": [{
                "operation_id": OP, "epoch": EPOCH, "outcome": "committed",
                "reason": 3, "source_event": None, "item_event_count": 0,
                "result_code": 0,
                "inbox_receipt": {"status": 1, "result_code": 0,
                                  "failure_stage": 0,
                                  "committed_at_present": True},
                "reference_count": 0}],
            "lineage_uid_reference_coverage": {"rows": 0, "root_rows": 1},
        }
        valid = Reconciler()
        valid.audit_lineage_uid_references(LINEAGE, EPOCH, "sql_partial", native, {})
        self.assertEqual(valid.counts, {})

        native["lineage_uid_reference_roots"][0]["inbox_receipt"][
            "committed_at_present"] = False
        invalid = Reconciler()
        invalid.audit_lineage_uid_references(LINEAGE, EPOCH, "sql_partial", native, {})
        self.assertEqual(invalid.counts["invalid_lineage_uid_reference_root"], 1)

    def test_lineage_source_claim_scope(self):
        snapshot = clean_snapshot()
        snapshot["backend"] = "sql_partial"
        snapshot["complete"] = False
        prior = {"lineage": LINEAGE, "source_event": "66" * 48,
                 "operation_id": "77" * 16, "operation_lineage": LINEAGE,
                 "operation_epoch": "88" * 16,
                 "operation_source_event": "66" * 48,
                 "operation_outcome": "committed", "operation_result_code": 0,
                 "operation_inbox_receipt": {"status": 1, "result_code": 0,
                                              "failure_stage": 0,
                                              "committed_at_present": True}}
        snapshot["source_claims"].append(prior)
        baseline = copy.deepcopy(prior)
        baseline.update(source_event="aa" * 48, operation_id="99" * 16,
                        operation_source_event="aa" * 48, operation_reason=38)
        snapshot["source_claims"].append(baseline)
        snapshot["source_claim_coverage"] = {
            "source_operations": 2, "missing_claim_operations": 0,
            "duplicate_source_values": 0}
        self.assertNotIn("orphan_source_claim", self.codes(snapshot))
        self.assertIn("baseline_source_claim", self.codes(snapshot))
        snapshot["source_claim_coverage"].update(
            missing_claim_operations=1, duplicate_source_values=1)
        codes = self.codes(snapshot)
        self.assertIn("lineage_missing_source_claim", codes)
        self.assertIn("lineage_duplicate_source_event", codes)
        prior["operation_source_event"] = "99" * 48
        self.assertIn("orphan_source_claim", self.codes(snapshot))
        prior["operation_source_event"] = "66" * 48
        prior["operation_inbox_receipt"]["committed_at_present"] = False
        self.assertIn("orphan_source_claim", self.codes(snapshot))
        prior["operation_source_event"] = prior["source_event"]
        prior["operation_outcome"] = "rejected"
        self.assertIn("orphan_source_claim", self.codes(snapshot))
        snapshot["source_claim_coverage"]["source_operations"] = True
        with self.assertRaisesRegex(SnapshotError, "source claim coverage"):
            Reconciler().audit(snapshot)

    def test_lineage_required_source_event_coverage(self):
        snapshot = clean_snapshot()
        snapshot["backend"] = "sql_partial"
        snapshot["complete"] = False
        snapshot["source_event_policy_coverage"] = {
            "required_committed_operations": 4, "missing_required_source_events": 2}
        report = Reconciler().audit(snapshot)
        self.assertEqual(report["exception_counts"]["lineage_missing_required_source_event"], 2)

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
        snapshot["receipts"][0]["committed_at_present"] = False
        self.assertIn("receipt_mismatch", self.codes(snapshot))
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
                                     "status": 1, "result_code": 9,
                                     "failure_stage": 0, "committed_at_present": True})
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
        snapshot["operations"][0]["realized_price_copper"] = 3
        self.assertIn("unexpected_realized_price", self.codes(snapshot))
        snapshot = clean_snapshot()
        snapshot["operations"][0].update(reason=21, realized_price_copper=3)
        self.assertEqual(view(snapshot, Reconciler().audit(snapshot), "prices", 10)["count"], 1)
        snapshot["operations"][0].update(outcome="rejected", result_code=9)
        snapshot["receipts"][0].update(result_code=9)
        self.assertIn("rejected_realized_price", self.codes(snapshot))
        self.assertEqual(view(snapshot, Reconciler().audit(snapshot), "prices", 10)["count"], 0)
        snapshot = clean_snapshot()
        snapshot["operations"][0]["reason"] = 21
        snapshot["operations"][0]["realized_price_copper"] = True
        self.assertIn("invalid_realized_price", self.codes(snapshot))
        snapshot["operations"][0]["realized_price_copper"] = 2**63
        self.assertIn("invalid_realized_price", self.codes(snapshot))
        self.assertEqual(view(snapshot, Reconciler().audit(snapshot), "prices", 10)["count"], 0)

    def test_domain_purchase_realized_price_matches_current_lineage_root(self):
        for reason in (24, 27):
            with self.subTest(reason=reason):
                snapshot = clean_snapshot()
                snapshot["operations"][0].update(reason=reason, realized_price_copper=3000)
                snapshot["native"]["realized_price_coverage"] = {
                    "column_available": True, "candidate_rows": 1, "missing_price_rows": 0}
                snapshot["native"]["lineage_realized_prices"] = [{
                    "operation_id": OP, "lineage": LINEAGE, "epoch": EPOCH,
                    "reason": reason, "outcome": "committed",
                    "result_code": 0,
                    "inbox_receipt": {"status": 1, "result_code": 0,
                                      "failure_stage": 0,
                                      "committed_at_present": True},
                    "realized_price_copper": 3000}]
                self.assertNotIn("realized_price_scope_mismatch", self.codes(snapshot))
                snapshot["native"]["lineage_realized_prices"][0][
                    "realized_price_copper"] = 3001
                self.assertIn("realized_price_scope_mismatch", self.codes(snapshot))

    def test_realized_price_roots_require_durable_inbox_receipts(self):
        snapshot = clean_snapshot()
        snapshot["operations"][0].update(reason=24, realized_price_copper=3000)
        snapshot["native"]["realized_price_coverage"] = {
            "column_available": True, "candidate_rows": 1, "missing_price_rows": 0}
        snapshot["native"]["lineage_realized_prices"] = [{
            "operation_id": OP, "lineage": LINEAGE, "epoch": EPOCH,
            "reason": 24, "outcome": "committed", "result_code": 0,
            "inbox_receipt": {"status": 1, "result_code": 0,
                              "failure_stage": 0, "committed_at_present": False},
            "realized_price_copper": 3000}]
        with self.assertRaises(SnapshotError):
            Reconciler().audit_lineage_realized_prices(
                LINEAGE, EPOCH, "sql_partial", snapshot["native"],
                {(OP,): snapshot["operations"][0]})

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
        second_source = "77" * 48
        second = copy.deepcopy(snapshot["operations"][0])
        second.update(operation_id=second_id, reason=41, source_event=second_source,
                      item_event_count=0, realized_price_copper=None)
        snapshot["operations"].append(second)
        snapshot["source_claims"].append({"lineage": LINEAGE,
                                          "source_event": second_source,
                                          "operation_id": second_id})
        snapshot["source_event_policy_coverage"] = {
            "required_committed_operations": 1, "missing_required_source_events": 0}
        snapshot["receipts"].append({"operation_id": second_id, "status": 1,
                                     "result_code": 0, "failure_stage": 0,
                                     "committed_at_present": True})
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
        snapshot["native"]["retired_mappings"] = [{
            "mapping_id": 9, "account_key": BANK, "native_id": 9,
            "active_native_id": None, "retiring_operation_id": second_id,
            "operation_lineage": LINEAGE, "operation_epoch": EPOCH,
            "operation_outcome": "committed", "operation_result_code": 0}]
        snapshot["native"]["retirement_coverage"] = {
            "rows": 1, "current_epoch_rows": 1,
            "matched_opening_origins": 1, "unmatched_current_epoch_rows": 0,
            "root_rows": 1}
        snapshot["native"]["retirement_roots"] = [{
            "operation_id": second_id, "lineage": LINEAGE, "epoch": EPOCH, "reason": 41,
            "outcome": "committed", "result_code": 0,
            "retirement_inbox_status": 1, "retirement_inbox_result_code": 0,
            "retirement_inbox_failure_stage": 0,
            "retirement_inbox_committed_at_present": True,
            "account_count": 2, "posting_count": 2, "child_count": 0,
            "item_event_count": 0,
            "effects": [copy.deepcopy(effect) for effect in snapshot["effects"]
                        if effect["operation_id"] == second_id],
            "postings": [copy.deepcopy(posting) for posting in snapshot["postings"]
                         if posting["operation_id"] == second_id]}]
        snapshot["native"]["holdings"] = [snapshot["native"]["holdings"][0]]
        snapshot["native"]["holdings"][0].update(balance=[10, 0, 0, 0], revision=3)
        self.assertEqual(self.codes(snapshot), set())
        snapshot["native"]["holdings"].append({"account_key": BANK,
                                                 "balance": [0, 0, 0, 0], "revision": 3})
        self.assertIn("retired_native_holding", self.codes(snapshot))

        snapshot["native"]["holdings"].pop()
        snapshot["native"]["retirement_roots"][0]["reason"] = 27
        snapshot["operations"][-1]["reason"] = 27
        self.assertIn("unauthorized_mapping_retirement", self.codes(snapshot))

        snapshot["operations"][-1]["reason"] = 41
        snapshot["native"]["retirement_roots"][0]["retirement_inbox_committed_at_present"] = False
        self.assertIn("invalid_mapping_retirement_root", self.codes(snapshot))

    def test_prior_epoch_retirement_keeps_its_root_evidence(self):
        snapshot = clean_snapshot()
        retired_key = key(2, 77)
        sink_key = key(8, 77)
        retirement_id = "66" * 16
        effects = [
            {"account_index": 0, "account_key": retired_key,
             "before": [5, 0, 0, 0], "after": [0, 0, 0, 0],
             "before_revision": 1, "after_revision": 2},
            {"account_index": 1, "account_key": sink_key,
             "before": [0, 0, 0, 0], "after": [5, 0, 0, 0],
             "before_revision": 0, "after_revision": 1},
        ]
        postings = [
            {"line_index": 0, "account_index": 0, "child_index": 0,
             "delta": [-5, 0, 0, 0], "copper_value": -5},
            {"line_index": 1, "account_index": 1, "child_index": 0,
             "delta": [5, 0, 0, 0], "copper_value": 5},
        ]
        snapshot["native"]["retired_mappings"] = [{
            "mapping_id": 77, "account_key": retired_key, "native_id": 77,
            "active_native_id": None, "retiring_operation_id": retirement_id,
            "operation_lineage": LINEAGE, "operation_epoch": "88" * 16,
            "operation_outcome": "committed", "operation_result_code": 0}]
        snapshot["native"]["retirement_coverage"] = {
            "rows": 1, "current_epoch_rows": 0,
            "matched_opening_origins": 0, "unmatched_current_epoch_rows": 0,
            "root_rows": 1}
        snapshot["native"]["retirement_roots"] = [{
            "operation_id": retirement_id, "lineage": LINEAGE, "epoch": "88" * 16,
            "reason": 41,
            "outcome": "committed", "result_code": 0,
            "retirement_inbox_status": 1, "retirement_inbox_result_code": 0,
            "retirement_inbox_failure_stage": 0,
            "retirement_inbox_committed_at_present": True,
            "account_count": 2, "posting_count": 2, "child_count": 0,
            "item_event_count": 0, "effects": effects, "postings": postings}]
        snapshot["native"]["mapping_creations"] = [{
            "mapping_id": 77, "account_kind": 2, "context_id": 0,
            "account_key": retired_key, "native_id": 77,
            "active_native_id": None, "creating_operation_id": None}]
        snapshot["native"]["mapping_creation_roots"] = []
        snapshot["native"]["mapping_creation_coverage"] = {
            "rows": 1, "creator_rows": 0, "root_rows": 0, "missing_roots": 0}
        self.assertEqual(self.codes(snapshot), set())
        snapshot["native"]["retired_mappings"][0]["native_id"] = 78
        self.assertIn("mapping_retirement_identity_mismatch", self.codes(snapshot))

    def test_duplicate_item_revision(self):
        snapshot = clean_snapshot()
        duplicate = copy.deepcopy(snapshot["ownership_events"][0])
        duplicate.update(operation_id="66" * 16, event_index=1)
        snapshot["ownership_events"].append(duplicate)
        self.assertIn("duplicate_uid_revision", self.codes(snapshot))

    def test_orphan_parent_is_checked_independently_of_item_history(self):
        for history_scope in (False, True):
            for has_origin in (False, True):
                with self.subTest(history_scope=history_scope, has_origin=has_origin):
                    snapshot = clean_snapshot()
                    if not has_origin:
                        snapshot["item_origins"] = []
                    if history_scope:
                        event = copy.deepcopy(snapshot["ownership_events"][0])
                        event.update(operation_outcome="committed", referenced=False)
                        snapshot["native"]["uid_history_events"] = [event]
                    # Keep history and current state consistent: the missing
                    # parent must be diagnosed without a stale-state mismatch.
                    snapshot["native"]["items"][0].update(parent=999, root=999)
                    snapshot["ownership_events"][0].update(parent=999, root=999)
                    if history_scope:
                        snapshot["native"]["uid_history_events"][0].update(
                            parent=999, root=999)
                    report = Reconciler().audit(snapshot)
                    self.assertEqual(report["exception_counts"].get("orphan_item_parent"), 1)
                    self.assertIn({"code": "orphan_item_parent", "uid": 81,
                                   "parent_uid": 999}, report["exceptions"])
                    self.assertNotIn("stale_native_item", report["exception_counts"])

    def test_valid_nested_native_items_and_orphan_ancestors(self):
        native = {
            (uid,): {"uid": uid, "revision": 1, "root": 83, "parent": parent,
                     "owner": [1, 7, 0], "state": "live"}
            for uid, parent in ((81, 82), (82, 83), (83, None))
        }
        origins = {identity: {**item, "origin": "baseline"}
                   for identity, item in native.items()}
        clean = Reconciler()
        clean.audit_items({}, {}, origins, native, {81, 82, 83})
        self.assertEqual(dict(clean.counts), {})

        del native[(83,)]
        orphan = Reconciler()
        orphan.audit_items({}, {}, origins, native, {81, 82, 83})
        self.assertEqual(orphan.counts["orphan_item_parent"], 1)
        self.assertIn({"code": "orphan_item_parent", "uid": 82,
                       "parent_uid": 83}, orphan.exceptions)

    def test_tombstone_parent_is_checked_without_an_origin(self):
        native = {(81,): {"uid": 81, "revision": 2, "root": 999, "parent": 999,
                          "owner": [8, 0, 0], "state": "tombstone"}}
        orphan = Reconciler()
        orphan.audit_items({}, {}, {}, native, {81})
        self.assertEqual(orphan.counts["orphan_item_parent"], 1)

    def test_native_topology_work_is_bounded_for_deep_custody(self):
        class MeasuredNative(dict):
            reads = 0

            def get(self, identity, default=None):
                self.reads += 1
                return super().get(identity, default)

            def __getitem__(self, identity):
                self.reads += 1
                return super().__getitem__(identity)

            def __contains__(self, identity):
                self.reads += 1
                return super().__contains__(identity)

        count = 1200
        native = MeasuredNative({
            (uid,): {"uid": uid, "revision": 1, "root": count,
                     "parent": uid + 1 if uid < count else None,
                     "owner": [1, 7, 0], "state": "live"}
            for uid in range(1, count + 1)
        })
        origins = {identity: {**item, "origin": "baseline"}
                   for identity, item in native.items()}
        for shape, parent, expected in (
                ("chain", None, {}), ("cycle", 1, {"cyclic_native_topology": count}),
                ("orphan", count + 1, {"orphan_item_parent": 1})):
            with self.subTest(shape=shape):
                native[(count,)]["parent"] = parent
                native.reads = 0
                reconciler = Reconciler(0)
                reconciler.audit_items({}, {}, origins, native, set(range(1, count + 1)))
                self.assertEqual(dict(reconciler.counts), expected)
                self.assertLessEqual(native.reads, 20 * count,
                                     "a bounded native snapshot must not trigger quadratic ancestor work")

    def test_native_topology_cycle_and_edge_diagnostics(self):
        base = {
            (uid,): {"uid": uid, "revision": 1, "root": 2, "parent": parent,
                     "owner": [1, 7, 0], "state": "live"}
            for uid, parent in ((1, 2), (2, None), (3, 1))
        }
        for name, changes, expected in (
                ("valid", {}, {}),
                ("cycle", {2: {"parent": 1}}, {"cyclic_native_topology": 3}),
                ("self_cycle", {2: {"parent": 2}}, {"cyclic_native_topology": 3}),
                ("root_identity", {uid: {"root": 99} for uid in (1, 2, 3)},
                 {"inconsistent_native_topology": 3}),
                ("edge", {3: {"root": 99}}, {"inconsistent_native_topology": 1}),
                ("orphan", {2: {"parent": 99}}, {"orphan_item_parent": 1}),
                ("mixed_cycle", {2: {"parent": 1, "state": "tombstone", "owner": [8, 0, 0]}},
                 {"inconsistent_native_topology": 2, "cyclic_native_topology": 1})):
            with self.subTest(name=name):
                native = copy.deepcopy(base)
                for uid, change in changes.items():
                    native[(uid,)].update(change)
                origins = {identity: {**item, "origin": "baseline"}
                           for identity, item in native.items()}
                reconciler = Reconciler()
                reconciler.audit_items({}, {}, origins, native, {1, 2, 3})
                self.assertEqual(dict(reconciler.counts), expected)

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

    def test_uid_scope_counts_other_and_unattributed_lineages(self):
        reconciler = Reconciler()
        reconciler.audit_uid_scope_coverage(
            "sql_partial",
            {"unanchored_ownership_uids": [90],
             "ambiguous_lineage_ownership_uids": [91], "uid_scope_coverage": {
                "ownership_uid_count": 2, "anchored_ownership_uid_count": 1,
                "unanchored_ownership_uid_count": 1,
                "other_lineage_ownership_uid_count": 3,
                "unattributed_ownership_uid_count": 4,
                "ambiguous_lineage_ownership_uid_count": 1}},
            {}, {}, {})
        self.assertEqual(reconciler.counts["unanchored_ownership_uid"], 1)
        self.assertEqual(reconciler.counts["other_lineage_ownership_uid"], 0)
        self.assertEqual(reconciler.counts["unattributed_ownership_uid"], 4)
        self.assertEqual(reconciler.counts["ambiguous_lineage_ownership_uid"], 1)
        self.assertIn({"code": "ambiguous_lineage_ownership_uid", "uid": 91},
                      reconciler.exceptions)

    def test_uid_history_requires_committed_ownership_operation(self):
        reconciler = Reconciler()
        reconciler.audit_lineage_uid_history(
            "sql_partial",
            {"uid_history_events": [{
                "operation_id": OP, "event_index": 0, "uid": 81,
                "before_revision": 0, "revision": 1, "root": 81, "parent": None,
                "owner": [1, 7, 0], "state": "live", "action": "create",
                "operation_epoch": EPOCH,
                "operation_outcome": "rejected", "referenced": False,
            }], "lineage_uid_references": [],
                "lineage_uid_reference_roots": [{
                    "operation_id": OP, "epoch": EPOCH, "outcome": "rejected"}]},
            {(81,): {"origin": "creation", "revision": 0, "root": 81,
                     "parent": None, "owner": [0, 0, 0], "state": "absent"}},
            {},
        )
        self.assertEqual(reconciler.counts["uid_history_operation_not_committed"], 1)

    def lineage_lifetime_report(self, actions, origin=None):
        origin = origin or creation_snapshot()["item_origins"][0]
        events = []
        for index, action in enumerate(actions):
            events.append({
                "operation_id": f"{index + 1:032x}", "event_index": 0, "uid": 81,
                "before_revision": origin["revision"] + index,
                "revision": origin["revision"] + index + 1,
                "root": 81, "parent": None,
                "owner": [8, 0, 0] if action == "destroy" else [1, 7, 0],
                "state": "tombstone" if action == "destroy" else "live",
                "action": action, "operation_outcome": "committed", "referenced": False,
            })
        current = {field: events[-1][field]
                   for field in ("uid", "revision", "root", "parent", "owner", "state")}
        reconciler = Reconciler()
        reconciler.audit_lineage_uid_history(
            "disposable", {"uid_history_events": events}, {(81,): origin}, {(81,): current})
        return reconciler

    def test_lineage_history_requires_a_creation_event(self):
        self.assertEqual(self.lineage_lifetime_report(["move"]).counts["missing_item_creation"], 1)

    def test_lineage_history_rejects_duplicate_creation(self):
        self.assertEqual(self.lineage_lifetime_report(["create", "create"]).counts["duplicate_uid"], 1)
        origin = clean_snapshot()["item_origins"][0]
        self.assertEqual(self.lineage_lifetime_report(["create"], origin).counts["duplicate_uid"], 1)

    def test_lineage_history_checks_creation_origin(self):
        origin = creation_snapshot()["item_origins"][0]
        origin["state"] = "live"
        self.assertEqual(self.lineage_lifetime_report(["create"], origin).counts[
            "invalid_item_creation_origin"], 1)

    def test_lineage_history_preserves_retired_uid_lifetimes(self):
        for action in ("create", "move"):
            with self.subTest(action=action):
                self.assertEqual(self.lineage_lifetime_report(
                    ["create", "destroy", action]).counts["resurrected_item_uid"], 1)

    def test_lineage_history_rejects_second_retirement(self):
        report = self.lineage_lifetime_report(["create", "destroy", "destroy"])
        self.assertEqual(report.counts["duplicate_item_retirement"], 1)
        origin = clean_snapshot()["item_origins"][0]
        origin.update(state="tombstone", owner=[8, 0, 0])
        self.assertEqual(self.lineage_lifetime_report(["destroy"], origin).counts[
            "duplicate_item_retirement"], 1)

    def test_epoch_history_rejects_retiring_opening_tombstone(self):
        snapshot = clean_snapshot()
        snapshot["item_origins"][0].update(state="tombstone", owner=[8, 0, 0])
        snapshot["ownership_events"][0].update(action="destroy", state="tombstone",
                                               owner=[8, 0, 0])
        snapshot["native"]["items"][0].update(state="tombstone", owner=[8, 0, 0])
        self.assertIn("duplicate_item_retirement", self.codes(snapshot))

    def test_supply_action_requires_matching_custody_state(self):
        for action, state in (("destroy", "live"), ("create", "tombstone")):
            for lineage in (False, True):
                with self.subTest(action=action, state=state, lineage=lineage):
                    snapshot = clean_snapshot()
                    event = snapshot["ownership_events"][0]
                    event.update(action=action, state=state)
                    snapshot["native"]["items"][0]["state"] = state
                    if lineage:
                        event.update(operation_outcome="committed", referenced=False)
                        report = Reconciler()
                        report.audit_lineage_uid_history(
                            "disposable", {"uid_history_events": [event]},
                            {(81,): snapshot["item_origins"][0]},
                            {(81,): snapshot["native"]["items"][0]})
                        codes = report.counts
                    else:
                        codes = self.codes(snapshot)
                    self.assertIn("invalid_item_supply_state", codes)

    def test_destroy_action_retires_uid_even_with_corrupt_live_state(self):
        report = Reconciler()
        report.audit_item_lifetime(81, clean_snapshot()["item_origins"][0], [
            {"action": "destroy", "state": "live", "operation_id": OP},
            {"action": "move", "state": "live", "operation_id": LEGACY},
        ])
        self.assertEqual(report.counts["invalid_item_supply_state"], 1)
        self.assertEqual(report.counts["resurrected_item_uid"], 1)

    def test_lineage_history_accepts_creation_move_and_retirement(self):
        for actions in (["create"], ["create", "move"], ["create", "move", "destroy"]):
            with self.subTest(actions=actions):
                self.assertEqual(dict(self.lineage_lifetime_report(actions).counts), {})
        self.assertEqual(dict(self.lineage_lifetime_report(
            ["move"], clean_snapshot()["item_origins"][0]).counts), {})

    def test_opening_tombstone_cannot_become_live_in_either_history_scope(self):
        snapshot = clean_snapshot()
        snapshot["item_origins"][0].update(state="tombstone", owner=[8, 0, 0])
        self.assertIn("resurrected_item_uid", self.codes(snapshot))
        report = self.lineage_lifetime_report(["move"], snapshot["item_origins"][0])
        self.assertEqual(report.counts["resurrected_item_uid"], 1)

    def test_uid_history_coverage_matches_the_full_event_and_unreferenced_sets(self):
        event = {
            "operation_id": OP, "event_index": 0, "uid": 81,
            "before_revision": 0, "revision": 1, "root": 81, "parent": None,
            "owner": [1, 7, 0], "state": "live", "action": "create",
            "operation_epoch": EPOCH,
            "operation_outcome": "committed", "referenced": False,
        }
        native = {
            "uid_history_events": [event], "lineage_uid_references": [],
            "lineage_uid_reference_roots": [{
                "operation_id": OP, "epoch": EPOCH, "outcome": "committed"}],
            "uid_event_coverage": {
                "tracked_uids": 1, "ledger_events": 1,
                "referenced_events": 0, "unreferenced_events": 1},
            "unreferenced_uid_events": [],
        }
        origin = {(81,): {"origin": "creation", "revision": 0, "root": 81,
                          "parent": None, "owner": [0, 0, 0], "state": "absent"}}
        reconciler = Reconciler()
        reconciler.audit_lineage_uid_history("sql_partial", native, origin, {})
        self.assertEqual(reconciler.counts["unreferenced_uid_history_scope_mismatch"], 1)

        native["unreferenced_uid_events"] = [event]
        native["uid_event_coverage"]["ledger_events"] = 0
        mismatch = Reconciler()
        mismatch.audit_lineage_uid_history("sql_partial", native, origin, {})
        self.assertEqual(mismatch.counts["lineage_uid_history_coverage_mismatch"], 1)

        native["uid_history_events"][0]["operation_epoch"] = "88" * 16
        mismatch_root = Reconciler()
        mismatch_root.audit_lineage_uid_history("sql_partial", native, origin, {})
        self.assertEqual(mismatch_root.counts["invalid_uid_history_root"], 1)

    def test_coin_pile_mapping_matches_payload_and_native_item(self):
        mapping_key = key(3, 101)
        native = {
            "coin_piles": [{"uid": 81, "owner": [1, 7, 0], "revision": 2,
                            "state": "live", "amounts": [4, 0, 0, 0]}],
            "coin_pile_mappings": [{"account_key": mapping_key, "uid": 81,
                                    "item_exists": True, "holding_valid": True,
                                    "balance": [4, 0, 0, 0], "revision": 2}],
            "coin_pile_coverage": {
                "rows": 1, "payload_rows": 1, "missing_payload_rows": 0,
                "mapped_live_rows": 1, "unmapped_live_rows": 0,
                "multiply_mapped_live_rows": 0, "dangling_mappings": 0,
                "invalid_mappings": 0},
        }
        reconciler = Reconciler()
        reconciler.audit_coin_pile_mappings(
            "sql_partial", native,
            {(mapping_key,): {"balance": [4, 0, 0, 0], "revision": 2}},
            {(81,): {"owner": [1, 7, 0], "revision": 2, "state": "live"}},
            LINEAGE)
        self.assertEqual(reconciler.counts, {})
        native["coin_pile_mappings"][0]["balance"] = [3, 0, 0, 0]
        mismatched = Reconciler()
        mismatched.audit_coin_pile_mappings(
            "sql_partial", native,
            {(mapping_key,): {"balance": [4, 0, 0, 0], "revision": 2}},
            {(81,): {"owner": [1, 7, 0], "revision": 2, "state": "live"}},
            LINEAGE)
        self.assertEqual(mismatched.counts["coin_pile_holding_mismatch"], 1)

    def test_coin_pile_lifecycle_source_matches_create_and_retire_events(self):
        raw = bytearray(48)
        raw[0:2] = (16).to_bytes(2, "little")
        raw[2:4] = (1).to_bytes(2, "little")
        raw[4] = 3
        raw[5:13] = (17).to_bytes(8, "little")
        raw[13:20] = b"COINACC"
        raw[20:28] = (4).to_bytes(8, "little")
        raw[28:36] = b"COINLIFE"
        raw[44:48] = (3).to_bytes(4, "little")
        native = {
            "mapping_creations": [{"mapping_id": 17, "account_kind": 3,
                                   "native_id": 700},
                                  {"mapping_id": 18, "account_kind": 3,
                                   "native_id": 81, "creating_operation_id": OP}],
            "lineage_uid_reference_roots": [{"operation_id": OP, "reason": 3,
                                             "outcome": "committed",
                                             "source_event": raw.hex()}],
            "lineage_uid_references": [
                {"operation_id": OP, "uid": 81, "ledger_action": "create"},
                {"operation_id": OP, "uid": 700, "ledger_action": "destroy"}],
        }
        reconciler = Reconciler()
        reconciler.audit_coin_pile_lifecycle_sources("sql_partial", native)
        self.assertEqual(reconciler.counts, {})
        mismatch_native = copy.deepcopy(native)
        mismatch_native["lineage_uid_references"].pop()
        mismatch = Reconciler()
        mismatch.audit_coin_pile_lifecycle_sources("sql_partial", mismatch_native)
        self.assertEqual(mismatch.counts["coin_pile_lifecycle_source_mismatch"], 1)
        native["lineage_uid_references"].append(
            {"operation_id": OP, "uid": 82, "ledger_action": "create"})
        count_mismatch = Reconciler()
        count_mismatch.audit_coin_pile_lifecycle_sources("sql_partial", native)
        self.assertEqual(count_mismatch.counts[
            "coin_pile_lifecycle_event_count_mismatch"], 1)
        native["lineage_uid_references"].pop()
        native["lineage_uid_references"][1]["uid"] = 701
        uid_mismatch = Reconciler()
        uid_mismatch.audit_coin_pile_lifecycle_sources("sql_partial", native)
        self.assertEqual(uid_mismatch.counts["coin_pile_lifecycle_uid_mismatch"], 1)
        native["lineage_uid_references"][1]["uid"] = 700
        native["mapping_creations"][1]["native_id"] = 82
        creation_mismatch = Reconciler()
        creation_mismatch.audit_coin_pile_lifecycle_sources("sql_partial", native)
        self.assertEqual(creation_mismatch.counts[
            "coin_pile_creation_origin_mismatch"], 1)

    def test_mapping_creation_root_recovers_postbaseline_account_origin(self):
        native = {
            "mapping_creations": [{
                "mapping_id": 7, "account_key": WALLET, "account_kind": 1,
                "context_id": 0, "native_id": 7, "active_native_id": 7,
                "creating_operation_id": OP}],
            "mapping_creation_coverage": {
                "rows": 1, "creator_rows": 1, "root_rows": 1, "missing_roots": 0},
            "mapping_creation_roots": [{
                "operation_id": OP, "lineage": LINEAGE, "epoch": EPOCH, "reason": 42,
                "outcome": "committed", "result_code": 0, "source_event": None,
                "creator_inbox_status": 1, "creator_inbox_result_code": 0,
                "creator_inbox_failure_stage": 0,
                "creator_inbox_committed_at_present": True,
                "account_count": 1, "posting_count": 0, "child_count": 0,
                "item_event_count": 0,
                "effects": [{"account_index": 0, "account_key": WALLET,
                             "before": [0, 0, 0, 0],
                             "after": [4, 0, 0, 0], "before_revision": 0,
                             "after_revision": 1}]}],
        }
        origins = {(WALLET,): {"origin": "creation", "balance": [0, 0, 0, 0],
                               "revision": 0}}
        operations = {(OP,): {
            "lineage": LINEAGE, "epoch": EPOCH, "reason": 42,
            "outcome": "committed", "result_code": 0, "source_event": None,
            "account_count": 1, "posting_count": 0, "child_count": 0,
            "item_event_count": 0}}
        effects = {(OP, 0): {
            "account_index": 0, "account_key": WALLET, "before": [0, 0, 0, 0],
            "after": [4, 0, 0, 0], "before_revision": 0, "after_revision": 1}}
        reconciler = Reconciler()
        reconciler.audit_mapping_creations("sql_partial", LINEAGE, native, origins,
                                           operations, effects, EPOCH)
        self.assertEqual(reconciler.counts, {})
        native["mapping_creation_roots"][0]["reason"] = 26
        operations[(OP,)]["reason"] = 26
        unauthorized = Reconciler()
        unauthorized.audit_mapping_creations("sql_partial", LINEAGE, native, origins,
                                             operations, effects, EPOCH)
        self.assertEqual(unauthorized.counts["unauthorized_mapping_creation"], 1)
        native["mapping_creation_roots"][0]["reason"] = 42
        operations[(OP,)]["reason"] = 42
        native["mapping_creation_roots"][0]["effects"][0]["before_revision"] = 1
        invalid = Reconciler()
        invalid.audit_mapping_creations("sql_partial", LINEAGE, native, origins,
                                        operations, effects, EPOCH)
        self.assertEqual(invalid.counts["invalid_mapping_creation_origin"], 1)

    def test_current_mapping_creator_root_matches_main_operation_evidence(self):
        root = {"operation_id": OP, "lineage": LINEAGE, "epoch": EPOCH, "reason": 42,
                "outcome": "committed", "result_code": 0, "source_event": None,
                "creator_inbox_status": 1, "creator_inbox_result_code": 0,
                "creator_inbox_failure_stage": 0,
                "creator_inbox_committed_at_present": True,
                "account_count": 1, "posting_count": 0, "child_count": 0,
                "item_event_count": 0, "effects": [{
                    "account_index": 0, "account_key": WALLET, "before": [0, 0, 0, 0],
                    "after": [4, 0, 0, 0], "before_revision": 0, "after_revision": 1}]}
        native = {"mapping_creations": [{
                      "mapping_id": 7, "account_key": WALLET, "account_kind": 1,
                      "context_id": 0, "native_id": 7, "active_native_id": 7,
                      "creating_operation_id": OP}],
                  "mapping_creation_roots": [root],
                  "mapping_creation_coverage": {"rows": 1, "creator_rows": 1,
                                                "root_rows": 1, "missing_roots": 0}}
        operations = {(OP,): {field: root[field] for field in (
            "lineage", "epoch", "reason", "outcome", "result_code", "source_event",
            "account_count", "posting_count", "child_count", "item_event_count")}}
        effects = {(OP, 0): root["effects"][0].copy()}
        valid = Reconciler()
        origins = {(WALLET,): {"origin": "creation", "balance": [0, 0, 0, 0],
                               "revision": 0}}
        valid.audit_mapping_creations("sql_partial", LINEAGE, native, origins,
                                      operations, effects, EPOCH)
        self.assertEqual(valid.counts, {})
        corrupted = copy.deepcopy(root)
        corrupted["effects"][0]["after"] = [5, 0, 0, 0]
        native["mapping_creation_roots"] = [corrupted]
        invalid = Reconciler()
        invalid.audit_mapping_creations("sql_partial", LINEAGE, native, origins,
                                        operations, effects, EPOCH)
        self.assertEqual(invalid.counts["invalid_mapping_creation_root"], 1)

    def test_prior_epoch_mapping_creator_still_requires_its_inbox_receipt(self):
        prior_epoch = "77" * 16
        root = {"operation_id": OP, "lineage": LINEAGE, "epoch": prior_epoch,
                "reason": 42, "outcome": "committed", "result_code": 0,
                "creator_inbox_status": 1, "creator_inbox_result_code": 0,
                "creator_inbox_failure_stage": 0,
                "creator_inbox_committed_at_present": True,
                "source_event": None, "account_count": 1, "posting_count": 0,
                "child_count": 0, "item_event_count": 0, "effects": [{
                    "account_index": 0, "account_key": WALLET,
                    "before": [0, 0, 0, 0], "after": [4, 0, 0, 0],
                    "before_revision": 0, "after_revision": 1}]}
        native = {"mapping_creations": [{
                      "mapping_id": 7, "account_key": WALLET, "account_kind": 1,
                      "context_id": 0, "native_id": 7, "active_native_id": 7,
                      "creating_operation_id": OP}],
                  "mapping_creation_roots": [root],
                  "mapping_creation_coverage": {"rows": 1, "creator_rows": 1,
                                                "root_rows": 1, "missing_roots": 0}}
        origins = {(WALLET,): {"origin": "creation", "balance": [0, 0, 0, 0],
                               "revision": 0}}
        valid = Reconciler()
        valid.audit_mapping_creations("sql_partial", LINEAGE, native, origins, {}, {}, EPOCH)
        self.assertEqual(valid.counts, {})
        invalid_root = copy.deepcopy(root)
        invalid_root["creator_inbox_committed_at_present"] = False
        native["mapping_creation_roots"] = [invalid_root]
        invalid = Reconciler()
        invalid.audit_mapping_creations("sql_partial", LINEAGE, native, origins, {}, {}, EPOCH)
        self.assertEqual(invalid.counts["invalid_mapping_creation_root"], 1)

    def test_baseline_installation_mapping_creator_uses_witnessed_root(self):
        installation = "66" * 16
        native = {
            "mapping_creations": [{
                "mapping_id": 7, "account_key": WALLET, "account_kind": 1,
                "context_id": 0, "native_id": 7, "active_native_id": 7,
                "creating_operation_id": installation}],
            "mapping_creation_coverage": {
                "rows": 1, "creator_rows": 1, "root_rows": 1, "missing_roots": 0},
            "mapping_creation_roots": [{
                "operation_id": installation, "lineage": LINEAGE, "epoch": EPOCH,
                "reason": 38, "outcome": "committed", "result_code": 0,
                "account_count": 0, "posting_count": 0, "child_count": 0,
                "item_event_count": 0, "effects": [],
                "creator_kind": "baseline_installation", "installation_phase": 2,
                "installation_selected_epoch": EPOCH, "installation_revision": 1,
                "installation_failure_stage": 0,
                "installation_committed_at_present": True,
                "baseline_operation_id": OP, "baseline_lineage": LINEAGE,
                "baseline_epoch": EPOCH, "baseline_reason": 38,
                "baseline_outcome": "committed", "baseline_result_code": 0}],
            "baseline_operation_ids": [OP],
        }
        origins = {(WALLET,): {"origin": "baseline", "balance": [0, 0, 0, 0],
                               "revision": 0}}
        reconciler = Reconciler()
        reconciler.audit_mapping_creations("sql_partial", LINEAGE, native, origins)
        self.assertEqual(reconciler.counts, {})
        native["baseline_operation_ids"] = []
        invalid = Reconciler()
        invalid.audit_mapping_creations("sql_partial", LINEAGE, native, origins)
        self.assertEqual(invalid.counts["invalid_mapping_creation_root"], 1)
        for field, value in (("installation_selected_epoch", "88" * 16),
                             ("installation_revision", 0),
                             ("installation_failure_stage", 1),
                             ("installation_committed_at_present", False)):
            corrupted = copy.deepcopy(native)
            corrupted["baseline_operation_ids"] = [OP]
            corrupted["mapping_creation_roots"][0][field] = value
            invalid = Reconciler()
            invalid.audit_mapping_creations("sql_partial", LINEAGE, corrupted, origins)
            self.assertEqual(invalid.counts["invalid_mapping_creation_root"], 1)

    def test_mapping_creation_coverage_counts_missing_roots_once(self):
        second_key = key(1, 8)
        native = {
            "mapping_creations": [
                {"mapping_id": 7, "account_key": WALLET, "account_kind": 1,
                 "context_id": 0, "native_id": 7, "active_native_id": 7,
                 "creating_operation_id": OP},
                {"mapping_id": 8, "account_key": second_key, "account_kind": 1,
                 "context_id": 0, "native_id": 8, "active_native_id": 8,
                 "creating_operation_id": OP}],
            "mapping_creation_coverage": {
                "rows": 2, "creator_rows": 2, "root_rows": 0, "missing_roots": 1},
            "mapping_creation_roots": [],
        }
        reconciler = Reconciler()
        reconciler.audit_mapping_creations("sql_partial", LINEAGE, native, {})
        self.assertEqual(reconciler.counts["missing_mapping_creation_root"], 2)

    def test_pending_claim_consumer_roots_require_complete_source_rows(self):
        native = {
            "pending_claim_consumers": [{
                "operation_id": OP, "epoch": EPOCH, "outcome": "committed",
                "result_code": 0, "source_rows": 1, "source_amount": 100,
                "inbox_receipt": {"status": 1, "result_code": 0,
                                  "failure_stage": 0,
                                  "committed_at_present": True},
                "pending_claim_debits": [{"account_key": key(5, 21), "amount": 100,
                                           "before": [100, 0, 0, 0],
                                           "after": [0, 0, 0, 0]}]}],
            "pending_claim_sources": [{
                "source_operation_id": "66" * 16, "source_slot": 1,
                "beneficiary_pid": 7, "amount": 100, "claim_operation_id": OP}],
            "pending_claim_consumer_coverage": {
                "rows": 1, "missing_source_rows": 0, "mismatched_source_amounts": 0},
        }
        reconciler = Reconciler()
        reconciler.audit_pending_claim_consumers("sql_partial", LINEAGE, native)
        self.assertEqual(reconciler.counts, {})
        incomplete = copy.deepcopy(native)
        incomplete["pending_claim_sources"] = []
        missing_export = Reconciler()
        missing_export.audit_pending_claim_consumers("sql_partial", LINEAGE, incomplete)
        self.assertEqual(missing_export.counts[
            "pending_claim_consumer_source_coverage_mismatch"], 1)
        missing_root_snapshot = copy.deepcopy(native)
        missing_root_snapshot["pending_claim_consumers"] = []
        missing_root_snapshot["pending_claim_consumer_coverage"].update(rows=0)
        missing_root = Reconciler()
        missing_root.audit_pending_claim_consumers(
            "sql_partial", LINEAGE, missing_root_snapshot)
        self.assertEqual(missing_root.counts["missing_pending_claim_consumer_root"], 1)
        native["pending_claim_consumers"][0]["inbox_receipt"][
            "committed_at_present"] = False
        with self.assertRaises(SnapshotError):
            Reconciler().audit_pending_claim_consumers("sql_partial", LINEAGE, native)
        native["pending_claim_consumers"][0].update(source_rows=0, source_amount=0)
        native["pending_claim_sources"] = []
        native["pending_claim_consumers"][0]["inbox_receipt"][
            "committed_at_present"] = True
        native["pending_claim_consumer_coverage"].update(missing_source_rows=1)
        missing = Reconciler()
        missing.audit_pending_claim_consumers("sql_partial", LINEAGE, native)
        self.assertEqual(missing.counts["pending_claim_consumer_without_sources"], 1)
        native["pending_claim_consumers"][0].update(source_rows=1, source_amount=99)
        native["pending_claim_sources"] = [{
            "source_operation_id": "66" * 16, "source_slot": 1,
            "beneficiary_pid": 7, "amount": 99, "claim_operation_id": OP}]
        native["pending_claim_consumer_coverage"].update(missing_source_rows=0,
                                                         mismatched_source_amounts=1)
        mismatched = Reconciler()
        mismatched.audit_pending_claim_consumers("sql_partial", LINEAGE, native)
        self.assertEqual(mismatched.counts["pending_claim_consumer_amount_mismatch"], 1)

    def test_unattributed_uid_history_is_reported_without_lineage_inference(self):
        native = {
            "unattributed_uid_events": [{
                "operation_id": "66" * 16, "event_index": 0, "uid": 90,
                "before_revision": 0, "revision": 1, "root": 90, "parent": None,
                "owner": [1, 7, 0], "state": "live", "action": "create"}],
            "unattributed_uid_event_coverage": {"uids": 1, "events": 1},
        }
        reconciler = Reconciler()
        reconciler.audit_unattributed_uid_history("sql_partial", native)
        self.assertEqual(reconciler.counts["unattributed_ownership_event"], 1)

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
