#!/usr/bin/env python3
"""Disposable-engine baseline storage constraints; opaque fixtures are not native proofs."""
from pathlib import Path
import select
import subprocess
import sys
import tempfile
from exact_item_runtime_probe import build_probe
import unittest

import test_economic_accounting_schema_mysql as fixtures

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'migrations'))
import verify_economic_baseline_schema as schema

binary = fixtures.binary
LINEAGE, EPOCH = fixtures.LINEAGE, fixtures.EPOCH

class BaselineSchemaTest(unittest.TestCase):
    execute = fixtures.AccountingSchemaTest.execute
    inbox = fixtures.AccountingSchemaTest.inbox
    setup_epoch = fixtures.AccountingSchemaTest.setup_epoch
    operation = fixtures.AccountingSchemaTest.operation

    @classmethod
    def setUpClass(cls):
        fixtures.AccountingSchemaTest.setUpClass.__func__(cls)
        cls.engine, cls.expected = schema.fingerprint(cls.client)
        if cls.expected != schema.EXPECTED[cls.engine]:
            raise RuntimeError('baseline storage metadata differs from the reviewed contract')
        cls.probe_directory = tempfile.TemporaryDirectory(prefix='baseline-runtime-probe-')
        cls.addClassCleanup(cls.probe_directory.cleanup)
        cls.probe_binary = build_probe(cls.probe_directory.name)
        for mode in ('session', 'metadata'):
            subprocess.run([str(cls.probe_binary), mode], env=cls.client.env,
                           check=True, capture_output=True, timeout=30)

    def assert_metadata_rejected(self):
        self.assertNotEqual(schema.fingerprint(self.client), (self.engine,self.expected))
        for verifier in (ROOT/'migrations/immutable/0032_economic_baseline.sh',
                         ROOT/'migrations/verify_runtime_compatibility.sh'):
            result = subprocess.run(['bash',str(verifier)],env=self.client.env,
                                    capture_output=True,text=True,timeout=30)
            self.assertNotEqual(result.returncode,0)
            self.assertIn('metadata fingerprint mismatch',result.stderr)
        native = subprocess.run([str(self.probe_binary),'metadata'],env=self.client.env,
                                capture_output=True,text=True,timeout=30)
        self.assertEqual(native.returncode,1,native.stderr)


    def control(self, epoch=EPOCH):
        return ('INSERT INTO economic_baseline_control(lineage,epoch,opening_account,creating_operation_id) VALUES('
                + LINEAGE + ',' + epoch + ',REPEAT(CHAR(1),40),' + binary(1) + ');')

    def batch(self, value=2, revision=1, epoch=EPOCH, **overrides):
        fields = dict(operation_id=binary(value), lineage=LINEAGE, epoch=epoch,
                      book_revision=str(revision), witness_version='1', holding_count='0', item_count='0',
                      witness_digest='REPEAT(CHAR(1),32)',
                      canonical_witness="CONCAT('EAB1',REPEAT(CHAR(0),188))")
        fields.update(overrides)
        return (self.operation(value, overrides={'epoch': epoch}) +
                'INSERT INTO economic_baseline_witness(' + ','.join(fields) + ') VALUES(' + ','.join(fields.values()) + ');')

    def reserve(self, value=2, kind=1, identity=777, epoch=EPOCH):
        return ('INSERT INTO economic_baseline_reservation VALUES(' + LINEAGE + ',' + epoch + ',' +
                str(kind) + ',' + str(identity) + ',' + binary(value) + ');')

    def prefix(self):
        return self.setup_epoch() + self.control()

    def test_empty_initial_state_and_no_activation(self):
        for table in schema.TABLES:
            self.assertEqual(self.execute('SELECT COUNT(*) FROM ' + table), ['0'])
        # Opening metadata does not change the explicitly inactive lineage.
        sql = self.setup_epoch() + 'UPDATE economic_lineage_state SET active_epoch=NULL WHERE lineage=' + LINEAGE + ';' + self.control() + self.batch()
        self.assertEqual(self.execute(sql + 'SELECT active_epoch IS NULL FROM economic_lineage_state WHERE lineage=' + LINEAGE + ';ROLLBACK;'), ['1'])

    def test_atomic_batch_and_rollback(self):
        sql = (self.prefix() + self.batch() + self.reserve() + self.reserve(kind=2) +
               'UPDATE economic_baseline_control SET revision=1,last_operation_id=' + binary(2) + ';' +
               'SELECT COUNT(*) FROM economic_baseline_reservation;ROLLBACK;')
        self.assertEqual(self.execute(sql + 'SELECT COUNT(*) FROM economic_baseline_control;'), ['2', '0'])

    def test_one_control_per_epoch_and_retained_parents(self):
        self.execute(self.prefix() + self.control(), failure='1062')
        self.execute(self.setup_epoch() + self.control(epoch=binary(999)), failure='1452')
        self.execute(self.prefix() + 'DELETE FROM economic_epoch WHERE lineage=' + LINEAGE + ';', failure='1451')
        self.execute(self.prefix() + self.batch() + 'DELETE FROM economic_baseline_control;', failure='1451')
        self.execute(self.prefix() + self.batch() + self.reserve() + 'DELETE FROM economic_baseline_witness;', failure='1451')
        self.execute(self.prefix() + self.batch() + 'DELETE FROM economic_accounting_operation WHERE operation_id=' + binary(2) + ';', failure='1451')

    def test_control_revision_shape(self):
        for assignment in ('revision=1', 'last_operation_id=' + binary(1), "opening_account=''", "opening_account=REPEAT(CHAR(1),39)"):
            self.execute(self.prefix() + 'UPDATE economic_baseline_control SET ' + assignment + ';', failure='3819|4025')
        self.execute(self.prefix() + 'UPDATE economic_baseline_control SET revision=18446744073709551615,last_operation_id=' + binary(1) + ';ROLLBACK;')

    def test_batch_requires_common_receipt_and_unique_revision(self):
        self.execute(self.prefix() + self.batch().split('INSERT INTO economic_baseline_witness')[0] +
                     'DELETE FROM economic_accounting_operation WHERE operation_id=' + binary(2) + ';' + 'INSERT INTO economic_baseline_witness' +
                     self.batch().split('INSERT INTO economic_baseline_witness')[1], failure='1452')
        self.execute(self.prefix() + self.batch() + self.batch(3), failure='1062')
        self.execute(self.prefix() + self.batch(revision=0), failure='3819|4025')
        self.execute(self.prefix() + self.batch(revision=18446744073709551615) + 'ROLLBACK;')

    def test_complete_witness_bounds_and_version(self):
        for fields in ({'witness_version':'2'}, {'holding_count':'3072'}, {'item_count':'6001'},
                       {'canonical_witness':"CONCAT('EAB1',REPEAT(CHAR(0),187))"},
                       {'canonical_witness':"CONCAT('EAB1',REPEAT(CHAR(0),189))"},
                       {'canonical_witness':'REPEAT(CHAR(0),192)'}, {'holding_count':'1'}, {'item_count':'1'}):
            self.execute(self.prefix() + self.batch(**fields), failure='3819|4025')
        maximum = self.batch(holding_count='3071', item_count='6000',
                             canonical_witness="CONCAT('EAB1',REPEAT(CHAR(0),872140))")
        self.assertEqual(self.execute(self.prefix() + maximum + 'SELECT OCTET_LENGTH(canonical_witness) FROM economic_baseline_witness;ROLLBACK;'), ['872144'])

    def test_cross_batch_lifetime_duplicate_and_separate_item_namespace(self):
        base = self.prefix() + self.batch() + self.reserve() + self.batch(3,2)
        self.execute(base + self.reserve(3), failure='1062')
        self.assertEqual(self.execute(base + self.reserve(3,kind=2) +
                                     'SELECT COUNT(*) FROM economic_baseline_reservation;ROLLBACK;'), ['2'])
        for kind, identity in ((0,1),(3,1),(1,0)):
            self.execute(self.prefix()+self.batch()+self.reserve(kind=kind,identity=identity), failure='3819|4025')
        self.execute(self.prefix()+self.batch()+self.reserve(identity=18446744073709551615)+'ROLLBACK;')

    def test_reservation_must_belong_to_batch_epoch(self):
        other = binary(201)
        second_epoch = ('INSERT INTO economic_epoch(lineage,epoch,ordinal,transition_kind,transition_digest,creating_operation_id) VALUES('
                        + LINEAGE + ',' + other + ',2,1,REPEAT(CHAR(1),32),' + binary(1) + ');')
        base = self.prefix() + self.batch() + self.reserve() + second_epoch + self.control(other) + self.batch(3,1,other)
        self.execute(base + self.reserve(2,epoch=other), failure='1452')
        self.assertEqual(self.execute(base + self.reserve(3,epoch=other) +
                                     'SELECT COUNT(*) FROM economic_baseline_reservation;ROLLBACK;'), ['2'])
        self.execute(self.prefix()+self.batch()+self.reserve(3),failure='1452')

    def test_concurrent_control_initialization_is_serialized(self):
        # Two independent real sessions. The contender cannot treat uncommitted
        # initialization as absence. Closing the owner rolls back all fixture rows.
        owner = subprocess.Popen(self.client.args + ['--unbuffered'], stdin=subprocess.PIPE,
                                 stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, env=self.client.env)
        try:
            owner.stdin.write(self.prefix()+"SELECT 'baseline-owner-ready';\n")
            owner.stdin.flush()
            self.assertTrue(select.select([owner.stdout], [], [], 15)[0], 'owner did not reach locked control')
            self.assertEqual(owner.stdout.readline().strip(), 'baseline-owner-ready')
            self.execute('SET SESSION innodb_lock_wait_timeout=1;START TRANSACTION;' + self.control(), failure='1205')
        finally:
            try:
                owner.communicate('ROLLBACK;\n', timeout=15)
            except subprocess.TimeoutExpired:
                owner.kill(); owner.communicate()
        self.assertEqual(self.execute('SELECT COUNT(*) FROM economic_baseline_control;'), ['0'])

    def test_exact_metadata_detects_type_index_and_check_damage(self):
        pairs = [
            ('ALTER TABLE economic_baseline_control MODIFY revision BIGINT NOT NULL DEFAULT 0;',
             'ALTER TABLE economic_baseline_control MODIFY revision BIGINT UNSIGNED NOT NULL DEFAULT 0;'),
            ('ALTER TABLE economic_baseline_witness DROP INDEX uq_economic_baseline_witness_revision;',
             'ALTER TABLE economic_baseline_witness ADD UNIQUE KEY uq_economic_baseline_witness_revision(lineage,epoch,book_revision);'),
        ]
        for damage, restore in pairs:
            try:
                self.execute(damage)
                self.assert_metadata_rejected()
            finally:
                self.execute(restore)
            self.assertEqual(schema.fingerprint(self.client), (self.engine,self.expected))
        # Enforcement and CHECK definitions are included in exact metadata.
        if self.engine == 'mysql8':
            try:
                self.execute('ALTER TABLE economic_baseline_witness ALTER CHECK ck_economic_baseline_witness_shape NOT ENFORCED;')
                self.assert_metadata_rejected()
            finally:
                self.execute('ALTER TABLE economic_baseline_witness ALTER CHECK ck_economic_baseline_witness_shape ENFORCED;')
        else:
            try:
                self.execute('ALTER TABLE economic_baseline_witness DROP CONSTRAINT ck_economic_baseline_witness_revision;')
                self.assert_metadata_rejected()
            finally:
                self.execute('ALTER TABLE economic_baseline_witness ADD CONSTRAINT ck_economic_baseline_witness_revision CHECK (book_revision>0);')
        self.assertEqual(schema.fingerprint(self.client), (self.engine,self.expected))

if __name__ == '__main__':
    unittest.main()
