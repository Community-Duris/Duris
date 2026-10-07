#!/usr/bin/env python3
"""Magma exact heart, three-output exchange, dispersal and source ownership contracts."""
import collections
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
import zone_story_quest_catalog as cat
import zone_story_quest_zone_inventory as inv


def bodies(area, kind):
    text = (ROOT / 'areas' / kind / (area + '.' + kind)).read_text()
    return {int(m[1]): m[2] for m in re.finditer(r'^#(\d+)\s*\n([\s\S]*?)(?=^#\d+\s*$|\Z)', text, re.M)}


def properties(body):
    rows = body.split('~', 4)[4].strip().splitlines()
    return list(map(int, rows[0].split())), list(map(int, rows[1].split()))


def edges(rooms):
    return [(v, int(m[1]), int(m[4]), int(m[5]), int(m[6])) for v, body in rooms.items()
            for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)', body, re.S)]


e = inv.area_evidence(ROOT, 'magma')
catalog = cat.production_catalog(ROOT)
mapping = next(m for m in catalog['story_mappings'] if m['source_area'] == 'magma')
assert (mapping['schema_version'], mapping['revision'], mapping['coverage']) == (3, 1, 'complete')
assert len(mapping['stories']) == 1 and len(mapping['contacts']) == 19 and not mapping['exclusions']
assert len(e['requests']) == 1 and len(e['dialogue']) == 2
q = e['requests'][0]['block']
assert (q['line'], q['giver_vnum'], q['give'], q['receive'], q['disappear']) == (34, 142014, [('I', 142000)], [('I', 142002), ('I', 142003), ('I', 142004)], True)
assert not e['requests'][0]['definition']['prerequisites']
assert [(d['kind'], d['line']) for d in e['dialogue']] == [('MA', 2), ('M', 15)]
story = mapping['stories'][0]
assert story['id'] == 'palenian-drake-heart' and story['contracts'] == [q['binding']]
prep, receipt = story['steps']
assert (prep['kind'], prep['item_vnums'], prep['count'], prep['optional']) == ('carried_item', [142000], 1, True)
assert receipt['kind'] == 'completion' and receipt['contracts'] == story['contracts']
assert sum(len(c['topics']) for c in mapping['contacts']) == 2
rooms, mobs, objects = (bodies('magma', kind) for kind in ('wld', 'mob', 'obj'))
assert (len(rooms), len(mobs), len(objects)) == (129, 19, 5)
assert set(rooms) == set(range(142000, 142129))
assert {c['mob_vnum'] for c in mapping['contacts']} == set(mobs)
for contact in mapping['contacts']:
    assert contact['keyword'] in mobs[contact['mob_vnum']].split('~')[0].split()
palenian = next(c for c in mapping['contacts'] if c['mob_vnum'] == 142014)
assert palenian['keyword'] == 'plaenian' and palenian['topics'] == ['hi', 'task']
assert (e['zone']['zone_number'], e['zone']['last_vnum'], e['zone']['reset_mode']) == (1420, 142128, 0)
assert len(e['reset_commands']) == 19
assert collections.Counter(r['command'] for r in e['reset_commands']) == {'M': 9, 'F': 9, 'G': 1}
resets = {r['line']: (r['command'], r['arguments'][:5]) for r in e['reset_commands']}
for line, command, args in [(12, 'M', [0, 142002, 1, 142001, 100]), (13, 'M', [0, 142017, 1, 142125, 100]),
                            (18, 'M', [0, 142009, 2, 142127, 100]), (19, 'M', [0, 142014, 1, 142127, 100]),
                            (20, 'M', [0, 142018, 1, 142127, 100]), (21, 'G', [1, 142000, 1, 0, 100]),
                            (22, 'M', [0, 142009, 2, 142128, 100])]:
    assert resets[line] == (command, args)
producers = []
for path in (ROOT / 'areas/zon').glob('*.zon'):
    for line in path.read_text().splitlines():
        if re.match(r'[OPGE]\s+-?\d+\s+142000\s', line):
            producers.append((path.stem, line.split()[:6]))
assert producers == [('magma', ['G', '1', '142000', '1', '0', '100'])]
heart, values = properties(objects[142000])
assert heart[0] == 8 and heart[7] == 16385 and heart[8] & 32768
assert properties(objects[142002])[0][0] == 3 and properties(objects[142002])[1][:4] == [58, 4, 4, 394]
assert properties(objects[142003])[0][0] == 9 and properties(objects[142004])[0][0] == 9
assert not any(properties(body)[0][0] in {25, 29} or re.search(r'^T\s*$', body, re.M) or '_proclib_' in body for body in objects.values())
graph = edges(rooms)
assert len(graph) == 612 and all(flags == 0 and key == 0 for _, _, flags, key, _ in graph)
grid = set(range(142001, 142126))
connections = {(a, d, b) for a, d, flags, key, b in graph if a in grid}
assert len(connections) == 600 and all(b in grid for a, d, b in connections)
reverse = {0: 2, 1: 3, 2: 0, 3: 1, 4: 5, 5: 4}
assert all((b, reverse[d], a) in connections for a, d, b in connections)
seen = {142001}
while True:
    more = seen | {b for a, d, b in connections if a in seen}
    if more == seen:
        break
    seen = more
assert seen == grid
assert {(d, b) for a, d, flags, key, b in graph if a == 142127} == {(0, 142126), (1, 142021), (2, 142126), (3, 142126), (4, 142021), (5, 142126), (6, 142126), (8, 142126)}
assert {(d, b) for a, d, flags, key, b in graph if a == 142128} == {(0, 142126), (1, 142126), (2, 142009), (3, 142126)}
assert not any(a in {142000, 142126} for a, d, flags, key, b in graph)
assert all(list(map(int, rooms[v].split('~', 2)[2].strip().splitlines()[0].split())) == [1420, 33820736, 0, 0] for v in grid)
assert not (ROOT / 'areas/shp/magma.shp').exists()
assert all(s['vnum'] not in rooms for s in e['special_assignments'])
assert [(s['vnum'], s['function']) for s in e['special_assignments']] == [(140854, 'ship_shop_proc')]
assert cat.report_for(catalog)['eligible_by_zone']['1420'] == 1
ids = set(re.findall(r'ZSQ-MAGMA-\d{2}', (ROOT / 'docs/design/zone-stories/MAGMA.md').read_text()))
assert ids == {'ZSQ-MAGMA-%02d' % i for i in range(1, 25)}
print('Magma exact heart and three-output exchange, conditional dispersal, grid sectors and effective source ownership passed')
