#!/usr/bin/env python3
"""Reproduce the tracked AREA artifact boundary; this is not a live loot roster."""
import argparse
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]
PILOTS = {21: 'Mayhem', 22: 'Symmetry', 922: 'mirrored ioun',
          19730: 'Avernus', 31514: 'Tsunami', 41350: 'wand of wonder',
          67243: 'living necroplasm'}
SLOT = r'obj_index\s*\[\s*real_object0\s*\(\s*(\d+)\s*\)\s*\]\s*\.func\.obj'


def source_templates():
    paths = {p.name.lower(): p for p in (ROOT/'areas/obj').glob('*.obj')}
    rows = []
    for area in (ROOT/'areas/AREA').read_text().splitlines():
        if not area.strip() or area.startswith('*'):
            continue
        path = paths[area.split()[0].lower()+'.obj']
        text = path.read_text(encoding='latin1')
        for match in re.finditer(r'^#(\d+)\s*\n', text, re.M):
            if int(match[1]) == 9999999:
                continue # Index sentinel, not an object template.
            start = match.end()
            strings = []
            for _ in range(4):
                end = text.find('~', start)
                if end < 0:
                    raise ValueError(f'Incomplete template strings: {path}:{match[1]}')
                strings.append(text[start:end].strip())
                start = end+1
            numeric = re.match(r'\s*((?:[-+]?\d+\s+){21}[-+]?\d+)', text[start:])
            if not numeric:
                raise ValueError(f'Invalid numeric template: {path}:{match[1]}')
            values = list(map(int, numeric[1].split()))
            if not values[6] & (1 << 28):
                continue
            rows.append(dict(vnum=int(match[1]), source=path.relative_to(ROOT).as_posix(),
                line=text.count('\n', 0, match.start())+1,
                name=re.sub(r'&(?:[+\-].|=..|[nN])', '', strings[1]), keywords=strings[0],
                type=values[0], extraFlags=values[6], wearFlags=values[7],
                extra2Flags=values[8], values=values[11:19]))
    duplicates = {r['vnum'] for r in rows if sum(x['vnum']==r['vnum'] for x in rows)>1}
    if duplicates:
        raise ValueError(f'Artifact template duplicates need index review: {sorted(duplicates)}')
    return sorted(rows, key=lambda r:r['vnum'])


def bindings():
    path = ROOT/'src/specs/specs.assign.c'
    raw = re.sub(r'/\*.*?\*/|//[^\n]*', '', path.read_text(), flags=re.S)
    literal = {int(v) for v in re.findall(SLOT, raw)}
    # Use the compiler's selected configuration, including THARKUN_ARTIS and
    # disabled branches. Resolve chained assignments in their execution order.
    pre = subprocess.run(['g++', '-E', '-P', '-x', 'c++', '-std=c++20',
        '-I'+str(ROOT/'src'), str(path)], check=True, text=True, capture_output=True).stdout
    pre = pre[pre.index('void assign_objects(void)\n{'):]
    result = {}
    for statement in pre.split(';'):
        targets = re.findall(SLOT, statement)
        callback = re.search(r'=\s*(\w+)\s*$', statement)
        if targets and callback:
            for vnum in targets:
                if int(vnum) in literal:
                    result[int(vnum)] = callback[1]
    return result


def inventory():
    bound = bindings()
    rows = source_templates()
    for row in rows:
        vnum = row['vnum']
        row['literalCallback'] = bound.get(vnum)
        row['placeholder'] = bool(re.search(r'golden.*token|token.*golden',
            row['name']+' '+row['keywords'], re.I))
        paths = ['base stats/affect bitvectors', 'equipment enchantments',
                 'Studio trigger/proc-library dynamic assignment']
        if row['type'] == 5:
            paths.append('weapon_proc: positive value[5] and value[7] select packed spell path')
        if row['type'] in (2, 3, 4):
            paths.append('do_recite/do_use: native scroll/wand/staff dispatch')
        if row['literalCallback']:
            paths.append('effective native callback and helper continuations')
        row['reviewPaths'] = paths
        row['disposition'] = ('placeholder; preserve source record; no invented ability' if row['placeholder'] else
            'default-off pilot: '+PILOTS[vnum] if vnum in PILOTS else
            'retain legacy; outside initial migration wave; requires individual contract and measured budget')
    return dict(corpus='Tracked areas/AREA in file order; artifact flag in object template extra_flags',
        liveAvailabilityVerified=False, configuration='Compiler defaults; THARKUN_ARTIS enabled',
        counts=dict(templates=len(rows), literalBindings=sum(bool(r['literalCallback']) for r in rows),
            callbacks=len({r['literalCallback'] for r in rows if r['literalCallback']}),
            withoutLiteralBinding=sum(not r['literalCallback'] for r in rows),
            goldenTokenPlaceholders=sum(r['placeholder'] for r in rows)), templates=rows)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    result = inventory()
    text = json.dumps(result, indent=2, ensure_ascii=False)+'\n'
    target = ROOT/'docs/reference/artifact_source_inventory.json'
    if args.check:
        if target.read_text(encoding='utf-8') != text:
            raise SystemExit('Artifact source inventory is stale; regenerate and review the change')
    else:
        target.write_text(text, encoding='utf-8')
    print(json.dumps(result['counts']))


if __name__ == '__main__':
    main()
