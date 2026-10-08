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
                 from_owner=[1, 7, 0],
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


class ProvenancePreviousOwnerTests(unittest.TestCase):
    def test_all_history_collections_preserve_known_previous_owner_without_private_fields(self):
        for collection in ('ownership_events', 'uid_history_events', 'unattributed_uid_events'):
            for previous in ([7, 0, 0], [1, 7, 0], [12, 2**64-1, 2**64-1]):
                with self.subTest(collection=collection, previous=previous):
                    snapshot = baseline()
                    destination = snapshot if collection == 'ownership_events' else snapshot['native']
                    destination[collection] = [dict(history()[0], from_owner=previous,
                                                   personal_alias='private-owner-alias')]
                    original = copy.deepcopy(snapshot)
                    result = view(snapshot, dict(exception_count=3), 'provenance', 100, uid=81)
                    self.assertEqual(result['count'], 1)
                    self.assertIn('from_owner', result['rows'][0])
                    self.assertEqual(result['rows'][0]['from_owner'], previous)
                    self.assertEqual(result['rows'][0]['owner'], [1, 7, 0])
                    self.assertEqual(result['coverage']['exception_count'], 3)
                    self.assertNotIn('private-', json.dumps(result))
                    self.assertNotIn('alias', json.dumps(result))
                    self.assertEqual(snapshot, original)

    def test_exact_projections_deduplicate_but_conflicting_or_unknown_sources_stay_visible(self):
        snapshot = baseline();event = history()[0]
        snapshot['ownership_events'] = [copy.deepcopy(event)]
        snapshot['native'].update(uid_history_events=[copy.deepcopy(event)],
                                  unattributed_uid_events=[copy.deepcopy(event)])
        self.assertEqual(view(snapshot, {}, 'provenance', 100, uid=81)['count'], 1)
        snapshot['native']['uid_history_events'][0]['from_owner'] = [1, 8, 0]
        del snapshot['native']['unattributed_uid_events'][0]['from_owner']
        original = copy.deepcopy(snapshot)
        for limit in (0, 1, 100):
            result = view(snapshot, {}, 'provenance', limit, uid=81)
            self.assertEqual(result['count'], 3)
            self.assertEqual(result['truncated'], limit < 3)
            self.assertEqual(len(result['rows']), min(limit, 3))
            if limit == 100:
                self.assertEqual([row.get('from_owner') for row in result['rows']],
                                 [[1, 7, 0], [1, 8, 0], None])
                self.assertNotIn('from_owner', result['rows'][2])
            self.assertEqual(snapshot, original)

    def test_malformed_present_previous_owner_refuses_the_view_without_private_values(self):
        invalid = (None, [], [1, 7], [1, 7, 0, 0], 'private-owner-alias',
                   [True, 7, 0], [1, 7.0, 0], [1, 7, True], [1, -1, 0],
                   [13, 7, 0], [1, 2**64, 0], [1, 7, 2**64])
        for collection in ('ownership_events', 'uid_history_events', 'unattributed_uid_events'):
            for previous in invalid:
                with self.subTest(collection=collection, previous=previous):
                    snapshot = baseline()
                    destination = snapshot if collection == 'ownership_events' else snapshot['native']
                    destination[collection] = [dict(history()[0], from_owner=previous)]
                    original = copy.deepcopy(snapshot)
                    with self.assertRaisesRegex(SnapshotError, '^invalid item previous owner$'):
                        view(snapshot, {}, 'provenance', 100, uid=81)
                    self.assertEqual(snapshot, original)

    def test_full_cli_preserves_conflicting_previous_owners_and_global_refusal_at_every_limit(self):
        import subprocess
        import tempfile
        snapshot = baseline();rows = history();rows[0]['from_owner'] = [1, 8, 0]
        snapshot['native'].update(items=[dict(position(slot=7), revision=5)], uid_history_events=rows)
        snapshot['ownership_events'] = [dict(rows[0], from_owner=[1, 9, 0],
                                            personal_alias='private-owner-alias')]
        report = Reconciler().audit(snapshot)
        self.assertGreater(report['exception_counts'].get('broken_item_owner_history', 0), 0)
        with tempfile.TemporaryDirectory(prefix='provenance-owner-') as folder:
            path = Path(folder)/'snapshot.json';payload=json.dumps(snapshot).encode();path.write_bytes(payload)
            for limit in (0, 1, 100):
                command=[sys.executable,str(ROOT/'scripts/reconcile_economy_accounting.py'),str(path),
                         '--view','provenance','--uid','81','--limit',str(limit)]
                result=subprocess.run(command,capture_output=True,text=True,timeout=30)
                self.assertEqual((result.returncode,result.stderr),(1,''))
                output=json.loads(result.stdout)
                self.assertEqual(output['count'],3)
                self.assertEqual(output['coverage']['exception_count'],report['exception_count'])
                self.assertEqual(len(output['rows']),min(limit,3))
                if limit == 100:
                    self.assertEqual([row['from_owner'] for row in output['rows'] if row['revision']==4],
                                     [[1,9,0],[1,8,0]])
                self.assertNotIn('private-',result.stdout)
                self.assertEqual(path.read_bytes(),payload)


