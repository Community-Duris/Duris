#!/usr/bin/env python3
"""Focused offline contract tests for the #268 external rollup engine."""
from __future__ import annotations

from collections import defaultdict
from copy import deepcopy
from datetime import date
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts" / "telemetry"))
sys.path.insert(0, str(ROOT / "tests" / "async"))

from rollup_definitions import (  # noqa: E402
    COHORT_ACTIVITY,
    COUNTER_FIELDS,
    ROLLUP_QUALITY_LATE_INPUT,
    ROLLUP_QUALITY_UTC_BACKWARD,
    ROLLUP_QUALITY_UTC_FANOUT,
    ROLLUP_QUALITY_UTC_MISMATCH,
    ROLLUP_QUALITY_UTC_UNKNOWN,
    RollupTarget,
    UNKNOWN_DAY,
    report_definition,
    safe_rate,
)
from db_access import (  # noqa: E402
    AmbiguousCommit,
    COHORT_COLUMNS,
    ConnectionSettings,
    MEMBER_COLUMNS,
    PLAYER_DAY_COLUMNS,
    PyMySQLRollupDatabase,
    SESSION_COLUMNS,
)
from rollup_engine import (  # noqa: E402
    BoundsExceeded,
    CheckpointConflict,
    RollupBounds,
    RollupEngine,
    RollupPageResult,
    SemanticError,
    build_page_contributions,
    merge_session_projection,
    split_utc_interval,
)
from telemetry_rollup_fixtures import FIXTURE_DIR, golden_rows  # noqa: E402
from telemetry_rollup_scenarios import non_additive_membership  # noqa: E402


class FakeDatabase:
    """A serial page adapter; SQL atomicity is covered at the real boundary."""

    def __init__(self, rows):
        self.rows = sorted((deepcopy(row) for row in rows), key=lambda row: row["ingest_id"])
        self.cursor = 0
        self.coverage_end = None
        self.calls = []

    def snapshot_high_watermark(self):
        return max((row["ingest_id"] for row in self.rows), default=0)

    def process_next_page(
        self, target, through, bounds, build_page, max_rows_remaining, max_bytes_remaining, origin_ingest_id=0
    ):
        if self.cursor == 0 and origin_ingest_id:
            self.cursor = origin_ingest_id
        if self.cursor >= through:
            return RollupPageResult(status="complete", cursor=self.cursor, start_cursor=self.cursor)
        page = [
            row for row in self.rows
            if self.cursor < row["ingest_id"] <= through
        ][: min(bounds.page_size, max_rows_remaining)]
        if not page:
            raise AssertionError("fake keyset unexpectedly empty before high-water")
        contribution = build_page(page, target, self.cursor, self.coverage_end)
        self.calls.append((self.cursor, through, len(page), contribution.output_fanout))
        self.cursor = contribution.page_last_ingest_id
        if contribution.coverage_end_utc_usec is not None:
            self.coverage_end = max(self.coverage_end or contribution.coverage_end_utc_usec, contribution.coverage_end_utc_usec)
        return RollupPageResult(
            status="committed",
            cursor=self.cursor,
            start_cursor=contribution.start_cursor,
            page_last_ingest_id=self.cursor,
            fetched_rows=len(page),
            estimated_bytes=contribution.estimated_bytes,
            output_fanout=contribution.output_fanout,
            quality_flags=contribution.state_quality_flags,
        )

    def publish_generation(self, target):
        return {"status": "published", "generation": target.generation}

    def read_report(self, target, report_name):
        raise NotImplementedError


class SpyCursor:
    def __init__(self, connection):
        self.connection = connection
        self.description = None
        self.rowcount = 0
        self._rows = []

    def execute(self, statement, parameters=()):
        self.connection.statements.append((statement, tuple(parameters)))
        if "MAX(ingest_id)" in statement:
            self.description = ("snapshot_high_watermark",)
            self._rows = [{"snapshot_high_watermark": 36}]
        elif statement.lstrip().startswith("SELECT"):
            self.description = ("ingest_id",)
            self._rows = [{"ingest_id": 11}]
        else:
            self._rows = []
        self.rowcount = len(self._rows)

    def fetchall(self):
        return list(self._rows)

    def close(self):
        pass


class SpyConnection:
    def __init__(self):
        self.statements = []
        self.closed = False

    def cursor(self):
        return SpyCursor(self)

    def close(self):
        self.closed = True


class SpyFactory:
    def __init__(self):
        self.connection = SpyConnection()
        self.connect_count = 0

    def connect(self):
        self.connect_count += 1
        return self.connection

    def close(self, connection):
        connection.close()


