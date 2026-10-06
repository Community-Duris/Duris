#!/usr/bin/env python3
"""Keep SQL ownership UID coverage scoped to known accounting lineages."""

from pathlib import Path
from decimal import Decimal
import json
import sys
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
from economic_sql_audit_snapshot import (ExportError, infer_created_mapping_origins,
                                         auction_escrow_balance,
                                         auction_escrow_mapping_is_live,
                                         native_source_count,
                                         native_mapping_identity_valid,
                                         read_native,
                                         append_committed_item_creation_origin,
                                         read_pending_claim_consumers,
                                         read_mapping_creations,
                                         read_uid_event_census)  # noqa: E402


class Cursor:
    def __init__(self):
        self.sql = ""

    def execute(self, sql, _params):
        self.sql = sql

    def fetchall(self):
        if "WHERE o.operation_id IS NULL" in self.sql:
            return [{"operation_id": b"z" * 16, "event_index": 0, "item_uid": 3,
                     "root_item_uid": 3, "parent_item_uid": None, "to_owner_type": 1,
                     "to_owner_id": 8, "to_owner_context_id": 0, "item_revision": 1,
                     "from_equipment_slot": 0, "to_equipment_slot": 5,
                     "from_owner_revision": 0, "reason_type": 2}]
        if "SELECT l.operation_id,l.event_index,l.item_uid" in self.sql:
            return [{
                "operation_id": bytes([uid + 96]) * 16, "event_index": 0, "item_uid": uid,
                "root_item_uid": uid, "parent_item_uid": None, "to_owner_type": 1,
                "to_owner_id": 7, "to_owner_context_id": 0, "item_revision": 1,
                "from_equipment_slot": 0, "to_equipment_slot": 5,
                "from_owner_revision": 0, "reason_type": 2, "operation_outcome": 1,
                "operation_epoch": b"e" * 16,
            } for uid in (1, 5)]
        if "SELECT DISTINCT l.item_uid,o.lineage" in self.sql:
            return [
                {"item_uid": 1, "lineage": b"l" * 16},
                {"item_uid": 5, "lineage": b"l" * 16},
                {"item_uid": 2, "lineage": b"x" * 16},
                {"item_uid": 3, "lineage": None},
                {"item_uid": 4, "lineage": b"l" * 16},
                {"item_uid": 4, "lineage": b"x" * 16},
            ]
        raise AssertionError(f"unexpected audit query: {self.sql}")


