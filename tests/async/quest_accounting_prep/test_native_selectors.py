#!/usr/bin/env python3
"""Execute current native availability/selection with production Q terms.

Only the exact bounded slice after reference/stock observation and before
custody/payload capture executes. Constructed NPC holdings prove selection,
not transfer, SQL admission, rewards, ACK or retirement.
"""
import argparse
import json
from pathlib import Path
import subprocess
import tempfile

from case_data import ROOT, blocks, digest
from _paths import extract_function

PRELUDE = r'''
#include <cassert>
#include <cstdint>
#include <cstddef>
#include <vector>
#include <unordered_set>
#include "world/quest_mobile_native_binding.h"
#include "world/native_quest_recovery_context.h"
struct obj_data { int vnum; uint64_t obj_uid; obj_data *next_content=nullptr; int type=0; };
using object=obj_data;
struct char_data { object *carrying=nullptr; uint64_t runtime_id=1; };
using character=char_data;
using P_char=character*; using P_obj=object*;
struct goal_data { int goal_type; int number; goal_data *next=nullptr; };
struct quest_complete_data { goal_data *give=nullptr, *receive=nullptr; };
constexpr int QUEST_GOAL_ITEM=1, QUEST_GOAL_ITEM_TYPE=2, QUEST_GOAL_COINS=3, QUEST_GOAL_SKILL=4, QUEST_GOAL_EXP=5;
enum class state { refused, not_matched, ready, prefix };
std::vector<uint64_t> selected_roots;
std::vector<native_quest_cost_requirement> selected_costs;
std::array<int64_t,4> original_cash{};
bool cash_observation_available=true;
bool quest_mobile_native_cash_reference_copy(const char_data *mob, uint64_t runtime,
                                           quest_mobile_native_cash_reference *output) noexcept {
 if(!mob || runtime!=mob->runtime_id || !output || !cash_observation_available) return false;
 quest_mobile_native_cash_reference candidate{};
 candidate.denominations=original_cash; candidate.cash_revision=31;
 *output=candidate; return true;
}
#define OBJ_VNUM(obj) ((obj)->vnum)
'''
POSTLUDE = r'''
struct recipe {
 std::vector<goal_data> give,receive; quest_complete_data completion;
 recipe(std::vector<goal_data> g,std::vector<goal_data> r):give(g),receive(r) {
  for(size_t i=0;i<give.size();++i) give[i].next=i+1<give.size()?&give[i+1]:nullptr;
  for(size_t i=0;i<receive.size();++i) receive[i].next=i+1<receive.size()?&receive[i+1]:nullptr;
  completion.give=give.empty()?nullptr:&give[0]; completion.receive=receive.empty()?nullptr:&receive[0];
 }
};
character mob;
void set_inventory(std::vector<object>& inventory) {
 for(size_t i=0;i<inventory.size();++i) inventory[i].next_content=i+1<inventory.size()?&inventory[i+1]:nullptr;
 mob.carrying=inventory.empty()?nullptr:&inventory[0]; selected_roots.clear(); selected_costs.clear();
}
'''


def goals(values):
    types = {'I': 'QUEST_GOAL_ITEM', 'C': 'QUEST_GOAL_COINS', 'E': 'QUEST_GOAL_EXP', 'S': 'QUEST_GOAL_SKILL'}
    return '{' + ','.join('{' + types[kind] + f',{number},nullptr' + '}' for kind, number in reversed(values)) + '}'


def inventory(vnums):
    return '{' + ','.join('{' + f'{vnum},{100 + index},nullptr,0' + '}' for index, vnum in enumerate(vnums)) + '}'


def current_selection():
    function = extract_function('world/quest.c', 'item_native_quest_preparation_state quest_native_completion_owner::prepare_original(')
    start = function.index('\t\tquest_mobile_native_cash_reference cash;')
    end = function.index('\t\tstd::vector<player_item_snapshot> selected;', start)
    return ('state select(P_char mob,const quest_complete_data *completion, '
            'const critical_operation_id *original_child=nullptr, '
            'const critical_command *triggering_acceptance=nullptr, '
            'const native_quest_recovery_receipt *triggering_receipt=nullptr) {\n' +
            function[start:end] +
            '(void)fee_operation; selected_roots=ordered_roots; selected_costs=attempted_costs; '
            'return success ? state::ready : state::prefix;\n}\n')


