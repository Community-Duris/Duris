#!/usr/bin/env python3
"""Focused offline budget regressions for issue #268."""
from __future__ import annotations

from types import SimpleNamespace
import sys
import time
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts" / "telemetry"))

from db_access import (  # noqa: E402
    AmbiguousCommit,
    REPORT_ROW_BYTE_BOUND,
    PyMySQLRollupDatabase,
)
from rollup import _run_killable_process, build_parser  # noqa: E402
from rollup_definitions import PUBLICATION_BUILDING, PUBLICATION_PUBLISHED, RollupTarget  # noqa: E402
from rollup_engine import BoundsExceeded, RollupBounds, RollupEngine, RollupPageResult  # noqa: E402


TARGET = RollupTarget(1, 1, 8, 7)


class _AmbiguousPublication(PyMySQLRollupDatabase):
    def __init__(self):
        super().__init__(object(), max_commit_retries=8, clock=lambda: 0.0)
        self.state = {
            "input_watermark": 0,
            "rebuild_through_ingest_id": 0,
            "publication_status": PUBLICATION_BUILDING,
        }
        self.commit_calls = 0
        self.deadlines = []

    def _ensure_connection(self):
        return object()

    def _prepare_transaction_budget(self, _bounds):
        self.deadlines.append(self._active_page_deadline)

    def _clear_transaction_budget(self):
        pass

    def _acquire_advisory_lock(self, _target, _timeout_s):
        pass

    def _release_advisory_lock(self):
        pass

    def _begin(self):
        pass

    def _rollback(self):
        pass

    def _drop_connection(self):
        pass

    def _fetch_state(self, _target, *, for_update):
        return self.state

    def _ensure_state_locked(self, _target, _through, _origin):
        return self.state

    def _published_generations_locked(self, _target):
        return []

    def _execute(self, _statement, _parameters=()):
        return [], 1, None

    def _commit(self):
        self.commit_calls += 1
        raise AmbiguousCommit("test-only publication ambiguity")


class _SlowCursor:
    description = None
    rowcount = 0

    def execute(self, _statement, _parameters=()):
        time.sleep(0.05)

    def fetchall(self):
        return []

    def close(self):
        pass


class _SlowConnection:
    def __init__(self):
        self.closed = False

    def cursor(self):
        return _SlowCursor()

    def commit(self):
        time.sleep(0.05)

    def rollback(self):
        self.closed = True

    def close(self):
        self.closed = True


class _ReportDatabase(PyMySQLRollupDatabase):
    def __init__(self):
        settings = SimpleNamespace(
            read_timeout_s=3.0,
            write_timeout_s=3.0,
        )
        factory = SimpleNamespace(settings=settings, close=lambda _connection: None)
        super().__init__(factory)
        self._connection = SimpleNamespace(_read_timeout=3.0, _write_timeout=3.0)
        self.statements = []
        self.state = {
            "definition_version": 1,
            "generation": 1,
            "environment_id": 8,
            "season_id": 7,
            "input_watermark": 3,
            "publication_status": PUBLICATION_PUBLISHED,
            "coverage_start_utc_usec": None,
            "coverage_end_utc_usec": None,
            "quality_flags": 0,
            "provisional": 1,
            "rebuild_from_ingest_id": 0,
            "rebuild_through_ingest_id": 3,
        }
        self.rows = [
            {"latest_checkpoint_revision": 0, "connected_usec": 0, "active_usec": 0,
             "idle_usec": 0, "unknown_usec": 0, "resident_usec": 0, "linkdead_usec": 0}
            for _ in range(3)
        ]

    def _ensure_connection(self):
        return self._connection

    def _execute(self, statement, parameters=()):
        self.statements.append((statement, tuple(parameters)))
        if "FROM telemetry_rollup_state" in statement:
            return [self.state], 1, None
        if "FROM telemetry_rollup_session" in statement:
            limit = int(parameters[-1])
            return self.rows[:limit], min(len(self.rows), limit), None
        return [], 0, None

    def _rollback(self):
        pass


