#!/usr/bin/env python3
"""Exercise actual accounting DDL on an explicitly disposable initialized schema.

The opaque blobs here test SQL storage constraints only. Pure codec tests and
future typed repository journeys establish their semantic contents separately.
Never sources checkout configuration or creates a connection without test guards.
"""
import importlib.util
import os
from pathlib import Path
import re
import subprocess
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('accounting_schema', ROOT/'migrations/verify_economy_accounting_schema.py')
schema = importlib.util.module_from_spec(spec)
spec.loader.exec_module(schema)

def binary(value, width=16):
    return "UNHEX('" + value.to_bytes(width, 'big').hex() + "')"

LINEAGE = binary(100)
EPOCH = binary(200)
SOURCE = binary(300,48)

class AccountingSchemaTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if (os.environ.get('ECONOMIC_ACCOUNTING_DISPOSABLE_SCHEMA') != '1' or
            os.environ.get('DB_HOST') != '127.0.0.1' or os.environ.get('DB_SOCKET') or
            not re.fullmatch(r'economic_schema_test_[A-Za-z0-9_]+', os.environ.get('DB_NAME',''))):
            raise RuntimeError('explicit disposable loopback schema is required')
        cls.client = schema.Client()
        cls.engine, cls.expected = schema.fingerprint(cls.client)
        if cls.expected != schema.EXPECTED[cls.engine]:
            raise RuntimeError('starting schema does not match the reviewed contract')
        # A transaction in each test keeps synthetic rows independent. mysql CLI
        # statements below are sent as a single session, not separate autocommits.

    def execute(self, sql, failure=None):
        result = subprocess.run(self.client.args, input=sql, text=True,
                                capture_output=True, env=self.client.env)
        if failure is None:
            self.assertEqual(result.returncode, 0, result.stderr)
            return result.stdout.strip().splitlines()
        self.assertNotEqual(result.returncode, 0, 'invalid SQL fixture was accepted')
        self.assertRegex(result.stderr, r'ERROR (?:' + failure + r') \(')
        return []

    def inbox(self, value):
        return ("INSERT INTO critical_operation_inbox(operation_id,command_hash,keys_hash,command_type,"
                "schema_version,payload_version,status,result_code,durable_revision,result_payload) VALUES("+
                binary(value)+",REPEAT(CHAR(1),32),REPEAT(CHAR(2),32),1,2,1,1,0,0,'');")

    def setup_epoch(self):
        return ("START TRANSACTION;" + self.inbox(1) +
                "INSERT INTO economic_epoch(lineage,epoch,ordinal,transition_kind,transition_digest,creating_operation_id) VALUES("+
                LINEAGE+","+EPOCH+",1,1,REPEAT(CHAR(1),32),"+binary(1)+");"+
                "INSERT INTO economic_lineage_state(lineage,active_epoch) VALUES("+LINEAGE+","+EPOCH+");")

    def operation(self, value, source=SOURCE, outcome=1, overrides=None):
        fields = dict(operation_id=binary(value),lineage=LINEAGE,epoch=EPOCH,
                      accounting_version='1',writer_id='1',policy_version='1',compiler_version='1',
                      actor_kind='1',actor_id='1',reason='1',source_event=source,
                      intent_digest='REPEAT(CHAR(1),32)',domain_digest='REPEAT(CHAR(2),32)',
                      plan_digest='REPEAT(CHAR(3),32)' if outcome==1 else 'NULL',
                      canonical_intent='REPEAT(CHAR(1),256)',
                      canonical_plan='REPEAT(CHAR(2),256)' if outcome==1 else 'NULL',
                      outcome=str(outcome),result_code='0' if outcome==1 else '1',
                      account_count='0',posting_count='0',child_count='0',item_event_count='0',
                      before_witness_count='0',after_witness_count='0')
        fields.update(overrides or {})
        return self.inbox(value)+"INSERT INTO economic_accounting_operation("+','.join(fields)+") VALUES("+','.join(fields.values())+");"

    def claim(self, value, lineage=LINEAGE, source=SOURCE, outcome=1):
        return "INSERT INTO economic_accounting_source_claim VALUES("+lineage+","+source+","+binary(value)+","+str(outcome)+");"

    def test_disposable_guard_rejects_socket_before_client_creation(self):
        environment = dict(ECONOMIC_ACCOUNTING_DISPOSABLE_SCHEMA='1',
                           DB_HOST='127.0.0.1', DB_NAME='economic_schema_test_guard',
                           DB_SOCKET='/tmp/unrelated-database.sock')
        with mock.patch.dict(os.environ, environment, clear=True), mock.patch.object(schema, 'Client') as client:
            with self.assertRaisesRegex(RuntimeError, 'disposable loopback'):
                AccountingSchemaTest.setUpClass()
            client.assert_not_called()

    def test_new_schema_does_not_activate_or_seed_values(self):
        for table in schema.TABLES:
            self.assertEqual(self.execute('SELECT COUNT(*) FROM '+table+';'), ['0'])

    def test_complete_transaction_and_rollback(self):
        prefix=self.setup_epoch()+self.operation(2)+self.claim(2)
        self.assertEqual(self.execute(prefix+"SELECT COUNT(*) FROM economic_accounting_source_claim; ROLLBACK; SELECT COUNT(*) FROM economic_accounting_operation;"),['1','0'])

    def test_source_uniqueness_and_matching_successful_operation(self):
        base=self.setup_epoch()+self.operation(2)
        for suffix in (self.claim(2,lineage=binary(101)),self.claim(2,source=binary(301,48))):
            self.execute(base+suffix,failure='1452')
        self.execute(self.setup_epoch()+self.operation(2,source='NULL')+self.claim(2),failure='1452')
        self.execute(self.setup_epoch()+self.operation(2,outcome=2)+self.claim(2),failure='1452')
        self.execute(base+self.claim(2,outcome=2),failure='3819|4025|1452')
        self.execute(base+self.claim(2)+self.operation(3)+self.claim(3),failure='1062')
        # Rejected evidence does not consume a source; a later successful root can.
        self.assertEqual(self.execute(self.setup_epoch()+self.operation(2,outcome=2)+self.operation(3)+self.claim(3)+"SELECT COUNT(*) FROM economic_accounting_source_claim; ROLLBACK;"),['1'])

    def test_rejected_operation_has_no_realized_plan_or_counts(self):
        for changed in ({'canonical_plan':'REPEAT(CHAR(2),256)'},{'account_count':'1'},
                        {'plan_digest':'REPEAT(CHAR(3),32)'},{'result_code':'0'}):
            self.execute(self.setup_epoch()+self.operation(2,outcome=2,overrides=changed),failure='3819|4025')

    def test_operation_caps_and_unknown_versions(self):
        for field,value in (('account_count','3073'),('posting_count','6145'),('child_count','65'),
                            ('item_event_count','3001'),('before_witness_count','6001'),
                            ('after_witness_count','6001'),('accounting_version','2'),
                            ('canonical_plan','REPEAT(CHAR(2),4194305)'),('canonical_intent',"''")):
            self.execute(self.setup_epoch()+self.operation(2,overrides={field:value}),failure='3819|4025')
        maximum=dict(account_count='3072',posting_count='6144',child_count='64',item_event_count='3000',
                     before_witness_count='6000',after_witness_count='6000',canonical_plan='REPEAT(CHAR(2),4194304)',canonical_intent='REPEAT(CHAR(1),8192)')
        self.execute(self.setup_epoch()+self.operation(2,overrides=maximum)+'ROLLBACK;')

    def test_retired_native_identity_gets_new_lifetime(self):
        def mapping():
            return "INSERT INTO economic_account_mapping(lineage,account_kind,context_id,backend_kind,locator_kind,native_id,active_native_id,creating_operation_id) VALUES("+LINEAGE+",1,0,1,1,7,7,"+binary(1)+");"
        self.execute(self.setup_epoch()+mapping()+mapping(),failure='1062')
        sql=(self.setup_epoch()+mapping()+"SET @first_mapping=LAST_INSERT_ID();"+
             "UPDATE economic_account_mapping SET active_native_id=NULL,retiring_operation_id="+binary(1)+",revision=revision+1 WHERE mapping_id=@first_mapping;"+
             mapping()+"SELECT LAST_INSERT_ID()>@first_mapping; SELECT COUNT(*) FROM economic_account_mapping; ROLLBACK;")
        self.assertEqual(self.execute(sql),['1','2'])
        self.execute(self.setup_epoch()+mapping()+"UPDATE economic_account_mapping SET active_native_id=8;",failure='3819|4025')
        # Data changes and allocator advancement do not change schema verification.
        self.assertEqual(schema.fingerprint(self.client),(self.engine,self.expected))

    def test_posting_keys_bounds_and_missing_accounts(self):
        base=self.setup_epoch()+self.operation(2)
        effect="INSERT INTO economic_accounting_account_effect VALUES("+binary(2)+",0,REPEAT(CHAR(1),40),0,0,0,0,1,0,0,0,0,1);"
        posting="INSERT INTO economic_accounting_coin_posting VALUES("+binary(2)+",0,0,0,0,1,0,0,0,1);"
        self.execute(base+posting,failure='1452')
        self.execute(base+effect+posting+posting,failure='1062')
        self.execute(base+effect+posting.replace(',0,0,0,0,1,',',0,1,0,0,1,'),failure='3819|4025')
        self.execute(base+effect+posting+'ROLLBACK;')

    def test_item_reference_matches_original_uid_and_revision(self):
        base=self.setup_epoch()+self.operation(2)+self.inbox(3)
        ledger=("INSERT INTO item_ownership_ledger(operation_id,event_index,item_uid,root_item_uid,from_owner_type,from_owner_id,from_owner_context_id,to_owner_type,to_owner_id,to_owner_context_id,item_revision,from_owner_revision,to_owner_revision,reason_type,source_site) VALUES("+binary(3)+",0,777,777,1,7,0,1,8,0,1,0,1,1,1);")
        def reference(uid=777,after=1,before=0):
            return "INSERT INTO economic_accounting_item_reference VALUES("+binary(2)+",0,0,0,"+str(uid)+","+str(before)+","+str(after)+","+binary(3)+",0);"
        self.execute(base+ledger+reference(uid=778),failure='1452')
        self.execute(base+ledger+reference(after=2),failure='1452')
        self.execute(base+ledger+reference(before=1),failure='3819|4025')
        self.execute(base+ledger+reference()+reference(),failure='1062')
        self.execute(base+ledger+reference()+'ROLLBACK;')

    def test_metadata_damage_is_detected(self):
        changes=[
            ('ALTER TABLE economic_accounting_account_effect MODIFY before_copper INT NOT NULL;',
             'ALTER TABLE economic_accounting_account_effect MODIFY before_copper BIGINT NOT NULL;'),
            ('ALTER TABLE economic_accounting_coin_posting DROP INDEX uq_economic_posting_event;',
             'ALTER TABLE economic_accounting_coin_posting ADD UNIQUE KEY uq_economic_posting_event(operation_id,event_index);'),
            ('ALTER TABLE economic_accounting_account_effect DROP CONSTRAINT chk_economic_effect_index;',
             'ALTER TABLE economic_accounting_account_effect ADD CONSTRAINT chk_economic_effect_index CHECK(account_index < 3072);'),
        ]
        if self.engine == 'mysql8':
            changes.append((
                'ALTER TABLE economic_accounting_account_effect ALTER CHECK chk_economic_effect_index NOT ENFORCED;',
                'ALTER TABLE economic_accounting_account_effect ALTER CHECK chk_economic_effect_index ENFORCED;',
            ))
        sealed = ['bash', str(ROOT/'migrations/immutable/0031_economy_accounting.sh')]
        for damage,repair in changes:
            try:
                self.execute(damage)
                self.assertNotEqual(schema.fingerprint(self.client),(self.engine,self.expected))
                result = subprocess.run(sealed, capture_output=True, text=True, env=self.client.env)
                self.assertNotEqual(result.returncode, 0, 'sealed verifier accepted damaged schema')
            finally:
                self.execute(repair)
            self.assertEqual(schema.fingerprint(self.client),(self.engine,self.expected))
            result = subprocess.run(sealed, capture_output=True, text=True, env=self.client.env)
            self.assertEqual(result.returncode, 0, result.stderr)

if __name__=='__main__':
    unittest.main()