class AmbiguousCommitAdapter(PyMySQLRollupDatabase):
    """Exercise cursor reconciliation without a live SQL server."""

    def __init__(self, rows):
        super().__init__(SpyFactory())
        self.rows = rows
        self.state = {
            "input_watermark": 0,
            "quality_flags": 0,
            "coverage_start_utc_usec": None,
            "coverage_end_utc_usec": None,
        }
        self.applied = 0
        self.commit_attempts = 0
        self.acknowledged = False

    def _ensure_connection(self):
        return object()

    def _prepare_transaction_budget(self, bounds):
        pass

    def _clear_transaction_budget(self):
        pass

    def _acquire_advisory_lock(self, target, timeout_s):
        pass

    def _release_advisory_lock(self):
        pass

    def _begin(self):
        pass

    def _rollback(self):
        pass

    def _drop_connection(self):
        pass

    def _ensure_state_locked(self, target, through_ingest_id, origin_ingest_id):
        self.state["rebuild_through_ingest_id"] = through_ingest_id
        return self.state

    def _fetch_raw_page(self, cursor, through_ingest_id, limit):
        return self.rows[:limit]

    def _apply_contribution(self, target, state, contribution):
        self.applied += 1
        self.state["input_watermark"] = contribution.page_last_ingest_id

    def _commit(self):
        self.commit_attempts += 1
        if not self.acknowledged:
            self.acknowledged = True
            raise AmbiguousCommit("test-only lost acknowledgement")

    def _reread_cursor_after_ambiguous(self, *args):
        return self.state["input_watermark"]


