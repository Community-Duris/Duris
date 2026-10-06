#!/usr/bin/env python3
"""Reconcile retained equipment positions without inferring missing slots."""

import copy
import json
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
from reconcile_economy_accounting import Reconciler, SnapshotError, TABLES, view


def position(slot=5):
    return dict(uid=81, revision=3, root=81, parent=None, owner=[1, 7, 0],
                state='live', equipment_slot=slot)


def baseline():
    return dict(schema_version=1, lineage='11' * 16, epoch='22' * 16,
                complete=True, quiescent=True, backend='disposable',
                native=dict(holdings=[], items=[position()]),
                **{name: [dict(position(), origin='baseline')] if name == 'item_origins' else []
                   for name in TABLES})


def history():
    return [dict(position(slot=after), operation_id=('%032x' % index), event_index=0,
                 before_revision=2 + index, revision=3 + index, action='move',
                 from_equipment_slot=before, to_equipment_slot=after,
                 operation_epoch='22' * 16, operation_outcome='committed', referenced=False)
            for index, before, after in ((1, 5, 6), (2, 6, 7))]


def audit_history(rows, current, lineage):
    auditor = Reconciler()
    origins = {(81,): dict(position(), origin='baseline')}
    if lineage:
        native = dict(uid_history_events=rows, lineage_uid_references=[])
        auditor.audit_lineage_uid_history('disposable', native, origins, {(81,): current})
    else:
        ownership = {(row['operation_id'], row['event_index']): row for row in rows}
        references = {(index,): dict(row, legacy_operation_id=row['operation_id'],
                      legacy_event_index=row['event_index'], after_revision=row['revision'])
                      for index, row in enumerate(rows)}
        auditor.audit_items(ownership, references, origins, {(81,): current})
    return auditor.counts


class ItemEquipmentTests(unittest.TestCase):
    def test_baseline_slot_only_drift_is_stale_native_authority(self):
        snapshot = baseline()
        self.assertEqual(Reconciler().audit(snapshot)['exception_count'], 0)
        snapshot['native']['items'][0]['equipment_slot'] = 6
        original = copy.deepcopy(snapshot)
        for limit in (0, 1, 100):
            result = Reconciler(limit).audit(snapshot)
            self.assertEqual(result['exception_counts'], {'stale_native_item': 1})
        self.assertEqual(snapshot, original)

    def test_both_history_paths_compare_current_slot_and_slot_continuity(self):
        for lineage in (False, True):
            with self.subTest(lineage=lineage):
                rows = history()
                current = dict(position(slot=7), revision=5)
                self.assertEqual(audit_history(rows, current, lineage), {})
                current['equipment_slot'] = 8
                self.assertEqual(audit_history(rows, current, lineage), {'stale_native_item': 1})
                current['equipment_slot'] = 7
                rows[1]['from_equipment_slot'] = 5
                self.assertEqual(audit_history(rows, current, lineage), {'broken_item_equipment_history': 1})

    def test_missing_equipment_evidence_is_preserved_without_zero_inference(self):
        for collection in ('item_origins', 'native'):
            snapshot = baseline()
            row = snapshot['item_origins'][0] if collection == 'item_origins' else snapshot['native']['items'][0]
            del row['equipment_slot']
            self.assertEqual(Reconciler().audit(snapshot)['exception_counts'], {'missing_item_equipment_evidence': 1})
        for lineage in (False, True):
            rows = history()
            del rows[0]['to_equipment_slot']
            self.assertEqual(audit_history(rows, dict(position(slot=7), revision=5), lineage),
                             {'missing_item_equipment_evidence': 1})
        # Historical snapshots with no slot projections remain readable.
        snapshot = baseline()
        del snapshot['item_origins'][0]['equipment_slot']
        del snapshot['native']['items'][0]['equipment_slot']
        self.assertEqual(Reconciler().audit(snapshot)['exception_count'], 0)

    def test_equipment_scalars_require_exact_unsigned_sixteen_bit_integers(self):
        for value in (True, 5.0, '5', None, -1, 65536):
            for collection in ('item_origins', 'native'):
                with self.subTest(value=value, collection=collection):
                    snapshot = baseline()
                    row = snapshot['item_origins'][0] if collection == 'item_origins' else snapshot['native']['items'][0]
                    row['equipment_slot'] = value
                    with self.assertRaisesRegex(SnapshotError, 'invalid item equipment slot'):
                        Reconciler().audit(snapshot)
            for lineage in (False, True):
                for field in ('from_equipment_slot', 'to_equipment_slot'):
                    with self.subTest(value=value, lineage=lineage, field=field):
                        rows = history()
                        rows[0][field] = value
                        self.assertEqual(audit_history(rows, dict(position(slot=7), revision=5), lineage),
                                         {'invalid_item_equipment_slot': 1, 'missing_item_equipment_evidence': 1})

    def test_valid_slot_boundaries_and_combined_drift_count_once(self):
        for value in (0, 65535):
            snapshot = baseline()
            snapshot['item_origins'][0]['equipment_slot'] = value
            snapshot['native']['items'][0]['equipment_slot'] = value
            self.assertEqual(Reconciler().audit(snapshot)['exception_count'], 0)
        snapshot['native']['items'][0].update(equipment_slot=0, revision=4)
        self.assertEqual(Reconciler().audit(snapshot)['exception_counts'], {'stale_native_item': 1})

    def test_nonzero_slots_require_live_player_or_native_mobile_roots(self):
        for owner, parent, state, slot in (([2, 7, 0], None, 'live', 5),
                                          ([1, 7, 0], 82, 'live', 5),
                                          ([1, 7, 0], None, 'tombstone', 5),
                                          ([12, 7, 0], None, 'live', 44)):
            for collection in ('item_origins', 'native'):
                with self.subTest(owner=owner, parent=parent, state=state, slot=slot, collection=collection):
                    snapshot = baseline()
                    row = snapshot['item_origins'][0] if collection == 'item_origins' else snapshot['native']['items'][0]
                    row.update(owner=owner, parent=parent, state=state, equipment_slot=slot)
                    with self.assertRaisesRegex(SnapshotError, 'invalid item equipment position'):
                        Reconciler().audit(snapshot)
        snapshot = baseline()
        for row in (snapshot['item_origins'][0], snapshot['native']['items'][0]):
            row.update(owner=[12, 7, 0], equipment_slot=43)
        self.assertEqual(Reconciler().audit(snapshot)['exception_count'], 0)

    def test_provenance_preserves_nonpersonal_equipment_transitions(self):
        snapshot = baseline()
        snapshot['ownership_events'] = [dict(history()[0], personal_alias='private-equipment-alias')]
        result = view(snapshot, dict(exception_count=0), 'provenance', 1, uid=81)
        self.assertEqual(result['rows'][0]['from_equipment_slot'], 5)
        self.assertEqual(result['rows'][0]['to_equipment_slot'], 6)
        self.assertNotIn('private-equipment-alias', json.dumps(result))
        self.assertLessEqual(len(result['rows']), 1)


if __name__ == '__main__':
    unittest.main(verbosity=2)
