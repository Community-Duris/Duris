#!/usr/bin/env python3
"""Compile production accounting primitives and execute golden/property tests."""
import copy
import json
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[2]
FOLDER=ROOT/'docs/persistence/economy_accounting'

def vector(value):
    return '{'+','.join(str(n) for n in value)+'}'

def cpp_position(state,revision,uid,destroyed=False,prior=None):
    if destroyed:
        if prior is None: raise ValueError('destruction needs retained topology')
        return '{{item_owner_type::destruction,0,0},'+f"{prior['root']},{prior['parent']},{revision},item_custody_state::destroyed"+'}'
    if state is None:
        return '{}'
    owner='{'+f"item_owner_type::{state['kind']},{state['identity']},{state.get('context',0)}"+'}'
    return '{'+owner+f",{state['root']},{state['parent']},{revision},item_custody_state::active"+'}'

def goldens(include_plans=False):
    registry=json.loads((FOLDER/'registry.json').read_text())
    kinds={r['id']:r for r in registry['account_kinds']}
    output=['void golden_cases() {']
    for row in registry['account_kinds']:
        output.append(f"static_assert(static_cast<unsigned>(economic_account_kind::{row['id']}) == {row['number']});")
    if include_plans:
        for row in registry['source_kinds']:
            output.append(f"static_assert(static_cast<unsigned>(economic_source_kind::{row['id']}) == {row['number']});")
        for row in registry['reasons']:
            output.append(f"static_assert(static_cast<unsigned>(economic_reason::{row['id']}) == {row['number']});")
    for fixture in json.loads((FOLDER/'golden.json').read_text())['fixtures']:
        holdings=copy.deepcopy(fixture['holdings'])
        custody={int(uid):cpp_position(state,1,int(uid)) for uid,state in fixture['custody'].items()}
        revisions={uid:1 for uid in custody}
        for operation in fixture['operations']:
            for event in operation['items']:
                custody.setdefault(event['uid'],'{}')
                revisions.setdefault(event['uid'],0)
        for operation in fixture['operations']:
            names=sorted({p['account'] for p in operation['postings']},key=lambda name:(kinds[holdings[name]['kind']]['number'],holdings[name]['identity'],holdings[name].get('context',0)))
            after={name:list(holdings[name]['balance']) for name in names}
            for posting in operation['postings']:
                name=posting['account']
                if kinds[holdings[name]['kind']]['sign']=='ordinary':
                    after[name]=[a+b for a,b in zip(after[name],posting['delta'])]
            output.append('{ std::vector<economic_account_effect> effects; std::vector<economic_coin_posting> postings;')
            for name in names:
                account=holdings[name];ordinary=kinds[account['kind']]['sign']=='ordinary'
                before=account['balance'] if ordinary else [0]*4
                current=after[name] if ordinary else [0]*4
                revision=1 if before!=current else 0
                output.append(f"effects.push_back({{key(economic_account_kind::{account['kind']}, {account['identity']}, {account.get('context',0)}), {vector(before)}, {vector(current)}, 0, {revision}}});")
                if ordinary: account['balance']=current
            for index,posting in enumerate(operation['postings']):
                value=sum(a*b for a,b in zip(posting['delta'],[1,10,100,1000]))
                output.append(f"postings.push_back({{{index},{names.index(posting['account'])},0,{vector(posting['delta'])},{value}}});")
            output.append('CHECK(economic_coin_effects_validate(effects, postings, 0) == error::ok);')
            output.append('std::vector<economic_item_snapshot> items_before,items_after; std::vector<economic_item_event> events;')
            for uid,state in sorted(custody.items()):
                output.append(f'items_before.push_back({{{uid},{state}}});')
            for event in operation['items']:
                uid=event['uid']
                initial=cpp_position(event['before'],revisions[uid],uid)
                revisions[uid]+=1
                final=cpp_position(event['after'],revisions[uid],uid,destroyed=event['after'] is None,prior=event['before'])
                output.append(f"events.push_back({{{event['event_index']},0,{uid},{initial},{final}}});")
                custody[uid]=final
            for uid,state in sorted(custody.items()):
                output.append(f'items_after.push_back({{{uid},{state}}});')
            output.append('CHECK(economic_item_effects_validate(items_before,items_after,events,0) == error::ok);')
            if include_plans:
                actor='operator_action' if operation['actor']=='operator' else 'domain'
                output.append(f"auto plan=base_plan(); plan.metadata.reason=economic_reason::{operation['reason']}; plan.metadata.actor_kind=economic_actor_kind::{actor};")
                output.append('plan.accounts=effects;plan.postings=postings;plan.items_before=items_before;plan.items_after=items_after;plan.item_events=events;')
                if operation['source_event']:
                    source_kind={'baseline':'baseline','quest_reward':'quest_completion','first_admission':'world_generation','item_destroy':'lifecycle'}.get(operation['reason'],'service')
                    output.append(f'plan.metadata.source_event=source_event(economic_source_kind::{source_kind});')
                output.append('roundtrip(plan);')
            output.append('}')
    output.append('}')
    return '\n'.join(output)+'\n'

def main():
    work=ROOT/'bin/tests/economic-accounting-types';work.mkdir(parents=True,exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='run-',dir=work) as temporary:
        temp=Path(temporary)
        (temp/'golden.inc').write_text(goldens())
        executable=temp/'accounting-types'
        command=shlex.split(os.environ.get('CXX','g++'))+[
            '-std=c++20','-Wall','-Wextra','-Wpedantic','-Werror','-O1','-g',
            '-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie',
            '-I'+str(ROOT/'src'),'-I'+str(temp),
            str(ROOT/'tests/async/economic_accounting_types_test.cpp'),
            str(ROOT/'src/economy/economic_accounting_types.c'),
            str(ROOT/'src/persistence/critical_command.c'),str(ROOT/'src/item/item_transfer_command.c'),'-lcrypto','-o',str(executable)]
        subprocess.run(command,check=True)
        environment=dict(os.environ,ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
        subprocess.run([str(executable)],check=True,env=environment)

if __name__=='__main__': main()