class ItemHistoryPositionTests(unittest.TestCase):
    def test_invalid_intermediate_positions_are_not_hidden_by_a_valid_final_row(self):
        changes = ({'root': 99}, {'parent': 81}, {'owner': [1, 0, 0]},
                   {'owner': [7, 1, 0]}, {'owner': [10, 7, 1]},
                   {'owner': [11, 7, 0]}, {'owner': [12, 7, 1]})
        for change in changes:
            for lineage in (False, True):
                rows = history()
                rows[0].update(to_equipment_slot=0, **change)
                rows[1].update(from_equipment_slot=0, from_owner=rows[0]['owner'])
                original = copy.deepcopy(rows)
                with self.subTest(change=change, lineage=lineage):
                    self.assertEqual(audit_history(rows, dict(position(slot=7), revision=5), lineage),
                                     {'invalid_item_history_position': 1})
                    self.assertEqual(rows, original)

    def test_resulting_slot_uses_the_event_projection_and_preserves_unknown(self):
        rows = history()
        rows[0].update(owner=[12, 7, 0], equipment_slot=0, to_equipment_slot=44)
        rows[1].update(from_owner=[12, 7, 0], from_equipment_slot=44)
        for lineage in (False, True):
            self.assertEqual(audit_history(rows, dict(position(slot=7), revision=5), lineage),
                             {'invalid_item_history_position': 1})
        del rows[0]['to_equipment_slot']
        for lineage in (False, True):
            self.assertEqual(audit_history(rows, dict(position(slot=7), revision=5), lineage),
                             {'missing_item_equipment_evidence': 1})

    def test_unanchored_and_unattributed_invalid_events_are_still_checked(self):
        rows = history();rows[0]['root'] = 99
        reader = Reconciler()
        reader.audit_lineage_uid_history('disposable', dict(uid_history_events=rows), {}, {})
        self.assertEqual(reader.counts['invalid_item_history_position'], 1)
        self.assertEqual(reader.counts['unknown_legacy_origin'], 1)
        reader = Reconciler()
        reader.audit_unattributed_uid_history('disposable', dict(unattributed_uid_events=rows,
            unattributed_uid_event_coverage=dict(uids=1, events=2)))
        self.assertEqual(dict(reader.counts), {'invalid_item_history_position': 1,
                                             'unattributed_ownership_event': 2})

    def test_selected_and_lineage_projections_count_a_bad_legacy_event_once(self):
        snapshot = baseline();rows = history();rows[0]['root'] = 99
        snapshot['ownership_events'] = copy.deepcopy(rows)
        snapshot['native'].update(items=[dict(position(slot=7), revision=5)], uid_history_events=rows)
        original = copy.deepcopy(snapshot)
        for limit in (0, 1, 100):
            result = Reconciler(limit).audit(snapshot)
            self.assertEqual(result['exception_counts'].get('invalid_item_history_position', 0), 1)
            self.assertLessEqual(len(result['exceptions']), limit)
            self.assertEqual(snapshot, original)

    def test_full_cli_refuses_invalid_history_at_every_view_and_limit(self):
        import subprocess
        import tempfile
        snapshot = baseline();rows = history();rows[0]['root'] = 99
        rows[0]['personal_alias'] = 'private-history-alias'
        snapshot['native'].update(items=[dict(position(slot=7), revision=5)], uid_history_events=rows)
        with tempfile.TemporaryDirectory(prefix='history-position-') as folder:
            path = Path(folder)/'snapshot.json';payload=json.dumps(snapshot).encode();path.write_bytes(payload)
            for name in ('exceptions', 'holdings', 'provenance', 'operation', 'supply', 'prices', 'routes'):
                for limit in (0, 1, 100):
                    command=[sys.executable,str(ROOT/'scripts/reconcile_economy_accounting.py'),str(path),
                             '--view',name,'--limit',str(limit)]
                    if name=='provenance':command += ['--uid','81']
                    if name=='operation':command += ['--operation-id','%032x' % 1]
                    result=subprocess.run(command,capture_output=True,text=True,timeout=30)
                    self.assertEqual((result.returncode,result.stderr),(1,''))
                    value=json.loads(result.stdout)
                    if name=='exceptions':self.assertEqual(value['exception_counts'],{'invalid_item_history_position':1})
                    else:self.assertEqual(value['coverage']['exception_count'],1)
                    self.assertNotIn('private-history',result.stdout)
                    self.assertEqual(path.read_bytes(),payload)


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

    def test_missing_owner_evidence_remains_separate_from_equipment_history(self):
        for lineage in (False, True):
            rows = history()
            for row in rows:
                del row['from_owner']
            original = copy.deepcopy(rows)
            self.assertEqual(audit_history(rows, dict(position(slot=7), revision=5), lineage),
                             {'missing_item_owner_evidence': 1})
            self.assertEqual(rows, original)
            del rows[0]['to_equipment_slot']
            self.assertEqual(audit_history(rows, dict(position(slot=7), revision=5), lineage),
                             {'missing_item_owner_evidence': 1, 'missing_item_equipment_evidence': 1})

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
                        expected = {'invalid_item_equipment_slot': 1, 'missing_item_equipment_evidence': 1}
                        if field == 'to_equipment_slot':
                            expected['invalid_item_history_position'] = 1
                        self.assertEqual(audit_history(rows, dict(position(slot=7), revision=5), lineage),
                                         expected)

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
