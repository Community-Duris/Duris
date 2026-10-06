#!/usr/bin/env python3
"""Original-capsule SQL audit: bounded reader and native/private-engine faults."""

import copy
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import time
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT/'scripts'))
sys.path.insert(0, str(ROOT/'tests/async'))
import economic_sql_canonical_audit as audit
import economic_restore_evidence as evidence
from test_plan5_child_identity import NATIVE_PROBE, child
from test_reconcile_economy_accounting import clean_snapshot, Reconciler
from reconcile_economy_accounting import SnapshotError, view


def native_mobile_stock():
    def string(value):
        return struct.pack('<I', len(value)) + value
    return (struct.pack('<IihQqibB', 1, -1, 7, 81, 0, 100, 1, 15) +
            string(b'\0\xff') + string(b'') * 3 + bytes(169) +
            struct.pack('<IhhQ', 1, 2, -3, 4) + struct.pack('<I', 1) +
            string(b'key') + string(b'description') + struct.pack('<BIii', 1, 2, 0, -1))


NATIVE_MOBILE_STOCK_DAMAGE = (
    ('object count', 0, struct.pack('<I', 4097)), ('parent', 4, struct.pack('<i', 0)),
    ('slot', 8, struct.pack('<h', 44)), ('zero UID', 10, bytes(8)),
    ('reserved UID', 10, struct.pack('<Q', 2**64-1)),
    ('vnum', 26, struct.pack('<i', -1)), ('literal mask', 31, b'\0'),
    ('string bound', 32, struct.pack('<I', 4097)),
    ('dynamic row budget', 219, struct.pack('<I', 8192)),
    ('extra row budget', 235, struct.pack('<I', 8192)),
    ('spellbook boolean', 261, b'\x02'), ('spell row budget', 262, struct.pack('<I', 8192)))


def native_mobile_forests():
    def forest(rows):
        result = bytearray(struct.pack('<I', len(rows)))
        for parent, slot, uid in rows:
            item = bytearray(native_mobile_stock()[4:])
            struct.pack_into('<ihQ', item, 0, parent, slot, uid)
            result.extend(item)
        return bytes(result)
    return [(label, valid, forest(rows)) for label, valid, rows in (
        ('equipped-carried-DFS', True, [(-1, 1, 81), (-1, 43, 82), (-1, 0, 83), (2, 0, 84), (3, 0, 85), (2, 0, 86)]),
        ('maximum UID', True, [(-1, 0, 2**64-2)]),
        ('maximum depth', True, [(index-1, 0, index+81) for index in range(32)]),
        ('depth overflow', False, [(index-1, 0, index+81) for index in range(33)]),
        ('duplicate UID', False, [(-1, 0, 81), (-1, 0, 81)]),
        ('reopened subtree', False, [(-1, 0, 81), (0, 0, 82), (-1, 0, 83), (0, 0, 84)]),
        ('negative parent', False, [(-2, 0, 81)]),
        ('forward parent', False, [(1, 0, 81), (-1, 0, 82)]),
        ('child equipment', False, [(-1, 0, 81), (0, 1, 82)]),
        ('equipment order', False, [(-1, 2, 81), (-1, 1, 82)]),
        ('equipment after carried', False, [(-1, 0, 81), (-1, 1, 82)]),
        ('negative equipment', False, [(-1, -1, 81)]))]


def native_mobile_image(version=2, state=1, items=b'\0' * 4, identity=42):
    """Modeled wire bytes; the original flatfile fixture supplies native proof."""
    source = struct.pack('<HH16s16sQI', 10, 1, b'\x47' + bytes(15), b'\x48' + bytes(15), 3, 5)
    reference = (b'QMNREF\0\0' + struct.pack('<HBBIQ', 1, 1, 0, 148, identity) +
                 b'\x46' + bytes(15) + source + struct.pack('<iiiQQ', 9001, 3000, 30, 2, 3))
    reference += hashlib.sha256(reference).digest()
    image = (b'QMNIMG\0\0' + struct.pack('<HBBI', version, state, 0,
             (216 if version == 1 else 256) + len(items)) + reference + b'\x49' + bytes(15))
    if version == 2:
        image += struct.pack('<Q4q', 4, *(0 if state == 2 else coin for coin in (5, 6, 7, 8)))
    image += struct.pack('<I', len(items)) + items
    return image + hashlib.sha256(image).digest()


class RestoreProjectionFixture:
    """Synthetic SQL JSON projections; native proof uses original C++ bytes."""

    def __init__(self, baseline=False, rejected=False):
        from test_plan5_child_identity import ChildIdentityTests
        from test_economic_sql_audit_origins import witness, baseline_projections
        self.baseline = witness() if baseline else None
        if baseline:
            self.encoded = self.baseline['canonical_plan']
            self.frozen = self.baseline['canonical_intent']
        else:
            snapshot = ChildIdentityTests().integer_snapshot(nested=True)
            plan = bytearray.fromhex(snapshot['operations'][0]['canonical_plan'])
            intent = bytearray(witness()['canonical_intent'])
            # Reuse the existing empty EAI1 model, binding its metadata and
            # digest to the existing child/custody EAP1 model before damage.
            for target, start, size in ((4, 4, 2), (12, 84, 12), (24, 96, 2), (32, 8, 48),
                                       (80, 56, 16), (96, 76, 8), (112, 104, 48), (192, 184, 32)):
                intent[target:target+size] = plan[start:start+size]
            intent[26], intent[27] = plan[72], plan[100]
            self.frozen = bytes(intent)
            plan[152:184] = evidence.decode_intent(self.frozen)['intent_digest']
            self.encoded = bytes(plan)
        plan = evidence.decode_plan(self.encoded)
        meta = plan['metadata']
        self.operation = meta[2].hex()
        self.rows = dict(metadata=[[*(value.hex() for value in meta[:3]),
            meta[3].hex() if any(meta[3]) else None, *meta[4:11], meta[11].hex() if meta[11] else None,
            plan['intent_digest'].hex(), plan['domain_digest'].hex(), plan['plan_digest'].hex(),
            len(self.frozen), len(self.encoded), 1, 0, *plan['counts']]],
            accounts=[[i, key.hex(), *before, *after, old, new]
                      for i, (key, before, after, old, new) in enumerate(plan['effects'])],
            postings=[[i, event, account, child, *delta, value]
                      for i, (event, account, child, delta, value) in enumerate(plan['postings'])],
            children=[[i+1, child.hex(), domain, discriminator, parent, relationship]
                      for i, (child, domain, discriminator, parent, relationship) in enumerate(plan['children'])],
            items=[[i, event, child, uid, old[6], new[6]]
                   for i, (event, child, uid, old, new) in enumerate(plan['events'])],
            custody=[[i, uid, new[4], new[5], *((7, 0, 0) if old[1] == 0 else (old[0], old[2], old[3])),
                      new[0], new[2], new[3], new[6], old[7], new[7]]
                     for i, (event, child, uid, old, new) in enumerate(plan['events'])])
        if baseline:
            self.rows['reservations'] = [[row['lineage'].hex(), row['epoch'].hex(), row['identity_kind'],
                row['identity_id'], row['operation_id'].hex()] for row in baseline_projections(self.baseline)[2]]
            value = self.baseline
            self.rows['witness'] = [[meta[0].hex(), meta[1].hex(), value['book_revision'], value['witness_version'],
                value['holding_count'], value['item_count'], value['witness_digest'].hex(), len(value['canonical_witness']),
                value['canonical_witness'][80:120].hex(), value['inbox_revision'], value['inbox_type'], value['inbox_schema'],
                value['inbox_payload'], value['inbox_result_payload'].hex(), value['inbox_keys_hash'].hex(),
                value['command_accepted_at_usec'], value['inbox_command_hash'].hex()]]
        if rejected:
            self.rows['metadata'][0][14] = self.rows['metadata'][0][16] = None
            self.rows['metadata'][0][17:25] = [2, 5, 0, 0, 0, 0, 0, 0]
        self.queries = []
        self.admission_column_count = '1'
        self.mobile_image = None
        self.mobile_row = None

    def sql(self, query):
        if not query.startswith('SELECT '):
            raise AssertionError(query)
        self.queries.append(query)
        if 'FROM quest_mobile_native' in query:
            if query.startswith('SELECT COUNT(*)'):
                return '0' if self.mobile_image is None else '1'
            if query.startswith('SELECT JSON_ARRAY('):
                return '' if self.mobile_row is None else json.dumps(self.mobile_row)
            if query.startswith('SELECT HEX(SUBSTRING('):
                offset, count = map(int, query.split('canonical_image,')[1].split(')')[0].split(','))
                return self.mobile_image[offset-1:offset-1+count].hex()
            raise AssertionError(query)
        if 'SELECT JSON_ARRAY(' in query:
            table = query.split(' FROM ', 1)[1].split(' ', 1)[0]
            name = {'economic_accounting_operation': 'metadata', 'economic_accounting_account_effect': 'accounts',
                'economic_accounting_coin_posting': 'postings', 'economic_accounting_child': 'children',
                'economic_accounting_item_reference': 'items', 'economic_baseline_reservation': 'reservations',
                'economic_baseline_witness': 'witness'}[table]
            if 'LEFT JOIN item_ownership_ledger' in query:
                name = 'custody'
            return '\n'.join(json.dumps(row) for row in self.rows[name])
        if query.startswith('SELECT LOWER(HEX(operation_id))'):
            return self.operation
        if query == 'SELECT COUNT(*) FROM economic_accounting_operation;':
            return '1'
        if query.startswith('SELECT HEX(SUBSTRING('):
            if 'canonical_intent' in query:
                return self.frozen.hex()
            if 'canonical_plan' in query:
                return self.encoded.hex()
            return self.baseline['canonical_witness'].hex()
        if 'information_schema.tables' in query:
            if "table_name='quest_mobile_native'" in query:
                return '1'
            return '3'
        if 'information_schema.columns' in query:
            return self.admission_column_count
        return '0'


