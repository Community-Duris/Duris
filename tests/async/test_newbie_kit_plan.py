#!/usr/bin/env python3
"""Exhaustive legacy selection parity and pure preparation for issue #160.

The table digest pins the original 5c3ee7afa nanny.c values independently of
this extraction. The oracle models legacy selection rather than calling it.
No game state, files, workers or database participate in C++ plan execution.
"""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import tempfile
from _paths import ROOT

text = (ROOT / 'src/account/newbie_kit_plan.c').read_text()
defines = (ROOT / 'src/core/defines.h').read_text()
values = {name: int(value) for name, value in re.findall(r'^#define (RACE_\w+|LAST_RACE)\s+(\d+)\b', defines, re.M)}
values.update({name: 1 << (int(bit) - 1) for name, bit in re.findall(r'^#define (CLASS_\w+)\s+BIT_(\d+)\b', defines, re.M)})
values['0'] = 0

def parse_tables(source):
    tables = {}
    for race, cls, items in re.findall(r'CREATE_KIT\(\s*(RACE_\w+),\s*(CLASS_\w+|0),\s*PROTECT\(\{([^}]+)\}\)\)', source):
        tables[(values[race], values[cls])] = [int(v) for v in re.findall(r'-?\d+', items) if int(v) != -1]
    extra = {}
    for name in ['thrikreen_good_eq', 'thrikreen_evil_eq', 'minotaur_good_eq', 'minotaur_evil_eq', 'blighter_stuff']:
        match = re.search(r'\b' + name + r'\[\]\s*=\s*\{([^}]+)\}', source)
        assert match, name
        extra[name] = [int(v) for v in re.findall(r'-?\d+', match[1]) if int(v) != -1]
    return tables, extra

def table_digest(tables, extra):
    payload = [sorted((race, cls, items) for (race, cls), items in tables.items()), extra]
    return hashlib.sha256(json.dumps(payload, sort_keys=True, separators=(',', ':')).encode()).hexdigest()

tables, extra = parse_tables(text)
assert table_digest(tables, extra) == '8a5363de3c01471b06dd012039a4c4b688db8ab489826a03b288e47be76499c1', 'legacy kit data changed'

fallback = {
    'PSIONICIST': ('PILLITHID', 'PSIONICIST'), 'MINDFLAYER': ('PILLITHID', 'PSIONICIST'),
    'ASSASSIN': ('HALFELF', 'ASSASSIN'), 'THIEF': ('HALFELF', 'THIEF'),
    'WARLOCK': ('HUMAN', 'ROGUE'), 'BERSERKER': ('ORC', 'BERSERKER'),
    'DREADLORD': ('LICH', 'DREADLORD'), 'AVENGER': ('HUMAN', 'PALADIN'),
    'THEURGIST': ('HALFELF', 'THEURGIST'), 'DRAGOON': ('HUMAN', 'WARRIOR'),
}
fallback = {values['CLASS_' + cls]: (values['RACE_' + race], values['CLASS_' + target]) for cls, (race, target) in fallback.items()}

def oracle(race, cls, align, all_classes, additions):
    items = list(tables.get((race, 0), []))
    for name, symbol in [('thrikreen', 'THRIKREEN'), ('minotaur', 'MINOTAUR')]:
        if race == values['RACE_' + symbol]:
            items += extra[name + ('_good_eq' if align >= 0 else '_evil_eq')]
    class_items = tables.get((race, cls))
    if class_items is None and all_classes:
        class_items = tables.get((values['RACE_HUMAN'], cls))
        if class_items is None and cls in fallback:
            class_items = tables.get(fallback[cls])
    if class_items is not None:
        items += class_items
    elif cls == values['CLASS_BLIGHTER']:
        items += extra['blighter_stuff']
    result = [(v, 1) for v in items]
    if additions:
        result += [(29319, 0)] + [(393, 0)] * 4 + [(458, 0)]
    return result

def digest(items):
    value = 14695981039346656037
    for vnum, regular in items:
        for part in (vnum, regular):
            value = ((value ^ part) * 1099511628211) & ((1 << 64) - 1)
    return value

