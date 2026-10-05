#!/usr/bin/env python3
"""Independent native-mobile custody grammar, without runtime authority changes."""

import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import time
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT/'scripts'))
import economic_restore_evidence as evidence
from economic_sql_audit_origins import decode_witness

# label, owner type, state, identity, context, root, parent, equipment slot
CASES = [
    ('player', 1, 1, 8, 0, 81, 0, 0),
    ('player_equipped', 1, 1, 8, 0, 81, 0, 43),
    ('player_historical_slot', 1, 1, 8, 0, 81, 0, 65535),
    ('mobile', 12, 1, 42, 0, 81, 0, 0),
    ('mobile_first_identity', 12, 1, 1, 0, 81, 0, 0),
    ('mobile_last_identity', 12, 1, 2**64-2, 0, 81, 0, 0),
    ('mobile_equipped_first', 12, 1, 42, 0, 81, 0, 1),
    ('mobile_equipped_last', 12, 1, 42, 0, 81, 0, 43),
    ('mobile_quarantined', 12, 3, 42, 0, 81, 0, 0),
    ('mobile_zero_identity', 12, 1, 0, 0, 81, 0, 0),
    ('mobile_reserved_identity', 12, 1, 2**64-1, 0, 81, 0, 0),
    ('mobile_context', 12, 1, 42, 1, 81, 0, 0),
    ('mobile_last_context', 12, 1, 42, 2**64-1, 81, 0, 0),
    ('mobile_slot_overflow', 12, 1, 42, 0, 81, 0, 44),
    ('mobile_slot_maximum', 12, 1, 42, 0, 81, 0, 65535),
    ('mobile_equipped_quarantined', 12, 3, 42, 0, 81, 0, 1),
    ('mobile_equipped_parent', 12, 1, 42, 0, 82, 82, 1),
    ('mobile_equipped_foreign_root', 12, 1, 42, 0, 82, 0, 1),
    ('mobile_zero_root', 12, 1, 42, 0, 0, 0, 0),
    ('mobile_self_parent', 12, 1, 42, 0, 81, 81, 0),
    ('mobile_destroyed', 12, 2, 42, 0, 81, 0, 0),
    ('mobile_unknown_state', 12, 4, 42, 0, 81, 0, 0),
    ('mobile_absent_nonzero', 12, 0, 42, 0, 81, 0, 0),
    ('unknown_owner', 13, 1, 42, 0, 81, 0, 0),
    ('shopkeeper_equipped', 9, 1, 42, 0, 81, 0, 1),
    ('collector_context', 10, 1, 42, 1, 81, 0, 0),
    ('pet', 11, 1, 42, 1, 81, 0, 0),
    ('pet_context_overflow', 11, 1, 42, 2**31, 81, 0, 0),
    ('system', 7, 1, 0, 0, 81, 0, 0),
    ('system_identity', 7, 1, 1, 0, 81, 0, 0),
    ('destruction', 8, 2, 0, 0, 81, 0, 0),
    ('absent', 0, 0, 0, 0, 0, 0, 0),
]


def position(case, revision=1):
    _, owner, state, identity, context, root, parent, slot = case
    return owner, state, identity, context, root, parent, 0 if state == 0 else revision, slot