class RestoreProjectionTests(unittest.TestCase):
    @staticmethod
    def aliases(value):
        return [float(value)] + ([bool(value)] if value in (0, 1) else [])

    def refuse(self, fixture, code):
        before = copy.deepcopy(fixture.rows)
        capsules = fixture.frozen, fixture.encoded
        with self.assertRaisesRegex(RuntimeError, 'restore_economic_' + code + '_mismatch'):
            evidence.require_integrity(fixture)
        self.assertEqual(fixture.rows, before)
        self.assertEqual((fixture.frozen, fixture.encoded), capsules)
        self.assertTrue(all(query.startswith('SELECT ') for query in fixture.queries))

    def test_intact_ordinary_baseline_and_rejected_controls(self):
        for baseline, rejected in ((False, False), (True, False), (False, True)):
            with self.subTest(baseline=baseline, rejected=rejected):
                fixture = RestoreProjectionFixture(baseline, rejected)
                before = copy.deepcopy(fixture.rows)
                evidence.require_integrity(fixture)
                self.assertEqual(fixture.rows, before)
                self.assertTrue(all(query.startswith('SELECT ') for query in fixture.queries))

    def test_sql_native_mobile_corruption_is_not_qualified(self):
        fixture = RestoreProjectionFixture()
        fixture.mobile_image = b'corrupt-native-mobile-image'
        fixture.mobile_row = [42, 2, 3, 1, len(fixture.mobile_image)]
        self.refuse(fixture, 'native_mobile')

    def test_sql_native_mobile_valid_versions_lifetimes_and_stock(self):
        for version, state, stock in ((1, 1, bytes(4)), (1, 2, bytes(4)), (2, 1, bytes(4)),
                                      (2, 2, bytes(4)), (1, 1, native_mobile_stock()),
                                      (2, 1, native_mobile_stock())):
            fixture = RestoreProjectionFixture()
            fixture.mobile_image = native_mobile_image(version, state, stock)
            fixture.mobile_row = [42, 2, 3, state, len(fixture.mobile_image)]
            before = fixture.mobile_image, copy.deepcopy(fixture.mobile_row)
            with self.subTest(version=version, state=state, stock=len(stock)):
                evidence.require_integrity(fixture)
                self.assertEqual((fixture.mobile_image, fixture.mobile_row), before)
                self.assertEqual(evidence.decode_native_mobile(fixture.mobile_image), (42, 2, 3, state))
                self.assertTrue(all(query.startswith('SELECT ') for query in fixture.queries))

    def test_sql_native_mobile_projection_types_values_and_binding(self):
        for index, changed in ((0, 0), (0, 43), (0, 2**64-1), (1, 0), (1, 3), (1, 2**64),
                               (2, 0), (2, 4), (3, 0), (3, 2), (3, 3), (4, 219),
                               (4, 4*1024*1024+1), (4, 261)):
            fixture = RestoreProjectionFixture()
            fixture.mobile_image = native_mobile_image()
            fixture.mobile_row = [42, 2, 3, 1, len(fixture.mobile_image)]
            fixture.mobile_row[index] = changed
            with self.subTest(index=index, changed=changed):
                self.refuse(fixture, 'native_mobile')
        for index in range(5):
            for changed in (True, 42.0, '42', None):
                fixture = RestoreProjectionFixture()
                fixture.mobile_image = native_mobile_image()
                fixture.mobile_row = [42, 2, 3, 1, len(fixture.mobile_image)]
                fixture.mobile_row[index] = changed
                with self.subTest(index=index, changed=changed):
                    self.refuse(fixture, 'native_mobile')

    def test_sql_native_mobile_stock_value_corruption_refuses(self):
        stock = native_mobile_stock()
        # The native oracle also consumes this deliberately modeled stock.
        for label, offset, changed in NATIVE_MOBILE_STOCK_DAMAGE:
            damaged = bytearray(stock)
            damaged[offset:offset+len(changed)] = changed
            fixture = RestoreProjectionFixture()
            fixture.mobile_image = native_mobile_image(items=bytes(damaged))
            fixture.mobile_row = [42, 2, 3, 1, len(fixture.mobile_image)]
            with self.subTest(label=label):
                self.refuse(fixture, 'native_mobile')

    def test_sql_native_mobile_forest_topology_and_depth(self):
        for label, valid, stock in native_mobile_forests():
            fixture = RestoreProjectionFixture()
            fixture.mobile_image = native_mobile_image(items=stock)
            fixture.mobile_row = [42, 2, 3, 1, len(fixture.mobile_image)]
            with self.subTest(label=label):
                if valid:
                    evidence.require_integrity(fixture)
                else:
                    self.refuse(fixture, 'native_mobile')

    def test_original_admission_time_and_command_hash_refuse_restore(self):
        for index, value in [(15, value) for value in (0, -1, 2**64, True, 123456.0, "123456", 123457)] + [
            (16, value) for value in (None, "00" * 32, "11" * 32, "11" * 31, "11" * 33)]:
            with self.subTest(index=index, value=value):
                fixture = RestoreProjectionFixture(baseline=True)
                fixture.rows['witness'][0][index] = value
                self.refuse(fixture, 'baseline_witness')

    def test_historical_null_admission_and_column_metadata(self):
        for column_count in ('0', '1'):
            fixture = RestoreProjectionFixture(baseline=True)
            fixture.admission_column_count = column_count
            fixture.rows['witness'][0][15] = None
            evidence.require_integrity(fixture)
            self.assertIsNone(fixture.rows['witness'][0][15])
            selected = next(query for query in fixture.queries if 'FROM economic_baseline_witness w JOIN' in query)
            self.assertIn('w.command_accepted_at_usec' if column_count == '1' else ',NULL,', selected)
        for column_count in ('2', '-1', '1.0', '', None):
            fixture = RestoreProjectionFixture(baseline=True)
            fixture.admission_column_count = column_count
            self.refuse(fixture, 'baseline_witness')

    def test_all_normalized_projection_families_require_exact_integers(self):
        for name, code in (('accounts', 'canonical_account'), ('postings', 'canonical_posting'),
                           ('children', 'canonical_child'), ('items', 'canonical_item'), ('custody', 'canonical_custody')):
            for index, value in enumerate(RestoreProjectionFixture().rows[name][0]):
                if type(value) is not int:
                    continue
                for alias in self.aliases(value):
                    with self.subTest(family=name, index=index, representation=type(alias).__name__):
                        fixture = RestoreProjectionFixture()
                        fixture.rows[name][0][index] = alias
                        self.refuse(fixture, code)

    def test_root_metadata_requires_exact_integers(self):
        for index in range(4, 11):
            for alias in self.aliases(RestoreProjectionFixture().rows['metadata'][0][index]):
                with self.subTest(index=index, representation=type(alias).__name__):
                    fixture = RestoreProjectionFixture()
                    fixture.rows['metadata'][0][index] = alias
                    self.refuse(fixture, 'metadata')

    def test_committed_counts_and_status_require_exact_integers(self):
        for index in range(17, 25):
            for alias in self.aliases(RestoreProjectionFixture().rows['metadata'][0][index]):
                with self.subTest(index=index, representation=type(alias).__name__):
                    fixture = RestoreProjectionFixture()
                    fixture.rows['metadata'][0][index] = alias
                    self.refuse(fixture, 'plan' if index < 19 else 'canonical_count')

    def test_rejected_controls_require_unsigned_integer_status_and_zero_counts(self):
        for index in range(17, 25):
            value = RestoreProjectionFixture(rejected=True).rows['metadata'][0][index]
            for alias in (*self.aliases(value), None):
                with self.subTest(index=index, representation=type(alias).__name__):
                    fixture = RestoreProjectionFixture(rejected=True)
                    fixture.rows['metadata'][0][index] = alias
                    self.refuse(fixture, 'plan')
        for code in (-1, 2**32, True, '5'):
            with self.subTest(result_code=code):
                fixture = RestoreProjectionFixture(rejected=True)
                fixture.rows['metadata'][0][18] = code
                self.refuse(fixture, 'plan')
        for code in (1, 2**32-1):
            fixture = RestoreProjectionFixture(rejected=True)
            fixture.rows['metadata'][0][18] = code
            evidence.require_integrity(fixture)

    def test_baseline_reservations_require_exact_integers(self):
        for index in (2, 3):
            for alias in self.aliases(RestoreProjectionFixture(baseline=True).rows['reservations'][0][index]):
                with self.subTest(index=index, representation=type(alias).__name__):
                    fixture = RestoreProjectionFixture(baseline=True)
                    fixture.rows['reservations'][0][index] = alias
                    self.refuse(fixture, 'baseline_reservation')