class _SlowPageDatabase:
    def snapshot_high_watermark(self):
        return 1

    def process_next_page(self, *_args, **_kwargs):
        time.sleep(0.05)
        return RollupPageResult(status="complete", cursor=1, start_cursor=0)


class RollupBudgetTest(unittest.TestCase):
    def test_adapter_rejects_retries_above_rollup_hard_maximum(self):
        with self.assertRaises(ValueError):
            PyMySQLRollupDatabase(object(), max_commit_retries=9)
        with self.assertRaises(ValueError):
            PyMySQLRollupDatabase(object(), max_commit_retries=1_000_000)

    def test_publication_consumes_one_retry_and_deadline_budget(self):
        database = _AmbiguousPublication()
        bounds = RollupBounds(max_runtime_s=1.0, max_retries=2)
        with self.assertRaises(AmbiguousCommit):
            database.publish_generation(TARGET, bounds=bounds)
        self.assertEqual(database.commit_calls, 3)
        self.assertTrue(database.deadlines)
        self.assertEqual(len(set(database.deadlines)), 1)
        self.assertEqual(database.deadlines[0], 1.0)

    def test_sql_call_overrun_fails_closed_and_commit_overrun_is_ambiguous(self):
        database = PyMySQLRollupDatabase(object())
        connection = _SlowConnection()
        database._connection = connection
        database._deadline = time.monotonic() + 0.005
        with self.assertRaises(BoundsExceeded):
            database._execute("SELECT slow_fake_statement")
        self.assertIsNone(database._connection)

        database = PyMySQLRollupDatabase(object())
        connection = _SlowConnection()
        database._connection = connection
        database._deadline = time.monotonic() + 0.005
        with self.assertRaises(AmbiguousCommit):
            database._commit()
        self.assertIsNone(database._connection)

    def test_engine_rejects_a_page_that_returns_after_deadline(self):
        with self.assertRaises(BoundsExceeded):
            RollupEngine(_SlowPageDatabase()).run(
                TARGET,
                through_ingest_id=1,
                bounds=RollupBounds(page_size=1, max_rows=1, max_runtime_s=0.005),
            )

    def test_report_reader_has_explicit_byte_cap_and_sql_sentinel_limit(self):
        database = _ReportDatabase()
        with self.assertRaises(BoundsExceeded):
            database.read_report(TARGET, "session_playtime", max_rows=1,
                                 max_bytes=REPORT_ROW_BYTE_BOUND, max_runtime_s=1.0)

        database = _ReportDatabase()
        snapshot = database.read_report(
            TARGET,
            "session_playtime",
            max_rows=1,
            max_bytes=REPORT_ROW_BYTE_BOUND * 3,
            max_runtime_s=1.0,
        )
        self.assertEqual(len(snapshot.rows), 1)
        self.assertTrue(snapshot.truncated)
        report_sql = next(sql for sql, _params in database.statements
                          if "FROM telemetry_rollup_session" in sql)
        report_params = next(params for sql, params in database.statements
                             if "FROM telemetry_rollup_session" in sql)
        self.assertEqual(report_params[-1], 2)
        self.assertIn("LIMIT %s", report_sql)

    def test_cli_exposes_budgets_for_publication_and_report(self):
        parser = build_parser()
        publish = parser.parse_args([
            "publish", "--definition-version", "1", "--generation", "1",
            "--environment-id", "8", "--season-id", "7",
            "--max-runtime-s", "4", "--max-retries", "1",
        ])
        report = parser.parse_args([
            "report", "--definition-version", "1", "--generation", "1",
            "--environment-id", "8", "--season-id", "7", "--name", "session_playtime",
            "--max-runtime-s", "5", "--max-bytes", "8192",
        ])
        self.assertEqual((publish.max_runtime_s, publish.max_retries), (4.0, 1))
        self.assertEqual((report.max_runtime_s, report.max_bytes), (5.0, 8192))

    def test_cli_watchdog_kills_a_nonreturning_worker(self):
        started = time.monotonic()
        with self.assertRaises(BoundsExceeded):
            _run_killable_process(time.sleep, (1.0,), timeout_s=0.2)
        self.assertLess(time.monotonic() - started, 0.8)


if __name__ == "__main__":
    unittest.main(verbosity=2)
