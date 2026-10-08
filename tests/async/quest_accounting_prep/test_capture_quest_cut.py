#!/usr/bin/env python3
"""Reader/query unit evidence with a fake cursor; never SQL/native acceptance."""

import copy
import io
import json
import os
from pathlib import Path
import re
import sys
import tempfile
import types
import unittest
from unittest.mock import Mock, patch

import capture_quest_cut as reader


LINEAGE, EPOCH = "11" * 16, "22" * 16
META = dict(source_commit="33" * 20, binary_sha256="44" * 32,
            schema_manifest_sha256="55" * 32, lineage=LINEAGE, epoch=EPOCH)


def item(uid, *, vnum=999001, owner=12, owner_id=900, root=None, parent=None, state=1):
    return dict(item_uid=uid, root_item_uid=uid if root is None else root,
                parent_item_uid=parent, owner_type=owner, owner_id=owner_id,
                owner_context_id=0, item_revision=2, vnum=vnum, state=state, equipment_slot=0)


def event(uid, operation=bytes.fromhex("66" * 16)):
    return dict(item_uid=uid, operation_id=operation, event_index=uid,
                root_item_uid=uid, parent_item_uid=None)


def mapping(mapping_id, *, native_id=900, active=True, lineage=LINEAGE, backend=1, locator=12):
    return dict(mapping_id=mapping_id, lineage=bytes.fromhex(lineage), account_kind=1,
                context_id=12, backend_kind=backend, locator_kind=locator, native_id=native_id,
                active_native_id=native_id if active else None,
                creating_operation_id=bytes.fromhex("77" * 16),
                retiring_operation_id=None if active else bytes.fromhex("88" * 16),
                revision=0 if active else 1)


class ReadCursor:
    """Interpret only the reader's filters; record the actual SQL and bindings."""

    def __init__(self, connection):
        self.connection = connection
        self.queries = []
        self.rows = []
        self.closed = False

    def execute(self, query, params=()):
        self.queries.append((query, tuple(params)))
        if query.count("%s") != len(params):
            raise AssertionError("SQL placeholder/binding count differs")
        if self.connection.failure and self.connection.failure[0] in query:
            raise self.connection.failure[1]
        if query.startswith("SET TRANSACTION ISOLATION LEVEL REPEATABLE READ"):
            self.rows = []
            return
        if query == "START TRANSACTION WITH CONSISTENT SNAPSHOT, READ ONLY":
            self.connection.in_transaction = 1
            self.rows = []
            return
        if not query.startswith("SELECT "):
            raise AssertionError("reader attempted a write")
        if "@@in_transaction" in query:
            self.rows = [dict(in_transaction=self.connection.in_transaction)]
            if "@@tx_isolation" in query:
                self.rows[0]["isolation_level"] = "REPEATABLE-READ"
            return
        table = re.search(r" FROM ([a-z_.]+)", query).group(1)
        if table == "information_schema.tables":
            self.rows = [dict(table_name=name, engine=self.connection.engines.get(name, self.connection.engine))
                         for name in params if name not in self.connection.missing_tables]
        elif "SUM(OCTET_LENGTH" in query:
            self.rows = [dict(size=self.connection.blob_sizes.get(table, 0),
                              row_count=self.connection.blob_counts.get(table, 0))]
        elif table == "economic_account_mapping":
            self.rows = sorted([row for row in self.connection.data.get(table, []) if row["mapping_id"] in params],
                               key=lambda row: row["mapping_id"])
        elif table == "economic_lineage_state":
            self.rows = [row for row in self.connection.data.get(table, []) if row["lineage"] == params[0]]
        elif table == "item_current_owner":
            offset = 1
            vnums, mobiles, watched = set(), set(), set()
            for clause, target in (("vnum", vnums), ("owner_id", mobiles), ("item_uid", watched)):
                match = re.search(r"\b" + clause + r" IN \(([^)]+)\)", query)
                if match:
                    count = match.group(1).count("%s")
                    target.update(params[offset:offset + count])
                    offset += count
            if offset != len(params):
                raise AssertionError("unexpected item selection shape")
            self.rows = sorted([row for row in self.connection.data[table] if
                (row["owner_type"] == 1 and row["owner_id"] == params[0]) or
                row["vnum"] in vnums or
                (row["owner_type"] == 12 and row["owner_id"] in mobiles) or
                row["item_uid"] in watched], key=lambda row: row["item_uid"])
        elif table == "item_ownership_ledger":
            self.rows = sorted([row for row in self.connection.data[table] if row["item_uid"] in params],
                               key=lambda row: (row["operation_id"], row["event_index"]))
        elif table == "economic_epoch":
            self.rows = self.connection.epochs
        elif "WHERE operation_id IN (" in query:
            self.rows = [row for row in self.connection.data.get(table, []) if row["operation_id"] in params]
        else:
            self.rows = self.connection.data.get(table, [])
        self.rows = copy.deepcopy(self.rows)
        match = re.search(r" LIMIT (\d+)$", query)
        if match:
            self.rows = self.rows[:int(match.group(1))]

    def fetchall(self):
        return self.rows

    def fetchone(self):
        return self.rows[0]

    def close(self):
        self.closed = True


