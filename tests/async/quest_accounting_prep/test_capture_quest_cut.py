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
            self.rows = [dict(table_name=name, engine=self.connection.engine) for name in params]
        elif "SUM(OCTET_LENGTH" in query:
            self.rows = [dict(size=self.connection.blob_sizes.get(table, 0), row_count=0)]
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
        self.blob_sizes = {}
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
        self.assertEqual(result["snapshot"], [dict(in_transaction=1, isolation_level="REPEATABLE-READ")])
        self.assertEqual(connection.reader_cursor.queries[:2], [
            ("SET TRANSACTION ISOLATION LEVEL REPEATABLE READ", ()),
            ("START TRANSACTION WITH CONSISTENT SNAPSHOT, READ ONLY", ())])
        self.assert_clean(connection)

    def test_empty_selection_is_identical_to_old_default(self):
        rows = [item(9, owner=1, owner_id=42)]
        self.assertEqual(capture(ReadConnection(rows)), capture(ReadConnection(rows), item_uids=[]))

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
        self.assertEqual(result["items"], [])
        self.assertTrue(connection.closed)
        self.assert_clean(connection)

    def test_cli_noninteger_selection_refuses_without_connecting(self):
        with tempfile.TemporaryDirectory(prefix="quest-capture-reader-") as directory:
            with self.assertRaises(SystemExit) as failure:
                self.cli(ReadConnection(), directory, ["--item-uid", "1;DROP TABLE items"])
            self.assertEqual(failure.exception.code, 2)
            self.assertIn("invalid int value", failure.exception.reader_stderr)
            self.assertEqual(failure.exception.reader_connect_calls, 0)
            self.assertFalse((Path(directory) / "capture.json").exists())

    def test_cli_refusals_and_read_failure_preserve_safeguards(self):
        for extra, env, message, read_failure in (
            (["--item-uid", "1", "1"], {}, "duplicate", False),
            (["--item-uid", str(2**64)], {}, "unsigned native UID", False),
            (["--item-uid", "500"], {"TEST_DB_HOST": "remote.invalid"}, "loopback", False),
            (["--item-uid", "500"], {"TEST_DB_DISPOSABLE": "0"}, "disposable", False),
            (["--database", "duris_production", "--item-uid", "500"], {}, "schema name", False),
            (["--server-sha256", "00" * 32, "--item-uid", "500"], {}, "binary pin", False),
            (["--item-uid", "500"], {}, "SQL read failed with code 1234", True),
        ):
            with self.subTest(message=message), tempfile.TemporaryDirectory(prefix="quest-capture-reader-") as directory:
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