class UidScopeTests(unittest.TestCase):
    def test_compound_item_actions_follow_retained_supply_endpoints(self):
        import copy
        import economic_sql_audit_snapshot as exporter

        cases = ((34, 7, 1, 1, "create"), (9, 7, 6, 1, "create"),
                 (34, 1, 8, 2, "destroy"), (33, 1, 8, 2, "destroy"),
                 (21, 1, 8, 2, "destroy"), (34, 1, 1, 2, "move"),
                 (34, 7, 1, 2, "move"), (2, 1, 8, 2, "create"),
                 (3, 1, 1, 2, "destroy"), (8, 1, 1, 2, "move"))
        for reason, old_owner, new_owner, revision, expected in cases:
            row = dict(operation_id=b"a" * 16, event_index=0, item_uid=81,
                       root_item_uid=81, parent_item_uid=None, from_owner_type=old_owner,
                       from_owner_id=0 if old_owner == 7 else 7, from_owner_context_id=0,
                       to_owner_type=new_owner, to_owner_id=0 if new_owner == 8 else 7,
                       to_owner_context_id=0, item_revision=revision, before_revision=revision - 1,
                       from_equipment_slot=0, to_equipment_slot=0, reason_type=reason,
                       operation_epoch=b"e" * 16, operation_outcome=1, personal_alias="private-compound-action")

            class ActionCursor:
                sql = ""
                def __init__(self):
                    self.queries = []
                def execute(self, query, _params=()):
                    self.sql = query
                    self.queries.append(query)
                def fetchone(self):
                    return dict(source_operations=0, missing_claim_operations=0,
                                duplicate_source_values=0, required_operations=0, missing_source_events=0,
                                root_count=0, plan_bytes=0, max_plan_bytes=0)
                def fetchall(self):
                    if "SELECT DISTINCT l.item_uid,o.lineage" in self.sql:
                        return [dict(item_uid=81, lineage=b"l" * 16), dict(item_uid=82, lineage=None)]
                    if "WHERE o.operation_id IS NULL" in self.sql:
                        return [dict(row, item_uid=82, root_item_uid=82)]
                    if "FROM economic_accounting_item_reference r" in self.sql:
                        return [dict(row, operation_id=b"b" * 16, child_index=0,
                                     before_revision=revision - 1, after_revision=revision,
                                     legacy_operation_id=row["operation_id"], legacy_event_index=0,
                                     epoch=b"e" * 16, outcome=1, item_event_count=1, ledger_uid=81)]
                    if "SELECT l.operation_id,l.event_index,l.item_uid" in self.sql:
                        return [dict(row)]
                    return []

            cursor = ActionCursor()
            original = copy.deepcopy(row)
            with self.subTest(reason=reason, old_owner=old_owner, new_owner=new_owner, revision=revision):
                evidence = exporter.read_evidence(cursor, b"l" * 16, b"e" * 16, False)
                references, _, _ = exporter.read_lineage_uid_references(cursor, b"l" * 16)
                history = read_uid_event_census(cursor, b"l" * 16, [{"uid": 81, "revision": 0}],
                                               {"item_references": []}, [], [])
                actions = [evidence["ownership_events"][0]["action"], references[0]["ledger_action"],
                           history[0][0]["action"], history[6][0]["action"]]
                self.assertEqual(actions, [expected] * 4)
                self.assertEqual(row, original)
                self.assertNotIn("private-compound", json.dumps([evidence, references, history]))
                queries = [query for query in cursor.queries
                           if "SELECT l.operation_id,l.event_index,l.item_uid" in query or
                           "FROM economic_accounting_item_reference r" in query]
                self.assertTrue(queries and all("l.from_owner_type" in query for query in queries))
                self.assertTrue(all(query.startswith("SELECT ") for query in cursor.queries))
                for action in actions[:3]:
                    origins, known = [], set()
                    appended = append_committed_item_creation_origin(
                        origins, known, 81, action, revision - 1, "committed")
                    self.assertEqual(appended, expected == "create" and revision == 1)
                    for outcome in ("rejected", "unknown"):
                        self.assertFalse(append_committed_item_creation_origin(
                            [], set(), 81, action, revision - 1, outcome))

    def test_coin_payload_bounds_refuse_before_mapping_payload_reads(self):
        from economic_sql_audit_snapshot import MAX_INPUT_BYTES, MAX_ITEM_PAYLOAD_BYTES, MAX_ROWS

        class BoundsCursor:
            def __init__(self, bounds):
                self.bounds = bounds
                self.payload_reads = []
                self.sql = ""

            def execute(self, sql, params=()):
                self.sql = sql
                if "coin_payload" in sql and "OCTET_LENGTH" not in sql:
                    self.payload_reads.append(sql)
                    raise ExportError("payload selected before its source bounds")

            def fetchone(self):
                return self.bounds

        for bounds in (None,
                       dict(row_count=MAX_ROWS + 1, payload_bytes=0, max_payload_bytes=0),
                       dict(row_count=1, payload_bytes=MAX_INPUT_BYTES + 1, max_payload_bytes=1),
                       dict(row_count=1, payload_bytes=MAX_ITEM_PAYLOAD_BYTES + 1,
                            max_payload_bytes=MAX_ITEM_PAYLOAD_BYTES + 1)):
            with self.subTest(bounds=bounds):
                cursor = BoundsCursor(bounds)
                with mock.patch('economic_sql_audit_snapshot.read_ship_coffers', return_value=([], {})), \
                     mock.patch('economic_sql_audit_snapshot.read_guild_treasuries', return_value=([], {})):
                    with self.assertRaisesRegex(ExportError, "coin-pile source exceeds audit bounds"):
                        read_native(cursor, b"l" * 16)
                self.assertEqual(cursor.payload_reads, [])

    def test_mapped_coin_payload_budget_counts_repeated_joined_bytes(self):
        from economic_sql_audit_snapshot import MAX_INPUT_BYTES

        class JoinedBoundsCursor:
            sql = ""
            payload_reads = 0

            def execute(self, sql, params=()):
                self.sql = sql
                if "coin_payload" in sql and "OCTET_LENGTH" not in sql:
                    self.payload_reads += 1
                    raise ExportError("payload selected before its joined bounds")

            def fetchone(self):
                if "economic_account_mapping" in self.sql:
                    return dict(payload_bytes=MAX_INPUT_BYTES + 1)
                return dict(row_count=1, payload_bytes=1, max_payload_bytes=1)

        cursor = JoinedBoundsCursor()
        with mock.patch('economic_sql_audit_snapshot.read_ship_coffers', return_value=([], {})), \
             mock.patch('economic_sql_audit_snapshot.read_guild_treasuries', return_value=([], {})):
            with self.assertRaisesRegex(ExportError, "mapped coin-pile source exceeds audit bounds"):
                read_native(cursor, b"l" * 16)
        self.assertEqual(cursor.payload_reads, 0)

    def test_native_mapping_coverage_requires_the_account_locator(self):
        class NativeCursor:
            sql = ""
            params = ()

            def execute(self, sql, params):
                self.sql = sql
                self.params = params

            def fetchone(self):
                return {"source_rows": 2, "unmapped_rows": 1}

        cursor = NativeCursor()
        self.assertEqual(native_source_count(
            cursor, b"l" * 16, "item_current_owner", "item_uid", 3,
            "vnum=3 AND state=1"), (2, 1))
        self.assertIn("AND m.locator_kind=3", cursor.sql)
        self.assertEqual(cursor.params, (b"l" * 16,))

    def test_mapping_identity_policy_covers_active_and_retired_rows(self):
        self.assertTrue(native_mapping_identity_valid(5, 5, 71, 71))
        self.assertTrue(native_mapping_identity_valid(5, 5, 71, None))
        self.assertFalse(native_mapping_identity_valid(5, 4, 71, 71))
        self.assertFalse(native_mapping_identity_valid(5, 5, 71, 72))
        self.assertFalse(native_mapping_identity_valid(5, 5, True, None))
        self.assertFalse(native_mapping_identity_valid(7, 7, 71, None))

    def test_only_funded_removed_auctions_keep_an_active_escrow_mapping(self):
        self.assertTrue(auction_escrow_mapping_is_live("OPEN", None))
        self.assertTrue(auction_escrow_mapping_is_live("REMOVED", 7))
        self.assertFalse(auction_escrow_mapping_is_live("REMOVED", 0))
        self.assertFalse(auction_escrow_mapping_is_live("REMOVED", None))

    def test_open_auction_without_bid_cannot_report_funded_escrow(self):
        self.assertEqual(auction_escrow_balance("OPEN", 0, 0), [0, 0, 0, 0])
        self.assertEqual(auction_escrow_balance("OPEN", 25, 7), [25, 0, 0, 0])
        self.assertIsNone(auction_escrow_balance("OPEN", 25, 0))
        self.assertIsNone(auction_escrow_balance("OPEN", -1, 7))

    def test_creation_origins_require_committed_zero_revision_create(self):
        origins = []
        known = set()
        self.assertFalse(append_committed_item_creation_origin(
            origins, known, 9001, "create", 0, "rejected"))
        self.assertFalse(append_committed_item_creation_origin(
            origins, known, 9001, "create", 0, "unknown"))
        self.assertFalse(append_committed_item_creation_origin(
            origins, known, 9001, "move", 0, "committed"))
        self.assertFalse(append_committed_item_creation_origin(
            origins, known, 9001, "create", 1, "committed"))
        self.assertTrue(append_committed_item_creation_origin(
            origins, known, 9001, "create", 0, "committed"))
        self.assertFalse(append_committed_item_creation_origin(
            origins, known, 9001, "create", 0, "committed"))
        self.assertEqual(origins, [{"uid": 9001, "origin": "creation", "revision": 0,
                                   "root": 9001, "parent": None, "owner": [0, 0, 0],
                                   "state": "absent", "equipment_slot": 0}])

    def test_global_ownership_census_is_partitioned_by_lineage(self):
        operation = b"a" * 16
        lineage = b"l" * 16
        result = read_uid_event_census(
            Cursor(), lineage,
            [{"uid": 1, "revision": 0}],
            {"item_references": [{
                "legacy_operation_id": operation.hex(),
                "legacy_event_index": 0, "uid": 1,
            }]},
            [], [],
        )
        (events, unreferenced, event_coverage, unanchored, ambiguous, scope,
         unattributed, unattributed_coverage) = result
        self.assertEqual(len(events), 2)
        self.assertEqual([(row['from_equipment_slot'], row['to_equipment_slot']) for row in events], [(0, 5), (0, 5)])
        self.assertEqual(events[0]["operation_outcome"], "committed")
        self.assertEqual({row["uid"] for row in unreferenced}, {5})
        self.assertEqual(event_coverage["referenced_events"], 1)
        self.assertEqual(unanchored, [5])
        self.assertEqual(ambiguous, [4])
        self.assertEqual([(row["uid"], row["action"]) for row in unattributed], [(3, "create")])
        self.assertEqual([(row['from_equipment_slot'], row['to_equipment_slot']) for row in unattributed], [(0, 5)])
        self.assertEqual(unattributed_coverage, {"uids": 1, "events": 1})
        self.assertEqual(scope, {
            "ownership_uid_count": 2, "anchored_ownership_uid_count": 1,
            "unanchored_ownership_uid_count": 1,
            "other_lineage_ownership_uid_count": 1,
            "unattributed_ownership_uid_count": 1,
            "ambiguous_lineage_ownership_uid_count": 1,
        })

    def test_mapping_creator_root_exports_cross_epoch_origin_evidence(self):
        lineage = b"l" * 16
        operation = b"a" * 16

        class MappingCursor:
            sql = ""

            def execute(self, sql, _params):
                self.sql = sql

            def fetchall(self):
                if "FROM economic_account_mapping WHERE lineage" in self.sql:
                    return [{"mapping_id": 7, "account_kind": 1, "context_id": 0,
                             "native_id": 7, "active_native_id": 7,
                             "creating_operation_id": operation}]
                if "FROM economic_accounting_operation o" in self.sql:
                    return [{"operation_id": operation, "lineage": lineage,
                             "epoch": b"e" * 16, "reason": 3, "outcome": 1,
                             "result_code": 0, "creator_inbox_status": 1,
                             "creator_inbox_result_code": 0,
                             "creator_inbox_failure_stage": 0,
                             "creator_inbox_committed_at_present": 1,
                             "source_event": None, "account_count": 1,
                             "posting_count": 0, "child_count": 0, "item_event_count": 0,
                             "account_index": 0,
                             "account_key": lineage + bytes.fromhex("01000100") +
                             (7).to_bytes(8, "little") + bytes(12),
                             "before_copper": 0, "before_silver": 0, "before_gold": 0,
                             "before_platinum": 0, "after_copper": 2, "after_silver": 0,
                             "after_gold": 0, "after_platinum": 0,
                             "before_revision": 0, "after_revision": 1}]
                raise AssertionError(f"unexpected query: {self.sql}")

        mappings, coverage, roots = read_mapping_creations(MappingCursor(), lineage)
        self.assertEqual(coverage, {"rows": 1, "creator_rows": 1,
                                    "root_rows": 1, "missing_roots": 0})
        inferred = infer_created_mapping_origins(lineage.hex(), mappings, roots, [])
        self.assertEqual(inferred, [{"account_key": mappings[0]["account_key"],
                                     "origin": "creation", "balance": [0, 0, 0, 0],
                                     "revision": 0}])

    def test_baseline_install_creator_exports_selected_epoch_receipt(self):
        lineage, epoch = b"l" * 16, b"e" * 16
        installation, baseline = b"i" * 16, b"b" * 16

        class InstallationCursor:
            sql = ""
            selected_epoch = epoch
            revision = 1
            failure_stage = 0
            committed_at_present = 1

            def execute(self, sql, _params):
                self.sql = sql

            def fetchall(self):
                if "FROM economic_account_mapping WHERE lineage" in self.sql:
                    return [{"mapping_id": 7, "account_kind": 1, "context_id": 0,
                             "native_id": 7, "active_native_id": 7,
                             "creating_operation_id": installation}]
                if "FROM economic_accounting_operation o" in self.sql:
                    return []
                if "FROM economic_sql_lifecycle_installation l" in self.sql:
                    return [{"operation_id": installation, "lineage": lineage,
                             "epoch": epoch, "baseline_operation_id": baseline,
                             "phase": 2, "selected_epoch": self.selected_epoch,
                             "revision": self.revision, "install_status": 1,
                             "install_result_code": 0,
                             "install_failure_stage": self.failure_stage,
                             "install_committed_at_present": self.committed_at_present,
                             "baseline_lineage": lineage,
                             "baseline_epoch": epoch, "baseline_reason": 38,
                             "baseline_outcome": 1, "baseline_result_code": 0}]
                raise AssertionError(f"unexpected query: {self.sql}")

        cursor = InstallationCursor()
        _, _, roots = read_mapping_creations(cursor, lineage)
        self.assertEqual(roots[0]["outcome"], "committed")
        self.assertEqual(roots[0]["installation_selected_epoch"], epoch.hex())
        self.assertEqual(roots[0]["installation_revision"], 1)
        cursor.selected_epoch = b"x" * 16
        _, _, roots = read_mapping_creations(cursor, lineage)
        self.assertEqual(roots[0]["outcome"], "unknown")
        cursor.selected_epoch = epoch
        cursor.committed_at_present = 0
        _, _, roots = read_mapping_creations(cursor, lineage)
        self.assertEqual(roots[0]["outcome"], "unknown")

    def test_pending_claim_consumer_reverse_query_finds_the_copper_debit(self):
        lineage = b"l" * 16
        operation = b"a" * 16

        class ConsumerCursor:
            sql = ""
            source_amount = Decimal("100")

            def execute(self, sql, _params):
                self.sql = sql

            def fetchall(self):
                if "FROM economic_accounting_operation o" not in self.sql or \
                        "economic_pending_claim_source" not in self.sql or \
                        "critical_operation_inbox" not in self.sql:
                    raise AssertionError(f"unexpected query: {self.sql}")
                return [{"operation_id": operation, "epoch": b"e" * 16,
                         "outcome": 1, "result_code": 0, "inbox_status": 1,
                         "inbox_result_code": 0, "inbox_failure_stage": 0,
                         "inbox_committed_at_present": 1,
                         "account_key": lineage + bytes.fromhex("01000500") +
                         (21).to_bytes(8, "little") + bytes(12),
                         "before_copper": 140, "before_silver": 0,
                         "before_gold": 0, "before_platinum": 0,
                         "after_copper": 40, "after_silver": 0,
                         "after_gold": 0, "after_platinum": 0,
                         "source_rows": 1, "source_amount": self.source_amount}]

        rows, coverage = read_pending_claim_consumers(ConsumerCursor(), lineage)
        self.assertIs(type(rows[0]["source_amount"]), int)
        self.assertEqual(json.loads(json.dumps(rows))[0]["source_amount"], 100)
        self.assertEqual(coverage, {"rows": 1, "missing_source_rows": 0,
                                    "mismatched_source_amounts": 0})
        self.assertEqual(rows[0]["pending_claim_debits"], [{
            "account_key": (lineage + bytes.fromhex("01000500") +
                            (21).to_bytes(8, "little") + bytes(12)).hex(),
            "amount": 100, "before": [140, 0, 0, 0], "after": [40, 0, 0, 0]}])
        self.assertEqual(rows[0]["inbox_receipt"], {
            "status": 1, "result_code": 0, "failure_stage": 0,
            "committed_at_present": True})

        cursor = ConsumerCursor()
        cursor.source_amount = Decimal("9007199254740993")
        rows, coverage = read_pending_claim_consumers(cursor, lineage)
        self.assertEqual(json.loads(json.dumps(rows))[0]["source_amount"], 9007199254740993)
        self.assertEqual(coverage["mismatched_source_amounts"], 1)
        cursor.source_amount = Decimal("100.5")
        with self.assertRaises(ExportError):
            read_pending_claim_consumers(cursor, lineage)


if __name__ == "__main__":
    unittest.main()
