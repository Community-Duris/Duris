#!/usr/bin/env python3
"""Exact EAB1 origin decoding and SQL read-only snapshot boundary checks."""

import copy
import hashlib
import json
from pathlib import Path
import struct
import sys
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
import economic_sql_audit_snapshot as snapshot_exporter
from economic_sql_audit_origins import OriginError, capture, decode_witness  # noqa: E402
from economic_sql_audit_snapshot import (native_source_count,
                                         read_lineage_realized_prices)  # noqa: E402

LINEAGE = bytes.fromhex("11" * 16)
EPOCH = bytes.fromhex("22" * 16)
OP = bytes.fromhex("33" * 16)


def key(kind, authority):
    return LINEAGE + struct.pack("<HHQQ4x", 1, kind, authority, 0)


OPENING = key(9, 99)


def witness():
    blob = (b"EAB1" + struct.pack("<HHII", 1, 192, 392, 0) + LINEAGE + EPOCH +
            bytes.fromhex("44" * 16) + struct.pack("<QQ", 7, 1) + OPENING +
            bytes.fromhex("55" * 32) + bytes.fromhex("66" * 32) +
            struct.pack("<II", 1, 1))
    assert len(blob) == 192
    blob += key(1, 7) + struct.pack("<4qQ", 5, 0, 0, 0, 4) + bytes.fromhex("77" * 32)
    blob += (struct.pack("<QBB6x5Q", 81, 1, 1, 7, 0, 81, 0, 2) +
             bytes.fromhex("88" * 32))
    assert len(blob) == 392
    return {"operation_id": OP, "book_revision": 1, "holding_count": 1,
            "item_count": 1, "witness_digest": hashlib.sha256(blob).digest(),
            "canonical_witness": blob, "reason": 38, "outcome": 1,
            "result_code": 0, "inbox_status": 1, "inbox_result": 0,
            "inbox_failure_stage": 0, "inbox_committed_at_present": 1}


class Cursor:
    def __init__(self, rows):
        self.rows = rows
        self.statements = []
        self.index = -1
        self.closed = False

    def execute(self, statement, params=None):
        self.index += 1
        self.statements.append((statement, params))

    def fetchone(self):
        return self.rows[self.index]

    def fetchall(self):
        return self.rows[self.index]

    def close(self):
        self.closed = True


class Connection:
    def __init__(self, control=None, rows=None):
        rows = [witness()] if rows is None else rows
        self.scan = Cursor([
            None, None,
            [{"table_name": name, "engine": "InnoDB"} for name in
             ("economic_baseline_control", "economic_baseline_witness",
              "economic_accounting_operation", "critical_operation_inbox")],
            {"opening_account": OPENING, "revision": 1, "last_operation_id": OP}
            if control is None else control,
            {"row_count": len(rows),
             "blob_bytes": sum(len(row["canonical_witness"]) for row in rows)},
            rows,
        ])
        self.rollbacks = 0

    def cursor(self):
        return self.scan

    def rollback(self):
        self.rollbacks += 1


