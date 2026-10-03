#!/usr/bin/env python3
"""Independent coffer evidence validation; no accounting lifetime is invented."""
import copy
import unittest
from unittest.mock import patch

from test_reconcile_economy_accounting import clean_snapshot
from reconcile_economy_accounting import MAX_ROWS, Reconciler, SnapshotError
import economic_sql_audit_snapshot as exporter


ROWS = [{"ship_id": 1, "copper": 7}, {"ship_id": 2, "copper": 0},
        {"ship_id": 3, "copper": None}, {"ship_id": 4, "copper": -1}]
COVERAGE = dict(rows=4, positive_rows=1, zero_rows=1, unknown_rows=1,
                invalid_rows=1, missing_revision_rows=4)


def observed():
    snapshot = clean_snapshot()
    snapshot["native"].update(ship_coffers=copy.deepcopy(ROWS),
                              ship_coffer_coverage=copy.deepcopy(COVERAGE))
    return snapshot


class ShipCofferAudit(unittest.TestCase):
    def test_raw_values_are_unqualified_and_never_added_to_mapped_holdings(self):
        snapshot = observed()
        original = copy.deepcopy(snapshot)
        report = Reconciler().audit(snapshot)
        self.assertEqual(snapshot, original)
        self.assertEqual(report["exception_counts"], {
            "unsupported_native_ship_coffer": 4, "missing_ship_coffer_revision": 4,
            "unknown_native_ship_coffer": 1, "invalid_native_ship_coffer": 1})
        self.assertTrue(all("ship_id" in row for row in report["exceptions"]))
        limited = Reconciler(limit=0).audit(snapshot)
        self.assertEqual(limited["exception_counts"], report["exception_counts"])
        self.assertEqual(limited["exceptions"], [])

    def test_invalid_identity_value_alias_and_duplicate_evidence_refuse(self):
        for row in ({"ship_id": True, "copper": 0}, {"ship_id": 0, "copper": 0},
                    {"ship_id": -1, "copper": 0}, {"ship_id": 2**31, "copper": 0},
                    {"ship_id": 1, "copper": True}, {"ship_id": 1, "copper": "7"},
                    {"ship_id": 1, "copper": 2**31}, {"ship_id": 1, "copper": -(2**31)-1},
                    {"ship_id": 1}, {"ship_id": 1, "copper": 7, "owner_name": "private"}):
            with self.subTest(row=row):
                snapshot=observed(); snapshot["native"]["ship_coffers"][0]=row
                with self.assertRaises(SnapshotError): Reconciler(limit=0).audit(snapshot)
        snapshot=observed(); snapshot["native"]["ship_coffers"][1]["ship_id"]=1
        with self.assertRaises(SnapshotError): Reconciler().audit(snapshot)

    def test_coverage_is_recomputed_instead_of_trusted(self):
        for field in COVERAGE:
            for value in (True, -1, MAX_ROWS+1, COVERAGE[field]+1):
                with self.subTest(field=field,value=value):
                    snapshot=observed(); snapshot["native"]["ship_coffer_coverage"][field]=value
                    with self.assertRaises(SnapshotError): Reconciler().audit(snapshot)
        for field in ("ship_coffers", "ship_coffer_coverage"):
            snapshot=observed(); del snapshot["native"][field]
            with self.assertRaises(SnapshotError): Reconciler().audit(snapshot)
        snapshot=observed()
        with patch('reconcile_economy_accounting.MAX_ROWS',3):
            with self.assertRaises(SnapshotError): Reconciler().audit(snapshot)

    def test_old_partial_export_reports_missing_coffer_evidence(self):
        reconciler=Reconciler(); reconciler.audit_ship_coffers("sql_partial", {})
        self.assertEqual(reconciler.counts["missing_ship_coffer_coverage"],1)

    def test_exporter_reads_only_bounded_id_and_value(self):
        class Cursor:
            def execute(self, sql, params):
                self.sql=sql; self.params=params
            def fetchall(self):
                return [{"id": row["ship_id"], "money": row["copper"]} for row in ROWS]
        cursor=Cursor(); rows,coverage=exporter.read_ship_coffers(cursor)
        self.assertEqual(rows,ROWS); self.assertEqual(coverage,COVERAGE)
        self.assertEqual(cursor.sql,"SELECT id,money FROM ships ORDER BY id LIMIT %s")
        self.assertEqual(cursor.params,(exporter.MAX_ROWS+1,))
        with patch.object(exporter,'MAX_ROWS',3):
            with self.assertRaises(exporter.ExportError): exporter.read_ship_coffers(cursor)


if __name__ == '__main__':
    unittest.main()
