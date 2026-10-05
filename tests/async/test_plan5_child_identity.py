#!/usr/bin/env python3
"""Independent child identity faults; native/SQL execution is explicitly gated."""

import copy
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tests/async'))
from test_reconcile_economy_accounting import (OP, clean_snapshot, linked_child_snapshot,
                                               two_child_snapshot, bind_original_plans)
from reconcile_economy_accounting import Reconciler, view


def child(parent, index, parent_index=0, domain=474, discriminator=0):
    identity = hashlib.sha256(bytes.fromhex(parent) + domain.to_bytes(4, 'little') +
                              discriminator.to_bytes(8, 'little')).digest()[:16].hex()
    return {'operation_id': OP, 'child_index': index, 'parent_index': parent_index,
            'child_operation_id': identity, 'domain_id': domain,
            'discriminator': discriminator, 'relationship': 1, 'receipt_operation_id': None}


def identity_snapshot(two=False):
    snapshot = two_child_snapshot() if two else linked_child_snapshot()
    snapshot['children'][0] = child(OP, 1)
    if two:
        snapshot['children'][1] = child(snapshot['children'][0]['child_operation_id'], 2, 1,
                                        2**32 - 1, 2**64 - 1)
    return bind_original_plans(snapshot)


class ChildIdentityTests(unittest.TestCase):
    def check(self, snapshot, code, count=1):
        before = copy.deepcopy(snapshot)
        for limit in (0, 1, 100):
            report = Reconciler(limit).audit(snapshot)
            self.assertEqual(report['exception_counts'].get(code), count, report)
            self.assertEqual(snapshot, before)
            self.assertNotIn('private-alias', json.dumps(report))
            for name, args in (('exceptions', {}), ('provenance', {'uid': 81})):
                result = view(snapshot, report, name, limit, **args)
                self.assertGreater(result.get('exception_count', result.get('coverage', {}).get('exception_count', 0)), 0)

    def test_valid_chain_is_independent_of_row_order_and_limits(self):
        for two in (False, True):
            snapshot = identity_snapshot(two)
            snapshot['children'].reverse()
            before = copy.deepcopy(snapshot)
            for limit in (0, 1, 100):
                report = Reconciler(limit).audit(snapshot)
                self.assertEqual(report['exception_count'], 0, report)
                self.assertEqual(snapshot, before)
        snapshot['children'][0]['receipt_operation_id'] = snapshot['children'][0]['child_operation_id']
        self.assertEqual(Reconciler().audit(snapshot)['exception_count'], 0)

    def test_malformed_child_ids_do_not_inherit_string_validity(self):
        for value in (None, True, 1, [], {}, '', '0'*32, 'g'*32, '44'*15, '44'*17,
                      'private-alias', '44'*16+'\n', 'AA'*16, OP):
            snapshot = identity_snapshot()
            snapshot['children'][0]['child_operation_id'] = value
            with self.subTest(value=value):
                self.check(snapshot, 'invalid_child_link')

    def test_derivation_binds_original_parent_domain_and_discriminator(self):
        for field, value in (('child_operation_id', '44'*16), ('domain_id', 475),
                             ('discriminator', 1), ('parent_index', 0)):
            snapshot = identity_snapshot(two=True)
            snapshot['children'][1][field] = value
            with self.subTest(field=field):
                self.check(snapshot, 'child_identity_mismatch')

    def test_child_derivation_metadata_requires_native_types_and_ranges(self):
        for field, values in (
                ('domain_id', (None, True, 1.0, '474', 0, -1, 2**32)),
                ('discriminator', (None, True, 0.0, '0', -1, 2**64)),
                ('relationship', (None, True, 1.0, '1', 0, 2, 65536)),
                ('receipt_operation_id', (True, 1, [], {}, '', '0'*32, '66'*16, 'private-alias'))):
            for value in values:
                snapshot = identity_snapshot()
                snapshot['children'][0][field] = value
                with self.subTest(field=field, value=value):
                    self.check(snapshot, 'invalid_child_link')

    def test_missing_child_identity_evidence_is_not_a_verified_legacy_child(self):
        for field in ('domain_id', 'discriminator', 'relationship', 'receipt_operation_id'):
            snapshot = identity_snapshot()
            del snapshot['children'][0][field]
            with self.subTest(field=field):
                self.check(snapshot, 'missing_child_identity_evidence')

    def test_duplicate_child_identity_is_separate_from_child_slot(self):
        snapshot = identity_snapshot(two=True)
        snapshot['children'][1] = {**snapshot['children'][0], 'child_index': 2}
        self.check(snapshot, 'duplicate_child_operation')
        duplicate = {**snapshot['children'][0]}
        del duplicate['domain_id']
        snapshot['children'].append(duplicate)
        self.check(snapshot, 'missing_child_identity_evidence')
        self.check(snapshot, 'duplicate_child')

    def test_duplicate_identity_across_roots_is_not_hidden_by_root_filter(self):
        snapshot = identity_snapshot(two=True)
        other = '66'*16
        snapshot['operations'].append({**snapshot['operations'][0], 'operation_id': other,
                                       'account_count': 0, 'posting_count': 0, 'child_count': 1,
                                       'item_event_count': 0, 'source_event': None})
        snapshot['receipts'].append({**snapshot['receipts'][0], 'operation_id': other})
        snapshot['children'].append({**snapshot['children'][0], 'operation_id': other})
        self.check(snapshot, 'duplicate_child_operation')
        report = Reconciler().audit(snapshot)
        result = view(snapshot, report, 'operation', 100, operation_id=OP)
        self.assertGreater(result['coverage']['exception_count'], 0)

    def test_cli_mismatch_is_global_bounded_and_read_only(self):
        snapshot = identity_snapshot(two=True)
        snapshot['children'][1]['discriminator'] = 1
        with tempfile.TemporaryDirectory(prefix='child-identity-cli-') as directory:
            path = Path(directory)/'snapshot.json'
            path.write_text(json.dumps(snapshot))
            before = path.read_bytes()
            for limit in (0, 1, 100):
                for name, filters in (('exceptions', []), ('operation', ['--operation-id', OP]),
                                      ('provenance', ['--uid', '81'])):
                    command = [sys.executable, str(ROOT/'scripts/reconcile_economy_accounting.py'),
                               str(path), '--view', name, '--limit', str(limit), *filters]
                    result = subprocess.run(command, capture_output=True, text=True)
                    self.assertEqual(result.returncode, 1, result.stdout+result.stderr)
                    self.assertEqual(result.stderr, '')
                    output = json.loads(result.stdout)
                    if name == 'exceptions':
                        self.assertEqual(output['exception_counts']['child_identity_mismatch'], 1)
                        self.assertLessEqual(len(output['exceptions']), limit)
                    else:
                        self.assertGreater(output['coverage']['exception_count'], 0)
                    self.assertEqual(path.read_bytes(), before)

    def test_coherent_child_rewrite_is_bound_to_original_plan(self):
        snapshot = identity_snapshot(two=True)
        snapshot['children'][0] = child(OP, 1, domain=475)
        snapshot['children'][1] = child(snapshot['children'][0]['child_operation_id'], 2, 1,
                                        2**32 - 1, 2**64 - 1)
        self.check(snapshot, 'original_plan_child_mismatch')
        self.assertNotIn('child_identity_mismatch', Reconciler().audit(snapshot)['exception_counts'])

    def test_posting_and_item_child_associations_bind_original_plan(self):
        for name, code in (('postings', 'original_plan_posting_mismatch'),
                           ('item_references', 'original_plan_item_mismatch')):
            snapshot = identity_snapshot(two=True)
            snapshot[name][0]['child_index'] = 0
            self.check(snapshot, code)

    def test_missing_original_plan_is_unverified_for_every_backend_and_limit(self):
        for backend in ('disposable', 'sql_partial', 'flatfile', 'unknown'):
            for value in (None, 'missing'):
                snapshot = identity_snapshot()
                snapshot['backend'] = backend
                if value is None:
                    snapshot['operations'][0]['canonical_plan'] = None
                else:
                    del snapshot['operations'][0]['canonical_plan']
                self.check(snapshot, 'missing_original_plan')
                self.assertEqual(Reconciler().audit(snapshot)['checked']['original_plans_verified'], 0)

    def test_original_plan_shape_digest_metadata_and_counts_are_strict(self):
        for value in (True, 7, {}, [], '', 'EAP1', 'private-alias', '00'*256,
                      identity_snapshot()['operations'][0]['canonical_plan'].upper()):
            snapshot = identity_snapshot()
            snapshot['operations'][0]['canonical_plan'] = value
            self.check(snapshot, 'invalid_original_plan')
        for field in ('plan_digest', 'intent_digest', 'domain_digest'):
            for value in (None, True, 'aa'*32, 'private-alias'):
                snapshot = identity_snapshot()
                snapshot['operations'][0][field] = value
                self.check(snapshot, 'original_plan_digest_mismatch')
        for field in ('accounting_version', 'writer_id', 'policy_version', 'compiler_version',
                      'actor_kind', 'actor_id'):
            for value in (None, True, 1.0, 'private-alias'):
                snapshot = identity_snapshot()
                snapshot['operations'][0][field] = value
                self.check(snapshot, 'original_plan_metadata_mismatch')
        for field in ('before_witness_count', 'after_witness_count'):
            for value in (None, True, 1.0, -1, 6001, 'private-alias'):
                snapshot = identity_snapshot()
                snapshot['operations'][0][field] = value
                self.check(snapshot, 'original_plan_count_mismatch')

    def test_original_plan_account_and_custody_projections_are_exact(self):
        snapshot = identity_snapshot()
        snapshot['effects'][0]['before_revision'] = 0
        self.check(snapshot, 'original_plan_account_mismatch')
        for field, value in (('from_owner', [1,7,0]), ('from_equipment_slot', True),
                             ('from_owner', [2,8,False]), ('to_equipment_slot', 1)):
            snapshot = identity_snapshot()
            snapshot['ownership_events'][0][field] = value
            self.check(snapshot, 'original_plan_custody_mismatch')
        for name, field, code in (('postings', 'event_index', 'original_plan_posting_mismatch'),
                                  ('item_references', 'line_index', 'original_plan_item_mismatch')):
            snapshot = identity_snapshot()
            snapshot[name][0][field] = False
            self.check(snapshot, code)

    def test_original_plan_sql_source_is_bounded_before_fetch(self):
        import economic_sql_audit_snapshot as exporter
        from reconcile_economy_accounting import MAX_INPUT_BYTES, MAX_ROWS
        from economic_restore_evidence import MAX_PLAN
        for changed in ({'root_count':MAX_ROWS+1}, {'plan_bytes':MAX_INPUT_BYTES//2+1},
                         {'max_plan_bytes':MAX_PLAN+1}):
            cursor = mock.Mock()
            cursor.fetchone.return_value = {'root_count':1, 'plan_bytes':512, 'max_plan_bytes':512, **changed}
            with mock.patch.object(exporter, 'bounded') as fetch:
                with self.assertRaisesRegex(exporter.ExportError, 'original plan source exceeds audit input limit'):
                    exporter.operation_rows(cursor, bytes.fromhex('11'*16), bytes.fromhex('22'*16), True)
                fetch.assert_not_called()
            self.assertTrue(cursor.execute.call_args.args[0].startswith('SELECT '))

    def test_original_plan_cli_preserves_global_findings_and_omits_capsules(self):
        with tempfile.TemporaryDirectory(prefix='original-plan-cli-') as directory:
            path = Path(directory)/'snapshot.json'
            snapshot = identity_snapshot(two=True)
            snapshot['postings'][0]['child_index'] = 0
            snapshot['operations'][0]['personal_alias'] = 'private-original-plan-alias'
            path.write_text(json.dumps(snapshot))
            before = path.read_bytes()
            for limit in (0, 1, 100):
                for name, filters in (('exceptions', []), ('operation', ['--operation-id', '77'*16]),
                                      ('operation', ['--operation-id', OP]), ('provenance', ['--uid','81'])):
                    command = [sys.executable, str(ROOT/'scripts/reconcile_economy_accounting.py'), str(path),
                               '--view',name,'--limit',str(limit),*filters]
                    result = subprocess.run(command, capture_output=True, text=True)
                    self.assertEqual(result.returncode, 1, result.stdout+result.stderr)
                    self.assertEqual(result.stderr, '')
                    output = json.loads(result.stdout)
                    self.assertEqual(output.get('coverage', output)['exception_count'], 1)
                    self.assertNotIn('canonical_plan', result.stdout)
                    self.assertNotIn(snapshot['operations'][0]['canonical_plan'], result.stdout)
                    self.assertNotIn('private-original-plan-alias', result.stdout)
                    self.assertEqual(path.read_bytes(), before)


NATIVE_PROBE = r'''#include "economy/economic_accounting_intent.h"
#include <cassert>
#include <iostream>
#include <limits>
critical_operation_id id(uint8_t byte) { critical_operation_id value; value.bytes.fill(byte); return value; }
template<class T> std::string hex(const T &bytes) {
    std::string out; for (uint8_t value : bytes) { out.push_back("0123456789abcdef"[value>>4]); out.push_back("0123456789abcdef"[value&15]); } return out;
}
int main() {
    economic_accounting_plan plan;
    auto &meta=plan.metadata;
    meta.lineage=id(0x11); meta.epoch=id(0x22); meta.operation_id=id(0x33);
    meta.actor_kind=economic_actor_kind::domain; meta.actor_id=7; meta.writer_id=1;
    meta.reason=economic_reason::bank_transfer;
    plan.accounts={{{meta.lineage,economic_account_kind::wallet,7,0},{10,0,0,0},{7,0,0,0},1,2},
                   {{meta.lineage,economic_account_kind::bank,9,0},{},{3,0,0,0},1,2}};
    plan.postings={{0,0,1,{-3,0,0,0},-3},{1,1,2,{3,0,0,0},3}};
    plan.children.resize(2);
    plan.children[0].domain=474;
    assert(critical_operation_id_derive(meta.operation_id,474,0,&plan.children[0].operation_id));
    plan.children[1].domain=UINT32_MAX; plan.children[1].discriminator=UINT64_MAX; plan.children[1].parent_index=1;
    assert(critical_operation_id_derive(plan.children[0].operation_id,UINT32_MAX,UINT64_MAX,&plan.children[1].operation_id));
    economic_item_position old;
    old.owner={item_owner_type::player,8,0}; old.root_uid=81; old.revision=1;
    old.state=item_custody_state::active;
    auto next=old; next.owner.id=7; next.revision=2;
    plan.items_before={{81,old}}; plan.items_after={{81,next}};
    plan.item_events={{0,1,81,old,next}};
    economic_frozen_intent intent;
    intent.admission.metadata=meta; intent.command_binding[0]=21; intent.domain_digest[0]=22;
    std::vector<uint8_t> frozen,encoded;
    assert(economic_intent_encode(intent,&frozen)==economic_accounting_error::ok);
    assert(economic_intent_digest(intent,&meta.intent_digest)==economic_accounting_error::ok);
    meta.domain_digest=intent.domain_digest;
    assert(economic_plan_encode(plan,&encoded)==economic_accounting_error::ok);
    economic_accounting_plan decoded;
    assert(economic_plan_decode(encoded,&decoded)==economic_accounting_error::ok);
    assert(economic_child_links_validate(meta.operation_id,decoded.children)==economic_accounting_error::ok);
    auto changed=decoded.children; changed[0].operation_id={};
    assert(economic_child_links_validate(meta.operation_id,changed)==economic_accounting_error::invalid_identity);
    changed=decoded.children; changed[0].operation_id=meta.operation_id;
    assert(economic_child_links_validate(meta.operation_id,changed)==economic_accounting_error::invalid_identity);
    changed=decoded.children; changed[1]=changed[0];
    assert(economic_child_links_validate(meta.operation_id,changed)==economic_accounting_error::duplicate_event);
    changed=decoded.children; changed[0].discriminator=1;
    assert(economic_child_links_validate(meta.operation_id,changed)==economic_accounting_error::payload_conflict);
    changed=decoded.children; changed[0].domain=475;
    assert(economic_child_links_validate(meta.operation_id,changed)==economic_accounting_error::payload_conflict);
    changed=decoded.children; changed[1].parent_index=0;
    assert(economic_child_links_validate(meta.operation_id,changed)==economic_accounting_error::payload_conflict);
    std::cout<<"{\"native_contract_cases\":7,\"intent\":\""<<hex(frozen)<<"\",\"plan\":\""<<hex(encoded)<<"\",\"children\":[";
    for(size_t i=0;i<decoded.children.size();++i) {
        const auto &link=decoded.children[i]; if(i)std::cout<<',';
        std::cout<<"{\"operation_id\":\""<<hex(meta.operation_id.bytes)<<"\",\"child_index\":"<<i+1
                 <<",\"child_operation_id\":\""<<hex(link.operation_id.bytes)<<"\",\"domain_id\":"<<link.domain
                 <<",\"discriminator\":"<<link.discriminator<<",\"parent_index\":"<<link.parent_index
                 <<",\"relationship\":"<<link.relationship<<",\"receipt_operation_id\":null}";
    }
    std::cout<<"]}\n";
}
'''


@unittest.skipUnless(os.environ.get('DURIS_PLAN5_CHILD_IDENTITY_NATIVE') == '1',
                     'requires explicitly selected native/private SQL qualification')
class NativeChildIdentityTests(unittest.TestCase):
    def test_native_child_plan_and_sql_cuts(self):
        import struct
        import pymysql
        import migration_runner as migrations
        import persistence_restore as restore
        import economic_sql_audit_snapshot as exporter
        from economic_restore_evidence import decode_plan
        from test_persistence_backup_integration import sql

        work = Path(os.environ['DURIS_PLAN5_CHILD_IDENTITY_ARTIFACTS']).resolve()
        self.assertFalse(work.exists())
        self.assertTrue(work.is_relative_to((ROOT/'bin').resolve()))
        work.mkdir(parents=True)
        source = work/'child-probe.cpp'
        source.write_text(NATIVE_PROBE)
        native = None
        sources = [str(source), 'src/economy/economic_accounting_types.c',
                   'src/economy/economic_accounting_plan.c', 'src/economy/economic_source_event.c', 'src/economy/economic_accounting_intent.c',
                   'src/persistence/critical_command.c', 'src/item/item_transfer_command.c', 'src/world/quest_mobile_native_reference.c',
                   'src/item/craft_pouch_mutation.c', 'src/combat/chaos_pouch_ledger.c',
                   'src/player/player_snapshot_codec.c']
        builds = []
        for backend in ('sql', 'flatfile'):
            binary = work/('child-probe-'+backend)
            command = ['g++', '-std=c++20', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
                       '-O1', '-g', '-fsanitize=address,undefined', '-fno-omit-frame-pointer',
                       '-fno-pie', '-no-pie', '-D__NO_TESTS__', '-Isrc']
            if backend == 'flatfile': command.append('-D__NO_MYSQL__')
            command += sources+['-lcrypto', '-o', str(binary)]
            started = time.monotonic()
            reuse = os.environ.get('DURIS_PLAN5_CHILD_IDENTITY_PROBE_REUSE')
            if reuse:
                previous = Path(reuse).resolve()
                self.assertTrue(previous.is_relative_to((ROOT/'bin').resolve()))
                self.assertEqual((previous/'child-probe.cpp').read_text(), NATIVE_PROBE)
                recorded = next(row for row in json.loads((previous/'native-builds.json').read_text())
                                if row['backend'] == backend)
                original = previous/('child-probe-'+backend)
                self.assertEqual(hashlib.sha256(original.read_bytes()).hexdigest(), recorded['binary_sha256'])
                previous_command = [str(source) if arg == str(previous/'child-probe.cpp') else arg
                                    for arg in recorded['command'][:-2]]
                self.assertEqual(previous_command, command[:-2])
                inputs = json.loads((ROOT/'tmp/plan5/child-identity-evidence-start.json').read_text())['source']
                for name in sources[1:]:
                    self.assertEqual(hashlib.sha256((ROOT/name).read_bytes()).hexdigest(), inputs[name]['sha256'])
                shutil.copy2(original, binary)
                self.assertEqual(hashlib.sha256(binary.read_bytes()).hexdigest(), recorded['binary_sha256'])
                shutil.copyfile(previous/(backend+'-compile.log'), work/(backend+'-compile.log'))
                compile_exit = recorded['compile_exit']
            else:
                compiled = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
                (work/(backend+'-compile.log')).write_text(compiled.stdout+compiled.stderr)
                self.assertEqual(compiled.returncode, 0, compiled.stdout+compiled.stderr)
                compile_exit = compiled.returncode
            result = subprocess.run([str(binary)], cwd=ROOT, capture_output=True, text=True,
                                    env=dict(os.environ, ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',
                                             UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1'))
            (work/(backend+'-stdout.log')).write_text(result.stdout)
            (work/(backend+'-stderr.log')).write_text(result.stderr)
            self.assertEqual(result.returncode, 0, result.stdout+result.stderr)
            self.assertEqual(result.stderr, '')
            output = json.loads(result.stdout)
            if native is not None: self.assertEqual(output, native)
            native = output
            builds.append({'backend': backend, 'command': command, 'compile_exit': compile_exit,
                           'run_exit': result.returncode, 'seconds': time.monotonic()-started,
                           'binary_sha256': hashlib.sha256(binary.read_bytes()).hexdigest(),
                           'verified_probe_reuse': bool(reuse), 'reuse_directory': reuse,
                           'native_contract_cases': output['native_contract_cases']})
            (work/'native-builds.json').write_text(json.dumps(builds, indent=2)+'\n')
        frozen, plan = bytes.fromhex(native['intent']), bytes.fromhex(native['plan'])
        decoded = decode_plan(plan)
        self.assertEqual(len(decoded['children']), 2)
        for index, row in enumerate(native['children']):
            parent = OP if not row['parent_index'] else native['children'][row['parent_index']-1]['child_operation_id']
            self.assertEqual(row, child(parent, index+1, row['parent_index'], row['domain_id'], row['discriminator']))

        baseline = {}
        for name in ('reconcile_economy_accounting', 'economic_sql_audit_snapshot'):
            path = work.parent/(name+'.py')
            spec = importlib.util.spec_from_file_location('plan5_child_preimage_'+name, path)
            module = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(module)
            module.REGISTRY_PATH = ROOT/'docs/persistence/economy_accounting/registry.json'
            baseline[name] = module

        captures, cuts = [], []
        self.cli_results = []
        for engine in ('mariadb', 'mysql'):
            candidate = work/engine
            candidate.mkdir(mode=0o700)
            with restore.private_database(candidate, engine) as env:
                version = sql(env, 'SELECT VERSION()')
                sql(env, payload=(ROOT/'migrations/bootstrap_multithread_safe.sql').read_bytes())
                with mock.patch.dict(os.environ, env, clear=True):
                    manifest = migrations.load_manifest()
                    executor = migrations.MysqlExecutor(manifest)
                    executor.adopt('fresh_bootstrap')
                    migrations.run_pending(manifest, executor)
                self.assertEqual(sql(env, 'SELECT sequence_number,migration_id FROM mud_schema_history ORDER BY sequence_number DESC LIMIT 1'),
                                 '56\t0056_spell_ward_durability')
                owner = pymysql.connect(unix_socket=env['DB_SOCKET'], user='root', database='duris_restore',
                                        autocommit=True, cursorclass=pymysql.cursors.DictCursor)
                try:
                    def insert(table, fields):
                        with owner.cursor() as cursor:
                            cursor.execute('INSERT INTO '+table+' ('+','.join(fields)+') VALUES ('+
                                           ','.join(['%s']*len(fields))+')', tuple(fields.values()))

                    creator = bytes([7])*16
                    insert('critical_operation_inbox', dict(operation_id=creator, command_hash=bytes([1])*32,
                           keys_hash=bytes([2])*32, command_type=1, schema_version=1, payload_version=1,
                           status=1, result_payload=b''))
                    insert('economic_epoch', dict(lineage=plan[8:24], epoch=plan[24:40], ordinal=2,
                           transition_kind=1, transition_digest=bytes([3])*32, creating_operation_id=creator))
                    insert('economic_lineage_state', dict(lineage=plan[8:24], active_epoch=None))
                    operation = plan[40:56]
                    insert('critical_operation_inbox', dict(operation_id=operation, command_hash=bytes([4])*32,
                           keys_hash=bytes([5])*32, command_type=3, schema_version=2, payload_version=1,
                           status=1, result_code=0, durable_revision=1, result_payload=b''))
                    with owner.cursor() as cursor:
                        cursor.execute('UPDATE critical_operation_inbox SET committed_at=CURRENT_TIMESTAMP(6) WHERE operation_id=%s', (operation,))
                    accounts, postings, children, before, after, events = struct.unpack_from('<6I', plan, 216)
                    insert('economic_accounting_operation', dict(operation_id=operation, lineage=plan[8:24],
                           epoch=plan[24:40], original_operation_id=None, accounting_version=1, writer_id=1,
                           policy_version=1, compiler_version=1, actor_kind=plan[72], actor_id=7,
                           reason=struct.unpack_from('<H', plan, 96)[0], source_event=None,
                           intent_digest=plan[152:184], domain_digest=plan[184:216],
                           plan_digest=hashlib.sha256(plan).digest(), canonical_intent=frozen,
                           canonical_plan=plan, outcome=1, result_code=0, account_count=accounts,
                           posting_count=postings, child_count=children, item_event_count=events,
                           before_witness_count=before, after_witness_count=after))
                    for index in range(accounts):
                        offset = 256+index*120
                        fields = dict(operation_id=operation, account_index=index, account_key=plan[offset:offset+40])
                        fields.update(zip((side+'_'+coin for side in ('before', 'after')
                                          for coin in ('copper', 'silver', 'gold', 'platinum')),
                                          struct.unpack_from('<8q', plan, offset+40)))
                        fields.update(zip(('before_revision', 'after_revision'), struct.unpack_from('<2Q', plan, offset+104)))
                        insert('economic_accounting_account_effect', fields)
                    for index in range(postings):
                        offset = 256+accounts*120+index*48
                        event, account, child_index, *delta, amount = struct.unpack_from('<IHH4qq', plan, offset)
                        fields = dict(operation_id=operation, line_index=index, event_index=event,
                                      account_index=account, child_index=child_index, copper_value=amount)
                        fields.update(zip(('delta_'+coin for coin in ('copper', 'silver', 'gold', 'platinum')), delta))
                        insert('economic_accounting_coin_posting', fields)
                    for row in native['children']:
                        insert('economic_accounting_child', {**row, 'operation_id': operation,
                               'child_operation_id': bytes.fromhex(row['child_operation_id'])})
                    insert('item_ownership_ledger', dict(operation_id=operation, event_index=0, item_uid=81,
                           root_item_uid=81, parent_item_uid=None, from_owner_type=1, from_owner_id=8,
                           from_owner_context_id=0, to_owner_type=1, to_owner_id=7, to_owner_context_id=0,
                           item_revision=2, from_owner_revision=1, to_owner_revision=2, reason_type=8,
                           source_site=1, from_equipment_slot=0, to_equipment_slot=0))
                    insert('economic_accounting_item_reference', dict(operation_id=operation, line_index=0,
                           event_index=0, child_index=1, item_uid=81, before_revision=1, after_revision=2,
                           legacy_operation_id=operation, legacy_event_index=0))
                    with owner.cursor() as cursor:
                        cursor.execute("CREATE USER 'child_reader'@'localhost' IDENTIFIED BY 'disposable-child-reader'")
                        cursor.execute("GRANT SELECT ON duris_restore.* TO 'child_reader'@'localhost'")
                    reader = pymysql.connect(unix_socket=env['DB_SOCKET'], user='child_reader',
                                             password='disposable-child-reader', database='duris_restore',
                                             autocommit=True, cursorclass=pymysql.cursors.DictCursor)
                    try:
                        with reader.cursor() as cursor:
                            with self.assertRaises(pymysql.MySQLError) as denied:
                                cursor.execute('UPDATE economic_lineage_state SET revision=revision')
                            self.assertEqual(denied.exception.args[0], 1142)

                        def inventory():
                            with owner.cursor() as cursor:
                                rows = []
                                for table in ('economic_lineage_state', 'economic_epoch', 'critical_operation_inbox',
                                              'economic_accounting_operation', 'economic_accounting_account_effect',
                                              'economic_accounting_coin_posting', 'economic_accounting_child',
                                              'economic_accounting_item_reference', 'item_ownership_ledger'):
                                    cursor.execute('SELECT * FROM '+table+' ORDER BY 1,2')
                                    rows.append(cursor.fetchall())
                                return rows

                        def capture(module, label):
                            before_rows = inventory()
                            cursor = mock.Mock(wraps=reader.cursor())
                            connection = mock.Mock(wraps=reader)
                            try:
                                cursor.execute('SET TRANSACTION ISOLATION LEVEL REPEATABLE READ')
                                cursor.execute('START TRANSACTION WITH CONSISTENT SNAPSHOT, READ ONLY')
                                result = module.read_evidence(cursor, plan[8:24], plan[24:40], True)
                            finally:
                                connection.rollback()
                                cursor.close()
                            connection.rollback.assert_called_once_with()
                            cursor.close.assert_called_once_with()
                            self.assertTrue(all(call.args[0].upper().startswith(('SELECT', 'SET TRANSACTION', 'START TRANSACTION'))
                                                for call in cursor.execute.call_args_list))
                            self.assertEqual(before_rows, inventory())
                            modeled = clean_snapshot()
                            modeled_origins = modeled['account_origins']
                            modeled_item_origins = modeled['item_origins']
                            modeled.update(result)
                            modeled['account_origins'] = modeled_origins
                            modeled['item_origins'] = modeled_item_origins
                            modeled['item_origins'][0]['owner'] = [1,8,0]
                            (work/(engine+'-'+label+'-snapshot.json')).write_text(json.dumps(modeled, indent=2)+'\n')
                            captures.append({'engine': engine, 'label': label, 'read_only': True,
                                             'rollback_calls': 1, 'queries': [call.args[0] for call in cursor.execute.call_args_list]})
                            return modeled

                        original = capture(exporter, 'intact')
                        self.assertEqual(original['children'], native['children'])
                        self.assertEqual(Reconciler().audit(original)['exception_count'], 0)
                        legacy_cut = capture(baseline['economic_sql_audit_snapshot'], 'legacy-projection')
                        self.assertEqual(baseline['reconcile_economy_accounting'].Reconciler().audit(legacy_cut)['exception_count'], 0)
                        self.assertEqual(Reconciler(0).audit(legacy_cut)['exception_counts'],
                                         {'missing_child_identity_evidence': 2, 'missing_original_plan': 1})
                        cuts.append({'engine': engine, 'kind': 'legacy_projection', 'red_clean': True, 'green_code': 'missing_child_identity_evidence'})
                        child_receipt = bytes.fromhex(native['children'][0]['child_operation_id'])
                        insert('critical_operation_inbox', dict(operation_id=child_receipt, command_hash=bytes([6])*32,
                               keys_hash=bytes([7])*32, command_type=1, schema_version=1, payload_version=1,
                               status=1, result_code=0, durable_revision=1, result_payload=b''))
                        with owner.cursor() as cursor:
                            cursor.execute('UPDATE economic_accounting_child SET receipt_operation_id=%s WHERE operation_id=%s AND child_index=1',
                                           (child_receipt, operation))
                        receipt_cut = capture(exporter, 'receipt-link')
                        self.assertEqual(receipt_cut['children'][0]['receipt_operation_id'], child_receipt.hex())
                        self.assertEqual(Reconciler().audit(receipt_cut)['exception_count'], 0)
                        with owner.cursor() as cursor:
                            cursor.execute('UPDATE economic_accounting_child SET receipt_operation_id=NULL WHERE operation_id=%s AND child_index=1', (operation,))
                        mutations = [('zero', 1, 'child_operation_id', bytes(16), 'invalid_child_link', 2),
                                     ('wrong_id', 1, 'child_operation_id', bytes.fromhex('44'*16), 'child_identity_mismatch', 2),
                                     ('domain', 1, 'domain_id', 475, 'child_identity_mismatch', 1),
                                     ('discriminator', 1, 'discriminator', 1, 'child_identity_mismatch', 1),
                                     ('parent', 2, 'parent_index', 0, 'child_identity_mismatch', 1)]
                        for label, index, field, value, code, count in mutations:
                            saved = native['children'][index-1][field]
                            if field == 'child_operation_id': saved = bytes.fromhex(saved)
                            with owner.cursor() as cursor:
                                cursor.execute('UPDATE economic_accounting_child SET '+field+'=%s WHERE operation_id=%s AND child_index=%s',
                                               (value, operation, index))
                            try:
                                changed = capture(exporter, label)
                                old = baseline['reconcile_economy_accounting'].Reconciler().audit(changed)
                                self.assertEqual(old['exception_count'], 0)
                                for limit in (0, 1, 100):
                                    report = Reconciler(limit).audit(changed)
                                    self.assertEqual(report['exception_counts'].get(code), count, report)
                                self.cli_check(changed, work/(engine+'-'+label+'-snapshot.json'), code, count)
                                cuts.append({'engine': engine, 'kind': label, 'red_clean': True, 'green_code': code,
                                             'green_count': count})
                            finally:
                                with owner.cursor() as cursor:
                                    cursor.execute('UPDATE economic_accounting_child SET '+field+'=%s WHERE operation_id=%s AND child_index=%s',
                                                   (saved, operation, index))
                        # The authoritative UNIQUE constraint remains installed.
                        before_rows = inventory()
                        with self.assertRaises(pymysql.MySQLError) as duplicate:
                            insert('economic_accounting_child', {**native['children'][0], 'operation_id': operation,
                                   'child_index': 3, 'child_operation_id': bytes.fromhex(native['children'][0]['child_operation_id'])})
                        self.assertEqual(duplicate.exception.args[0], 1062)
                        self.assertEqual(before_rows, inventory())
                        corrupt_export = copy.deepcopy(original)
                        corrupt_export['children'][1]['child_operation_id'] = corrupt_export['children'][0]['child_operation_id']
                        self.assertEqual(Reconciler(0).audit(corrupt_export)['exception_counts']['duplicate_child_operation'], 1)
                        self.assertEqual(baseline['reconcile_economy_accounting'].Reconciler().audit(corrupt_export)['exception_count'], 0)
                        cuts.append({'engine': engine, 'kind': 'duplicate_export', 'red_clean': True, 'green_code': 'duplicate_child_operation'})
                        self.assertEqual(Reconciler().audit(capture(exporter, 'restored'))['exception_count'], 0)
                        for label, code in (('coherent_children', 'original_plan_child_mismatch'),
                                             ('posting_child', 'original_plan_posting_mismatch'),
                                             ('item_child', 'original_plan_item_mismatch')):
                            saved_rows = inventory()
                            try:
                                with owner.cursor() as cursor:
                                    if label == 'coherent_children':
                                        first = child(OP,1,domain=475)
                                        second = child(first['child_operation_id'],2,1,2**32-1,2**64-1)
                                        for row in (first,second):
                                            cursor.execute('UPDATE economic_accounting_child SET domain_id=%s,child_operation_id=%s '
                                                           'WHERE operation_id=%s AND child_index=%s',
                                                           (row['domain_id'],bytes.fromhex(row['child_operation_id']),operation,row['child_index']))
                                    elif label == 'posting_child':
                                        cursor.execute('UPDATE economic_accounting_coin_posting SET child_index=0 '
                                                       'WHERE operation_id=%s AND line_index=0',(operation,))
                                    else:
                                        cursor.execute('UPDATE economic_accounting_item_reference SET child_index=0 '
                                                       'WHERE operation_id=%s AND line_index=0',(operation,))
                                changed = capture(exporter,label)
                                self.assertEqual(baseline['reconcile_economy_accounting'].Reconciler().audit(changed)['exception_count'],0)
                                self.assertEqual(Reconciler(0).audit(changed)['exception_counts'],{code:1})
                                self.cli_check(changed,work/(engine+'-'+label+'-snapshot.json'),code,1)
                                cuts.append({'engine':engine,'kind':label,'red_clean':True,'green_code':code,'green_count':1})
                            finally:
                                with owner.cursor() as cursor:
                                    if label == 'coherent_children':
                                        for row in native['children']:
                                            cursor.execute('UPDATE economic_accounting_child SET domain_id=%s,child_operation_id=%s '
                                                           'WHERE operation_id=%s AND child_index=%s',
                                                           (row['domain_id'],bytes.fromhex(row['child_operation_id']),operation,row['child_index']))
                                    elif label == 'posting_child':
                                        cursor.execute('UPDATE economic_accounting_coin_posting SET child_index=1 '
                                                       'WHERE operation_id=%s AND line_index=0',(operation,))
                                    else:
                                        cursor.execute('UPDATE economic_accounting_item_reference SET child_index=1 '
                                                       'WHERE operation_id=%s AND line_index=0',(operation,))
                            self.assertEqual(saved_rows,inventory())
                            self.assertEqual(Reconciler().audit(capture(exporter,label+'-repaired'))['exception_count'],0)
                        (work/(engine+'-results.json')).write_text(json.dumps({'engine': engine, 'version': version,
                            'schema_sequence': 56, 'active_epoch': None, 'unique_constraint_enforced': True,
                            'native_parent_plan': True, 'modeled_native_holdings': True,
                            'source_capture_qualified': False, 'release_qualified': False}, indent=2)+'\n')
                    finally:
                        reader.close()
                finally:
                    owner.close()
        (work/'sql-captures.json').write_text(json.dumps(captures, indent=2)+'\n')
        (work/'fault-results.json').write_text(json.dumps(cuts, indent=2)+'\n')
        (work/'cli-results.json').write_text(json.dumps(self.cli_results, indent=2)+'\n')
        self.assertEqual(len(captures), 30)
        self.assertEqual(len(cuts), 20)
        self.assertEqual(len(self.cli_results), 96)
        print('PLAN5_CHILD_IDENTITY '+json.dumps({'native_configurations': 2, 'native_contract_cases': 14,
              'sql_engines': 2, 'read_only_captures': 30, 'red_clean_faults': 20,
              'schema': 'canonical0056', 'modeled_native_holdings': True, 'release_qualified': False}), flush=True)

    def cli_check(self, snapshot, path, code, count):
        before = path.read_bytes()
        for limit in (0, 1, 100):
            for name, filters in (('exceptions', []), ('operation', ['--operation-id', OP])):
                command = [sys.executable, str(ROOT/'scripts/reconcile_economy_accounting.py'),
                           str(path), '--view', name, '--limit', str(limit), *filters]
                result = subprocess.run(command, capture_output=True, text=True)
                # Malformed IDs cause the strict ID-only operation view to refuse.
                expected = 2 if code == 'invalid_child_link' and name == 'operation' else 1
                self.assertEqual(result.returncode, expected, result.stdout+result.stderr)
                if expected == 2:
                    self.assertEqual(result.stdout, '')
                    self.assertTrue(result.stderr.startswith('reconciliation failed: '), result.stderr)
                    self.assertIn('child_operation_id', result.stderr)
                else:
                    self.assertEqual(result.stderr, '')
                    output = json.loads(result.stdout)
                    if name == 'exceptions': self.assertEqual(output['exception_counts'].get(code), count)
                    else: self.assertGreater(output['coverage']['exception_count'], 0)
                self.assertEqual(path.read_bytes(), before)
                self.cli_results.append({'command': command, 'exit': result.returncode,
                                         'finding_code': code, 'finding_count': count,
                                         'strict_refusal': expected == 2,
                                         'input_unchanged': True, 'stderr': result.stderr})


@unittest.skipUnless(os.environ.get('DURIS_PLAN5_CHILD_IDENTITY_BUDGET') == '1',
                     'requires explicitly selected child workload qualification')
class ChildIdentityBudgetTests(unittest.TestCase):
    def test_near_limit_child_snapshot_is_bounded_and_limit_invariant(self):
        import resource
        from reconcile_economy_accounting import MAX_INPUT_BYTES
        work = Path(os.environ['DURIS_PLAN5_CHILD_IDENTITY_BUDGET_ARTIFACTS']).resolve()
        self.assertFalse(work.exists())
        self.assertTrue(work.is_relative_to((ROOT/'bin').resolve()))
        work.mkdir(parents=True)
        snapshot = clean_snapshot()
        wallet = snapshot['account_origins'][0]['account_key']
        snapshot['account_origins'] = [{'account_key': wallet, 'origin': 'baseline',
                                        'balance': [0, 0, 0, 0], 'revision': 1}]
        snapshot['item_origins'] = []
        snapshot['native']['items'] = []
        snapshot['native']['holdings'] = [{'account_key': wallet, 'balance': [0, 0, 0, 0],
                                          'revision': 501}]
        for name in ('operations', 'effects', 'postings', 'children', 'item_references',
                     'ownership_events', 'receipts', 'source_claims'):
            snapshot[name] = []
        for index in range(500):
            root = (index+1).to_bytes(16, 'little').hex()
            snapshot['operations'].append({'operation_id': root, 'lineage': snapshot['lineage'],
                'epoch': snapshot['epoch'], 'reason': 1, 'outcome': 'committed', 'result_code': 0,
                'source_event': None, 'account_count': 1, 'posting_count': 64,
                'child_count': 64, 'item_event_count': 0, 'realized_price_copper': None})
            snapshot['effects'].append({'operation_id': root, 'account_index': 0, 'account_key': wallet,
                'before': [0, 0, 0, 0], 'after': [0, 0, 0, 0],
                'before_revision': index+1, 'after_revision': index+2})
            snapshot['receipts'].append({'operation_id': root, 'status': 1, 'result_code': 0,
                'failure_stage': 0, 'committed_at_present': True})
            children = sorted(({**child(root, number+1, discriminator=number), 'operation_id': root}
                               for number in range(64)), key=lambda row: row['child_operation_id'])
            for number, row in enumerate(children):
                row['child_index'] = number+1
                snapshot['children'].append(row)
                amount = 1 if number % 2 else -1
                snapshot['postings'].append({'operation_id': root, 'line_index': number,
                    'account_index': 0, 'child_index': number+1,
                    'delta': [amount, 0, 0, 0], 'copper_value': amount})
        bind_original_plans(snapshot)
        snapshot['padding'] = ''
        payload = json.dumps(snapshot, separators=(',', ':')).encode()
        self.assertLess(len(payload), MAX_INPUT_BYTES)
        unpadded = len(payload)
        snapshot['padding'] = 'x'*(MAX_INPUT_BYTES-len(payload))
        payload = json.dumps(snapshot, separators=(',', ':')).encode()
        self.assertEqual(len(payload), MAX_INPUT_BYTES)
        path = work/'snapshot.json'
        path.write_bytes(payload)
        results = []
        for limit in (0, 1, 100):
            command = [sys.executable, str(ROOT/'scripts/reconcile_economy_accounting.py'),
                       str(path), '--view', 'exceptions', '--limit', str(limit)]
            started = time.monotonic()
            result = subprocess.run(command, capture_output=True, text=True)
            seconds = time.monotonic()-started
            # This cumulative maximum conservatively includes earlier CLI children.
            peak = resource.getrusage(resource.RUSAGE_CHILDREN).ru_maxrss
            (work/('limit-'+str(limit)+'-stdout.json')).write_text(result.stdout)
            (work/('limit-'+str(limit)+'-stderr.log')).write_text(result.stderr)
            self.assertEqual(result.returncode, 0, result.stdout+result.stderr)
            self.assertEqual(json.loads(result.stdout)['exception_count'], 0)
            self.assertEqual(result.stderr, '')
            self.assertLess(seconds, 30)
            self.assertLess(peak, 256*1024)
            results.append({'limit': limit, 'command': command, 'exit': 0, 'seconds': seconds,
                            'cumulative_child_peak_rss_kib': peak})
        self.assertEqual(hashlib.sha256(path.read_bytes()).hexdigest(), hashlib.sha256(payload).hexdigest())
        summary = {'modeled_roots': 500, 'children': 32000, 'postings': 32000,
                   'encoded_bytes': MAX_INPUT_BYTES, 'unpadded_bytes': unpadded,
                   'seconds_budget': 30, 'rss_budget_kib': 256*1024, 'results': results,
                   'release_host_qualified': False, 'source_capture_qualified': False}
        (work/'results.json').write_text(json.dumps(summary, indent=2)+'\n')
        print('PLAN5_CHILD_IDENTITY_BUDGET '+json.dumps(summary, sort_keys=True), flush=True)


if __name__ == '__main__':
    unittest.main(verbosity=2)
