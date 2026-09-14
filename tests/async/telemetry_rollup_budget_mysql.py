#!/usr/bin/env python3
"""Real disposable-SQL budget acceptance checks for issue #268."""
from __future__ import annotations

import os
from pathlib import Path
import re
import unittest

import pymysql

ROOT = Path(__file__).resolve().parents[2]
import sys
sys.path.insert(0, str(ROOT))

from scripts.telemetry.db_access import (  # noqa: E402
    REPORT_ROW_BYTE_BOUND,
    BoundsExceeded,
    ConnectionSettings,
    PyMySQLConnectionFactory,
    PyMySQLRollupDatabase,
)
from scripts.telemetry.rollup_engine import RollupBounds, RollupEngine  # noqa: E402
from scripts.telemetry.rollup_definitions import RollupTarget  # noqa: E402
from telemetry_rollup_scenarios import non_additive_membership  # noqa: E402


TABLES = (
    "telemetry_rollup_session",
    "telemetry_player_day",
    "telemetry_cohort_day",
    "telemetry_cohort_member",
    "telemetry_rollup_state",
    "telemetry_interval",
)


class RollupRealBudgetTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if os.environ.get("TELEMETRY_ROLLUP_TEST") != "1":
            raise RuntimeError("explicit disposable SQL opt-in required")
        cls.name = os.environ["ROLLUP_TEST_DATABASE"]
        if not re.fullmatch(r"duris_268_[a-z0-9_]*test", cls.name):
            raise RuntimeError("requires task-owned duris_268_*test database")
        cls.admin = pymysql.connect(
            host="127.0.0.1",
            user="root",
            database=cls.name,
            password=os.environ["ROLLUP_TEST_ROOT_PASSWORD"],
            autocommit=True,
            cursorclass=pymysql.cursors.DictCursor,
            connect_timeout=2,
            read_timeout=3,
        )

    @classmethod
    def tearDownClass(cls):
        cls.admin.close()

    def setUp(self):
        self.adapters = []
        with self.admin.cursor() as cursor:
            for table in TABLES:
                cursor.execute("TRUNCATE TABLE " + table)

    def tearDown(self):
        for adapter in self.adapters:
            adapter.close()

    def database(self, role):
        settings = ConnectionSettings(
            host="127.0.0.1",
            database=self.name,
            user="duris268_" + role,
            password=os.environ["ROLLUP_TEST_" + role.upper() + "_PASSWORD"],
        )
        adapter = PyMySQLRollupDatabase(PyMySQLConnectionFactory(settings))
        self.adapters.append(adapter)
        return adapter

    def load(self, rows):
        with self.admin.cursor() as cursor:
            for row in rows:
                columns = tuple(row)
                cursor.execute(
                    "INSERT INTO telemetry_interval (" + ",".join(columns) + ") VALUES ("
                    + ",".join(["%s"] * len(columns)) + ")",
                    tuple(row[column] for column in columns),
                )

    def test_real_report_query_obeys_row_byte_and_time_budgets(self):
        rows, _expected = non_additive_membership()
        self.load(rows)
        target = RollupTarget(1, 1, 8, 7)
        writer = self.database("rollup")
        result = RollupEngine(writer).run(target, bounds=RollupBounds(page_size=2))
        self.assertTrue(result.complete)
        RollupEngine(writer).publish(target, bounds=RollupBounds(max_runtime_s=5.0, max_retries=1))

        reader = self.database("report")
        with self.assertRaises(BoundsExceeded):
            reader.read_report(
                target,
                "session_playtime",
                max_rows=1,
                max_bytes=REPORT_ROW_BYTE_BOUND,
                max_runtime_s=5.0,
            )

        limits = []
        execute = reader._execute

        def capture(statement, parameters=()):
            if "FROM telemetry_rollup_session" in statement:
                limits.append(parameters[-1])
            return execute(statement, parameters)

        reader._execute = capture
        snapshot = RollupEngine(reader).report(
            target,
            "session_playtime",
            max_rows=1,
            max_bytes=REPORT_ROW_BYTE_BOUND * 4,
            max_runtime_s=5.0,
        )
        self.assertEqual(len(snapshot.rows), 1)
        self.assertTrue(snapshot.truncated)
        self.assertEqual(limits, [2])

        with self.assertRaises(BoundsExceeded):
            reader.read_membership_contributions(
                target,
                membership_kind=1,
                max_rows=1,
                max_bytes=REPORT_ROW_BYTE_BOUND * 4,
                max_runtime_s=5.0,
            )

        with self.assertRaises(BoundsExceeded):
            reader.read_report(
                target,
                "session_playtime",
                max_rows=1,
                max_bytes=REPORT_ROW_BYTE_BOUND * 4,
                max_runtime_s=0.000001,
            )


if __name__ == "__main__":
    unittest.main(verbosity=2)