def main_body(case_id, acceptance):
    terms = blocks(case_id)
    lines = ['int main() {']
    for index, block in enumerate(terms):
        lines.append(f"recipe r{index}({goals(block['give'])},{goals(block['receive'])});")
    if case_id == 'QP02':
        lines += [f'std::vector<object> items={inventory([19006] * 3)}; set_inventory(items);',
                  'const auto first=select(&mob,&r3.completion);',
                  'assert(selected_roots.empty());',
                  'assert(select(&mob,&r2.completion)==state::ready && selected_roots.size()==3);',
                  'assert((selected_roots==std::vector<uint64_t>{100,101,102}));']
        if acceptance:
            lines += ['if(first!=state::not_matched) return 30;', 'return 0;', '}']
            return '\n'.join(lines)
        # Zero constructed original cash selects the actual preliminary shortage;
        # current COINS support does not mean an unsupported-kind refusal.
        lines.append('assert(first==state::not_matched);')
        lines += ['for(int count:{1,2,4}) {',
                  'std::vector<object> paid; for(int i=0;i<count;++i) paid.push_back({19006,static_cast<uint64_t>(200+i),nullptr,0});',
                  'set_inventory(paid); assert(select(&mob,&r3.completion)==state::not_matched && selected_roots.empty()); }',
                  f'std::vector<object> short_items={inventory([19006] * 2)}; set_inventory(short_items);',
                  'assert(select(&mob,&r2.completion)==state::not_matched && selected_roots.empty());']
    else:
        for index, block in enumerate(terms):
            inputs = [number for kind, number in block['give'] if kind == 'I']
            remaining = list(enumerate(inputs + [inputs[0]]))
            ordered = []
            for vnum in reversed(inputs):
                position, _ = next(row for row in remaining if row[1] == vnum)
                ordered.append(100 + position)
                remaining = [row for row in remaining if row[0] != position]
            expected = ','.join(map(str, ordered))
            lines += ['{', f'std::vector<object> items={inventory(inputs + [inputs[0]])}; set_inventory(items);',
                      f'assert(select(&mob,&r{index}.completion)==state::ready && selected_roots.size()=={len(inputs)});',
                      f'assert((selected_roots==std::vector<uint64_t>{{{expected}}}));',
                      'for(size_t i=0;i<selected_roots.size();++i) {',
                      'for(size_t j=0;j<i;++j) assert(selected_roots[i]!=selected_roots[j]); }',
                      f'for(uint64_t uid:selected_roots) assert(uid!={100+len(inputs)});',
                      '}', '{', f'std::vector<object> short_items={inventory(inputs[:-1])}; set_inventory(short_items);',
                      f'assert(select(&mob,&r{index}.completion)==state::not_matched && selected_roots.empty());', '}']
            if case_id == 'QP01':
                lines += ['{', f'std::vector<object> wrong={inventory([43703,43703,43703,44164])}; set_inventory(wrong);',
                          f'assert(select(&mob,&r{index}.completion)==state::not_matched && selected_roots.empty());', '}']
    lines += [
        '{',
        'recipe overlap({{QUEST_GOAL_ITEM_TYPE,7,nullptr},{QUEST_GOAL_ITEM,10,nullptr}},{});',
        'std::vector<object> items={{10,100,nullptr,7}}; set_inventory(items);',
        'assert(select(&mob,&overlap.completion)==state::prefix);',
        'assert((selected_roots==std::vector<uint64_t>{100}));',
        '}', '{',
        'recipe fees({{QUEST_GOAL_COINS,60,nullptr},{QUEST_GOAL_COINS,60,nullptr},{QUEST_GOAL_ITEM,10,nullptr}},{});',
        'original_cash={100,0,0,0}; std::vector<object> items={{10,100,nullptr,7}}; set_inventory(items);',
        'assert(select(&mob,&fees.completion)==state::ready);',
        'assert((selected_costs==std::vector<native_quest_cost_requirement>{{0,60},{1,60}}));',
        'native_quest_cost_projection cost;',
        'assert(native_quest_cost_project(original_cash,31,selected_costs,&cost)==native_quest_cost_projection_result::ok);',
        'assert(cost.attempts.size()==2 && cost.attempts[0].outcome==native_quest_cost_attempt_outcome::charged);',
        'assert(cost.attempts[1].outcome==native_quest_cost_attempt_outcome::insufficient);',
        'assert(cost.after[0]==40 && cost.after_revision==32);',
        'original_cash={59,0,0,0}; set_inventory(items);',
        'assert(select(&mob,&fees.completion)==state::not_matched && selected_roots.empty());',
        'original_cash={100,0,0,0}; cash_observation_available=false; set_inventory(items);',
        'assert(select(&mob,&fees.completion)==state::refused && selected_roots.empty());',
        'cash_observation_available=true; original_cash={};',
        '}',
    ]
    return '\n'.join(lines + ['}'])