class MobilePositionTests(unittest.TestCase):
    def witness(self, state=1, identity=42, context=0):
        from test_economic_sql_audit_origins import baseline_root, witness
        data = bytearray(witness([])['canonical_witness'])
        data[200:202] = bytes((12, state))
        struct.pack_into('<QQ', data, 208, identity, context)
        return baseline_root(bytes(data))

    def test_original_native_mobile_witness_identity_and_quarantine(self):
        for state in (1, 3):
            for identity in (1, 42, 2**64-2):
                row = self.witness(state, identity)
                data = row['canonical_witness']
                with self.subTest(state=state, identity=identity):
                    holdings, items = decode_witness(row, data[16:32], data[32:48], data[80:120])
                    self.assertEqual(holdings, [])
                    self.assertEqual(items[0]['owner'], [12, identity, 0])

    def test_original_native_mobile_witness_refuses_invalid_lifetime(self):
        for state, identity, context in ((2, 42, 0), (1, 0, 0), (1, 2**64-1, 0),
                                         (1, 42, 1), (1, 42, 2**64-1)):
            row = self.witness(state, identity, context)
            data = row['canonical_witness']
            with self.subTest(state=state, identity=identity, context=context), self.assertRaises(ValueError):
                decode_witness(row, data[16:32], data[32:48], data[80:120])

    def test_native_mobile_identity_and_quarantine_are_valid(self):
        for case in CASES:
            if case[0] in ('mobile', 'mobile_first_identity', 'mobile_last_identity', 'mobile_quarantined'):
                with self.subTest(case=case[0]):
                    evidence.valid_position(81, position(case))

    def test_native_mobile_invalid_identity_context_and_state_refuse(self):
        for case in CASES:
            if case[0] in ('mobile_zero_identity', 'mobile_reserved_identity', 'mobile_context',
                           'mobile_last_context', 'mobile_destroyed', 'mobile_unknown_state', 'mobile_absent_nonzero'):
                with self.subTest(case=case[0]), self.assertRaises(ValueError):
                    evidence.valid_position(81, position(case))

    def test_native_mobile_equipment_bounds_and_topology(self):
        for case in CASES:
            if case[0] in ('mobile_equipped_first', 'mobile_equipped_last'):
                with self.subTest(case=case[0]):
                    evidence.valid_position(81, position(case))
            elif case[0].startswith(('mobile_slot_', 'mobile_equipped_')):
                with self.subTest(case=case[0]), self.assertRaises(ValueError):
                    evidence.valid_position(81, position(case))

    def test_historical_player_equipment_and_pet_owner_rules(self):
        for case in CASES:
            if case[0] in ('player', 'player_equipped', 'player_historical_slot', 'pet', 'system', 'destruction', 'absent'):
                with self.subTest(case=case[0]):
                    evidence.valid_position(81, position(case))
            elif case[0] in ('unknown_owner', 'shopkeeper_equipped', 'collector_context', 'pet_context_overflow', 'system_identity'):
                with self.subTest(case=case[0]), self.assertRaises(ValueError):
                    evidence.valid_position(81, position(case))


