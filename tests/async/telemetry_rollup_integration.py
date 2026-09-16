#!/usr/bin/env python3
"""Actual #268 engine journeys in an explicitly opted-in disposable SQL database.

Requires migrations 0014/0017 and root/rollup/report test credentials in the
ROLLUP_TEST_* environment. Raw fixture setup is administrative test code only.
The actual engine always runs with the restricted rollup or read-only report role.
"""
import json
import os
from pathlib import Path
import re
import sys
import unittest
from dataclasses import replace

import pymysql

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
from scripts.telemetry.db_access import (AmbiguousCommit, ConnectionSettings,
    GenerationConflict, PyMySQLConnectionFactory, PyMySQLRollupDatabase)
from scripts.telemetry.rollup_engine import BoundsExceeded, RollupBounds, RollupEngine
from scripts.telemetry.rollup_definitions import RollupTarget
from telemetry_rollup_acceptance import assert_golden_storage, select_scope, SCOPE
from telemetry_rollup_fixtures import FIXTURE_DIR, golden_rows
from telemetry_rollup_scenarios import non_additive_membership

TABLES = ('telemetry_rollup_session', 'telemetry_player_day', 'telemetry_cohort_day',
          'telemetry_cohort_member', 'telemetry_rollup_state')


class FaultDatabase(PyMySQLRollupDatabase):
    def __init__(self, factory, fault):
        super().__init__(factory)
        self.fault = fault
        self.reconciliations = 0

    def _commit(self):
        fault, self.fault = self.fault, None
        if fault == 'after_commit':
            super()._commit()
            raise AmbiguousCommit('test lost durable COMMIT acknowledgement')
        if fault in ('before_commit', 'crash'):
            self._drop_connection()  # actual server rolls back this transaction
            if fault == 'crash':
                raise RuntimeError('injected process interruption before COMMIT')
            raise AmbiguousCommit('test socket loss before COMMIT')
        return super()._commit()

    def _reread_cursor_after_ambiguous(self, *args, **kwargs):
        self.reconciliations += 1
        return super()._reread_cursor_after_ambiguous(*args, **kwargs)