HARNESS = r'''
#include "account/newbie_kit_plan.h"
#include "core/defines.h"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <algorithm>
int main() {
    const auto coverage = newbie_kit_template_vnums();
    for (int race = 0; race <= LAST_RACE; ++race)
    for (int cls = 0; cls < CLASS_COUNT; ++cls)
    for (int align : {-1, 0, 1})
    for (bool all : {false, true})
    for (bool additions : {false, true}) {
        newbie_kit_input input{race, align, 1 << cls, all,
            (1U << cls) == CLASS_BLIGHTER, additions, additions, additions};
        const auto plan = make_newbie_kit_plan(input);
        uint64_t hash = 14695981039346656037ULL;
        for (const auto &item : plan) {
            assert(std::binary_search(coverage.begin(), coverage.end(), item.vnum));
            hash = (hash ^ item.vnum) * 1099511628211ULL;
            hash = (hash ^ item.regular) * 1099511628211ULL;
        }
        std::cout << hash << '\n';
    }
    std::vector<newbie_kit_item> selection{{1,true},{2,true},{3,true},{4,false},{2,true}};
    std::vector<newbie_item_facts> facts{{true,false,false},{true,true,true},
        {false,true,false},{true,false,true},{true,true,true}};
    std::vector<int> spells{10,17,99};
    const auto prepared = prepare_newbie_kit_items(selection, facts, spells);
    assert(prepared.size() == 3); // rejection and unavailable item omitted, duplicates preserved
    assert(prepared[0].item.vnum == 2 && prepared[0].spells == spells);
    assert(prepared[1].item.vnum == 4 && prepared[1].spells.empty());
    assert(prepared[2].item.vnum == 2 && prepared[2].spells == spells);
    spells.clear(); facts.clear(); selection.clear();
    assert(prepared[0].spells.size() == 3); // job-local ownership
    assert(prepare_newbie_kit_items({{1,true}}, {}, {}).empty());
    newbie_kit_input bad; bad.race = -1;
    assert(make_newbie_kit_plan(bad).empty());
    bad.race = LAST_RACE + 1;
    assert(make_newbie_kit_plan(bad).empty());
}
'''

# Integration boundaries: bounded main-thread capture/publication, no cold fallback.
nanny = (ROOT / 'src/account/nanny.c').read_text()
loader = nanny.split('void load_obj_to_newbies(P_char ch)', 1)[1].split('/* check for a legal player name', 1)[0]
assert 'read_object(' not in loader
assert loader.index('item_movement_transaction_player_busy(ch)') < loader.index('make_newbie_kit_plan(input)')
assert loader.index('prepare_newbie_kit_items(') < loader.index('instantiate_object_template(')
assert loader.index('add_newbie_keyword(obj)') < loader.index('obj_to_char(obj, ch)')
assert loader.index('obj_to_char(obj, ch)') < loader.index('item_creation_grant_mark_blocking(ch)')
assert 'P_char' not in text and 'P_obj' not in text and 'object_list' not in text
boot = (ROOT / 'src/world/db.c').read_text().split('void boot_db(', 1)[1]
assert boot.index('cache_object_template(vnum)') > boot.index('dead_obj_pool = mm_create')
assert boot.index('cache_object_template(vnum)') > boot.index('assign_objects()')

if __name__ == '__main__':
    with tempfile.TemporaryDirectory(prefix='newbie-plan-') as tmp:
        source, binary = Path(tmp) / 'plan.cpp', Path(tmp) / 'plan'
        source.write_text(HARNESS)
        subprocess.run(['g++', '-std=c++20', '-Wall', '-Wextra', '-Werror',
            '-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-no-pie',
            '-I', str(ROOT / 'src'), str(source), str(ROOT / 'src/account/newbie_kit_plan.c'),
            '-o', str(binary)], check=True)
        actual = subprocess.check_output([str(binary)], text=True).splitlines()
    expected = [str(digest(oracle(race, 1 << cls, align, all_classes, additions)))
        for race in range(values['LAST_RACE'] + 1) for cls in range(30)
        for align in [-1, 0, 1] for all_classes in [False, True] for additions in [False, True]]
    assert actual == expected, next((i, got, want) for i, (got, want) in enumerate(zip(actual, expected)) if got != want)
    print(f'newbie plan parity: {len(expected)} selections; {len(tables)} legacy table cells; pure preparation passed')