class ReadConnection:
    def __init__(self, items=(), history=(), *, legacy=False):
        self.data = dict(item_current_owner=list(items), item_ownership_ledger=list(history),
                         player_data=[dict(pid=42, copper=0, silver=0, gold=0, platinum=0, exp=0)])
        self.epochs = [] if legacy else [dict(lineage=bytes.fromhex(LINEAGE), epoch=bytes.fromhex(EPOCH))]
        self.engine = "InnoDB"
        self.engines = {}
        self.missing_tables = set()
        self.blob_sizes = {}
        self.blob_counts = {}
        self.failure = None
        self.in_transaction = 0
        self.rollback_calls = 0
        self.rollback_error = None
        self.keep_transaction = False
        self.closed = False
        self.cursor_calls = 0
        self.reader_cursor = ReadCursor(self)

    def cursor(self):
        self.cursor_calls += 1
        return self.reader_cursor

    def rollback(self):
        self.rollback_calls += 1
        if self.rollback_error:
            raise self.rollback_error
        if not self.keep_transaction:
            self.in_transaction = 0

    def close(self):
        self.closed = True


def capture(connection, **kwargs):
    return reader.capture(connection, "QP03", 42, [], [], dict(META), **kwargs)


class CaptureReaderTests(unittest.TestCase):
    def assert_clean(self, connection):
        self.assertEqual(connection.rollback_calls, 1)
        self.assertEqual(connection.in_transaction, 0)
        self.assertTrue(connection.reader_cursor.closed)

    def queries(self, connection, table):
        return [(sql, params) for sql, params in connection.reader_cursor.queries if " FROM " + table + " " in sql]

    def test_original_default_filters_and_output(self):
        connection = ReadConnection([
            item(1, owner=1, owner_id=42), item(2, vnum=16013, owner=8, state=2),
            item(3), item(4, owner=3, owner_id=500), item(5, owner=1, owner_id=99)],
            [event(uid) for uid in range(1, 6)])
        result = capture(connection)
        self.assertEqual([row["item_uid"] for row in result["items"]], [1, 2])
        self.assertEqual([row["item_uid"] for row in result["ownership_events"]], [1, 2])
        sql, params = self.queries(connection, "item_current_owner")[0]
        self.assertEqual(sql, "SELECT item_uid,root_item_uid,parent_item_uid,owner_type,owner_id,owner_context_id,"
            "item_revision,vnum,state,equipment_slot FROM item_current_owner WHERE (owner_type=1 AND owner_id=%s)"
            " OR (vnum IN (%s,%s,%s,%s,%s)) ORDER BY item_uid LIMIT 2049")
        self.assertEqual(params, (42, 16013, 16014, 16015, 16075, 16080))
        self.assertNotIn("watched_item_uids", result["meta"])
        self.assertNotIn("missing_item_uids", result["meta"])
        self.assertNotIn("account_mappings", result)
        self.assertNotIn("lineage_head", result)
        self.assertNotIn("watched_mapping_ids", result["meta"])
        self.assertNotIn("missing_mapping_ids", result["meta"])
        self.assertNotIn("mapping_observation_authenticated", result["meta"])
        self.assertEqual(self.queries(connection, "economic_account_mapping"), [])
        self.assertEqual(self.queries(connection, "economic_lineage_state"), [])
        self.assertNotIn("economic_account_mapping", self.queries(connection, "information_schema.tables")[0][1])
        self.assertEqual(result["snapshot"], [dict(in_transaction=1, isolation_level="REPEATABLE-READ")])
        self.assertEqual(connection.reader_cursor.queries[:2], [
            ("SET TRANSACTION ISOLATION LEVEL REPEATABLE READ", ()),
            ("START TRANSACTION WITH CONSISTENT SNAPSHOT, READ ONLY", ())])
        self.assert_clean(connection)

    def test_empty_selection_is_identical_to_old_default(self):
        rows = [item(9, owner=1, owner_id=42)]
        self.assertEqual(capture(ReadConnection(rows)), capture(ReadConnection(rows), item_uids=[]))
        self.assertEqual(capture(ReadConnection(rows)), capture(ReadConnection(rows), mapping_ids=[]))

    def test_original_mobile_filter_is_preserved(self):
        connection = ReadConnection([item(1), item(2, owner_id=901), item(3, owner=3, owner_id=900)])
        # No native image is supplied by this fake: this is a filter test only.
        result = reader.capture(connection, "QP03", 42, [900], [], dict(META), item_uids=[2])
        self.assertEqual([row["item_uid"] for row in result["items"]], [1, 2])
        sql, params = self.queries(connection, "item_current_owner")[0]
        self.assertIn("(owner_type=12 AND owner_id IN (%s)) OR (item_uid IN (%s))", sql)
        self.assertEqual(params[-2:], (900, 2))
        self.assert_clean(connection)

    def test_observed_foreign_root_and_descendant_survive_movement_and_destruction(self):
        before = ReadConnection([item(101), item(102, root=101, parent=101)], [event(101), event(102)])
        after = ReadConnection([item(101, owner=3, owner_id=777),
                                item(102, owner=8, root=101, parent=101, state=2)],
                               [event(101), event(102)])
        first = reader.capture(before, "QP03", 42, [900], [], dict(META))
        observed = [row["item_uid"] for row in first["items"]]
        self.assertEqual(observed, [101, 102])
        self.assertEqual(capture(ReadConnection(after.data["item_current_owner"])),
                         capture(ReadConnection()))
        result = capture(after, item_uids=observed)
        self.assertEqual(result["items"], after.data["item_current_owner"])
        self.assertEqual([row["item_uid"] for row in result["ownership_events"]], observed)
        self.assertEqual(result["meta"]["watched_item_uids"], observed)
        self.assertEqual(result["meta"]["missing_item_uids"], [])
        self.assert_clean(before)
        self.assert_clean(after)

    def test_missing_current_row_keeps_history_without_reconstructing_item(self):
        connection = ReadConnection(history=[event(200)])
        result = capture(connection, item_uids=[201, 200])
        self.assertEqual(result["items"], [])
        self.assertEqual(result["meta"]["watched_item_uids"], [200, 201])
        self.assertEqual(result["meta"]["missing_item_uids"], [200, 201])
        self.assertEqual([row["item_uid"] for row in result["ownership_events"]], [200])
        self.assertEqual(self.queries(connection, "item_ownership_ledger")[0][1], (200, 201))
        # Real reader discovers the retained operation from history, then queries
        # economic evidence. Empty fake evidence remains empty, never synthesized.
        self.assertEqual(self.queries(connection, "economic_accounting_operation")[0][1],
                         (bytes.fromhex("66" * 16),))
        self.assertEqual(result["operations"], [])
        self.assert_clean(connection)

    def test_selection_is_not_recursive_and_queries_bind_exact_uids(self):
        high = 2**64 - 2
        connection = ReadConnection([item(high, owner=3), item(400, owner=3, root=high, parent=high)])
        result = capture(connection, item_uids=[high])
        self.assertEqual([row["item_uid"] for row in result["items"]], [high])
        sql, params = self.queries(connection, "item_current_owner")[0]
        self.assertIn("(item_uid IN (%s))", sql)
        self.assertNotIn(str(high), sql)
        self.assertEqual(params[-1], high)
        self.assert_clean(connection)

    def test_union_does_not_duplicate_existing_items_or_history_bindings(self):
        connection = ReadConnection([item(10, owner=1, owner_id=42), item(11, owner=3)], [event(10), event(11)])
        result = capture(connection, item_uids=[11, 10])
        self.assertEqual([row["item_uid"] for row in result["items"]], [10, 11])
        self.assertEqual(self.queries(connection, "item_ownership_ledger")[0][1], (10, 11))
        self.assert_clean(connection)

    def test_invalid_duplicate_and_oversized_selections_refuse_before_sql(self):
        for selection in ([0], [-1], [True], [1.0], ["1"], [None], [2**64 - 1], [2**64],
                          [1, 1], list(range(1, reader.MAX_ROWS + 2))):
            with self.subTest(selection=repr(selection)[:70]):
                connection = ReadConnection()
                with self.assertRaises(ValueError):
                    capture(connection, item_uids=selection)
                self.assertEqual(connection.cursor_calls, 0)
                self.assertEqual(connection.rollback_calls, 0)

    def test_bounded_iterator_stops_at_first_excess_uid(self):
        visited = []
        def selection():
            for uid in range(1, reader.MAX_ROWS + 100):
                visited.append(uid)
                yield uid
        with self.assertRaisesRegex(ValueError, "selection exceeds"):
            capture(ReadConnection(), item_uids=selection())
        self.assertEqual(len(visited), reader.MAX_ROWS + 1)

    def test_maximum_distinct_selection_is_allowed_but_missing_rows_stay_absent(self):
        connection = ReadConnection()
        watched = list(range(1, reader.MAX_ROWS + 1))
        result = capture(connection, item_uids=watched)
        self.assertEqual(result["items"], [])
        self.assertEqual(result["meta"]["missing_item_uids"], watched)
        self.assertEqual(len(self.queries(connection, "item_ownership_ledger")[0][1]), reader.MAX_ROWS)
        self.assert_clean(connection)

    def test_item_and_history_row_budgets_still_refuse_and_cleanup(self):
        for overflow in ("items", "history"):
            with self.subTest(overflow=overflow):
                connection = ReadConnection(
                    [item(uid, vnum=16013) for uid in range(1, reader.MAX_ROWS + 2)] if overflow == "items" else [],
                    [event(1)] * (reader.MAX_ROWS + 1) if overflow == "history" else [])
                with self.assertRaisesRegex(ValueError, "row limit"):
                    capture(connection, item_uids=[1])
                self.assert_clean(connection)

    def test_original_byte_blob_and_aggregate_budgets_refuse_and_cleanup(self):
        row = item(1, owner=3)
        row["reader_unit_padding"] = "x" * reader.MAX_BYTES
        connection = ReadConnection([row])
        with self.assertRaisesRegex(ValueError, "byte limit"):
            capture(connection, item_uids=[1])
        self.assert_clean(connection)
        connection = ReadConnection()
        connection.blob_sizes["quest_reward_obligation"] = reader.MAX_BYTES // 2 + 1
        with self.assertRaisesRegex(ValueError, "BLOB budget"):
            capture(connection, item_uids=[1])
        self.assert_clean(connection)
        connection = ReadConnection()
        # Individually bounded projections can still exceed the original total.
        padding = "x" * (reader.MAX_BYTES // 2)
        connection.data["player_data"][0]["reader_unit_padding"] = padding
        connection.data["world_quest_accomplished"] = [dict(reader_unit_padding=padding)]
        with self.assertRaisesRegex(ValueError, "combined quest cut byte limit"):
            capture(connection, item_uids=[1])
        self.assert_clean(connection)

    def test_original_engine_epoch_player_and_legacy_gates_remain(self):
        for gate in ("engine", "epoch", "player", "legacy-epoch", "legacy-identity"):
            with self.subTest(gate=gate):
                connection = ReadConnection()
                kwargs = dict(item_uids=[1])
                if gate == "engine":
                    connection.engine = "MyISAM"
                elif gate == "epoch":
                    connection.epochs = []
                elif gate == "player":
                    connection.data["player_data"] = []
                else:
                    kwargs["legacy_no_epoch"] = True
                meta = dict(META)
                if gate == "legacy-epoch":
                    meta.update(lineage=None, epoch=None)
                with self.assertRaises(ValueError):
                    reader.capture(connection, "QP03", 42, [], [], meta, **kwargs)
                self.assert_clean(connection)
        connection = ReadConnection(legacy=True)
        result = reader.capture(connection, "QP03", 42, [], [], dict(META, lineage=None, epoch=None),
                                legacy_no_epoch=True, item_uids=[1])
        self.assertEqual(result["meta"]["authority"], "legacy-no-epoch")
        self.assertEqual(result["meta"]["missing_item_uids"], [1])
        self.assert_clean(connection)

    def test_read_failure_still_rolls_back_and_closes_cursor(self):
        connection = ReadConnection()
        connection.failure = ("FROM item_current_owner", RuntimeError("fake read failure"))
        with self.assertRaisesRegex(RuntimeError, "fake read failure"):
            capture(connection, item_uids=[1])
        self.assert_clean(connection)

    def test_rollback_failure_and_uncleared_transaction_close_cursor(self):
        for failed_rollback in (False, True):
            with self.subTest(failed_rollback=failed_rollback):
                connection = ReadConnection()
                if failed_rollback:
                    connection.rollback_error = RuntimeError("fake rollback failure")
                    error, message = RuntimeError, "fake rollback failure"
                else:
                    connection.keep_transaction = True
                    error, message = ValueError, "transaction did not close"
                with self.assertRaisesRegex(error, message):
                    capture(connection, item_uids=[1])
                self.assertEqual(connection.rollback_calls, 1)
                self.assertTrue(connection.reader_cursor.closed)

    def test_explicit_mapping_ids_retain_original_retired_replacement_and_foreign_literals(self):
        connection = ReadConnection()
        rows = [mapping(900, active=False), mapping(901, native_id=901),
                mapping(902, lineage="99" * 16, backend=2, locator=54321)]
        connection.data["economic_account_mapping"] = rows + [mapping(903)]
        head = dict(lineage=bytes.fromhex(LINEAGE), active_epoch=bytes.fromhex("aa" * 16), revision=7)
        connection.data["economic_lineage_state"] = [head,
            dict(lineage=bytes.fromhex("99" * 16), active_epoch=bytes.fromhex(EPOCH), revision=8)]
        result = reader.capture(connection, "QP03", 42, [900], [], dict(META), mapping_ids=[902, 900, 901])
        expected = [{key: value.hex() if isinstance(value, bytes) else value for key, value in row.items()}
                    for row in rows]
        self.assertEqual(result["account_mappings"], expected)
        self.assertEqual(result["meta"]["watched_mapping_ids"], [900, 901, 902])
        self.assertEqual(result["meta"]["missing_mapping_ids"], [])
        self.assertIs(result["meta"]["mapping_observation_authenticated"], False)
        self.assertEqual(result["meta"]["authority"], "native")
        self.assertEqual(result["lineage_head"], [dict(lineage=LINEAGE, active_epoch="aa" * 16, revision=7)])
        sql, params = self.queries(connection, "economic_account_mapping")[0]
        self.assertEqual(sql, "SELECT mapping_id,lineage,account_kind,context_id,backend_kind,locator_kind,native_id,"
            "active_native_id,creating_operation_id,retiring_operation_id,revision FROM economic_account_mapping "
            "WHERE mapping_id IN (%s,%s,%s) ORDER BY mapping_id LIMIT 2049")
        self.assertEqual(params, (900, 901, 902))
        self.assertEqual(self.queries(connection, "economic_lineage_state"), [
            ("SELECT lineage,active_epoch,revision FROM economic_lineage_state WHERE lineage=%s LIMIT 2049",
             (bytes.fromhex(LINEAGE),))])
        self.assertEqual(connection.cursor_calls, 1)
        self.assertFalse(any("LOCK" in query or "FOR UPDATE" in query for query, _ in connection.reader_cursor.queries))
        self.assert_clean(connection)

    def test_equal_numeric_native_and_mapping_ids_do_not_infer_a_selection(self):
        connection = ReadConnection()
        connection.data["economic_account_mapping"] = [mapping(900), mapping(901, native_id=900)]
        result = reader.capture(connection, "QP02", 42, [900], [], dict(META))
        self.assertNotIn("account_mappings", result)
        self.assertEqual(self.queries(connection, "economic_account_mapping"), [])
        result = reader.capture(connection, "QP02", 42, [900], [], dict(META), mapping_ids=[901])
        self.assertEqual([row["mapping_id"] for row in result["account_mappings"]], [901])
        self.assertEqual(result["account_mappings"][0]["native_id"], 900)
        self.assertEqual(self.queries(connection, "economic_account_mapping")[0][1], (901,))
        result = reader.capture(ReadConnection(), "QP02", 42, [900], [], dict(META), mapping_ids=[901])
        self.assertEqual(result["account_mappings"], [])
        self.assertEqual(result["meta"]["missing_mapping_ids"], [901])
        self.assertEqual(connection.rollback_calls, 2)
        self.assertTrue(connection.reader_cursor.closed)

    def test_absent_mapping_and_missing_or_null_lineage_head_stay_observations(self):
        for head in ([], [dict(lineage=bytes.fromhex(LINEAGE), active_epoch=None, revision=9)]):
            with self.subTest(head=head):
                connection = ReadConnection()
                connection.data["economic_account_mapping"] = [mapping(20)]
                connection.data["economic_lineage_state"] = head
                result = capture(connection, mapping_ids=[21, 20])
                self.assertEqual(result["meta"]["missing_mapping_ids"], [21])
                self.assertEqual(result["lineage_head"], [] if not head else
                                 [dict(lineage=LINEAGE, active_epoch=None, revision=9)])
                self.assertEqual(result["operations"], [])
                self.assertEqual(result["inbox"], [])
                self.assert_clean(connection)

    def test_mapping_lifetime_ids_join_existing_historical_observations_without_synthesis(self):
        connection = ReadConnection(history=[event(200)])
        connection.data["economic_account_mapping"] = [mapping(10, active=False), mapping(11)]
        ids = tuple(bytes.fromhex(pair * 16) for pair in ("66", "77", "88"))
        receipt = dict(operation_id=ids[1], result_payload=b"literal-reader-unit-payload", status=2)
        connection.data["critical_operation_inbox"] = [receipt,
            dict(operation_id=bytes.fromhex("99" * 16), result_payload=b"unselected", status=2)]
        result = capture(connection, mapping_ids=[11, 10], item_uids=[200])
        self.assertEqual(result["inbox"], [dict(operation_id=ids[1].hex(),
            result_payload=b"literal-reader-unit-payload".hex(), status=2)])
        self.assertEqual(result["operations"], [])  # absent receipts/operations are never invented
        for table in ("economic_accounting_item_reference", "economic_accounting_operation",
                      "critical_operation_inbox", "economic_accounting_account_effect",
                      "economic_accounting_coin_posting", "economic_accounting_source_claim"):
            for _, params in self.queries(connection, table):
                self.assertEqual(params, ids)
        inbox = self.queries(connection, "critical_operation_inbox")
        self.assertIn("SUM(OCTET_LENGTH(result_payload))", inbox[0][0])
        self.assertNotIn("SUM(OCTET_LENGTH", inbox[1][0])
        self.assert_clean(connection)

    def test_mapping_selection_validation_and_bounded_iterator_before_sql(self):
        for selection in ([0], [-1], [True], [1.0], ["1"], [None], [2**64], [1, 1],
                          list(range(1, reader.MAX_ROWS + 2))):
            with self.subTest(selection=repr(selection)[:70]):
                connection = ReadConnection()
                with self.assertRaises(ValueError):
                    capture(connection, mapping_ids=selection)
                self.assertEqual(connection.cursor_calls, 0)
        visited = []
        def selection():
            for mapping_id in range(1, reader.MAX_ROWS + 100):
                visited.append(mapping_id)
                yield mapping_id
        with self.assertRaisesRegex(ValueError, "selection exceeds"):
            capture(ReadConnection(), mapping_ids=selection())
        self.assertEqual(len(visited), reader.MAX_ROWS + 1)

    def test_mapping_unsigned_sql_maximum_and_selection_budget_are_allowed(self):
        connection = ReadConnection()
        maximum = 2**64 - 1
        connection.data["economic_account_mapping"] = [mapping(maximum)]
        watched = list(range(1, reader.MAX_ROWS)) + [maximum]
        result = capture(connection, mapping_ids=watched)
        sql, params = self.queries(connection, "economic_account_mapping")[0]
        self.assertEqual(params, tuple(watched))
        self.assertNotIn(str(maximum), sql)
        self.assertEqual(result["account_mappings"][0]["mapping_id"], maximum)
        self.assertEqual(result["meta"]["missing_mapping_ids"], watched[:-1])
        self.assert_clean(connection)

    def test_mapping_tables_are_transactional_only_when_opted_in(self):
        for table in ("economic_account_mapping", "economic_lineage_state"):
            for missing in (False, True):
                with self.subTest(table=table, missing=missing):
                    connection = ReadConnection()
                    if missing:
                        connection.missing_tables.add(table)
                    else:
                        connection.engines[table] = "MyISAM"
                    with self.assertRaisesRegex(ValueError, "missing or not transactional"):
                        capture(connection, mapping_ids=[10])
                    self.assert_clean(connection)
                    # No new engine dependency on the original default path.
                    capture(connection)

    def test_mapping_reads_and_confirmed_rollback_refuse_without_returning_a_cut(self):
        for table in ("economic_account_mapping", "economic_lineage_state", "critical_operation_inbox"):
            with self.subTest(table=table):
                connection = ReadConnection()
                connection.data["economic_account_mapping"] = [mapping(10)]
                connection.failure = ("FROM " + table, RuntimeError("mapping read failed"))
                with self.assertRaisesRegex(RuntimeError, "mapping read failed"):
                    capture(connection, mapping_ids=[10])
                self.assert_clean(connection)
        for failed_rollback in (False, True):
            connection = ReadConnection()
            if failed_rollback:
                connection.rollback_error = RuntimeError("mapping rollback failed")
                error, message = RuntimeError, "mapping rollback failed"
            else:
                connection.keep_transaction = True
                error, message = ValueError, "transaction did not close"
            with self.assertRaisesRegex(error, message):
                capture(connection, mapping_ids=[10])
            self.assertEqual(connection.rollback_calls, 1)
            self.assertTrue(connection.reader_cursor.closed)

    def test_mapping_row_byte_aggregate_and_receipt_blob_limits_keep_cleanup(self):
        for budget in ("row", "byte", "aggregate", "blob", "combined-blob", "blob-row"):
            with self.subTest(budget=budget):
                connection = ReadConnection()
                connection.data["economic_account_mapping"] = [mapping(10)]
                selection = [10]
                if budget == "row":
                    # A corrupt/non-schema cursor can return excess rows despite the predicate.
                    connection.data["economic_account_mapping"] *= reader.MAX_ROWS + 1
                    message = "row limit"
                elif budget in ("byte", "aggregate"):
                    connection.data["economic_account_mapping"][0]["reader_unit_padding"] = "x" * (
                        reader.MAX_BYTES if budget == "byte" else reader.MAX_BYTES // 2)
                    if budget == "aggregate":
                        connection.data["player_data"][0]["reader_unit_padding"] = "x" * (reader.MAX_BYTES // 2)
                    message = "byte limit"
                else:
                    if budget == "blob-row":
                        connection.blob_counts["critical_operation_inbox"] = reader.MAX_ROWS + 1
                    elif budget == "combined-blob":
                        connection.blob_sizes["quest_reward_obligation"] = reader.MAX_BYTES // 4
                        connection.blob_sizes["critical_operation_inbox"] = reader.MAX_BYTES // 4 + 1
                    else:
                        connection.blob_sizes["critical_operation_inbox"] = reader.MAX_BYTES // 2 + 1
                    message = "receipt BLOB budget"
                with self.assertRaisesRegex(ValueError, message):
                    capture(connection, mapping_ids=selection)
                if budget in ("blob", "combined-blob", "blob-row"):
                    self.assertEqual(len(self.queries(connection, "critical_operation_inbox")), 1)
                self.assert_clean(connection)

    def test_optional_mapping_legacy_refusal_preserves_empty_legacy_path(self):
        connection = ReadConnection(legacy=True)
        meta = dict(META, lineage=None, epoch=None)
        with self.assertRaisesRegex(ValueError, "legacy capture cannot"):
            reader.capture(connection, "QP03", 42, [], [], meta, legacy_no_epoch=True, mapping_ids=[10])
        self.assertEqual(connection.cursor_calls, 0)
        result = reader.capture(connection, "QP03", 42, [], [], meta, legacy_no_epoch=True, mapping_ids=[])
        self.assertEqual(result["meta"]["authority"], "legacy-no-epoch")
        self.assertNotIn("account_mappings", result)
        self.assert_clean(connection)

    def cli(self, connection, directory, extra=(), environment=None):
        class FakeSQLError(Exception):
            pass
        client = types.SimpleNamespace(connect=Mock(return_value=connection), MySQLError=FakeSQLError,
                                       cursors=types.SimpleNamespace(DictCursor=object()))
        output = Path(directory) / "capture.json"
        args = ["capture_quest_cut.py", "--case", "QP03", "--database", "quest_accounting_test_" + "ab" * 8,
                "--pid", "42", "--lineage", LINEAGE, "--epoch", EPOCH, "--server", "unused-reader-unit-ELF",
                "--server-sha256", "44" * 32, "--source-commit", "33" * 20, "--output", str(output), *extra]
        env = dict(TEST_DB_DISPOSABLE="1", TEST_DB_HOST="127.0.0.1", TEST_DB_USER="reader-unit",
                   TEST_DB_PASSWORD="reader-unit-placeholder")
        env.update(environment or {})
        with patch.object(sys, "argv", args), patch.dict(os.environ, env, clear=True), \
             patch.dict(sys.modules, pymysql=client), patch.object(reader, "digest", return_value="44" * 32), \
             patch("sys.stdout", new_callable=io.StringIO), patch("sys.stderr", new_callable=io.StringIO) as stderr:
            try:
                reader.main()
            except SystemExit as error:
                error.reader_stderr = stderr.getvalue()
                error.reader_connect_calls = client.connect.call_count
                raise
        return client, output

    def test_cli_passes_watched_uids_and_closes_connection(self):
        connection = ReadConnection([item(500, owner=3)])
        with tempfile.TemporaryDirectory(prefix="quest-capture-reader-") as directory:
            client, output = self.cli(connection, directory, ["--item-uid", "501", "--item-uid", "500"])
            result = json.loads(output.read_text())
        self.assertEqual(result["meta"]["watched_item_uids"], [500, 501])
        self.assertEqual(result["meta"]["missing_item_uids"], [501])
        self.assertEqual(result["items"][0]["item_uid"], 500)
        self.assertEqual(client.connect.call_args.kwargs["host"], "127.0.0.1")
        self.assertTrue(connection.closed)
        self.assert_clean(connection)

    def test_cli_without_option_preserves_output(self):
        connection = ReadConnection([item(500, owner=3)])
        with tempfile.TemporaryDirectory(prefix="quest-capture-reader-") as directory:
            _, output = self.cli(connection, directory)
            result = json.loads(output.read_text())
        self.assertNotIn("watched_item_uids", result["meta"])
        self.assertNotIn("missing_item_uids", result["meta"])
        self.assertNotIn("watched_mapping_ids", result["meta"])
        self.assertNotIn("account_mappings", result)
        self.assertNotIn("lineage_head", result)
        self.assertEqual(result["items"], [])
        self.assertTrue(connection.closed)
        self.assert_clean(connection)

    def test_cli_noninteger_selection_refuses_without_connecting(self):
        for option in ("--item-uid", "--mapping-id"):
            with self.subTest(option=option), tempfile.TemporaryDirectory(prefix="quest-capture-reader-") as directory:
                with self.assertRaises(SystemExit) as failure:
                    self.cli(ReadConnection(), directory, [option, "1;DROP TABLE items"])
                self.assertEqual(failure.exception.code, 2)
                self.assertIn("invalid int value", failure.exception.reader_stderr)
                self.assertEqual(failure.exception.reader_connect_calls, 0)
                self.assertFalse((Path(directory) / "capture.json").exists())

    def test_cli_mapping_selection_output_and_deadlines(self):
        connection = ReadConnection()
        connection.data["economic_account_mapping"] = [mapping(500, active=False)]
        with tempfile.TemporaryDirectory(prefix="quest-capture-reader-") as directory:
            client, output = self.cli(connection, directory, ["--mapping-id", "501", "--mapping-id", "500"])
            result = json.loads(output.read_text())
        self.assertEqual(result["meta"]["watched_mapping_ids"], [500, 501])
        self.assertEqual(result["meta"]["missing_mapping_ids"], [501])
        self.assertEqual(result["account_mappings"][0]["mapping_id"], 500)
        self.assertIsNone(result["account_mappings"][0]["active_native_id"])
        self.assertEqual(result["lineage_head"], [])
        self.assertIs(result["meta"]["mapping_observation_authenticated"], False)
        self.assertEqual({key: client.connect.call_args.kwargs[key] for key in
            ("host", "connect_timeout", "read_timeout", "write_timeout", "autocommit")},
            dict(host="127.0.0.1", connect_timeout=5, read_timeout=10, write_timeout=5, autocommit=True))
        self.assertTrue(connection.closed)
        self.assert_clean(connection)

    def test_cli_mapping_legacy_option_refuses_before_connect(self):
        with tempfile.TemporaryDirectory(prefix="quest-capture-reader-") as directory:
            with self.assertRaises(SystemExit) as failure:
                self.cli(ReadConnection(legacy=True), directory, ["--legacy-no-epoch", "--mapping-id", "10"])
            self.assertIn("legacy capture cannot", failure.exception.reader_stderr)
            self.assertEqual(failure.exception.reader_connect_calls, 0)
            self.assertFalse((Path(directory) / "capture.json").exists())

    def test_cli_output_remains_exclusive_with_optional_mapping(self):
        connection = ReadConnection()
        with tempfile.TemporaryDirectory(prefix="quest-capture-reader-") as directory:
            output = Path(directory) / "capture.json"
            output.write_text("reader-unit-existing-output")
            with self.assertRaises(SystemExit) as failure:
                self.cli(connection, directory, ["--mapping-id", "10"])
            self.assertEqual(failure.exception.code, 1)
            self.assertEqual(output.read_text(), "reader-unit-existing-output")
        self.assertTrue(connection.closed)
        self.assert_clean(connection)

    def test_cli_refusals_and_read_failure_preserve_safeguards(self):
        cases = (
            (["--item-uid", "1", "1"], {}, "duplicate", False),
            (["--item-uid", str(2**64)], {}, "unsigned native UID", False),
            (["--item-uid", "500"], {"TEST_DB_HOST": "remote.invalid"}, "loopback", False),
            (["--item-uid", "500"], {"TEST_DB_DISPOSABLE": "0"}, "disposable", False),
            (["--database", "duris_production", "--item-uid", "500"], {}, "schema name", False),
            (["--server-sha256", "00" * 32, "--item-uid", "500"], {}, "binary pin", False),
            (["--item-uid", "500"], {}, "SQL read failed with code 1234", True),
        )
        for option in ("--item-uid", "--mapping-id"):
            for extra, env, message, read_failure in cases:
                extra = [option if arg == "--item-uid" else arg for arg in extra]
                if option == "--mapping-id":
                    message = message.replace("unsigned native UID", "unsigned SQL ID")
                with self.subTest(option=option, message=message), tempfile.TemporaryDirectory(prefix="quest-capture-reader-") as directory:
                    connection = ReadConnection()
                    # main catches the client's error class, so inject it via execute
                    # only after the fake client is installed by the CLI wrapper.
                    if read_failure:
                        def fail_query(query, params=()):
                            raise sys.modules["pymysql"].MySQLError(1234, "reader-unit error")
                        connection.reader_cursor.execute = fail_query
                    with self.assertRaises(SystemExit) as failure:
                        self.cli(connection, directory, extra, env)
                    self.assertEqual(failure.exception.code, 1)
                    self.assertIn(message, failure.exception.reader_stderr)
                    self.assertFalse((Path(directory) / "capture.json").exists())
                    if read_failure:
                        self.assertTrue(connection.closed)
                        self.assertEqual(connection.rollback_calls, 1)
                        self.assertTrue(connection.reader_cursor.closed)
                    else:
                        self.assertEqual(connection.cursor_calls, 0)
                        self.assertEqual(failure.exception.reader_connect_calls, 0)


if __name__ == "__main__":
    unittest.main()