class RollupIntegration(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if os.environ.get('TELEMETRY_ROLLUP_TEST') != '1':
            raise RuntimeError('explicit disposable SQL opt-in required')
        cls.name = os.environ['ROLLUP_TEST_DATABASE']
        if not re.fullmatch(r'duris_268_[a-z0-9_]*test', cls.name):
            raise RuntimeError('requires task-owned duris_268_*test database')
        cls.admin = pymysql.connect(host='127.0.0.1', user='root', database=cls.name,
            password=os.environ['ROLLUP_TEST_ROOT_PASSWORD'], autocommit=True,
            cursorclass=pymysql.cursors.DictCursor, connect_timeout=2, read_timeout=3)
        cls.golden_results = []

    @classmethod
    def tearDownClass(cls):
        cls.admin.close()
        result_path = Path(os.environ.get('ROLLUP_TEST_RESULTS', str(ROOT / 'bin/tests/issue268-golden-results.json')))
        result_path.parent.mkdir(parents=True, exist_ok=True)
        result_path.write_text(json.dumps(cls.golden_results, indent=2))

    def setUp(self):
        self.adapters = []
        self.reset()

    def tearDown(self):
        for db in self.adapters:
            db.close()

    def reset(self):
        for db in getattr(self, 'adapters', []):
            db.close()
        with self.admin.cursor() as c:
            for table in (*TABLES, 'telemetry_interval'):
                c.execute('TRUNCATE TABLE ' + table)

    def load(self, rows):
        with self.admin.cursor() as c:
            for row in rows:
                columns = tuple(row)
                c.execute('INSERT INTO telemetry_interval (' + ','.join(columns) + ') VALUES (' +
                          ','.join(['%s'] * len(columns)) + ')', tuple(row[k] for k in columns))

    def database(self, role='rollup', fault=None):
        settings = ConnectionSettings(host='127.0.0.1', database=self.name,
            user='duris268_' + role, password=os.environ['ROLLUP_TEST_' + role.upper() + '_PASSWORD'])
        factory = PyMySQLConnectionFactory(settings)
        db = FaultDatabase(factory, fault) if fault else PyMySQLRollupDatabase(factory)
        self.adapters.append(db)
        return db

    def run_engine(self, db=None, generation=1, **kwargs):
        db = db or self.database()
        target = RollupTarget(1, generation, 8, 7)
        result = RollupEngine(db).run(target, bounds=kwargs.pop('bounds', RollupBounds(page_size=1)), **kwargs)
        self.assertTrue(result.complete)
        return db, target, result

    def signature(self, generation):
        output = {}
        for table in TABLES[:-1]:
            rows = select_scope(self.admin, table, (1, generation, 8, 7))
            output[table] = sorted((json.dumps({k:v for k,v in row.items() if k != 'generation'},
                                               sort_keys=True, default=str) for row in rows))
        return output

    def test_all_golden_actual_sql_and_partition_rebuild(self):
        paths = sorted(FIXTURE_DIR.glob('*.json'))
        self.assertEqual(len(paths), 10)
        for path in paths:
            with self.subTest(fixture=path.name):
                self.reset()
                fixture, rows = golden_rows(path)
                self.load(rows)
                db, target, result = self.run_engine()
                counts = assert_golden_storage(self, self.admin, fixture, rows, target.scope_tuple)
                original = self.signature(1)
                _, _, replay = self.run_engine(db)
                self.assertEqual(replay.fetched_rows, 0)
                self.assertEqual(self.signature(1), original)
                self.run_engine(db, generation=2, bounds=RollupBounds(page_size=100))
                self.assertEqual(self.signature(2), original)
                RollupEngine(db).publish(target)
                report = RollupEngine(self.database('report')).report(target, 'session_playtime')
                self.assertFalse(report.truncated)
                self.assertEqual(report.coverage.input_watermark, result.final_cursor)
                if not any(r['record_kind'] == 3 for r in rows):
                    self.assertTrue(all(r['connected_usec'] is None for r in report.rows))
                self.golden_results.append({'fixture': path.name, 'facts': len(rows), 'status': 'PASS', **counts})
        self.assertEqual(len(self.golden_results), len(paths))
        self.assertEqual(len({r['fixture'] for r in self.golden_results}), len(paths))

    def test_crash_and_both_commit_ambiguities(self):
        rows, _ = non_additive_membership()
        self.load(rows)
        db, _, _ = self.run_engine()
        expected = self.signature(1)
        for generation, fault in enumerate(('before_commit', 'after_commit', 'crash'), 2):
            with self.subTest(fault=fault):
                faulty = self.database(fault=fault)
                if fault == 'crash':
                    with self.assertRaisesRegex(RuntimeError, 'injected process'):
                        self.run_engine(faulty, generation=generation)
                    self.assertEqual(select_scope(self.admin, 'telemetry_rollup_state', (1,generation,8,7)), ())
                    self.assertTrue(all(not select_scope(self.admin,t,(1,generation,8,7)) for t in TABLES[:-1]))
                self.run_engine(faulty, generation=generation)
                self.assertEqual(self.signature(generation), expected)
                if fault != 'crash':
                    assert isinstance(faulty, FaultDatabase)
                    self.assertGreater(faulty.reconciliations, 0)

    def test_publication_switch_and_incomplete_rebuild(self):
        rows, _ = non_additive_membership()
        self.load(rows)
        db, target, _ = self.run_engine()
        engine = RollupEngine(db)
        engine.publish(target)
        replacement = RollupTarget(1, 2, 8, 7)
        with self.assertRaises(BoundsExceeded):
            engine.run(replacement, bounds=RollupBounds(page_size=1, max_rows=1))
        with self.assertRaises(GenerationConflict):
            engine.publish(replacement)
        self.assertEqual(select_scope(self.admin,'telemetry_rollup_state',target.scope_tuple)[0]['publication_status'],1)
        engine.run(replacement)
        engine.publish(replacement)
        with self.admin.cursor() as c:
            c.execute('SELECT generation FROM telemetry_rollup_state WHERE publication_status=1')
            self.assertEqual(c.fetchall(), [{'generation': 2}])
        with self.assertRaises(GenerationConflict):
            engine.publish(target)

    def test_unpublished_report_is_not_served(self):
        _, rows = golden_rows(FIXTURE_DIR/'normal_interval.json')
        self.load(rows)
        db,target,_ = self.run_engine()
        with self.assertRaises(GenerationConflict):
            RollupEngine(self.database('report')).report(target, 'session_playtime')

    def test_contribution_limit_cannot_silently_truncate_distribution(self):
        rows, _ = non_additive_membership()
        self.load(rows)
        db,target,_ = self.run_engine()
        with self.assertRaises(BoundsExceeded):
            db.read_membership_contributions(target, membership_kind=1, max_rows=1)
        with self.assertRaises(BoundsExceeded):
            db.read_checkpoint_contributions(target, max_rows=1)

    def test_precommit_budget_failure_is_not_ambiguous(self):
        db = self.database()
        db._ensure_connection()
        db._deadline = db.clock() - 1
        with self.assertRaises(BoundsExceeded):
            db._commit()

    def test_retry_does_not_restart_the_page_deadline(self):
        _, rows = golden_rows(FIXTURE_DIR/'normal_interval.json')
        self.load(rows)
        db = self.database(fault='before_commit')
        now = [0.0]
        db.clock = lambda: now[0]
        prepare = db._prepare_transaction_budget
        deadlines = []
        def capture(bounds):
            prepare(bounds)
            deadlines.append(db._deadline)
        db._prepare_transaction_budget = capture
        commit = db._commit
        def advance_and_commit():
            now[0] = 0.75
            return commit()
        db._commit = advance_and_commit
        result = RollupEngine(db, clock=lambda:now[0]).run(RollupTarget(1,1,8,7),
                    bounds=RollupBounds(page_size=100,max_runtime_s=1.0))
        self.assertTrue(result.complete)
        self.assertTrue(all(deadline <= 1.0 for deadline in deadlines), deadlines)

    def test_large_scope_lock_names_do_not_collide(self):
        db = self.database()
        largest = (1 << 64) - 1
        left = RollupTarget(1,1,largest,largest-1)
        right = RollupTarget(1,1,largest,largest)
        self.assertNotEqual(db._advisory_lock_name(left),db._advisory_lock_name(right))
        self.assertLessEqual(len(db._advisory_lock_name(left)),64)

    def test_publication_state_enumeration_has_a_hard_limit(self):
        import scripts.telemetry.db_access as access
        from unittest.mock import patch as mock_patch
        db,target,_ = self.run_engine()
        with self.admin.cursor() as c:
            for generation in (2,3):
                c.execute('INSERT INTO telemetry_rollup_state '
                          '(definition_version,generation,environment_id,season_id) VALUES (1,%s,9,7)',
                          (generation,))
        # Actual SQL, with a small capacity for the regression rather than a
        # huge fixture. No report is superseded when state capacity is exceeded.
        with mock_patch.object(access,'PUBLICATION_STATE_LIMIT',2,create=True):
            with self.assertRaises(BoundsExceeded):
                RollupEngine(db).publish(target)
        self.assertEqual(select_scope(self.admin,'telemetry_rollup_state',target.scope_tuple)[0]['publication_status'],0)

    def test_budget_cleanup_does_not_leave_advisory_lock(self):
        db = self.database()
        target = RollupTarget(1, 1, 8, 7)
        name = db._advisory_lock_name(target)
        db._acquire_advisory_lock(target, 0.1)
        db._statement_count = 0
        db._statement_limit = 0
        db._release_advisory_lock()
        with self.admin.cursor() as c:
            c.execute('SELECT IS_FREE_LOCK(%s) AS free', (name,))
            self.assertEqual(c.fetchone()['free'], 1)

    def test_nonfinite_limits_are_rejected(self):
        for value in (float('nan'), float('inf')):
            with self.assertRaises(ValueError):
                RollupBounds(max_runtime_s=value).validate()
            with self.assertRaises(ValueError):
                ConnectionSettings('127.0.0.1',self.name,'test','',read_timeout_s=value).validate()

    def test_real_membership_grain_and_distributions(self):
        from datetime import date
        from statistics import median
        from scripts.telemetry.rollup_definitions import safe_rate
        rows, expected = non_additive_membership()
        self.load(rows)
        db,target,_ = self.run_engine()
        RollupEngine(db).publish(target)
        reader = self.database('report')
        members = reader.read_membership_contributions(target, membership_kind=1)
        durations = sorted(m.duration_usec for m in members if m.utc_day == date(1970,1,1)
                           and m.zone_vnum == rows[0]['zone_vnum'])
        self.assertEqual(durations, expected['first_day_main_member_durations'])
        self.assertEqual(median(durations), expected['first_day_main_median_usec'])
        self.assertNotEqual(median(durations), expected['first_day_main_mean_usec'])
        report = RollupEngine(reader).report(target, 'cohort_activity')
        self.assertFalse(report.definition.account_metrics_available)
        self.assertEqual(len({m.subject_id for m in members}),expected['unique_subjects_all_days_and_cohorts'])
        self.assertGreater(sum(r['subject_count'] for r in report.rows),len({m.subject_id for m in members}))
        self.assertIsNone(safe_rate(0, 0))
        self.assertEqual(safe_rate(sum(durations), sum(durations)), 1)

    def test_actual_raw_query_plan(self):
        rows, _ = non_additive_membership()
        self.load(rows)
        db = self.database()
        execute = db._execute
        plans = []
        def capture(sql, params=()):
            if 'FROM telemetry_interval' in sql and 'WHERE ingest_id' in sql:
                with self.admin.cursor() as c:
                    c.execute('EXPLAIN '+sql, params)
                    plans.extend(c.fetchall())
            return execute(sql, params)
        db._execute = capture
        self.run_engine(db)
        self.assertTrue(plans)
        self.assertTrue(all(p['key']=='PRIMARY' and p['type']=='range' for p in plans))

    def test_empty_input_preserves_unknown_coverage_without_zero_rows(self):
        db,target,_ = self.run_engine()
        RollupEngine(db).publish(target)
        report = RollupEngine(self.database('report')).report(target,'cohort_activity')
        self.assertEqual(report.rows, ())
        self.assertIsNone(report.coverage.coverage_start_utc_usec)
        self.assertTrue(report.coverage.provisional)

    def test_raw_fetch_obeys_byte_budget_before_materialization(self):
        rows, _ = non_additive_membership()
        self.load(rows)
        db = self.database()
        original = db._fetch_raw_page
        requested = []
        def capture(cursor, through, limit):
            requested.append(limit)
            self.assertLessEqual(limit, 1, "4096-byte page must not fetch an arbitrary 100-row buffer")
            return original(cursor, through, limit)
        db._fetch_raw_page = capture
        self.run_engine(db, bounds=RollupBounds(page_size=100, max_page_bytes=4096))
        self.assertTrue(requested)

    def test_zero_retry_budget_is_honored(self):
        rows, _ = non_additive_membership()
        self.load(rows)
        db = self.database(fault='before_commit')
        with self.assertRaises(AmbiguousCommit):
            self.run_engine(db, bounds=RollupBounds(page_size=1, max_retries=0))
        self.assertFalse(select_scope(self.admin,'telemetry_rollup_state',(1,1,8,7)))

    def test_page_statement_timeout_reaches_server(self):
        _, rows = golden_rows(FIXTURE_DIR/'normal_interval.json')
        self.load(rows)
        db = self.database()
        prepare = db._prepare_transaction_budget
        observed = []
        def capture(bounds):
            prepare(bounds)
            values, _, _ = db._execute('SELECT @@max_statement_time AS seconds')
            observed.append(float(values[0]['seconds']))
            self.assertLessEqual(observed[-1], bounds.statement_timeout_s)
        db._prepare_transaction_budget = capture
        self.run_engine(db, bounds=RollupBounds(statement_timeout_s=0.1))
        self.assertTrue(observed)

    def test_report_state_and_rows_share_one_snapshot(self):
        _, rows = golden_rows(FIXTURE_DIR/'normal_interval.json')
        self.load(rows)
        writer,target,_ = self.run_engine(through_ingest_id=rows[0]['ingest_id'])
        RollupEngine(writer).publish(target)
        reader = self.database('report')
        fetch = reader._fetch_state
        fired = False
        def interleave(*args, **kwargs):
            nonlocal fired
            result = fetch(*args, **kwargs)
            if not fired:
                fired = True
                self.run_engine(writer)
            return result
        reader._fetch_state = interleave
        snapshot = RollupEngine(reader).report(target, 'session_playtime')
        self.assertTrue(fired)
        self.assertTrue(all(r['input_watermark'] <= snapshot.coverage.input_watermark for r in snapshot.rows))


if __name__ == '__main__':
    unittest.main(verbosity=2)
