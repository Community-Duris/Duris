#!/usr/bin/env python3
"""Negative and behavioral checks for the backend-neutral accounting contract."""
import copy
import importlib.util
import json
from pathlib import Path
import unittest
import tempfile

ROOT=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('accounting_contract',ROOT/'scripts/validate_economy_accounting.py')
contract=importlib.util.module_from_spec(spec)
spec.loader.exec_module(contract)
FOLDER=ROOT/'docs/persistence/economy_accounting'

class AccountingContractTest(unittest.TestCase):
    def setUp(self):
        self.registry=json.loads((FOLDER/'registry.json').read_text())
        self.examples={f['id']:f for f in json.loads((FOLDER/'golden.json').read_text())['fixtures']}

    def validate(self,name):
        return contract.validate_fixture(self.examples[name],self.registry)

    def test_all_golden_examples(self):
        contract.validate_registry(self.registry)
        for name in self.examples:
            with self.subTest(name=name): self.validate(name)

    def test_wallet_bank_exact_denominations(self):
        holdings,_=self.validate('wallet_bank')
        self.assertEqual(holdings['wallet']['balance'],[0,1,5,0])
        self.assertEqual(holdings['bank']['balance'],[0,0,5,0])

    def test_staged_settlement_does_not_debit_buyer_twice(self):
        holdings,custody=self.validate('auction_staged')
        self.assertEqual(contract.copper(holdings['buyer']['balance']),2000)
        self.assertEqual(contract.copper(holdings['seller']['balance']),9500)
        self.assertEqual(contract.copper(holdings['escrow']['balance']),0)
        self.assertEqual(contract.copper(holdings['claim']['balance']),0)
        self.assertEqual(custody['701']['identity'],2)

    def test_exact_replay_does_not_repeat_effects(self):
        before=self.validate('expense')
        f=self.examples['expense'];f['operations'].append(copy.deepcopy(f['operations'][0]))
        self.assertEqual(self.validate('expense'),before)

    def test_changed_replay_payload_is_conflict(self):
        f=self.examples['expense'];changed=copy.deepcopy(f['operations'][0]);changed['actor']='operator'
        f['operations'].append(changed)
        with self.assertRaisesRegex(contract.ContractError,'payload conflict'): self.validate('expense')

    def test_fresh_operation_cannot_repeat_reward_source(self):
        f=self.examples['reward'];changed=copy.deepcopy(f['operations'][0]);changed['operation_id']='aa'*16
        f['operations'].append(changed)
        with self.assertRaisesRegex(contract.ContractError,'duplicate source'): self.validate('reward')

    def test_missing_source(self):
        self.examples['reward']['operations'][0]['source_event']=None
        with self.assertRaisesRegex(contract.ContractError,'source event'): self.validate('reward')

    def test_unknown_reason(self):
        self.examples['expense']['operations'][0]['reason']='arbitrary_balance'
        with self.assertRaisesRegex(contract.ContractError,'unknown reason'): self.validate('expense')

    def test_wrong_issuance_sign(self):
        for p in self.examples['reward']['operations'][0]['postings']: p['delta']=[-n for n in p['delta']]
        with self.assertRaisesRegex(contract.ContractError,'issuance sign'): self.validate('reward')

    def test_unbalanced(self):
        self.examples['expense']['operations'][0]['postings'][-1]['delta'][0]+=1
        with self.assertRaisesRegex(contract.ContractError,'unbalanced'): self.validate('expense')

    def test_scalar_balance_does_not_excuse_negative_denomination(self):
        self.examples['expense']['operations'][0]['postings'][0]['delta']=[-5,-2,-1,0]
        with self.assertRaisesRegex(contract.ContractError,'negative holding'): self.validate('expense')

    def test_copper_overflow(self):
        with self.assertRaisesRegex(contract.ContractError,'copper overflow'): contract.copper([0,0,0,2**63-1])
        with self.assertRaisesRegex(contract.ContractError,'denomination overflow'): contract.copper([True,0,0,0])

    def test_duplicate_account_identity(self):
        f=self.examples['wallet_bank'];f['holdings']['alias']=copy.deepcopy(f['holdings']['wallet'])
        with self.assertRaisesRegex(contract.ContractError,'duplicate account'): self.validate('wallet_bank')

    def test_duplicate_registry_id(self):
        self.registry['reasons'].append(copy.deepcopy(self.registry['reasons'][0]))
        with self.assertRaisesRegex(contract.ContractError,'duplicate'): contract.validate_registry(self.registry)

    def test_stale_item_before(self):
        self.examples['same_owner_container_move']['operations'][0]['items'][0]['before']['identity']=99
        with self.assertRaisesRegex(contract.ContractError,'stale item'): self.validate('same_owner_container_move')

    def test_cycle_rejected(self):
        f=self.examples['same_owner_container_move'];f['custody']['501']['parent']=502
        f['custody']['502']['root']=501;f['custody']['502']['parent']=501
        with self.assertRaisesRegex(contract.ContractError,'cyclic'): self.validate('same_owner_container_move')

    def test_containment_cannot_cross_owner_context(self):
        custody = {
            '1': dict(kind='pet', identity=7, context=100, root=1, parent=0),
            '2': dict(kind='pet', identity=7, context=200, root=1, parent=1),
        }
        with self.assertRaisesRegex(contract.ContractError, 'inconsistent owner/root'):
            contract.topology(custody)
        custody['2']['context'] = 100
        contract.topology(custody)

    def test_custody_context_is_bounded_and_defaults_to_zero(self):
        custody = {'1': dict(kind='player', identity=7, root=1, parent=0)}
        contract.topology(custody)
        for context in (-1, 2**64, True, '0'):
            with self.subTest(context=context):
                custody['1']['context'] = context
                with self.assertRaisesRegex(contract.ContractError, 'invalid custody context'):
                    contract.topology(custody)

    def test_multiple_ordered_events_for_same_item(self):
        f=self.examples['same_owner_container_move'];op=f['operations'][0];event=op['items'][0]
        second=copy.deepcopy(event);second.update(event_index=1,before=copy.deepcopy(event['after']),after=copy.deepcopy(event['before']))
        op['items'].append(second)
        _,custody=self.validate('same_owner_container_move')
        self.assertEqual(custody['502'],event['before'])

    def test_unlinked_item_event(self):
        self.examples['same_owner_container_move']['operations'][0]['items'][0]['operation_id']='aa'*16
        with self.assertRaisesRegex(contract.ContractError,'unlinked item'): self.validate('same_owner_container_move')

    def test_duplicate_item_event_index(self):
        f=self.examples['same_owner_container_move'];f['operations'][0]['items']*=2
        with self.assertRaisesRegex(contract.ContractError,'duplicate item event'): self.validate('same_owner_container_move')

    def test_intent_cannot_reduce_existing_payload_capacity(self):
        self.registry['limits']['intent_bytes']=16384
        with self.assertRaisesRegex(contract.ContractError,'intent'): contract.validate_registry(self.registry)

    def test_inventory_rejects_missing_backend_and_dependency(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);(root/'src').mkdir()
            (root/'src/example.c').write_text('void example() {}\n')
            (root/'test.py').write_text('print("synthetic fixture")\n')
            writer=dict(id='example',reason='bank_transfer',owner='test',authority_boundary='test',
                classification='transfer',integration_issue=480,coverage='legacy',path='src/example.c',
                symbol='example',test_candidates=['test.py'],
                backends={name:dict(status='unverified') for name in ('mysql','mariadb','flatfile')})
            inventory=dict(schema_version=1,writers=[writer],census=[],census_complete=False)
            contract.validate_inventory(inventory,self.registry,root)
            missing=copy.deepcopy(inventory);del missing['writers'][0]['backends']['flatfile']
            with self.assertRaisesRegex(contract.ContractError,'missing backend'):
                contract.validate_inventory(missing,self.registry,root)
            writer['integration_issue']=491
            with self.assertRaisesRegex(contract.ContractError,'slice dependency'):
                contract.validate_inventory(inventory,self.registry,root)

    def test_inventory_rejects_stale_source_coordinates(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);(root/'src').mkdir()
            (root/'src/example.c').write_text('void example() { ch->points.cash[index] += amount; }\n')
            writer=dict(id='example',reason='bank_transfer',owner='test',authority_boundary='test',
                classification='transfer',integration_issue=480,coverage='legacy',path='src/example.c',
                symbol='example',test_candidates=[],sites=[['src/example.c',1,'direct_cash_assignment']],
                backends={name:dict(status='unverified') for name in ('mysql','mariadb','flatfile')})
            inventory=dict(schema_version=1,writers=[writer],census=contract.scan_sources(root))
            contract.validate_inventory(inventory,self.registry,root)
            for field,value in ((0,'src/missing.c'),(1,2),(2,'item_lifecycle')):
                stale=copy.deepcopy(inventory);stale['writers'][0]['sites'][0][field]=value
                with self.subTest(field=field), self.assertRaisesRegex(contract.ContractError,'source site'):
                    contract.validate_inventory(stale,self.registry,root)

    def test_unproven_projection_cannot_bypass_release_gate(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);(root/'src').mkdir();(root/'src/example.c').write_text('void example() {}\n')
            writer=dict(id='example',reason='item_move',owner='test',authority_boundary='test',
                classification='hydration',integration_issue=486,coverage='projection',path='src/example.c',
                symbol='example',test_candidates=[],evidence=[],
                backends={name:dict(status='projection') for name in ('mysql','mariadb','flatfile')})
            inventory=dict(schema_version=1,writers=[writer],census=[],census_complete=True)
            self.registry['status']='frozen'
            with self.assertRaisesRegex(contract.ContractError,'executable evidence'):
                contract.validate_inventory(inventory,self.registry,root,release=True)

    def test_census_detects_direct_cash_and_ignores_comments(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);(root/'src').mkdir()
            (root/'src/example.c').write_text('void example() { ch->points.cash[index] += amount; }\n// ADD_MONEY(ch,1,0,0,0);\n')
            sites=contract.scan_sources(root)
            self.assertEqual([(s['line'],s['family']) for s in sites],[(1,'direct_cash_assignment')])

if __name__=='__main__': unittest.main()
