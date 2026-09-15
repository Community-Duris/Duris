#!/usr/bin/env python3
"""Real MariaDB acceptance tests for the #269 report reader.

This manual suite requires an explicitly provisioned, task-owned
``duris_269_*test`` database and a separate SELECT-only report role. It never
creates production objects and never targets the configured game database.
The database must already contain the #268 aggregate tables plus the raw table
used only by the denial assertion.
"""
from __future__ import annotations

from datetime import date
from dataclasses import replace
import os
from pathlib import Path
import re
import sys
import tempfile
import unittest

import pymysql

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts" / "telemetry"))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from db_access import ConnectionSettings, PyMySQLConnectionFactory  # noqa: E402
from report import (  # noqa: E402
    AdministratorReportService,
    ReportCache,
    ReportFilters,
    ReportNotPublished,
    ReportDatabaseError,
    ReportRequest,
    _build_query,
)
from rollup_definitions import RollupTarget  # noqa: E402
from telemetry_reports_fixtures import (  # noqa: E402
    cohort_rows,
    member_rows,
    session_rows,
    state_row,
)


TABLES = (
    "telemetry_rollup_session",
    "telemetry_player_day",
    "telemetry_cohort_day",
    "telemetry_cohort_member",
    "telemetry_rollup_state",
)
TARGET = RollupTarget(1, 7, 8, 9)


class _FailingFactory:
    def __init__(self, settings):
        self.settings = settings

    def connect(self):
        raise OSError("intentional report outage")

    def close(self, _connection):
        pass


class TelemetryReportsMariaDBTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if os.environ.get("TELEMETRY_REPORT_TEST") != "1":
            raise RuntimeError("explicit TELEMETRY_REPORT_TEST=1 required")
        cls.database = os.environ.get("REPORT_TEST_DATABASE", "")
        if not re.fullmatch(r"duris_269_[a-z0-9_]*test", cls.database):
            raise RuntimeError("REPORT_TEST_DATABASE must be a task-owned duris_269_*test database")
        cls.host = os.environ.get("REPORT_TEST_HOST", "127.0.0.1")
        cls.port = int(os.environ.get("REPORT_TEST_PORT", "3306"))
        cls.admin_user = os.environ.get("REPORT_TEST_ADMIN_USER", "root")
        cls.admin_password = os.environ.get("REPORT_TEST_ADMIN_PASSWORD")
        cls.report_user = os.environ.get("REPORT_TEST_REPORT_USER", "duris269_report")
        cls.report_password = os.environ.get("REPORT_TEST_REPORT_PASSWORD")
        if cls.admin_password is None or cls.report_password is None:
            raise RuntimeError("REPORT_TEST_ADMIN_PASSWORD and REPORT_TEST_REPORT_PASSWORD are required")
        common = {
            "host": cls.host,
            "port": cls.port,
            "database": cls.database,
            "charset": "utf8mb4",
            "autocommit": True,
            "cursorclass": pymysql.cursors.DictCursor,
            "connect_timeout": 2,
            "read_timeout": 3,
            "write_timeout": 3,
        }
        cls.admin = pymysql.connect(user=cls.admin_user, password=cls.admin_password, **common)
        cls.report = pymysql.connect(user=cls.report_user, password=cls.report_password, **common)
        with cls.admin.cursor() as cursor:
            cursor.execute(
                "SELECT table_name AS table_name FROM information_schema.tables "
                "WHERE table_schema=DATABASE() AND table_name IN (" +
                ",".join(["%s"] * len((*TABLES, "telemetry_interval"))) + ")",
                (*TABLES, "telemetry_interval"),
            )
            names = {row["table_name"] for row in cursor.fetchall()}
        required = set(TABLES) | {"telemetry_interval"}
        if names != required:
            missing = sorted(required - names)
            raise RuntimeError("#268 aggregate/raw fixture tables are missing: " + ",".join(missing))

    def setUp(self):
        self.reset_storage()
        self.seed_storage()

    @classmethod
    def tearDownClass(cls):
        if hasattr(cls, "admin"):
            cls.reset_storage()
            cls.report.close()
            cls.admin.close()

    @classmethod
    def reset_storage(cls):
        with cls.admin.cursor() as cursor:
            for table in (*TABLES, "telemetry_interval"):
                cursor.execute("TRUNCATE TABLE " + table)

    @classmethod
    def seed_storage(cls):
        with cls.admin.cursor() as cursor:
            state = state_row()
            cls.insert(cursor, "telemetry_rollup_state", state)
            building = state_row(generation=8, publication_status=0)
            cls.insert(cursor, "telemetry_rollup_state", building)
            for row in session_rows():
                cls.insert(cursor, "telemetry_rollup_session", row | {
                    "definition_version": 1,
                    "generation": 7,
                    "environment_id": 8,
                    "season_id": 9,
                })
            for row in cohort_rows():
                cls.insert(cursor, "telemetry_cohort_day", row | {
                    "definition_version": 1,
                    "generation": 7,
                    "environment_id": 8,
                    "season_id": 9,
                })
            for row in member_rows():
                cls.insert(cursor, "telemetry_cohort_member", row | {
                    "definition_version": 1,
                    "generation": 7,
                    "environment_id": 8,
                    "season_id": 9,
                })

    @staticmethod
    def insert(cursor, table, row):
        columns = tuple(row)
        cursor.execute(
            "INSERT INTO " + table + " (" + ",".join(columns) + ") VALUES (" +
            ",".join(["%s"] * len(columns)) + ")",
            tuple(row[column] for column in columns),
        )

    def factory(self):
        settings = ConnectionSettings(
            host=self.host,
            port=self.port,
            database=self.database,
            user=self.report_user,
            password=self.report_password,
            read_timeout_s=3.0,
            write_timeout_s=3.0,
        )
        return PyMySQLConnectionFactory(settings)

    def service(self, cache=None):
        return AdministratorReportService(self.factory(), cache=cache)

    @staticmethod
    def request(name="cohort", **kwargs):
        generation = kwargs.pop("generation", 7)
        return ReportRequest(
            report_name=name,
            definition_version=1,
            environment_id=8,
            season_id=9,
            generation=generation,
            **kwargs,
        )

    def test_filtered_keyset_pages_skip_nonmatching_rows_exactly(self):
        filtered = self.service().read(self.request(
            max_rows=1,
            filters=ReportFilters(
                date_from=date(2024, 1, 2),
                date_to=date(2024, 1, 3),
                faction_id=4,
            ),
        ))
        self.assertEqual(len(filtered["rows"]), 1)
        self.assertEqual(filtered["rows"][0]["faction_id"], 4)
        self.assertFalse(filtered["truncated"])

        request = self.request("cohort", max_rows=1)
        seen = []
        pages = 0
        while True:
            response = self.service().read(request)
            pages += 1
            seen.extend(
                (
                    row["utc_day"],
                    row["level_band"],
                    row["faction_id"],
                    row["category"],
                )
                for row in response["rows"]
            )
            if not response["has_more"]:
                break
            self.assertTrue(response["next_cursor"])
            request = ReportRequest(
                report_name="cohort",
                definition_version=1,
                environment_id=8,
                season_id=9,
                generation=7,
                after=response["next_cursor"],
                max_rows=1,
            )
        self.assertEqual(pages, 4)
        self.assertEqual(len(seen), 4)
        self.assertEqual(len(set(seen)), 4)

    def test_time_and_faction_summaries_use_aggregate_only_rows(self):
        time_report = self.service().read(self.request("time"))
        self.assertEqual(time_report["summary"]["duration_usec"], 10_000_000)
        self.assertEqual(time_report["summary"]["cohort_bucket_count"], 4)
        self.assertEqual({row["category"] for row in time_report["rows"]}, {0, 1, 2})
        faction_report = self.service().read(self.request("faction"))
        self.assertEqual(faction_report["summary"]["duration_usec"], 10_000_000)
        self.assertEqual(faction_report["summary"]["cohort_bucket_count"], 4)
        self.assertEqual({row["faction_id"] for row in faction_report["rows"]}, {0, 4, 8})
        self.assertNotIn("bucket_kind", faction_report["rows"][0])

    def test_empty_published_scope_returns_no_rows_and_null_rate(self):
        self.reset_storage()
        with self.admin.cursor() as cursor:
            self.insert(cursor, "telemetry_rollup_state", state_row())
        response = self.service().read(self.request("cohort"))
        self.assertEqual(response["rows"], [])
        self.assertTrue(response["summary"]["complete"])
        self.assertIsNone(response["summary"]["attributable_fraction"]["value"])
        self.assertEqual(response["summary"]["attributable_fraction"]["denominator"], 0)

    def test_cache_cannot_cross_database_identity_during_outage(self):
        with tempfile.TemporaryDirectory(prefix="duris-269-db-bound-cache-") as directory:
            first = AdministratorReportService(self.factory(), cache=ReportCache(directory))
            first.read(self.request("cohort"))
            other = self.factory()
            other.settings = replace(other.settings, database=self.database + "_other")
            with self.assertRaises(ReportDatabaseError):
                AdministratorReportService(other, cache=ReportCache(directory)).read(self.request("cohort"))

    def test_return_summary_counts_distinct_sessions_without_account_linkage(self):
        response = self.service().read(self.request("return", max_rows=100))
        self.assertEqual(response["summary"]["subject_count"], 2)
        self.assertEqual(response["summary"]["returning_subject_count"], 1)
        self.assertEqual(response["summary"]["logical_session_count"], 3)
        self.assertIsNone(response["summary"]["distinct_account_count"])
        self.assertFalse(response["definition"]["account_metrics_available"])

    def test_newest_published_generation_is_served_while_generation_eight_builds(self):
        request = ReportRequest(
            report_name="playtime",
            definition_version=1,
            environment_id=8,
            season_id=9,
            max_rows=100,
        )
        response = self.service().read(request)
        self.assertEqual(response["scope"]["generation"], 7)
        self.assertTrue(response["provisional"])
        rows = {row["session_boot_id"]: row for row in response["rows"]}
        self.assertTrue(rows[11]["checkpoint_totals_available"])
        self.assertEqual(rows[11]["connected_usec"], 3_000_000)
        self.assertFalse(rows[12]["checkpoint_totals_available"])
        self.assertIsNone(rows[12]["connected_usec"])
        with self.assertRaises(ReportNotPublished):
            self.service().read(self.request("playtime", generation=8))

    def test_last_published_cache_survives_report_connection_outage(self):
        request = self.request("cohort")
        with tempfile.TemporaryDirectory(prefix="duris-269-real-cache-") as directory:
            cache = ReportCache(directory)
            fresh = self.service(cache=cache).read(request)
            offline = _FailingFactory(self.factory().settings)
            cached = AdministratorReportService(offline, cache=cache).read(request)
            self.assertFalse(fresh["served_from_cache"])
            self.assertTrue(cached["served_from_cache"])
            self.assertEqual(cached["cache"]["status"], "last_published")
            self.assertEqual(cached["scope"], fresh["scope"])
            self.assertEqual(cached["rows"], fresh["rows"])

    def test_read_only_role_denies_raw_and_mutating_queries(self):
        with self.report.cursor() as cursor:
            for statement in (
                "SELECT ingest_id FROM telemetry_interval LIMIT 1",
                "SELECT subject_id FROM telemetry_player_day LIMIT 1",
                "UPDATE telemetry_rollup_state SET input_watermark=0 WHERE 0",
                "CREATE TABLE duris_269_forbidden (id INT)",
            ):
                with self.subTest(statement=statement), self.assertRaises(pymysql.err.OperationalError) as error:
                    cursor.execute(statement)
                self.assertIn(error.exception.args[0], (1044, 1142))

    def test_grouped_provisional_flag_preserves_any_uncertain_member(self):
        with self.admin.cursor() as cursor:
            cursor.execute("UPDATE telemetry_cohort_day SET provisional=0 WHERE utc_day='2024-01-01'")
        report = self.service().read(self.request("faction"))
        group = next(row for row in report["rows"] if row["faction_id"] == 4)
        self.assertEqual(group["provisional"], 1)

    def test_return_pages_never_claim_population_completeness(self):
        # A subject can recur after another subject: storage order is by date
        # and dimensions, NOT by subject. Per-page counts cannot be added.
        with self.admin.cursor() as cursor:
            cursor.execute("UPDATE telemetry_cohort_member SET utc_day='2024-01-03' WHERE session_boot_id=12")
        request = self.request("return", max_rows=1)
        recovered = {}
        pages = 0
        while True:
            response = self.service().read(request)
            pages += 1
            self.assertFalse(response["summary"]["complete"])
            self.assertIsNone(response["summary"]["return_fraction"]["value"])
            for row in response["rows"]:
                self.assertIsNone(row["returning_subject"])
                recovered.setdefault(row["subject_id"], set()).update(
                    tuple(identity) for identity in row["logical_session_ids"])
            if not response["has_more"]:
                break
            request = self.request("return", max_rows=1, after=response["next_cursor"])
        self.assertEqual(pages, 3)
        self.assertEqual({key: len(value) for key, value in recovered.items()}, {1001: 2, 1002: 1})

    def test_aggregate_query_plans_use_scoped_indexes(self):
        requests = (
            self.request("cohort", filters=ReportFilters(date_from=date(2024, 1, 2))),
            self.request("playtime"),
            self.request("return", filters=ReportFilters(date_from=date(2024, 1, 2))),
        )
        targets = (TARGET, TARGET, TARGET)
        expected_keys = ("PRIMARY", "idx_rollup_session_subject", "PRIMARY")
        for request, target, expected in zip(requests, targets, expected_keys, strict=True):
            sql, parameters, _order = _build_query(request, target, None, 2)
            self.assertNotIn("telemetry_interval", sql)
            with self.subTest(report=request.report_name), self.report.cursor() as cursor:
                cursor.execute("EXPLAIN " + sql, parameters)
                plan = cursor.fetchone()
                self.assertEqual(plan["key"], expected)
                self.assertIn(plan["type"], {"range", "ref", "index"})


if __name__ == "__main__":
    unittest.main(verbosity=2)