class CanonicalAuditTests(unittest.TestCase):
    def connection(self):
        connection = mock.Mock()
        cursor = connection.cursor.return_value
        cursor.fetchmany.side_effect = [[{'count': 16}], [], [{'count': 0}], [], [{'mobiles': 0}], [], [{'size': 0}], [],
                                       [{'unwanted': 0}], [], [{'claims': 0}], [], [{'unwanted_claim': 0}], []]
        return connection, cursor

    def test_bounded_scalar_projection_and_select_only(self):
        connection, cursor = self.connection()
        cursor.fetchmany.side_effect = [[{'count': 0}], []]
        executor = audit.CursorExecutor(cursor)
        self.assertEqual(executor.sql('SELECT COUNT(*) FROM economic_accounting_operation;'), '0')
        self.assertIn('LIMIT 100001', cursor.execute.call_args.args[0])
        with self.assertRaisesRegex(audit.AuditError, 'SELECT only'):
            executor.sql('UPDATE economic_lineage_state SET revision=revision')
        self.assertEqual(cursor.execute.call_count, 1)

    def test_root_limit_refuses_before_capsule_reads(self):
        connection, cursor = self.connection()
        cursor.fetchmany.side_effect = [[{'count': 16}], [], [{'count': audit.MAX_ROWS+1}], []]
        with mock.patch.object(audit, 'require_integrity') as verifier:
            with self.assertRaisesRegex(audit.AuditError, 'root count'):
                audit.capture(connection)
            verifier.assert_not_called()
        connection.rollback.assert_called_once_with()
        cursor.close.assert_called_once_with()

    def test_missing_or_nontransactional_source_refuses(self):
        connection, cursor = self.connection()
        cursor.fetchmany.side_effect = [[{'count': 15}], []]
        with mock.patch.object(audit, 'require_integrity') as verifier:
            with self.assertRaisesRegex(audit.AuditError, 'not InnoDB'):
                audit.capture(connection)
            verifier.assert_not_called()
        connection.rollback.assert_called_once_with()
        cursor.close.assert_called_once_with()

    def test_capsule_budget_refuses_before_decoding(self):
        connection, cursor = self.connection()
        cursor.fetchmany.side_effect = [[{'count': 16}], [], [{'count': 1}], [],
                                       [{'mobiles': 0}], [], [{'size': audit.MAX_INPUT_BYTES+1}], []]
        with mock.patch.object(audit, 'require_integrity') as verifier:
            with self.assertRaisesRegex(audit.AuditError, 'capsules exceed'):
                audit.capture(connection)
            verifier.assert_not_called()
        connection.rollback.assert_called_once_with()
        cursor.close.assert_called_once_with()

    def test_native_mobile_budget_refuses_before_capsule_reads(self):
        for mobiles in (-1, audit.MAX_ROWS+1):
            connection, cursor = self.connection()
            cursor.fetchmany.side_effect = [[{'count': 16}], [], [{'count': 0}], [],
                                           [{'mobiles': mobiles}], []]
            with self.subTest(mobiles=mobiles), mock.patch.object(audit, 'require_integrity') as verifier:
                with self.assertRaisesRegex(audit.AuditError, 'native mobile count'):
                    audit.capture(connection)
                verifier.assert_not_called()
            connection.rollback.assert_called_once_with()
            cursor.close.assert_called_once_with()

    def test_orphan_and_rejected_details_refuse_before_decoding(self):
        connection, cursor = self.connection()
        cursor.fetchmany.side_effect = [[{'count': 16}], [], [{'count': 0}], [], [{'mobiles': 0}], [], [{'size': 0}], [], [{'unwanted': 1}], []]
        with mock.patch.object(audit, 'require_integrity') as verifier:
            with self.assertRaisesRegex(audit.AuditError, 'orphan_or_rejected_detail'):
                audit.capture(connection)
            verifier.assert_not_called()
        connection.rollback.assert_called_once_with()
        cursor.close.assert_called_once_with()

    def test_streamed_detail_limits_refuse(self):
        for limit in ('rows', 'bytes'):
            with self.subTest(limit=limit):
                cursor = mock.Mock()
                cursor.fetchmany.side_effect = [[{'value': '123'}, {'value': '456'}], []]
                with mock.patch.object(audit, 'MAX_ROWS', 1 if limit == 'rows' else 100), \
                        mock.patch.object(audit, 'MAX_INPUT_BYTES', 100 if limit == 'rows' else 4):
                    with self.assertRaisesRegex(audit.AuditError, 'input limit'):
                        audit.CursorExecutor(cursor).sql('SELECT value FROM detail')

    def test_malformed_scalar_refuses(self):
        for row in ({'a': 1, 'b': 2}, {'a': None}, {'a': b'private-bytes'}):
            with self.subTest(row=row):
                cursor = mock.Mock()
                cursor.fetchmany.side_effect = [[row], []]
                with self.assertRaises(audit.AuditError):
                    audit.CursorExecutor(cursor).sql('SELECT value FROM detail')

    def test_fixed_refusal_and_verifier_failure_always_roll_back(self):
        connection, cursor = self.connection()
        with mock.patch.object(audit, 'require_integrity', side_effect=RuntimeError('restore_economic_canonical_child_mismatch')):
            with self.assertRaisesRegex(audit.AuditError, 'canonical_child_mismatch'):
                audit.capture(connection)
        connection.rollback.assert_called_once_with()
        cursor.close.assert_called_once_with()

    def test_transaction_start_failure_and_rollback_failure_close_cursor(self):
        connection, cursor = self.connection()
        cursor.execute.side_effect = RuntimeError('start')
        connection.rollback.side_effect = RuntimeError('rollback')
        with self.assertRaisesRegex(RuntimeError, 'rollback'):
            audit.capture(connection)
        cursor.close.assert_called_once_with()

    def test_success_discloses_database_scope_and_remaining_gates(self):
        connection, cursor = self.connection()
        with mock.patch.object(audit, 'require_integrity') as verifier:
            result = audit.capture(connection)
            verifier.assert_called_once()
        self.assertEqual(result['scope'], 'database')
        self.assertEqual(result['retained_roots'], 0)
        self.assertTrue(result['read_only'])
        for field in ('complete_command_receipts_authenticated', 'source_capture_qualified', 'release_qualified'):
            self.assertFalse(result[field])
        connection.rollback.assert_called_once_with()
        cursor.close.assert_called_once_with()


    def test_source_claim_limit_refuses_before_original_capsule_decoding(self):
        connection, cursor = self.connection()
        cursor.fetchmany.side_effect = [[{'count': 16}], [], [{'count': 1}], [], [{'mobiles': 0}], [], [{'size': 512}], [],
                                       [{'unwanted': 0}], [], [{'claims': audit.MAX_ROWS+1}], []]
        with mock.patch.object(audit, 'require_integrity') as verifier:
            with self.assertRaisesRegex(audit.AuditError, 'source_claim'):
                audit.capture(connection)
            verifier.assert_not_called()
        connection.rollback.assert_called_once_with()
        cursor.close.assert_called_once_with()

    def test_database_wide_source_claim_disagreement_refuses_before_decoding(self):
        connection, cursor = self.connection()
        cursor.fetchmany.side_effect = [[{'count': 16}], [], [{'count': 1}], [], [{'mobiles': 0}], [], [{'size': 512}], [],
                                       [{'unwanted': 0}], [], [{'claims': 1}], [], [{'unwanted_claim': 1}], []]
        with mock.patch.object(audit, 'require_integrity') as verifier:
            with self.assertRaisesRegex(audit.AuditError, 'source_claim'):
                audit.capture(connection)
            verifier.assert_not_called()
        connection.rollback.assert_called_once_with()
        cursor.close.assert_called_once_with()