def run(case_id, acceptance=False, cost_objects=(), library_paths=()):
    source = (ROOT / 'src/world/quest.c').read_text()
    pulse = extract_function('world/quest.c', 'void quest_native_gameplay_owner::pulse(')
    decision = pulse[pulse.index('if (phase == native_quest_gameplay_phase::choose_branch)'):]
    assert 'if (prepared == item_native_quest_preparation_state::not_matched)' in decision
    refusal = decision[decision.index('if (prepared == item_native_quest_preparation_state::refused)'):]
    assert refusal.split('}', 1)[0].count('native_quest_gameplay_phase::blocked') == 1
    dispatch = source[source.index('if (cmd == CMD_GIVE)'):source.index('if (cmd == CMD_GIVE)')+2500]
    assert dispatch.index('submit_native_quest_give') < dispatch.index('submit_durable_quest_offering')
    program = PRELUDE + current_selection() + POSTLUDE + main_body(case_id, acceptance)
    build_root = ROOT / 'bin/tests'
    build_root.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='quest-prep-selector-', dir=build_root) as directory:
        cpp, executable = Path(directory) / 'selector.cpp', Path(directory) / 'selector'
        cpp.write_text(program)
        subprocess.run(['g++', '-std=c++20', '-O0', '-Wall', '-Wextra', '-Werror',
                        '-I'+str(ROOT/'src'), '-ffunction-sections', '-fdata-sections',
                        '-no-pie', str(cpp), *cost_objects, '-Wl,--gc-sections',
                        *['-L'+path for path in library_paths], '-lcrypto',
                        '-o', str(executable)], check=True, timeout=300)
        result = subprocess.run([str(executable)], check=False, timeout=60)
        if result.returncode:
            raise AssertionError(f'{case_id}: native selection failed code {result.returncode}; '
                                 '30=paid first branch failed the authentic not-matched shortage contract')
    return dict(case=case_id, mode='acceptance' if acceptance else 'current observation',
                owner='quest_native_completion_owner::prepare_original availability/selection slice',
                result='component passed; constructed NPC stock; native custody/SQL/publication untested',
                source_sha256=digest(ROOT / 'src/world/quest.c'))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--case', choices=('QP01', 'QP02', 'QP05', 'QP06'))
    parser.add_argument('--acceptance', action='store_true', help='QP02 requires reachable supported backpack')
    parser.add_argument('--cost-object', action='append', default=[], required=True,
                        help='Real ordinary production native_cost/critical_command object; repeat as needed')
    parser.add_argument('--library-path', action='append', default=[])
    args = parser.parse_args()
    print(json.dumps([run(k, args.acceptance, args.cost_object, args.library_path) for k in
                      ([args.case] if args.case else ('QP01', 'QP02', 'QP05', 'QP06'))], indent=2))