class ItemRevisionTests(unittest.TestCase):
    @staticmethod
    def event(uid, revision, owner_revision):
        return {"operation_id": OP, "event_index": 0, "item_uid": uid,
                "root_item_uid": uid, "parent_item_uid": None,
                "to_owner_type": 1, "to_owner_id": 7, "to_owner_context_id": 0,
                "item_revision": revision, "from_owner_revision": owner_revision,
                "reason_type": 1, "operation_epoch": EPOCH, "operation_outcome": 1}

    def census(self, origins, events, unattributed=False):
        ownership = [{"item_uid": row["item_uid"],
                      "lineage": None if unattributed else LINEAGE} for row in events]
        rows = [ownership, [], events] if unattributed else [ownership, events]
        with mock.patch.object(snapshot_exporter, "bounded", side_effect=rows):
            return snapshot_exporter.read_uid_event_census(
                None, LINEAGE, origins, {"item_references": []}, [], [])

    def test_item_history_cut_uses_uid_revision_not_owner_revision(self):
        origins = [{"uid": 81, "revision": 3}]
        # Old event must be excluded despite a high aggregate counter; the
        # event after the witness must survive despite a lower owner counter.
        result = self.census(origins, [self.event(81, 3, 99), self.event(81, 4, 1)])
        self.assertEqual([(row["before_revision"], row["revision"])
                          for row in result[0]], [(3, 4)])
        self.assertEqual(result[2]["ledger_events"], 1)
        self.assertEqual(result[1], result[0])

    def test_unattributed_history_uses_uid_revision(self):
        result = self.census([{"uid": 81, "revision": 3}],
                             [self.event(81, 3, 99), self.event(81, 4, 1)], True)
        self.assertEqual([(row["before_revision"], row["revision"])
                          for row in result[6]], [(3, 4)])
        self.assertEqual(result[7], {"uids": 1, "events": 1})

    def test_lineage_reference_uses_individual_item_revision(self):
        row = {**self.event(81, 4, 99), "child_index": 0,
               "before_revision": 3, "after_revision": 4,
               "legacy_operation_id": OP, "legacy_event_index": 0,
               "epoch": EPOCH, "outcome": 1, "item_event_count": 1,
               "ledger_uid": 81}
        with mock.patch.object(snapshot_exporter, "bounded", side_effect=[[row], []]):
            references, _, _ = snapshot_exporter.read_lineage_uid_references(None, LINEAGE)
        self.assertEqual(references[0]["ledger_before_revision"], 3)
        row["ledger_uid"] = None
        row["item_revision"] = None
        with mock.patch.object(snapshot_exporter, "bounded", side_effect=[[row], []]):
            references, _, _ = snapshot_exporter.read_lineage_uid_references(None, LINEAGE)
        self.assertIsNone(references[0]["ledger_before_revision"])

    def test_zero_item_revision_refuses_before_history_filter(self):
        for unattributed in (False, True):
            with self.subTest(unattributed=unattributed):
                with self.assertRaisesRegex(snapshot_exporter.ExportError,
                                            "invalid native item ledger revision"):
                    self.census([{"uid": 81, "revision": 3}],
                                [self.event(81, 0, 99)], unattributed)


