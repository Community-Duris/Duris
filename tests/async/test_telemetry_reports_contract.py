#!/usr/bin/env python3
"""Offline contracts for the #269 administrator report interface."""
from __future__ import annotations

from datetime import date
from dataclasses import replace
import json
import io
from contextlib import redirect_stderr
from unittest.mock import patch
import os
from pathlib import Path
import sys
import tempfile
import time
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts" / "telemetry"))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from report import (  # noqa: E402
    AdministratorReportService,
    ConnectionSettings,
    BoundsExceeded,
    ReportCache,
    ReportDatabase,
    ReportFilters,
    ReportRequest,
    _QueryPage,
    _build_query,
    _decode_cursor,
    _encode_cursor,
    _run_killable_process,
    main as report_main,
    report_catalog,
    render_catalog_text,
)
from rollup_definitions import RollupTarget  # noqa: E402
from telemetry_reports_fixtures import (  # noqa: E402
    cohort_rows,
    expected_playtime_summary,
    member_rows,
    session_rows,
    state_row,
)


TARGET = RollupTarget(1, 7, 8, 9)


class _FixedReportDatabase(ReportDatabase):
    def __init__(self, rows, *, name, truncated=False, sentinel=None):
        super().__init__(object())
        self.fixed_rows = tuple(rows)
        self.fixed_name = name
        self.fixed_truncated = truncated
        self.fixed_sentinel = sentinel

    def _begin_snapshot(self):
        pass

    def _read_published_state(self, _request):
        return state_row()

    def _read_page(self, _request, _target):
        return _QueryPage(
            rows=self.fixed_rows,
            sentinel=self.fixed_sentinel,
            truncated=self.fixed_truncated,
            next_cursor=None,
            source_rows_fetched=len(self.fixed_rows) + (1 if self.fixed_sentinel else 0),
        )

    def close(self):
        pass


class _FailingFactory:
    settings = ConnectionSettings(host="localhost", database="offline", user="report", password="")

    def connect(self):
        raise OSError("offline test")

    def close(self, _connection):
        pass


