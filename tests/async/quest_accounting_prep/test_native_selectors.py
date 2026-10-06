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
struct object { int vnum; uint64_t obj_uid; object *next_content=nullptr; int type=0; };
struct character { object *carrying=nullptr; };
using P_char=character*; using P_obj=object*;
struct goal_data { int goal_type; int number; goal_data *next=nullptr; };
struct quest_complete_data { goal_data *give=nullptr, *receive=nullptr; };
constexpr int QUEST_GOAL_ITEM=1, QUEST_GOAL_ITEM_TYPE=2, QUEST_GOAL_COINS=3, QUEST_GOAL_SKILL=4, QUEST_GOAL_EXP=5;
enum class state { refused, not_matched, ready, prefix };
std::vector<uint64_t> selected_roots;
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
 mob.carrying=inventory.empty()?nullptr:&inventory[0]; selected_roots.clear();
}
'''


def goals(values):
    types = {'I': 'QUEST_GOAL_ITEM', 'C': 'QUEST_GOAL_COINS', 'E': 'QUEST_GOAL_EXP', 'S': 'QUEST_GOAL_SKILL'}
    return '{' + ','.join('{' + types[kind] + f',{number},nullptr' + '}' for kind, number in reversed(values)) + '}'


def inventory(vnums):
    return '{' + ','.join('{' + f'{vnum},{100 + index},nullptr,0' + '}' for index, vnum in enumerate(vnums)) + '}'


def current_selection():
    function = extract_function('world/quest.c', 'item_native_quest_preparation_state quest_native_completion_owner::prepare_original(')
    start = function.index('\t\t// Preserve the original preliminary duplicate/availability checks.')
    end = function.index('\t\tstd::vector<player_item_snapshot> selected;', start)
    return ('state select(P_char mob,const quest_complete_data *completion) {\n' +
            function[start:end] +
            'selected_roots=ordered_roots; return success ? state::ready : state::prefix;\n}\n')


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
        lines.append('assert(first==state::refused);')
        lines += ['for(int count:{1,2,4}) {',
                  'std::vector<object> paid; for(int i=0;i<count;++i) paid.push_back({19006,static_cast<uint64_t>(200+i),nullptr,0});',
                  'set_inventory(paid); assert(select(&mob,&r3.completion)==state::refused && selected_roots.empty()); }',
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
    return '\n'.join(lines + ['}'])


def run(case_id, acceptance=False):
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
                        str(cpp), '-o', str(executable)], check=True)
        result = subprocess.run([str(executable)], check=False)
        if result.returncode:
            raise AssertionError(f'{case_id}: native selection failed code {result.returncode}; '
                                 '30=paid first branch refuses before supported backpack')
    return dict(case=case_id, mode='acceptance' if acceptance else 'current observation',
                owner='quest_native_completion_owner::prepare_original availability/selection slice',
                result='component passed; constructed NPC stock; native custody/SQL/publication untested',
                source_sha256=digest(ROOT / 'src/world/quest.c'))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--case', choices=('QP01', 'QP02', 'QP05', 'QP06'))
    parser.add_argument('--acceptance', action='store_true', help='QP02 requires reachable supported backpack')
    args = parser.parse_args()
    print(json.dumps([run(k, args.acceptance) for k in
                      ([args.case] if args.case else ('QP01', 'QP02', 'QP05', 'QP06'))], indent=2))