class RollupSemanticsTest(unittest.TestCase):
    def setUp(self):
        self.target = RollupTarget(1, 1, 8, 7)

    def test_utc_split_conserves_and_rejects_ambiguous_input(self):
        day = 86_400_000_000
        slices = split_utc_interval(day - 50, day + 50, 100)
        self.assertEqual([(item.duration_usec, item.known_utc) for item in slices], [(50, True), (50, True)])
        self.assertEqual(sum(item.duration_usec for item in slices), 100)
        cases = (
            (None, None, ROLLUP_QUALITY_UTC_UNKNOWN),
            (0, 99, ROLLUP_QUALITY_UTC_MISMATCH),
            (100, 0, ROLLUP_QUALITY_UTC_BACKWARD),
        )
        for start, end, flag in cases:
            with self.subTest(start=start, end=end):
                result = split_utc_interval(start, end, 100)
                self.assertEqual(result, (result[0],))
                self.assertEqual(result[0].utc_day, UNKNOWN_DAY)
                self.assertEqual(result[0].duration_usec, 100)
                self.assertFalse(result[0].known_utc)
                self.assertTrue(result[0].quality_flags & flag)
        result = split_utc_interval(0, 100 * 86_400_000_000, 100 * 86_400_000_000, max_fanout=2)
        self.assertEqual(result[0].duration_usec, 100 * 86_400_000_000)
        self.assertTrue(result[0].quality_flags & ROLLUP_QUALITY_UTC_FANOUT)

    def test_golden_pages_conserve_all_interval_buckets(self):
        for path in sorted(FIXTURE_DIR.glob("*.json")):
            fixture, rows = golden_rows(path)
            contribution = build_page_contributions(
                rows,
                self.target,
                max_page_bytes=16 * 1024 * 1024,
                max_output_fanout=100_000,
            )
            totals = dict.fromkeys(COUNTER_FIELDS, 0)
            for day in contribution.player_days.values():
                for name in COUNTER_FIELDS:
                    totals[name] += day.counters[name]
            expected = fixture["expected"]["metrics"]["conservation"]
            self.assertEqual(totals, {name: expected[name] for name in COUNTER_FIELDS}, path.name)
            self.assertEqual(
                sum(delta.observed_intervals for delta in contribution.sessions.values()),
                sum(row["record_kind"] == 1 for row in rows),
            )

    def test_non_additive_subject_members_are_not_account_metrics(self):
        rows, expected = non_additive_membership()
        contribution = build_page_contributions(
            rows,
            self.target,
            max_page_bytes=16 * 1024 * 1024,
            max_output_fanout=100_000,
        )
        subject_durations = defaultdict(int)
        for member in contribution.members.values():
            if member.membership_kind == 1 and member.utc_day == date(1970, 1, 1) and member.zone_vnum == 100:
                subject_durations[member.subject_id] += member.duration_usec
        self.assertEqual(sorted(subject_durations.values()), expected["first_day_main_member_durations"])
        self.assertFalse(report_definition("cohort_activity").account_metrics_available)
        self.assertTrue(contribution.state_quality_flags & ROLLUP_QUALITY_LATE_INPUT)

        first = build_page_contributions(rows[:5], self.target, max_page_bytes=16 * 1024 * 1024)
        second = build_page_contributions(
            rows[5:],
            self.target,
            start_cursor=first.page_last_ingest_id,
            prior_coverage_end_utc_usec=first.coverage_end_utc_usec,
            max_page_bytes=16 * 1024 * 1024,
        )
        self.assertTrue(second.state_quality_flags & ROLLUP_QUALITY_LATE_INPUT)

    def test_checkpoint_is_latest_absolute_plane_not_a_sum(self):
        fixture, rows = golden_rows(FIXTURE_DIR / "checkpoint_revision_conflict.json")
        contribution = build_page_contributions(rows, self.target, max_page_bytes=1_000_000)
        delta = next(iter(contribution.sessions.values()))
        projection = merge_session_projection(None, delta)
        self.assertEqual(projection["latest_checkpoint_revision"], 3)
        self.assertEqual(projection["connected_usec"], 150)
        self.assertEqual(projection["resident_usec"], 190)
        self.assertEqual(projection["covered_resident_usec"], 0)
        self.assertEqual(fixture["expected"]["metrics"]["checkpoint"]["latest_revision"], 3)

    def test_stale_checkpoint_does_not_regress_and_same_revision_conflicts(self):
        fixture, rows = golden_rows(FIXTURE_DIR / "checkpoint_revision_conflict.json")
        first = build_page_contributions(rows[:1], self.target, max_page_bytes=1_000_000)
        projection = merge_session_projection(None, next(iter(first.sessions.values())))
        stale = build_page_contributions(rows[1:2], self.target, max_page_bytes=1_000_000)
        stale_projection = merge_session_projection(projection, next(iter(stale.sessions.values())))
        self.assertEqual(stale_projection["latest_checkpoint_revision"], 2)
        self.assertEqual(stale_projection["connected_usec"], 100)
        conflict = build_page_contributions(rows[2:3], self.target, max_page_bytes=1_000_000)
        newer = merge_session_projection(projection, next(iter(conflict.sessions.values())))
        self.assertEqual(newer["latest_checkpoint_revision"], 3)
        conflicting = deepcopy(next(iter(conflict.sessions.values())))
        conflicting.checkpoint_counters["connected_usec"] += 1
        with self.assertRaises(CheckpointConflict):
            merge_session_projection(newer, conflicting)

    def test_engine_uses_fixed_high_water_and_keyset_pages(self):
        _, rows = golden_rows(FIXTURE_DIR / "normal_interval.json")
        database = FakeDatabase(rows)
        result = RollupEngine(database).run(
            self.target,
            through_ingest_id=rows[-1]["ingest_id"],
            bounds=RollupBounds(page_size=1, max_rows=2, max_page_bytes=1_000_000, max_total_bytes=2_000_000),
        )
        self.assertTrue(result.complete)
        self.assertEqual(result.final_cursor, rows[-1]["ingest_id"])
        self.assertEqual([call[0] for call in database.calls], [0, rows[0]["ingest_id"]])
        self.assertTrue(all(call[1] == rows[-1]["ingest_id"] for call in database.calls))

    def test_ambiguous_commit_reread_does_not_reapply_page(self):
        _, rows = golden_rows(FIXTURE_DIR / "normal_interval.json")
        adapter = AmbiguousCommitAdapter(rows[:1])
        bounds = RollupBounds(page_size=1, max_rows=1, max_page_bytes=1_000_000, max_total_bytes=2_000_000)
        result = adapter.process_next_page(
            self.target,
            rows[0]["ingest_id"],
            bounds,
            lambda page, target, cursor, prior: build_page_contributions(
                page,
                target,
                start_cursor=cursor,
                prior_coverage_end_utc_usec=prior,
                max_page_bytes=1_000_000,
            ),
            1,
            2_000_000,
        )
        self.assertEqual(result.status, "committed")
        self.assertEqual(adapter.applied, 1)
        self.assertEqual(adapter.commit_attempts, 1)

    def test_bounds_fail_before_cursor_acknowledgement(self):
        _, rows = golden_rows(FIXTURE_DIR / "normal_interval.json")
        database = FakeDatabase(rows)
        with self.assertRaises(BoundsExceeded):
            RollupEngine(database).run(
                self.target,
                through_ingest_id=rows[-1]["ingest_id"],
                bounds=RollupBounds(
                    page_size=1,
                    max_rows=1,
                    max_page_bytes=1_000_000,
                    max_total_bytes=2_000_000,
                ),
            )
        self.assertEqual(database.cursor, rows[0]["ingest_id"])

    def test_invalid_raw_kind_fails_closed(self):
        _, rows = golden_rows(FIXTURE_DIR / "normal_interval.json")
        bad = deepcopy(rows)
        bad[0]["record_kind"] = 99
        with self.assertRaises(SemanticError):
            build_page_contributions(bad, self.target, max_page_bytes=1_000_000)

    def test_sql_boundary_is_named_keyset_and_one_connection(self):
        factory = SpyFactory()
        database = PyMySQLRollupDatabase(factory)
        self.assertEqual(database.snapshot_high_watermark(), 36)
        page = database._fetch_raw_page(7, 36, 10)
        self.assertEqual(page, [{"ingest_id": 11}])
        self.assertEqual(factory.connect_count, 1)
        statements = [statement for statement, _parameters in factory.connection.statements]
        raw_statement = next(statement for statement in statements if "FORCE INDEX" in statement)
        self.assertNotIn("SELECT *", raw_statement.upper())
        self.assertNotIn("OFFSET", raw_statement.upper())
        self.assertIn("FORCE INDEX (PRIMARY)", raw_statement)
        self.assertIn("ingest_id>%s AND ingest_id<=%s ORDER BY ingest_id LIMIT %s", raw_statement)
        database.close()
        self.assertTrue(factory.connection.closed)

    def test_sql_projection_shapes_match_additive_support_tables(self):
        _, rows = golden_rows(FIXTURE_DIR / "normal_interval.json")
        contribution = build_page_contributions(
            rows,
            self.target,
            max_page_bytes=16 * 1024 * 1024,
            max_output_fanout=100_000,
        )
        factory = SpyFactory()
        database = PyMySQLRollupDatabase(factory)
        for delta in contribution.sessions.values():
            database._write_session(merge_session_projection(None, delta))
        for delta in contribution.player_days.values():
            database._write_player_day(delta)
        for delta in contribution.cohorts.values():
            database._write_cohort(delta)
        for delta in contribution.members.values():
            database._write_member(delta)
        inserts = {
            "telemetry_rollup_session": len(SESSION_COLUMNS),
            "telemetry_player_day": len(PLAYER_DAY_COLUMNS),
            "telemetry_cohort_day": len(COHORT_COLUMNS),
            "telemetry_cohort_member": len(MEMBER_COLUMNS),
        }
        for table, expected_parameters in inserts.items():
            statement, parameters = next(
                (statement, parameters)
                for statement, parameters in factory.connection.statements
                if statement.startswith(f"INSERT INTO {table}")
            )
            self.assertEqual(len(parameters), expected_parameters, table)
            self.assertNotIn("provisional", statement if table == "telemetry_cohort_member" else "")
        database.close()

    def test_connection_settings_reject_unverified_direct_remote(self):
        with self.assertRaises(ValueError):
            ConnectionSettings(
                host="telemetry.example.invalid",
                database="rollups",
                user="rollup",
                password="not-printed",
            ).validate()
        ConnectionSettings(
            host="127.0.0.1",
            database="rollups",
            user="rollup",
            password="not-printed",
        ).validate()

    def test_public_projection_does_not_turn_storage_sentinels_into_facts(self):
        session_row = {
            "latest_checkpoint_revision": 0,
            **{name: 0 for name in COUNTER_FIELDS},
        }
        public_session = PyMySQLRollupDatabase._public_report_rows(
            "session_playtime", [session_row]
        )[0]
        self.assertFalse(public_session["checkpoint_totals_available"])
        self.assertTrue(all(public_session[name] is None for name in COUNTER_FIELDS))
        public_cohort = PyMySQLRollupDatabase._public_report_rows(
            "cohort_activity", [{"utc_day": UNKNOWN_DAY}]
        )[0]
        self.assertIsNone(public_cohort["utc_day"])
        self.assertEqual(public_cohort["bucket_kind"], "unknown")

    def test_zero_denominator_is_null_not_zero(self):
        self.assertIsNone(safe_rate(10, 0))
        self.assertEqual(safe_rate(1, 4), 0.25)


if __name__ == "__main__":
    unittest.main(verbosity=2)