class TelemetryReportsContractTest(unittest.TestCase):
    def request(self, name="cohort", **kwargs):
        return ReportRequest(
            report_name=name,
            definition_version=1,
            environment_id=8,
            season_id=9,
            generation=7,
            **kwargs,
        )

    def test_catalog_has_only_supported_aggregate_reports(self):
        catalog = {entry["name"]: entry for entry in report_catalog()}
        self.assertEqual(set(catalog), {"playtime", "cohort", "return", "time", "faction"})
        for entry in catalog.values():
            self.assertEqual(entry["definition_version"], 1)
            self.assertFalse(entry["account_metrics_available"])
            self.assertNotIn("xp_per_hour", entry["metrics"])
            self.assertNotIn("progression_metrics", entry["metrics"])
            self.assertIn("account_distinct_count", entry["unavailable_metrics"])
            self.assertIn("quality_flags", entry["metrics"])
        self.assertIn("microseconds", catalog["playtime"]["units"]["*_usec"])
        self.assertIn("fraction", catalog["return"]["units"]["return_fraction"])

    def test_catalog_text_is_connection_free_and_explicit(self):
        text = render_catalog_text()
        self.assertIn("report_schema_version: 1", text)
        self.assertIn("playtime", text)
        self.assertIn("return_fraction", text)
        self.assertIn("unavailable:", text)

    def test_scoped_filters_are_sql_predicates_before_limit(self):
        request = self.request(
            filters=ReportFilters(
                date_from=date(2024, 1, 2),
                date_to=date(2024, 1, 3),
                level_band=50,
                faction_id=4,
                category=2,
            )
        )
        sql, parameters, order = _build_query(request, TARGET, None, 2)
        self.assertIn("FROM telemetry_cohort_day FORCE INDEX (PRIMARY)", sql)
        self.assertIn("utc_day >= %s", sql)
        self.assertIn("utc_day < %s", sql)
        self.assertIn("level_band=%s", sql)
        self.assertIn("faction_id=%s", sql)
        self.assertIn("category=%s", sql)
        self.assertLess(sql.index("WHERE"), sql.index("LIMIT"))
        self.assertEqual(parameters[:4], TARGET.scope_tuple)
        self.assertEqual(parameters[4:9], (date(2024, 1, 2), date(2024, 1, 3), 50, 4, 2))
        self.assertEqual(parameters[-1], 2)
        self.assertEqual(order, ("utc_day", "level_band", "class_id", "race_id", "faction_id", "zone_vnum", "config_id", "category"))

    def test_grouped_seek_is_inserted_before_group_by(self):
        request = self.request(
            "time",
            filters=ReportFilters(date_from=date(2024, 1, 1), date_to=date(2024, 1, 4)),
        )
        first_sql, first_params, order = _build_query(request, TARGET, None, 2)
        row = {"utc_day": date(2024, 1, 2), "category": 2}
        cursor = _encode_cursor(request, TARGET, order, row)
        paged = replace(request, after=cursor)
        values = _decode_cursor(cursor, paged, TARGET, order)
        second_sql, second_params, _ = _build_query(paged, TARGET, values, 2)
        self.assertNotIn("GROUP BY utc_day,category AND", second_sql)
        self.assertLess(second_sql.index("category>%s"), second_sql.index("GROUP BY"))
        self.assertLess(second_sql.index("GROUP BY"), second_sql.index("ORDER BY"))
        self.assertEqual(first_params[-1], 2)
        self.assertEqual(second_params[-3:-1], (date(2024, 1, 2), 2))
        self.assertEqual(second_sql.count("LIMIT %s"), 1)

    def test_cursor_is_bound_to_scope_and_filter_set(self):
        request = self.request(filters=ReportFilters(faction_id=4))
        order = ("utc_day", "category")
        row = {"utc_day": date(2024, 1, 2), "category": 2}
        token = _encode_cursor(request, TARGET, order, row)
        self.assertEqual(_decode_cursor(token, request, TARGET, order), (date(2024, 1, 2), 2))
        with self.assertRaises(ValueError):
            _decode_cursor(token, replace(request, filters=ReportFilters(faction_id=8)), TARGET, order)
        with self.assertRaises(ValueError):
            _decode_cursor(token, request, RollupTarget(1, 8, 8, 9), order)

    def test_playtime_summary_keeps_checkpoint_plane_separate(self):
        database = _FixedReportDatabase(session_rows(), name="playtime")
        response = database.read(self.request("playtime"))
        expected = expected_playtime_summary()
        for key, value in expected.items():
            self.assertEqual(response["summary"][key], value)
        self.assertEqual(response["summary"]["checkpoint_totals_available_session_count"], 1)
        self.assertFalse(response["summary"]["checkpoint_totals_summed"])
        available = next(row for row in response["rows"] if row["session_boot_id"] == 11)
        unavailable = next(row for row in response["rows"] if row["session_boot_id"] == 12)
        self.assertTrue(available["checkpoint_totals_available"])
        self.assertEqual(available["connected_usec"], 3_000_000)
        self.assertFalse(unavailable["checkpoint_totals_available"])
        self.assertIsNone(unavailable["connected_usec"])
        self.assertIsNone(unavailable["active_usec"])
        self.assertEqual(response["summary"]["active_fraction"]["numerator"], 4_000_000)
        self.assertEqual(response["summary"]["active_fraction"]["denominator"], 9_000_000)
        self.assertTrue(response["provisional"])
        self.assertEqual(response["watermark"], 42)

    def test_cohort_unknown_day_is_explicit_and_counts_are_not_summed(self):
        database = _FixedReportDatabase(cohort_rows(), name="cohort")
        response = database.read(self.request("cohort"))
        unknown = next(row for row in response["rows"] if row["bucket_kind"] == "unknown")
        self.assertIsNone(unknown["utc_day"])
        self.assertEqual(unknown["category_name"], "unknown")
        calendar = next(row for row in response["rows"] if row["bucket_kind"] == "calendar")
        self.assertEqual(calendar["utc_day"], "2024-01-01")
        self.assertEqual(response["summary"]["duration_usec"], 10_000_000)
        self.assertFalse(response["summary"]["distinct_counts_additive"])
        self.assertEqual(response["coverage"]["snapshot_high_watermark"], 42)
        json.dumps(response)

    def test_empty_report_is_complete_with_explicit_zero_denominator(self):
        response = _FixedReportDatabase([], name="cohort").read(self.request("cohort"))
        self.assertEqual(response["rows"], [])
        self.assertTrue(response["summary"]["complete"])
        self.assertEqual(response["summary"]["cohort_row_count"], 0)
        self.assertEqual(response["summary"]["duration_usec"], 0)
        self.assertIsNone(response["summary"]["attributable_fraction"]["value"])
        self.assertEqual(response["summary"]["attributable_fraction"]["denominator"], 0)
        self.assertEqual(response["summary"]["quality_flags"], 0)

    def test_return_summary_uses_distinct_logical_session_identity(self):
        database = _FixedReportDatabase(member_rows(), name="return")
        response = database.read(self.request("return"))
        self.assertEqual(response["summary"]["subject_count"], 2)
        self.assertEqual(response["summary"]["returning_subject_count"], 1)
        self.assertEqual(response["summary"]["logical_session_count"], 3)
        self.assertEqual(response["summary"]["return_fraction"]["numerator"], 1)
        self.assertEqual(response["summary"]["return_fraction"]["denominator"], 2)
        self.assertIsNone(response["summary"]["distinct_account_count"])

    def test_cache_returns_last_published_page_on_connection_failure(self):
        request = self.request("cohort")
        payload = {
            "report": "cohort",
            "scope": {"generation": 7},
            "rows": [],
            "truncated": False,
            "served_from_cache": False,
        }
        with tempfile.TemporaryDirectory(prefix="duris-269-report-cache-") as directory:
            cache = ReportCache(directory)
            service = AdministratorReportService(_FailingFactory(), cache=cache)
            cache.write(request, payload, namespace=service.cache_namespace)
            response = service.read(request)
            self.assertTrue(response["served_from_cache"])
            self.assertEqual(response["cache"]["status"], "last_published")
            mode = Path(directory).stat().st_mode & 0o777
            self.assertEqual(mode, 0o700)
            cached_mode = next(Path(directory).glob("*.json")).stat().st_mode & 0o777
            self.assertEqual(cached_mode, 0o600)

    def test_report_source_has_no_raw_fact_or_game_connection_boundary(self):
        source = (ROOT / "scripts/telemetry/report.py").read_text(encoding="utf-8")
        self.assertNotIn("telemetry_interval", source)
        self.assertIn("TELEMETRY_REPORT_DB_", source)
        self.assertNotIn('os.environ.get("DB_USER")', source)
        self.assertNotIn('os.environ.get("DB_PASSWD")', source)
        self.assertIn("READ ONLY", source)
        self.assertIn("START TRANSACTION WITH CONSISTENT SNAPSHOT", source)

    def test_cli_deadline_is_a_clean_failure_not_a_traceback(self):
        with patch("report._run_killable_process", side_effect=BoundsExceeded("test deadline")):
            with redirect_stderr(io.StringIO()) as errors:
                status = report_main(["report", "--name", "cohort",
                                      "--definition-version", "1", "--environment-id", "8", "--season-id", "9"])
        self.assertEqual(status, 2)
        self.assertIn("test deadline", errors.getvalue())

    def test_full_json_payload_must_fit_byte_budget(self):
        request = self.request("cohort", max_bytes=4096)
        database = _FixedReportDatabase(cohort_rows(), name="cohort")
        try:
            response = database.read(request)
        except BoundsExceeded:
            return  # Fail-closed if metadata and rows cannot fit.
        self.assertLessEqual(len((json.dumps(response, sort_keys=True) + "\n").encode()), request.max_bytes)

    def test_cli_worker_is_killable(self):
        started = time.monotonic()
        with self.assertRaises(BoundsExceeded):
            _run_killable_process(time.sleep, (1.0,), timeout_s=0.1)
        self.assertLess(time.monotonic() - started, 0.8)


if __name__ == "__main__":
    unittest.main(verbosity=2)