def native_source():
    from test_plan5_child_identity import NATIVE_PROBE
    prefix = '#include "economy/economic_baseline_adapter.h"\n' + NATIVE_PROBE.split('    std::cout<<', 1)[0]
    # The older maintained child probe omits this original item cut. Keep the
    # grammar fixture coherent without importing its unrelated audit successor.
    # A later coherent probe may already carry these exact retained seed lines.
    custody_seed = r'''    economic_item_position old;
    old.owner={item_owner_type::player,8,0}; old.root_uid=81; old.revision=1;
    old.state=item_custody_state::active;
    auto next=old; next.owner.id=7; next.revision=2;
    plan.items_before={{81,old}}; plan.items_after={{81,next}};
    plan.item_events={{0,1,81,old,next}};
'''
    anchor = '    economic_frozen_intent intent;\n'
    assert prefix.count(anchor) == 1
    if '    economic_item_position old;\n' not in prefix:
        assert 'auto next=' not in prefix and 'plan.item_events=' not in prefix
        prefix = prefix.replace(anchor, custody_seed + anchor, 1)
    else:
        assert prefix.count(custody_seed) == 1
    rows = ',\n'.join('{"'+label+'",'+','.join(str(value)+'ULL' for value in fields)+'}'
                      for label,*fields in CASES)
    return prefix + r'''
    assert(encoded.size()==912);
    auto put=[](std::vector<uint8_t>&bytes,size_t offset,uint64_t value,size_t width) {
        for(size_t i=0;i<width;++i)bytes[offset+i]=static_cast<uint8_t>(value>>(8*i));
    };
    struct shape { const char*name; uint64_t owner,state,id,context,root,parent,slot; };
    const shape cases[]={
''' + rows + r'''
    };
    economic_baseline_batch batch;
    batch.lineage=meta.lineage; batch.epoch=meta.epoch; batch.preparation_id=id(0x44);
    batch.actor_id=7; batch.batch_index=1;
    batch.opening_account={meta.lineage,economic_account_kind::opening,99,0};
    batch.boundary_digest[0]=5; batch.coverage_digest[0]=6;
    economic_baseline_item item;
    item.snapshot={81,next}; item.source_digest[0]=8; batch.items={item};
    std::optional<economic_prepared_baseline> prepared;
    assert(economic_baseline_prepare(batch,&prepared)==economic_accounting_error::ok);
    std::vector<uint8_t> witness;
    assert(economic_baseline_encode(*prepared,&witness)==economic_accounting_error::ok);
    assert(witness.size()==280);
    for(const auto&test:cases) {
        for(bool before:{true,false}) {
            auto bytes=encoded;
            for(size_t offset: before ? std::array<size_t,2>{664,800} : std::array<size_t,2>{728,856}) {
                put(bytes,offset,test.owner,1); put(bytes,offset+1,test.state,1);
                put(bytes,offset+8,test.id,8); put(bytes,offset+16,test.context,8);
                put(bytes,offset+24,test.root,8); put(bytes,offset+32,test.parent,8);
                put(bytes,offset+40,test.state ? (before ? 1 : 2) : 0,8);
                put(bytes,offset+48,test.slot,2);
            }
            economic_accounting_plan candidate;
            const bool accepted=economic_plan_decode(bytes,&candidate)==economic_accounting_error::ok;
            std::cout<<"{\"name\":\""<<test.name<<(before ? "-before" : "-after")
                <<"\",\"kind\":\"plan\",\"accepted\":"<<(accepted ? "true" : "false")
                <<",\"bytes\":\""<<hex(bytes)<<"\"}\n";
        }
        // EAB1 contains no equipment field. Keep these complete single-root cuts
        // within the origin decoder's scope; original EAP1 checks its topology.
        if(test.slot || test.root!=81 || test.parent || test.state==0)continue;
        auto bytes=witness;
        put(bytes,200,test.owner,1); put(bytes,201,test.state,1);
        put(bytes,208,test.id,8); put(bytes,216,test.context,8);
        std::optional<economic_prepared_baseline> candidate;
        const bool accepted=economic_baseline_decode(bytes,&candidate)==economic_accounting_error::ok;
        std::cout<<"{\"name\":\""<<test.name<<"-witness\",\"kind\":\"witness\",\"accepted\":"
            <<(accepted ? "true" : "false")<<",\"bytes\":\""<<hex(bytes)<<"\"}\n";
    }
}
'''


@unittest.skipUnless(os.environ.get('DURIS_PLAN5_MOBILE_GRAMMAR_NATIVE') == '1',
                     'requires explicitly selected native SQL/flatfile grammar checks')