class OriginTests(unittest.TestCase):
    def test_orphan_export_is_bounded_and_does_not_invent_a_lineage(self):
        cursor = mock.Mock()
        cursor.fetchall.side_effect = [[{"operation_id": OP, "row_index": 3}], [], [], []]
        rows, coverage = snapshot_exporter.read_orphan_evidence(cursor)
        self.assertEqual(rows, [{"table": "effects", "operation_id": OP.hex(), "row_index": 3}])
        self.assertEqual(coverage, {"scope": "database", "table_counts": {
            "effects": 1, "postings": 0, "children": 0, "item_references": 0}})
        for call in cursor.execute.call_args_list:
            sql, params = call.args
            self.assertTrue(sql.startswith("SELECT "))
            self.assertIn("WHERE o.operation_id IS NULL", sql)
            self.assertNotIn("lineage=", sql)
            self.assertIn("LIMIT %s", sql)
            self.assertEqual(params, (snapshot_exporter.MAX_ROWS + 1,))
        cursor.fetchall.side_effect = [[{"operation_id": OP, "row_index": 0}],
                                      [{"operation_id": OP, "row_index": 1}]]
        with mock.patch.object(snapshot_exporter, "MAX_ROWS", 1):
            with self.assertRaisesRegex(snapshot_exporter.ExportError, "orphan evidence collection"):
                snapshot_exporter.read_orphan_evidence(cursor)

    def test_native_mapping_coverage_is_scoped_to_selected_lineage(self):
        class Cursor:
            query = None
            parameters = None

            def execute(self, query, parameters):
                self.query = query
                self.parameters = parameters

            def fetchone(self):
                return {"source_rows": 2, "unmapped_rows": 1}

        cursor = Cursor()
        self.assertEqual(native_source_count(cursor, LINEAGE, "player_data", "pid", 1),
                         (2, 1))
        self.assertIn("m.lineage=%s", cursor.query)
        self.assertEqual(cursor.parameters, (LINEAGE,))

    def test_realized_price_query_uses_registry_candidate_reasons(self):
        class Cursor:
            query = None
            parameters = None

            def execute(self, query, parameters):
                self.query = query
                self.parameters = parameters

            def fetchall(self):
                return []

        registry = json.loads((ROOT / "docs/persistence/economy_accounting/registry.json")
                              .read_text(encoding="utf-8"))
        expected = [row["number"] for row in registry["reasons"]
                    if row.get("realized_price_required") is True]
        cursor = Cursor()
        rows, coverage = read_lineage_realized_prices(cursor, LINEAGE, True)
        self.assertEqual(rows, [])
        self.assertEqual(coverage["candidate_rows"], 0)
        self.assertIn("o.reason IN (" + ",".join("%s" for _ in expected) + ")",
                      cursor.query)
        self.assertEqual(cursor.parameters[:-1], (LINEAGE, *expected))
        self.assertEqual(cursor.parameters[-1], 100_001)

    def test_exact_witness_and_read_only_capture(self):
        account_origins, item_origins = decode_witness(witness(), LINEAGE, EPOCH, OPENING)
        self.assertEqual(account_origins[0]["balance"], [5, 0, 0, 0])
        self.assertEqual(account_origins[0]["revision"], 4)
        self.assertEqual(item_origins[0]["uid"], 81)
        self.assertEqual(item_origins[0]["owner"], [1, 7, 0])
        connection = Connection()
        result = capture(connection, LINEAGE, EPOCH)
        self.assertEqual(result["format"], "economic_sql_audit_origins_v1")
        self.assertEqual(result["account_origins"], account_origins)
        self.assertEqual(result["item_origins"], item_origins)
        self.assertEqual(result["baseline_operation_ids"], [OP.hex()])
        self.assertEqual(connection.rollbacks, 1)
        self.assertTrue(connection.scan.closed)
        statements = [sql.upper() for sql, _ in connection.scan.statements]
        self.assertIn("START TRANSACTION WITH CONSISTENT SNAPSHOT, READ ONLY", statements)
        self.assertTrue(all(sql.startswith(("SET TRANSACTION", "START TRANSACTION", "SELECT"))
                            for sql in statements))
        self.assertEqual(connection.scan.statements[3][1], (LINEAGE, EPOCH))
        witness_query = connection.scan.statements[5][0]
        self.assertIn("i.failure_stage AS inbox_failure_stage", witness_query)
        self.assertIn("i.committed_at IS NOT NULL", witness_query)

    def test_digest_header_count_and_origin_corruption_refuse(self):
        for change in ("digest", "header", "count", "lineage", "source", "owner"):
            with self.subTest(change=change):
                row = copy.deepcopy(witness())
                blob = bytearray(row["canonical_witness"])
                if change == "digest":
                    row["witness_digest"] = bytes(32)
                elif change == "header":
                    blob[4] = 2
                elif change == "count":
                    row["holding_count"] = 0
                elif change == "lineage":
                    blob[16] ^= 1
                elif change == "source":
                    blob[272:304] = bytes(32)
                else:
                    blob[312] = 0
                if change != "digest":
                    row["canonical_witness"] = bytes(blob)
                    row["witness_digest"] = hashlib.sha256(blob).digest()
                with self.assertRaises((OriginError, ValueError)):
                    decode_witness(row, LINEAGE, EPOCH, OPENING)

    def test_uncommitted_or_missing_control_rolls_back(self):
        for field, value in (("inbox_status", 0), ("inbox_failure_stage", 1),
                             ("inbox_committed_at_present", 0)):
            with self.subTest(field=field):
                bad = witness()
                bad[field] = value
                connection = Connection(rows=[bad])
                with self.assertRaisesRegex(OriginError, "uncommitted"):
                    capture(connection, LINEAGE, EPOCH)
                self.assertEqual(connection.rollbacks, 1)
                self.assertTrue(connection.scan.closed)
        connection = Connection(control={"opening_account": OPENING,
                                         "revision": 2, "last_operation_id": OP})
        with self.assertRaisesRegex(OriginError, "revision gap"):
            capture(connection, LINEAGE, EPOCH)
        self.assertEqual(connection.rollbacks, 1)
        connection = Connection()
        connection.scan.rows[2][0]["engine"] = "MyISAM"
        with self.assertRaisesRegex(OriginError, "not InnoDB"):
            capture(connection, LINEAGE, EPOCH)
        self.assertEqual(connection.rollbacks, 1)

    def test_cross_witness_duplicate_origin_refuses(self):
        first = witness()
        second = copy.deepcopy(first)
        second["operation_id"] = bytes.fromhex("99" * 16)
        second["book_revision"] = 2
        connection = Connection(
            control={"opening_account": OPENING, "revision": 2,
                     "last_operation_id": second["operation_id"]}, rows=[first, second])
        with self.assertRaisesRegex(OriginError, "duplicate baseline account"):
            capture(connection, LINEAGE, EPOCH)
        self.assertEqual(connection.rollbacks, 1)


if __name__ == "__main__":
    unittest.main()
