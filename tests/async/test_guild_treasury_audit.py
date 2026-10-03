#!/usr/bin/env python3
"""Validate raw native guild money without inventing lifetime or revision."""
import copy
import unittest
from unittest.mock import patch

from test_reconcile_economy_accounting import clean_snapshot
from reconcile_economy_accounting import MAX_ROWS, Reconciler, SnapshotError
import economic_sql_audit_snapshot as exporter

ROWS = [{"guild_id": 1, "balance": [1, 2, 3, 4]},
        {"guild_id": 2, "balance": [0, 0, 0, 0]},
        {"guild_id": 2**32-1, "balance": [2**32-1]*4}]
COVERAGE = dict(rows=3, positive_rows=2, zero_rows=1, missing_revision_rows=3)


def observed():
    snapshot = clean_snapshot()
    snapshot["native"].update(guild_treasuries=copy.deepcopy(ROWS),
                              guild_treasury_coverage=copy.deepcopy(COVERAGE))
    return snapshot


class GuildTreasuryAudit(unittest.TestCase):
    def test_full_unsigned_values_remain_unqualified_outside_mapped_holdings(self):
        snapshot = observed()
        original = copy.deepcopy(snapshot)
        report = Reconciler().audit(snapshot)
        self.assertEqual(snapshot, original)
        self.assertEqual(report["exception_counts"], {
            "unsupported_native_guild_treasury": 3, "missing_guild_money_revision": 3})
        self.assertTrue(all("guild_id" in row for row in report["exceptions"]))
        limited = Reconciler(limit=0).audit(snapshot)
        self.assertEqual(limited["exception_counts"], report["exception_counts"])
        self.assertEqual(limited["exceptions"], [])

    def test_invalid_identity_balance_alias_or_revision_refuse(self):
        invalid = [{"guild_id": identity, "balance": [0]*4}
                   for identity in (True, 0, -1, 2**32, "1")]
        invalid += [{"guild_id": 1, "balance": balance}
                    for balance in (None, [], [0]*3, [0]*5, (0,)*4,
                                    [True, 0, 0, 0], [None, 0, 0, 0],
                                    [-1, 0, 0, 0], [2**32, 0, 0, 0], ["1", 0, 0, 0])]
        invalid += [{"guild_id": 1}, dict(ROWS[0], owner_name="private"),
                    dict(ROWS[0], revision=9), dict(ROWS[0], outcome_revision=9)]
        for row in invalid:
            with self.subTest(row=row):
                snapshot = observed()
                snapshot["native"]["guild_treasuries"][0] = row
                with self.assertRaises(SnapshotError):
                    Reconciler(limit=0).audit(snapshot)
        snapshot = observed()
        snapshot["native"]["guild_treasuries"][1]["guild_id"] = 1
        with self.assertRaises(SnapshotError):
            Reconciler().audit(snapshot)

    def test_coverage_is_independently_recomputed_and_bounded(self):
        for field in COVERAGE:
            for value in (True, -1, MAX_ROWS+1, COVERAGE[field]+1):
                with self.subTest(field=field, value=value):
                    snapshot = observed()
                    snapshot["native"]["guild_treasury_coverage"][field] = value
                    with self.assertRaises(SnapshotError):
                        Reconciler().audit(snapshot)
        for field in ("guild_treasuries", "guild_treasury_coverage"):
            snapshot = observed()
            del snapshot["native"][field]
            with self.assertRaises(SnapshotError):
                Reconciler().audit(snapshot)
        with patch('reconcile_economy_accounting.MAX_ROWS', 2):
            with self.assertRaises(SnapshotError):
                Reconciler().audit(observed())

    def test_old_partial_export_reports_missing_native_guild_money(self):
        reconciler = Reconciler()
        reconciler.audit_guild_treasuries("sql_partial", {})
        self.assertEqual(reconciler.counts["missing_guild_treasury_coverage"], 1)

    def test_exporter_selects_only_bounded_native_id_and_money(self):
        class Cursor:
            def execute(self, sql, params):
                self.sql, self.params = sql, params
            def fetchall(self):
                return [dict(id=row["guild_id"], **dict(zip(
                    ("copper", "silver", "gold", "platinum"), row["balance"]))) for row in ROWS]
        cursor = Cursor()
        rows, coverage = exporter.read_guild_treasuries(cursor)
        self.assertEqual(rows, ROWS)
        self.assertEqual(coverage, COVERAGE)
        self.assertEqual(cursor.sql,
                         "SELECT id,copper,silver,gold,platinum FROM guilds ORDER BY id LIMIT %s")
        self.assertEqual(cursor.params, (exporter.MAX_ROWS+1,))
        with patch.object(exporter, 'MAX_ROWS', 2):
            with self.assertRaises(exporter.ExportError):
                exporter.read_guild_treasuries(cursor)
        for value in (None, True, -1, 2**32, "1"):
            with self.subTest(value=value), patch.object(
                    cursor, 'fetchall', return_value=[dict(id=1, copper=value, silver=0, gold=0, platinum=0)]):
                with self.assertRaises(exporter.ExportError):
                    exporter.read_guild_treasuries(cursor)


if __name__ == '__main__':
    unittest.main()
