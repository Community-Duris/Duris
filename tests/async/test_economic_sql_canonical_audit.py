#!/usr/bin/env python3
"""Original-capsule SQL audit: bounded reader and native/private-engine faults."""

import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import time
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT/'scripts'))
sys.path.insert(0, str(ROOT/'tests/async'))
import economic_sql_canonical_audit as audit
from test_plan5_child_identity import NATIVE_PROBE, child
from test_reconcile_economy_accounting import clean_snapshot, Reconciler


class CanonicalAuditTests(unittest.TestCase):
    def connection(self):
        connection = mock.Mock()
        cursor = connection.cursor.return_value
        cursor.fetchmany.side_effect = [[{'count': 14}], [], [{'count': 0}], [], [{'size': 0}], [], [{'unwanted': 0}], []]
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
        cursor.fetchmany.side_effect = [[{'count': 14}], [], [{'count': audit.MAX_ROWS+1}], []]
        with mock.patch.object(audit, 'require_integrity') as verifier:
            with self.assertRaisesRegex(audit.AuditError, 'root count'):
                audit.capture(connection)
            verifier.assert_not_called()
        connection.rollback.assert_called_once_with()
        cursor.close.assert_called_once_with()

    def test_missing_or_nontransactional_source_refuses(self):
        connection, cursor = self.connection()
        cursor.fetchmany.side_effect = [[{'count': 13}], []]
        with mock.patch.object(audit, 'require_integrity') as verifier:
            with self.assertRaisesRegex(audit.AuditError, 'not InnoDB'):
                audit.capture(connection)
            verifier.assert_not_called()
        connection.rollback.assert_called_once_with()
        cursor.close.assert_called_once_with()

    def test_capsule_budget_refuses_before_decoding(self):
        connection, cursor = self.connection()
        cursor.fetchmany.side_effect = [[{'count': 14}], [], [{'count': 1}], [],
                                       [{'size': audit.MAX_INPUT_BYTES+1}], []]
        with mock.patch.object(audit, 'require_integrity') as verifier:
            with self.assertRaisesRegex(audit.AuditError, 'capsules exceed'):
                audit.capture(connection)
            verifier.assert_not_called()
        connection.rollback.assert_called_once_with()
        cursor.close.assert_called_once_with()

    def test_orphan_and_rejected_details_refuse_before_decoding(self):
        connection, cursor = self.connection()
        cursor.fetchmany.side_effect = [[{'count': 14}], [], [{'count': 0}], [], [{'size': 0}], [], [{'unwanted': 1}], []]
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
        source.write_text(native_source)
        builds, native = [], None
        sources = [str(source), 'src/economy/economic_accounting_types.c',
                   'src/economy/economic_accounting_plan.c', 'src/economy/economic_source_event.c',
                   'src/economy/economic_accounting_intent.c', 'src/persistence/critical_command.c',
                   'src/item/item_transfer_command.c', 'src/world/quest_mobile_native_reference.c',
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
                self.assertEqual(sql(env, 'SELECT sequence_number FROM mud_schema_history ORDER BY sequence_number DESC LIMIT 1'), '56')
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
                        compiler_version=1, actor_kind=1, actor_id=7, reason=1, source_event=None,
                        intent_digest=plan['intent_digest'], domain_digest=plan['domain_digest'],
                        plan_digest=plan['plan_digest'], canonical_intent=frozen, canonical_plan=encoded,
                        outcome=1, result_code=0)
                    fields.update(zip(('account_count','posting_count','child_count','before_witness_count',
                                       'after_witness_count','item_event_count'), plan['counts']))
                    insert('economic_accounting_operation', fields)
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
                    insert('item_ownership_ledger', dict(operation_id=legacy, event_index=0, item_uid=81,
                        root_item_uid=81, parent_item_uid=None, from_owner_type=1, from_owner_id=8,
                        from_owner_context_id=0, to_owner_type=1, to_owner_id=7, to_owner_context_id=0,
                        item_revision=2, from_owner_revision=1, to_owner_revision=2, reason_type=8,
                        source_site=1, from_equipment_slot=0, to_equipment_slot=0))
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
                                    'economic_accounting_item_reference','item_ownership_ledger'):
                                    cursor.execute('SELECT * FROM '+table+' ORDER BY 1,2')
                                    result[table] = cursor.fetchall()
                                return result
                        def check(label, code=None):
                            before = inventory()
                            wrapped = mock.Mock(wraps=reader)
                            cursor = mock.Mock(wraps=reader.cursor())
                            wrapped.cursor.return_value = cursor
                            if code:
                                with self.assertRaisesRegex(audit.AuditError, code): audit.capture(wrapped)
                            else:
                                report = audit.capture(wrapped)
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
                            ('custody_owner','item_ownership_ledger','from_owner_id=%s','event_index=0',(9,),(8,),'canonical_custody'),
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
                                    items[0]['owner'] = [1,8,0]
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
                        with owner.cursor() as cursor:
                            cursor.execute('UPDATE economic_accounting_operation SET outcome=2,result_code=9,canonical_plan=NULL,plan_digest=NULL,'
                                'account_count=0,posting_count=0,child_count=0,before_witness_count=0,after_witness_count=0,item_event_count=0 WHERE operation_id=%s', (operation,))
                        try:
                            check('rejected_details','orphan_or_rejected_detail')
                        finally:
                            with owner.cursor() as cursor:
                                cursor.execute('UPDATE economic_accounting_operation SET outcome=1,result_code=0,canonical_plan=%s,plan_digest=%s,'
                                    'account_count=2,posting_count=2,child_count=2,before_witness_count=1,after_witness_count=1,item_event_count=1 WHERE operation_id=%s',
                                    (encoded,plan['plan_digest'],operation))
                        self.assertEqual(before,inventory())
                        check('rejected_details-repaired')
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
        self.assertEqual(len(results),38)


if __name__ == '__main__':
    unittest.main()