class NativeMobileGrammarTests(unittest.TestCase):
    def test_original_native_plan_and_witness_grammar_matches_independent_readers(self):
        work = Path(os.environ['DURIS_PLAN5_MOBILE_GRAMMAR_ARTIFACTS']).resolve()
        self.assertTrue(work.is_relative_to((ROOT/'bin').resolve()))
        self.assertFalse(work.exists())
        work.mkdir(parents=True)
        source = work/'mobile-grammar.cpp'
        source.write_text(native_source())
        sources = [str(source), 'src/economy/economic_accounting_types.c',
                   'src/economy/economic_accounting_plan.c', 'src/economy/economic_source_event.c',
                   'src/economy/economic_accounting_intent.c', 'src/economy/economic_baseline_adapter.c',
                   'src/economy/economic_baseline_codec.c', 'src/persistence/critical_command.c',
                   'src/item/item_transfer_command.c', 'src/world/quest_mobile_native_reference.c',
                   'src/item/craft_pouch_mutation.c', 'src/combat/chaos_pouch_ledger.c',
                   'src/player/player_snapshot_codec.c']
        builds, output = [], None
        for backend in ('sql','flatfile'):
            binary = work/('mobile-grammar-'+backend)
            command = ['g++','-std=c++20','-Wall','-Wextra','-Wpedantic','-Werror','-O1','-g',
                       '-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie',
                       '-D__NO_TESTS__','-Isrc']
            if backend == 'flatfile':
                command.append('-D__NO_MYSQL__')
            command += sources+['-lcrypto','-o',str(binary)]
            started = time.monotonic()
            compiled = subprocess.run(command,cwd=ROOT,capture_output=True,text=True)
            (work/(backend+'-compile.log')).write_text(compiled.stdout+compiled.stderr)
            self.assertEqual(compiled.returncode,0,compiled.stdout+compiled.stderr)
            ran = subprocess.run([str(binary)],capture_output=True,text=True,
                env=dict(os.environ,ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',
                         UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1'))
            (work/(backend+'-stdout.jsonl')).write_text(ran.stdout)
            (work/(backend+'-stderr.log')).write_text(ran.stderr)
            self.assertEqual((ran.returncode,ran.stderr),(0,''))
            if output is not None:
                self.assertEqual(ran.stdout,output)
            output = ran.stdout
            builds.append({'backend':backend,'command':command,'compile_exit':compiled.returncode,
                           'run_exit':ran.returncode,'seconds':time.monotonic()-started,
                           'binary_sha256':hashlib.sha256(binary.read_bytes()).hexdigest(),
                           'verified_probe_reuse':False})
            (work/'native-builds.json').write_text(json.dumps(builds,indent=2)+'\n')
        differences = []
        rows = [json.loads(line) for line in output.splitlines()]
        self.assertEqual(len(rows),2*len(CASES)+sum(not case[7] and case[5]==81 and not case[6] and case[2]!=0 for case in CASES))
        for row in rows:
            data = bytes.fromhex(row['bytes'])
            try:
                if row['kind'] == 'plan':
                    evidence.decode_plan(data)
                else:
                    record = {'canonical_witness':data,'witness_digest':hashlib.sha256(data).digest(),
                              'holding_count':0,'item_count':1}
                    decode_witness(record,data[16:32],data[32:48],data[80:120])
            except ValueError:
                accepted = False
            else:
                accepted = True
            if accepted != row['accepted']:
                differences.append({'name':row['name'],'kind':row['kind'],
                                    'native_accepted':row['accepted'],'independent_accepted':accepted})
        (work/'decoder-differences.json').write_text(json.dumps(differences,indent=2)+'\n')
        self.assertEqual(differences,[])
        print('MOBILE_GRAMMAR '+json.dumps({'cases_per_backend':len(rows),'backends':2,
                                           'native_accepts':sum(row['accepted'] for row in rows),
                                           'independent_agreement':True}),flush=True)


@unittest.skipUnless(os.environ.get('DURIS_PLAN5_BASELINE_VERSION_NATIVE') == '1',
                     'requires explicitly selected independent C++ baseline reference checks')
class IndependentBaselineVersionTests(unittest.TestCase):
    """Reference capsules exercise read-only C++ consumers, not native v2 writers."""

    @classmethod
    def setUpClass(cls):
        cls.work = Path(os.environ['DURIS_PLAN5_BASELINE_VERSION_ARTIFACTS']).resolve()
        assert cls.work.is_relative_to((ROOT/'bin').resolve()) and not cls.work.exists()
        cls.work.mkdir(parents=True)
        source = cls.work/'independent-baseline.cpp'
        source.write_text(r'''#include "qualify_flatfile_economic_lifecycle.h"
#include <iostream>
using namespace restore_economic_authority;
identity id(uint8_t value) { identity result; result.fill(value); return result; }
int main(int argc, char **argv) {
    if (argc != 3) return 2;
    try {
        const std::filesystem::path root=argv[1], directory=root/"economic-evidence";
        const auto lineage=id(0x11), epoch=id(0x22), creator=id(0x44);
        auto intent=file_bytes(directory,"model.eai",8192), plan=file_bytes(directory,"model.eap",4*1024*1024);
        auto payload=file_bytes(directory,"model.ebc",48), command=file_bytes(directory,"model.ccm",8192);
        identity operation; std::copy_n(plan.begin()+40,16,operation.begin());
        epoch_marker marker; marker.epoch=epoch; marker.initialization=baseline_initialization::initialized;
        marker.opening.fill(0); std::copy(lineage.begin(),lineage.end(),marker.opening.begin());
        marker.opening[16]=1; marker.opening[18]=9; marker.opening[20]=99;
        if (std::string(argv[2]) == "baseline") {
            restore_economic_baseline::checker reader(root);
            reader.observe(lineage,epoch,operation,intent,payload,plan,1);
            reader.finish(lineage,{marker});
        } else {
            marker.origin=initialization_origin::lifecycle_owner;
            marker.initializing_operation=marker.creating_operation=creator;
            marker.ordinal=marker.transition_kind=1;
            bytes coverage={'D','U','R','I','S','-','F','L','A','T','F','I','L','E','-','C','O','V','E','R','A','G','E','-','V','1'};
            put(coverage,0,8); put(coverage,0,8); marker.transition_digest=hash(coverage);
            epoch_catalog catalog; catalog.entries={marker};
            bytes control(88); std::copy(epoch.begin(),epoch.end(),control.begin()+16); control[80]=1;
            restore_economic_lifecycle::checker reader(root);
            reader.load(lineage,catalog,control,[](auto){return false;});
            reader.record(operation,command,plan,1,0,0,0); need(reader.finish()==1);
        }
        return 0;
    } catch (...) { std::cerr<<"native_restore_qualification_failed\n"; return 1; }
}
''')
        cls.binary = cls.work/'independent-baseline'
        command = ['g++', '-std=c++20', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O1', '-g',
                   '-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-fno-pie', '-no-pie',
                   '-I'+str(ROOT/'scripts'), str(source), '-lcrypto', '-o', str(cls.binary)]
        started = time.monotonic()
        result = subprocess.run(command, capture_output=True, text=True)
        (cls.work/'compile.log').write_text(result.stdout+result.stderr)
        (cls.work/'compile.json').write_text(json.dumps(dict(command=command, exit=result.returncode,
            seconds=time.monotonic()-started), indent=2)+'\n')
        assert result.returncode == 0, result.stdout+result.stderr

    @staticmethod
    def frame(magic, body):
        return magic + struct.pack('<II', 1, len(body)) + hashlib.sha256(body).digest() + body

    def files(self, row):
        from test_economic_sql_audit_origins import baseline_projections
        blob, operation = row['canonical_witness'], row['operation_id']
        payload = b'EBC1'+struct.pack('<HHII', 1, 48, len(blob), 0)+hashlib.sha256(blob).digest()
        command = (b'CCM1'+struct.pack('<I', 2)+operation+
                   struct.pack('<HHHBBQIII', 20, 1, 6, 4, 0, 1, 1, 0, 48)+
                   struct.pack('<B7xQ', 9, 0x45434f4e42415345)+payload+
                   struct.pack('<I', 256)+row['canonical_intent'])
        base = 'baseline-'+blob[16:32].hex()+'-'+blob[32:48].hex()+'-'
        files = {'model.eai':row['canonical_intent'], 'model.eap':row['canonical_plan'],
                 'model.ebc':payload, 'model.ccm':command, base+operation.hex()+'.eab':blob}
        indexes = []
        for slot in range(16):
            rows = sorted((item['identity_kind'], item['identity_id'])
                          for item in baseline_projections(row)[2] if item['identity_id'] % 16 == slot)
            body = blob[16:48]+struct.pack('<II', slot, len(rows))
            body += b''.join(struct.pack('<QQ', kind, identity)+operation for kind, identity in rows)
            encoded = self.frame(b'DUREBI1\0', body)
            files[base+format(slot, 'x')+'.ebi'] = encoded
            indexes.append(hashlib.sha256(encoded).digest())
        body = blob[16:48]+blob[80:120]+struct.pack('<Q', 1)+operation+b''.join(indexes)
        files[base+'head.ebc'] = self.frame(b'DUREBC1\0', body)
        return files

    def check(self, label, files, valid, mode='baseline'):
        os.umask(0o077)
        with tempfile.TemporaryDirectory(prefix='duris-baseline-version-') as temporary:
            root = Path(temporary)
            directory = root/'economic-evidence'
            directory.mkdir(mode=0o700)
            for name, data in files.items():
                (directory/name).write_bytes(data)
            def retained():
                return {path.name:(path.read_bytes(), path.stat().st_mode, path.stat().st_nlink)
                        for path in directory.iterdir()}
            before = retained()
            result = subprocess.run([str(self.binary), str(root), mode], capture_output=True, text=True,
                timeout=30, env=dict(os.environ, ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',
                                    UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1'))
            self.assertEqual(retained(), before, label+': independent reader changed evidence')
            evidence_path = self.work/label
            evidence_path.mkdir()
            for name, data in files.items():
                (evidence_path/name).write_bytes(data)
            (evidence_path/'result.json').write_text(json.dumps(dict(exit=result.returncode,
                expected_valid=valid, mode=mode, stdout=result.stdout, stderr=result.stderr,
                bytes_metadata_unchanged=True), indent=2)+'\n')
            self.assertEqual(result.returncode == 0, valid, label+': '+result.stderr)
            self.assertEqual(result.stdout, '')
            self.assertEqual(result.stderr, '' if valid else 'native_restore_qualification_failed\n')

    def test_baseline_exact_versions_equipment_forest_and_reservation_books(self):
        from test_economic_sql_audit_origins import BaselineVersionTests
        for version in (1, 2):
            positions = [(81, (12, 1, 42, 0, 81, 0, 2, 43 if version == 2 else 0)),
                         (82, (1, 1, 7, 0, 82, 0, 3, 65535 if version == 2 else 0))]
            row = BaselineVersionTests.row(version, positions)
            clean = self.files(row)
            self.check('baseline-v'+str(version), clean, True)
            name = next(name for name in clean if name.endswith('.eab'))
            mutations = [(0, b'EAB2' if version == 1 else b'EAB1'), (4, struct.pack('<H', 3)),
                         (202, b'\x01'), (216, struct.pack('<Q', 1)), (208, struct.pack('<Q', 2**64-1)),
                         (184, struct.pack('<I', 3072)), (188, struct.pack('<I', 6001))]
            if version == 2:
                mutations += [(248, struct.pack('<H', 44)), (250, b'\x01'), (256, bytes(32)),
                              (248, struct.pack('<H', 1))]
            for index, (offset, value) in enumerate(mutations):
                files = dict(clean)
                changed = BaselineVersionTests.changed(row, offset, value)
                files[name] = changed['canonical_witness']
                files['model.ebc'] = b'EBC1'+struct.pack('<HHII', 1, 48, len(files[name]), 0)+hashlib.sha256(files[name]).digest()
                self.check('baseline-v'+str(version)+'-invalid-'+str(index), files, False)
            files = dict(clean)
            index_name = next(name for name in clean if name.endswith('-1.ebi'))
            body = bytearray(files[index_name][48:])
            body[56] ^= 1
            files[index_name] = self.frame(b'DUREBI1\0', body)
            head_name = next(name for name in clean if name.endswith('head.ebc'))
            head = bytearray(files[head_name][48:])
            head[128:160] = hashlib.sha256(files[index_name]).digest()
            files[head_name] = self.frame(b'DUREBC1\0', head)
            self.check('baseline-v'+str(version)+'-reservation-resealed', files, False)
        row = BaselineVersionTests.row(2, [(81, (1, 1, 7, 0, 81, 0, 2, 0))])
        clean = self.files(row)
        witness_name = next(name for name in clean if name.endswith('.eab'))
        for case in CASES:
            candidate = BaselineVersionTests.row(2, [(81, position(case))])
            files = dict(clean)
            blob = candidate['canonical_witness']
            files[witness_name] = blob
            files['model.eai'], files['model.eap'] = candidate['canonical_intent'], candidate['canonical_plan']
            files['model.ebc'] = b'EBC1'+struct.pack('<HHII', 1, 48, len(blob), 0)+hashlib.sha256(blob).digest()
            try:
                evidence.forest({81:position(case)})
            except ValueError:
                valid = False
            else:
                valid = case[2] in (1, 2, 3)
            self.check('baseline-v2-grammar-'+case[0], files, valid)
        for version in (1, 2):
            from test_economic_sql_audit_origins import key
            maximum = BaselineVersionTests.row(version,
                [(index+1, (1, 1, 7, 0, index+1, 0, 2, 0)) for index in range(6000)],
                [key(1, index+1) for index in range(3071)])
            self.check('baseline-v'+str(version)+'-maximum', self.files(maximum), True)

    def test_lifecycle_empty_book_accepts_exact_versions_without_item_scope(self):
        from test_economic_sql_audit_origins import BaselineVersionTests, baseline_root
        coverage = hashlib.sha256(b'DURIS-FLATFILE-COVERAGE-V1'+bytes(16)).digest()
        for version in (1, 2):
            row = BaselineVersionTests.row(version, [])
            blob = bytearray(row['canonical_witness'])
            blob[72:80], blob[152:184] = bytes(8), coverage
            row = baseline_root(bytes(blob))
            files = self.files(row)
            creator, epoch = blob[48:64], blob[32:48]
            receipt = (creator+blob[16:48]+struct.pack('<QQ', 7, 1)+bytes(32)+coverage+blob[120:152]+b'\x03'+
                       blob[80:120]+row['operation_id']+struct.pack('<Q', 1)+epoch+struct.pack('<Q', 1)+
                       epoch+bytes(16)+creator+struct.pack('<QH', 1, 1)+coverage+b'\x02'+creator+blob[80:120]+
                       bytes(12))
            for value in (files['model.ccm'], bytes(blob), files['model.eap']):
                receipt += struct.pack('<I', len(value))+value
            name = 'lifecycle-'+creator.hex()+'.elr'
            files[name] = self.frame(b'DURELR\0\0', receipt)
            self.check('lifecycle-v'+str(version), files, True, 'lifecycle')
            # Find the exact retained capsule rather than duplicating variable receipt widths.
            witness_offset = files[name].index(bytes(blob))
            for label, offset, data in (('mixed', 0, b'EAB2' if version == 1 else b'EAB1'),
                                        ('unknown', 4, struct.pack('<H', 3)),
                                        ('items', 188, struct.pack('<I', 1))):
                changed = dict(files)
                encoded = bytearray(files[name])
                encoded[witness_offset+offset:witness_offset+offset+len(data)] = data
                changed[name] = self.frame(b'DURELR\0\0', encoded[48:])
                self.check('lifecycle-v'+str(version)+'-'+label, changed, False, 'lifecycle')