@unittest.skipUnless(os.environ.get('DURIS_PLAN5_CANONICAL_NATIVE') == '1',
                     'requires explicitly selected native and fresh private SQL checks')
class NativeCanonicalAuditTests(unittest.TestCase):
    def test_original_plans_and_projection_faults_both_engines(self):
        import pymysql
        import migration_runner as migrations
        import persistence_restore as restore
        import economic_sql_audit_snapshot as exporter
        from economic_restore_evidence import decode_plan
        from test_persistence_backup_integration import sql

        work = Path(os.environ['DURIS_PLAN5_CANONICAL_ARTIFACTS']).resolve()
        self.assertFalse(work.exists())
        self.assertTrue(work.is_relative_to((ROOT/'bin').resolve()))
        work.mkdir(parents=True)
        source = work/'canonical-probe.cpp'
        # The shared independent probe now includes the custody transition.
        native_source = NATIVE_PROBE
        if os.environ.get('DURIS_PLAN5_CANONICAL_MOBILE') == '1':
            native_source = native_source.replace('old.owner={item_owner_type::player,8,0};',
                'old.owner={item_owner_type::native_mobile,42,0}; old.equipment_slot=43;')
            native_source = native_source.replace('next.owner.id=7;',
                'next.owner={item_owner_type::player,7,0}; next.equipment_slot=0;')
        if os.environ.get('DURIS_PLAN5_CANONICAL_SOURCE') == '1':
            native_source = native_source.replace('meta.reason=economic_reason::bank_transfer;',
                'meta.reason=economic_reason::bank_transfer; '
                'meta.source_event=economic_source_event{economic_source_kind::legacy_import,id(0x66),id(0x77),1,0};')
        source.write_text(native_source)
        builds, native = [], None
        sources = [str(source), 'src/economy/economic_accounting_types.c',
                   'src/economy/economic_accounting_plan.c', 'src/economy/economic_source_event.c',
                   'src/economy/economic_accounting_intent.c', 'src/persistence/critical_command.c',
                   'src/item/item_transfer_command.c', 'src/world/quest_mobile_native_reference.c',
                   'src/economy/shop_trade_recovery_manifest.c',
                   'src/item/craft_pouch_mutation.c',
                   'src/combat/chaos_pouch_ledger.c', 'src/player/player_snapshot_codec.c']
        for backend in ('sql', 'flatfile'):
            binary = work/('canonical-probe-'+backend)
            command = ['g++', '-std=c++20', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
                       '-O1', '-g', '-fsanitize=address,undefined', '-fno-omit-frame-pointer',
                       '-fno-pie', '-no-pie', '-D__NO_TESTS__', '-Isrc']
            if backend == 'flatfile': command.append('-D__NO_MYSQL__')
            command += sources+['-lcrypto', '-o', str(binary)]
            started = time.monotonic()
            reuse = os.environ.get('DURIS_PLAN5_CANONICAL_PROBE_REUSE')
            if reuse:
                previous = Path(reuse).resolve()
                self.assertTrue(previous.is_relative_to((ROOT/'bin').resolve()))
                self.assertEqual((previous/'canonical-probe.cpp').read_text(), native_source)
                recorded = next(row for row in json.loads((previous/'native-builds.json').read_text())
                                if row['backend'] == backend)
                inputs = json.loads((previous.parent/(previous.name+'-inputs.json')).read_text())
                for name in sources[1:]:
                    self.assertEqual(hashlib.sha256((ROOT/name).read_bytes()).hexdigest(), inputs[name])
                expected = [str(source) if arg == str(previous/'canonical-probe.cpp') else
                            str(binary) if arg == str(previous/('canonical-probe-'+backend)) else arg
                            for arg in recorded['command']]
                self.assertEqual(expected, command)
                original = previous/('canonical-probe-'+backend)
                self.assertEqual(hashlib.sha256(original.read_bytes()).hexdigest(), recorded['binary_sha256'])
                self.assertEqual((recorded['compile_exit'],recorded['run_exit']), (0,0))
                shutil.copy2(original,binary)
                shutil.copyfile(previous/(backend+'-compile.log'),work/(backend+'-compile.log'))
                compile_exit = recorded['compile_exit']
            else:
                compiled = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
                (work/(backend+'-compile.log')).write_text(compiled.stdout+compiled.stderr)
                self.assertEqual(compiled.returncode, 0, compiled.stdout+compiled.stderr)
                compile_exit = compiled.returncode
            ran = subprocess.run([str(binary)], capture_output=True, text=True,
                env=dict(os.environ, ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',
                         UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1'))
            (work/(backend+'-stdout.log')).write_text(ran.stdout)
            (work/(backend+'-stderr.log')).write_text(ran.stderr)
            self.assertEqual((ran.returncode, ran.stderr), (0, ''))
            output = json.loads(ran.stdout)
            if native is not None: self.assertEqual(native, output)
            native = output
            builds.append({'backend': backend, 'command': command, 'compile_exit': compile_exit,
                           'run_exit': ran.returncode, 'seconds': time.monotonic()-started,
                           'verified_probe_reuse': bool(reuse),'reuse_directory': reuse,
                           'binary_sha256': hashlib.sha256(binary.read_bytes()).hexdigest()})
            (work/'native-builds.json').write_text(json.dumps(builds, indent=2)+'\n')
        frozen, encoded = bytes.fromhex(native['intent']), bytes.fromhex(native['plan'])
        plan = decode_plan(encoded)
        results = []
        for engine in ('mariadb', 'mysql'):
            with restore.private_database(work/engine, engine) as env:
                version = sql(env, 'SELECT VERSION()')
                sql(env, payload=(ROOT/'migrations/bootstrap_multithread_safe.sql').read_bytes())
                with mock.patch.dict(os.environ, env, clear=True):
                    manifest = migrations.load_manifest()
                    executor = migrations.MysqlExecutor(manifest)
                    executor.adopt('fresh_bootstrap')
                    migrations.run_pending(manifest, executor)
                self.assertEqual(sql(env, 'SELECT sequence_number FROM mud_schema_history ORDER BY sequence_number DESC LIMIT 1'), '61')
                owner = pymysql.connect(unix_socket=env['DB_SOCKET'], user='root', database='duris_restore',
                                        autocommit=True, cursorclass=pymysql.cursors.DictCursor)
                try:
                    def insert(table, fields):
                        with owner.cursor() as cursor:
                            cursor.execute('INSERT INTO '+table+' ('+','.join(fields)+') VALUES ('+
                                           ','.join(['%s']*len(fields))+')', tuple(fields.values()))
                    operation = encoded[40:56]
                    creator, legacy = bytes([7])*16, bytes([0x55])*16
                    for identity in (creator, operation, legacy):
                        insert('critical_operation_inbox', dict(operation_id=identity, command_hash=bytes([1])*32,
                            keys_hash=bytes([2])*32, command_type=3, schema_version=2, payload_version=1,
                            status=1, result_code=0, durable_revision=1, result_payload=b''))
                    with owner.cursor() as cursor:
                        cursor.execute('UPDATE critical_operation_inbox SET committed_at=CURRENT_TIMESTAMP(6)')
                    insert('economic_epoch', dict(lineage=encoded[8:24], epoch=encoded[24:40], ordinal=2,
                        transition_kind=1, transition_digest=bytes([3])*32, creating_operation_id=creator))
                    insert('economic_lineage_state', dict(lineage=encoded[8:24], active_epoch=None))
                    fields = dict(operation_id=operation, lineage=encoded[8:24], epoch=encoded[24:40],
                        original_operation_id=None, accounting_version=1, writer_id=1, policy_version=1,
                        compiler_version=1, actor_kind=1, actor_id=7, reason=1, source_event=plan['metadata'][-1],
                        intent_digest=plan['intent_digest'], domain_digest=plan['domain_digest'],
                        plan_digest=plan['plan_digest'], canonical_intent=frozen, canonical_plan=encoded,
                        outcome=1, result_code=0)
                    fields.update(zip(('account_count','posting_count','child_count','before_witness_count',
                                       'after_witness_count','item_event_count'), plan['counts']))
                    insert('economic_accounting_operation', fields)
                    if plan['metadata'][-1] is not None:
                        insert('economic_accounting_source_claim', dict(lineage=encoded[8:24],
                            source_event=plan['metadata'][-1], operation_id=operation, outcome=1))
                    for index, (key, before, after, oldrev, newrev) in enumerate(plan['effects']):
                        fields = dict(operation_id=operation, account_index=index, account_key=key,
                                      before_revision=oldrev, after_revision=newrev)
                        fields.update(zip((side+'_'+coin for side in ('before','after')
                            for coin in ('copper','silver','gold','platinum')), (*before,*after)))
                        insert('economic_accounting_account_effect', fields)
                    for index, (event, account, link, delta, amount) in enumerate(plan['postings']):
                        fields = dict(operation_id=operation, line_index=index, event_index=event,
                                      account_index=account, child_index=link, copper_value=amount)
                        fields.update(zip(('delta_'+coin for coin in ('copper','silver','gold','platinum')), delta))
                        insert('economic_accounting_coin_posting', fields)
                    for row in native['children']:
                        insert('economic_accounting_child', {**row, 'operation_id': operation,
                            'child_operation_id': bytes.fromhex(row['child_operation_id'])})
                    native_before, native_after = plan['events'][0][3:]
                    insert('item_ownership_ledger', dict(operation_id=legacy, event_index=0, item_uid=81,
                        root_item_uid=81, parent_item_uid=None, from_owner_type=native_before[0], from_owner_id=native_before[2],
                        from_owner_context_id=native_before[3], to_owner_type=native_after[0], to_owner_id=native_after[2], to_owner_context_id=native_after[3],
                        item_revision=2, from_owner_revision=1, to_owner_revision=2, reason_type=8,
                        source_site=1, from_equipment_slot=native_before[7], to_equipment_slot=native_after[7]))
                    insert('economic_accounting_item_reference', dict(operation_id=operation, line_index=0,
                        event_index=0, child_index=1, item_uid=81, before_revision=1, after_revision=2,
                        legacy_operation_id=legacy, legacy_event_index=0))
                    with owner.cursor() as cursor:
                        cursor.execute("CREATE USER 'canonical_reader'@'localhost' IDENTIFIED BY 'private-canonical-reader'")
                        cursor.execute("GRANT SELECT ON duris_restore.* TO 'canonical_reader'@'localhost'")
                    reader = pymysql.connect(unix_socket=env['DB_SOCKET'], user='canonical_reader',
                        password='private-canonical-reader', database='duris_restore', autocommit=True,
                        cursorclass=pymysql.cursors.SSDictCursor)
                    try:
                        with reader.cursor() as cursor:
                            with self.assertRaises(pymysql.MySQLError) as denied:
                                cursor.execute('UPDATE economic_lineage_state SET revision=revision')
                            self.assertEqual(denied.exception.args[0], 1142)
                        def inventory():
                            with owner.cursor() as cursor:
                                result = {}
                                for table in ('critical_operation_inbox','economic_lineage_state','economic_epoch',
                                    'economic_accounting_operation','economic_accounting_account_effect',
                                    'economic_accounting_coin_posting','economic_accounting_child',
                                    'economic_accounting_item_reference','economic_accounting_source_claim',
                                    'item_ownership_ledger'):
                                    cursor.execute('SELECT * FROM '+table+' ORDER BY 1,2')
                                    result[table] = cursor.fetchall()
                                return result
                        def check(label, code=None):
                            before = inventory()
                            wrapped = mock.Mock(wraps=reader)
                            cursor = mock.Mock(wraps=reader.cursor())
                            wrapped.cursor.return_value = cursor
                            try:
                                report = audit.capture(wrapped)
                            except audit.AuditError as error:
                                observed = str(error)
                                report = None
                            else:
                                observed = None
                            observation = {'engine': engine, 'label': label, 'expected_refusal': code,
                                'observed_refusal': observed, 'report': report,
                                'unchanged': before == inventory(), 'rollback_calls': wrapped.rollback.call_count}
                            (work/(engine+'-'+label+'-observation.json')).write_text(json.dumps(observation,indent=2)+'\n')
                            if code:
                                self.assertIsNotNone(observed, observation)
                                self.assertIn(code, observed)
                            else:
                                self.assertIsNone(observed, observation)
                                self.assertEqual(report['retained_roots'], 1)
                                self.assertFalse(report['release_qualified'])
                            wrapped.rollback.assert_called_once_with()
                            cursor.close.assert_called_once_with()
                            queries = [call.args[0] for call in cursor.execute.call_args_list]
                            self.assertTrue(all(query.startswith(('SELECT ','SET TRANSACTION ','START TRANSACTION ')) for query in queries))
                            self.assertEqual(before, inventory())
                            command = [sys.executable, str(ROOT/'scripts/economic_sql_canonical_audit.py'),
                                '--host','127.0.0.1','--socket',env['DB_SOCKET'],'--user','canonical_reader',
                                '--database','duris_restore','--password-env','PLAN5_CANONICAL_PASSWORD']
                            ran = subprocess.run(command, capture_output=True, text=True,
                                env=dict(os.environ, PLAN5_CANONICAL_PASSWORD='private-canonical-reader'))
                            self.assertEqual(ran.returncode, 2 if code else 0, ran.stdout+ran.stderr)
                            if code:
                                self.assertEqual(ran.stdout, '')
                                self.assertIn(code, ran.stderr)
                                self.assertNotIn(encoded.hex(), ran.stderr)
                            else:
                                self.assertEqual(ran.stderr, '')
                                self.assertEqual(json.loads(ran.stdout)['retained_roots'], 1)
                            self.assertEqual(before, inventory())
                            results.append({'engine': engine,'version': version,'label': label,'code': code,
                                'command': command,'exit': ran.returncode,'queries': queries,
                                'read_only': True,'rollback_calls': 1,'unchanged': True})
                            (work/'results.json').write_text(json.dumps(results,indent=2)+'\n')
                        check('intact')
                        # Private storage aliases retain exact original values.
                        # Record whether each engine's SQL JSON preserves an
                        # integer or exposes a float. Canonical columns and rows
                        # are restored after every cut; these are not releases.
                        for table, field, code in (
                            ('economic_accounting_account_effect', 'after_silver', 'canonical_account'),
                            ('economic_accounting_coin_posting', 'delta_silver', 'canonical_posting'),
                            ('economic_accounting_child', 'domain_id', 'canonical_child'),
                            ('economic_accounting_item_reference', 'before_revision', 'canonical_item'),
                            ('item_ownership_ledger', 'to_owner_context_id', 'canonical_custody')):
                            with owner.cursor() as cursor:
                                cursor.execute('SELECT COLUMN_TYPE,IS_NULLABLE FROM information_schema.columns '
                                    'WHERE table_schema=DATABASE() AND table_name=%s AND column_name=%s', (table,field))
                                original_column = cursor.fetchone()
                                cursor.execute('SHOW CREATE TABLE '+table)
                                original_schema = cursor.fetchone()['Create Table']
                            original_definition = next(line.strip().rstrip(',')
                                for line in original_schema.splitlines()
                                if line.lstrip().startswith('`'+field+'` '))
                            self.assertEqual(original_column['IS_NULLABLE'], 'NO')
                            for storage in ('DOUBLE', 'DECIMAL(20,1)'):
                                label = 'numeric_'+code+'_'+('float' if storage == 'DOUBLE' else 'decimal')
                                before = inventory()
                                with owner.cursor() as cursor:
                                    cursor.execute('ALTER TABLE '+table+' MODIFY '+field+' '+storage+' NOT NULL')
                                try:
                                    with owner.cursor() as cursor:
                                        cursor.execute('SELECT JSON_ARRAY('+field+') AS value FROM '+table)
                                        projected = [json.loads(row['value'])[0] for row in cursor.fetchall()]
                                    self.assertTrue(projected)
                                    self.assertTrue(all(type(value) in (int, float) for value in projected))
                                    floating = any(type(value) is float for value in projected)
                                    (work/(engine+'-'+label+'-projection.json')).write_text(json.dumps(
                                        {'table':table,'field':field,'storage':storage,'values':projected,
                                         'types':[type(value).__name__ for value in projected],
                                         'original_schema':original_schema,
                                         'original_definition':original_definition,
                                         'expected_refusal':floating},indent=2)+'\n')
                                    check(label, 'restore_economic_'+code+'_mismatch' if floating else None)
                                finally:
                                    with owner.cursor() as cursor:
                                        cursor.execute('ALTER TABLE '+table+' MODIFY '+original_definition)
                                        cursor.execute('SHOW CREATE TABLE '+table)
                                        self.assertEqual(cursor.fetchone()['Create Table'], original_schema)
                                self.assertEqual(before, inventory())
                                check(label+'_restored')
                        # Damage only the saved JSON projection, after reading
                        # the original native capsules with the SELECT-only role.
                        # Typed SQL integers themselves cannot retain these aliases.
                        before = inventory()
                        try:
                            reader.begin()
                            with reader.cursor(pymysql.cursors.DictCursor) as cursor:
                                cut = exporter.read_evidence(cursor,encoded[8:24],encoded[24:40],True)
                        finally:
                            reader.rollback()
                        modeled = clean_snapshot()
                        origins, items = modeled['account_origins'], modeled['item_origins']
                        items[0]['owner'] = [native_before[0], native_before[2], native_before[3]]
                        modeled.update(cut)
                        modeled['account_origins'], modeled['item_origins'] = origins, items
                        self.assertEqual(Reconciler().audit(modeled)['exception_count'],0)
                        self.assertEqual(Reconciler().audit(modeled)['checked']['original_plans_verified'],1)
                        (work/(engine+'-saved-intact.json')).write_text(json.dumps(modeled,indent=2)+'\n')
                        saved_results = []
                        for label, table, field, index, value in (
                            ('posting_float','postings','copper_value',None,float(modeled['postings'][0]['copper_value'])),
                            ('reference_uid_float','item_references','uid',None,81.0),
                            ('ledger_uid_float','ownership_events','uid',None,81.0),
                            ('ledger_root_float','ownership_events','root',None,81.0),
                            ('owner_type_float','ownership_events','owner',0,1.0),
                            ('owner_type_bool','ownership_events','owner',0,True),
                            ('owner_id_float','ownership_events','owner',1,7.0),
                            ('owner_context_float','ownership_events','owner',2,0.0),
                            ('owner_context_bool','ownership_events','owner',2,False),
                            ('nullable_parent_zero','ownership_events','parent',None,0),
                            ('nullable_parent_float','ownership_events','parent',None,0.0),
                            ('nullable_parent_bool','ownership_events','parent',None,False)):
                            damaged = copy.deepcopy(modeled)
                            if index is None: damaged[table][0][field] = value
                            else: damaged[table][0][field][index] = value
                            expected = {'postings':'original_plan_posting_mismatch',
                                'item_references':'original_plan_item_mismatch'}.get(table,'original_plan_custody_mismatch')
                            path = work/(engine+'-saved-'+label+'.json')
                            path.write_text(json.dumps(damaged,indent=2)+'\n')
                            original_bytes = path.read_bytes()
                            for limit in (0,1,100):
                                report = Reconciler(limit).audit(damaged)
                                self.assertEqual(report['exception_counts'].get(expected),1,report)
                                self.assertEqual(report['checked']['original_plans_verified'],0)
                                for name, filters in (('exceptions',[]),('operation',['--operation-id',operation.hex()]),
                                                       ('provenance',['--uid','81'])):
                                    args = {'operation_id':operation.hex()} if name == 'operation' else {'uid':81} if name == 'provenance' else {}
                                    output = view(damaged,report,name,limit,**args)
                                    self.assertGreater(output.get('exception_count',output.get('coverage',{}).get('exception_count',0)),0)
                                    self.assertNotIn(encoded.hex(),json.dumps(output))
                                    command = [sys.executable,str(ROOT/'scripts/reconcile_economy_accounting.py'),
                                        str(path),'--view',name,'--limit',str(limit),*filters]
                                    ran = subprocess.run(command,capture_output=True,text=True)
                                    self.assertEqual((ran.returncode,ran.stderr),(1,''),ran.stdout+ran.stderr)
                                    cli_output = json.loads(ran.stdout)
                                    self.assertGreater(cli_output.get('exception_count',cli_output.get('coverage',{}).get('exception_count',0)),0)
                                    self.assertNotIn(encoded.hex(),ran.stdout)
                                    self.assertNotIn('private-alias',ran.stdout)
                                    saved_results.append({'label':label,'expected':expected,'limit':limit,'view':name,
                                        'command':command,'exit':ran.returncode,'report':cli_output,'read_only':True})
                            self.assertEqual(path.read_bytes(),original_bytes)
                            self.assertEqual(damaged,json.loads(original_bytes))
                        self.assertEqual(before,inventory())
                        (work/(engine+'-saved-results.json')).write_text(json.dumps(saved_results,indent=2)+'\n')
                        self.assertEqual(len(saved_results),108)
                        position_results = []
                        for collection, positions in (('native',modeled['native']['items']),
                                                      ('item_origins',modeled['item_origins'])):
                            for field,index in (('uid',None),('root',None),('parent',None),
                                                ('owner',0),('owner',1),('owner',2)):
                                original = positions[0][field] if index is None else positions[0][field][index]
                                aliases = ([0,0.0,False] if original is None else
                                    [float(original)] + ([bool(original)] if original in (0,1) else []))
                                for value in aliases:
                                    damaged = copy.deepcopy(modeled)
                                    target = damaged['native']['items'] if collection == 'native' else damaged['item_origins']
                                    if index is None: target[0][field] = value
                                    else: target[0][field][index] = value
                                    target[0]['personal_alias'] = 'private-position-alias'
                                    label = collection+'-'+field+'-'+str(index)+'-'+type(value).__name__
                                    path = work/(engine+'-position-'+label+'.json')
                                    path.write_text(json.dumps(damaged,indent=2)+'\n')
                                    original_bytes = path.read_bytes()
                                    for limit in (0,1,100):
                                        with self.assertRaisesRegex(SnapshotError,'invalid item position'):
                                            Reconciler(limit).audit(damaged)
                                        for name,filters in (
                                                ('exceptions',[]),('holdings',[]),('supply',[]),('prices',[]),
                                                ('routes',[]),('provenance',['--uid','81']),
                                                ('operation',['--operation-id',operation.hex()])):
                                            command = [sys.executable,str(ROOT/'scripts/reconcile_economy_accounting.py'),
                                                str(path),'--view',name,'--limit',str(limit),*filters]
                                            ran = subprocess.run(command,capture_output=True,text=True,timeout=30)
                                            self.assertEqual((ran.returncode,ran.stdout,ran.stderr),
                                                (2,'','reconciliation failed: invalid item position\n'))
                                            position_results.append(dict(label=label,limit=limit,view=name,
                                                command=command,exit=ran.returncode,read_only=True))
                                    self.assertEqual(path.read_bytes(),original_bytes)
                                    self.assertEqual(damaged,json.loads(original_bytes))
                        self.assertEqual(before,inventory())
                        (work/(engine+'-position-results.json')).write_text(json.dumps(position_results,indent=2)+'\n')
                        constraints = []
                        for table, field, where, value in (
                            ('economic_accounting_coin_posting','event_index','line_index=0',9),
                            ('economic_accounting_item_reference','line_index','event_index=0',9),
                            ('economic_accounting_operation','canonical_intent','1=1',b'')):
                            before = inventory()
                            with owner.cursor() as cursor:
                                with self.assertRaises(pymysql.MySQLError) as denied:
                                    cursor.execute('UPDATE '+table+' SET '+field+'=%s WHERE operation_id=%s AND '+where,(value,operation))
                                self.assertIn(denied.exception.args[0], (4025,3819))
                            self.assertEqual(before,inventory())
                            constraints.append({'table': table,'field': field,'error': denied.exception.args[0],
                                                'schema_check_enforced': True,'unchanged': True})
                        (work/(engine+'-constraints.json')).write_text(json.dumps(constraints,indent=2)+'\n')
                        first = child(operation.hex(),1,domain=475)
                        second = child(first['child_operation_id'],2,1,2**32-1,2**64-1)
                        mutations = [('coherent_children','economic_accounting_child',
                            'domain_id=%s,child_operation_id=%s','child_index=1',(475,bytes.fromhex(first['child_operation_id'])),
                            (474,bytes.fromhex(native['children'][0]['child_operation_id'])),'canonical_child'),
                            ('posting_child','economic_accounting_coin_posting','child_index=%s','line_index=0',(2,),(1,),'canonical_posting'),
                            ('item_child','economic_accounting_item_reference','child_index=%s','event_index=0',(2,),(1,),'canonical_item'),
                            ('custody_owner','item_ownership_ledger','from_owner_id=%s','event_index=0',(9,),(native_before[2],),'canonical_custody'),
                            ('root_actor','economic_accounting_operation','actor_id=%s','1=1',(9,),(7,),'metadata'),
                            ('plan_digest','economic_accounting_operation','plan_digest=%s','1=1',(bytes(32),),(plan['plan_digest'],),'plan'),
                            ('corrupt_intent','economic_accounting_operation','canonical_intent=%s','1=1',(b'\0'+frozen[1:],),(frozen,),'intent')]
                        for label, table, assignment, where, damage, repair, code in mutations:
                            identity = legacy if table == 'item_ownership_ledger' else operation
                            query = 'UPDATE '+table+' SET '+assignment+' WHERE operation_id=%s AND '+where
                            before = inventory()
                            with owner.cursor() as cursor:
                                cursor.execute(query,(*damage,identity))
                                if label == 'coherent_children':
                                    cursor.execute('UPDATE economic_accounting_child SET child_operation_id=%s WHERE operation_id=%s AND child_index=2',
                                        (bytes.fromhex(second['child_operation_id']), operation))
                            try:
                                # Both independent readers now refuse these original-plan cuts.
                                if label in ('coherent_children','posting_child','item_child'):
                                    with owner.cursor() as cursor:
                                        cut = exporter.read_evidence(cursor,encoded[8:24],encoded[24:40],True)
                                    modeled = clean_snapshot()
                                    origins, items = modeled['account_origins'], modeled['item_origins']
                                    items[0]['owner'] = [native_before[0], native_before[2], native_before[3]]
                                    modeled.update(cut)
                                    modeled['account_origins'] = origins
                                    modeled['item_origins'] = items
                                    (work/(engine+'-'+label+'-projection.json')).write_text(json.dumps(modeled,indent=2)+'\n')
                                    projection_report = Reconciler().audit(modeled)
                                    (work/(engine+'-'+label+'-projection-report.json')).write_text(json.dumps(projection_report,indent=2)+'\n')
                                    expected = {'coherent_children': 'original_plan_child_mismatch',
                                                'posting_child': 'original_plan_posting_mismatch',
                                                'item_child': 'original_plan_item_mismatch'}[label]
                                    self.assertEqual(projection_report['exception_counts'], {expected: 1}, projection_report)
                                check(label,'restore_economic_'+code+'_mismatch')
                            finally:
                                with owner.cursor() as cursor:
                                    cursor.execute(query,(*repair,identity))
                                    if label == 'coherent_children':
                                        cursor.execute('UPDATE economic_accounting_child SET child_operation_id=%s WHERE operation_id=%s AND child_index=2',
                                            (bytes.fromhex(native['children'][1]['child_operation_id']),operation))
                            self.assertEqual(before,inventory())
                            check(label+'-repaired')
                        before = inventory()
                        claim = dict(lineage=encoded[8:24],source_event=plan['metadata'][-1],
                                     operation_id=operation,outcome=1) if plan['metadata'][-1] is not None else None
                        with owner.cursor() as cursor:
                            if claim:
                                cursor.execute('DELETE FROM economic_accounting_source_claim WHERE operation_id=%s',(operation,))
                            cursor.execute('UPDATE economic_accounting_operation SET outcome=2,result_code=9,canonical_plan=NULL,plan_digest=NULL,'
                                'account_count=0,posting_count=0,child_count=0,before_witness_count=0,after_witness_count=0,item_event_count=0 WHERE operation_id=%s', (operation,))
                        try:
                            check('rejected_details','orphan_or_rejected_detail')
                        finally:
                            with owner.cursor() as cursor:
                                cursor.execute('UPDATE economic_accounting_operation SET outcome=1,result_code=0,canonical_plan=%s,plan_digest=%s,'
                                    'account_count=2,posting_count=2,child_count=2,before_witness_count=1,after_witness_count=1,item_event_count=1 WHERE operation_id=%s',
                                    (encoded,plan['plan_digest'],operation))
                            if claim:
                                insert('economic_accounting_source_claim',claim)
                        self.assertEqual(before,inventory())
                        check('rejected_details-repaired')
                        if claim:
                            with owner.cursor() as cursor:
                                cursor.execute('DELETE FROM economic_accounting_source_claim WHERE operation_id=%s',(operation,))
                            try:
                                check('missing_source_claim','source_claim')
                            finally:
                                insert('economic_accounting_source_claim',claim)
                            for label, field, damage in (
                                ('foreign_source_claim','lineage',bytes([0x99])*16),
                                ('changed_source_claim','source_event',bytes([0x99])*48),
                                ('orphan_source_claim','operation_id',bytes([0x99])*16)):
                                before = inventory()
                                with owner.cursor() as cursor:
                                    with self.assertRaises(pymysql.MySQLError) as denied:
                                        cursor.execute('UPDATE economic_accounting_source_claim SET '+field+'=%s', (damage,))
                                    self.assertEqual(denied.exception.args[0],1452)
                                    self.assertEqual(before,inventory())
                                    # Model corrupt evidence imported with FK checks disabled.
                                    # Normal writes above must still enforce the native schema.
                                    cursor.execute('SET SESSION FOREIGN_KEY_CHECKS=0')
                                    try:
                                        cursor.execute('UPDATE economic_accounting_source_claim SET '+field+'=%s',(damage,))
                                    finally:
                                        cursor.execute('SET SESSION FOREIGN_KEY_CHECKS=1')
                                try:
                                    check(label,'source_claim')
                                finally:
                                    with owner.cursor() as cursor:
                                        cursor.execute('UPDATE economic_accounting_source_claim SET '+field+'=%s',(claim[field],))
                                self.assertEqual(before,inventory())
                            check('source_claim_restored')
                        orphan = bytes([0x77])*16
                        with owner.cursor() as cursor:
                            cursor.execute('SET SESSION FOREIGN_KEY_CHECKS=0')
                            try:
                                cursor.execute('INSERT INTO economic_accounting_account_effect SELECT %s,account_index,account_key,'
                                    'before_copper,before_silver,before_gold,before_platinum,after_copper,after_silver,after_gold,after_platinum,'
                                    'before_revision,after_revision FROM economic_accounting_account_effect WHERE operation_id=%s AND account_index=0',
                                    (orphan,operation))
                            finally:
                                cursor.execute('SET SESSION FOREIGN_KEY_CHECKS=1')
                        try:
                            check('orphan_detail','orphan_or_rejected_detail')
                        finally:
                            with owner.cursor() as cursor:
                                cursor.execute('DELETE FROM economic_accounting_account_effect WHERE operation_id=%s',(orphan,))
                        self.assertEqual(before,inventory())
                        check('orphan_detail-repaired')
                    finally:
                        reader.close()
                finally:
                    owner.close()
        (work/'results.json').write_text(json.dumps(results,indent=2)+'\n')
        self.assertEqual(len(results),88 if plan['metadata'][-1] is not None else 78)


if __name__ == '__main__':
    unittest.main()
