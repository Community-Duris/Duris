#!/usr/bin/env python3
"""Killing Fields exact note, true container source, service and access qualification."""
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


e = inv.area_evidence(ROOT, 'killing_fields')
catalog = cat.production_catalog(ROOT)
mapping = next(m for m in catalog['story_mappings'] if m['source_area'] == 'killing_fields')
assert (mapping['schema_version'], mapping['revision'], mapping['coverage']) == (3, 1, 'complete')
assert len(mapping['stories']) == 1 and len(mapping['contacts']) == 9 and not mapping['exclusions']
assert len(e['requests']) == 1 and len(e['dialogue']) == 2
q = e['requests'][0]['block']
assert (q['line'], q['giver_vnum'], q['give'], q['receive'], q['disappear']) == (14, 33300, [('I', 33302)], [('C', 45000)], True)
assert not e['requests'][0]['definition']['prerequisites']
story = mapping['stories'][0]
assert story['id'] == 'maelron-missing-message' and story['contracts'] == [q['binding']]
prep, receipt = story['steps']
assert (prep['kind'], prep['item_vnums'], prep['count'], prep['optional']) == ('carried_item', [33302], 1, True)
assert receipt['kind'] == 'completion' and receipt['contracts'] == story['contracts']
assert sum(len(c['topics']) for c in mapping['contacts']) == 3
assert next(c for c in mapping['contacts'] if c['mob_vnum'] == 33300)['topics'] == ['hi', 'hello', 'friend']
rooms, mobs, objects = (bodies('killing_fields', kind) for kind in ('wld', 'mob', 'obj'))
assert (len(rooms), len(mobs), len(objects)) == (198, 9, 15)
assert set(rooms) == set(range(33300, 33498))
for contact in mapping['contacts']:
    assert contact['keyword'] in mobs[contact['mob_vnum']].split('~')[0].split()
assert (e['zone']['zone_number'], e['zone']['last_vnum'], e['zone']['reset_mode']) == (333, 33497, 1)
assert len(e['reset_commands']) == 26
assert collections.Counter(r['command'] for r in e['reset_commands']) == {'O': 2, 'P': 4, 'M': 10, 'E': 4, 'G': 6}
resets = {r['line']: (r['command'], r['arguments'][:5]) for r in e['reset_commands']}
for line, command, args in [(10, 'O', [0, 33306, 1, 33475, 100]), (14, 'P', [1, 33302, 1, 33306, 100]),
                            (19, 'M', [0, 33301, 1, 33375, 100]), (22, 'M', [0, 33305, 11, 33377, 100]),
                            (23, 'M', [0, 33308, 1, 33385, 100]), (30, 'M', [0, 33300, 1, 33469, 100])]:
    assert resets[line] == (command, args)
assert not [r for r in e['reset_commands'] if r['command'] in {'G', 'E', 'O'} and r['arguments'][1] == 33302]
container_header, container_values = properties(objects[33306])
assert container_header[0] == 15 and not container_header[7] & 1 and container_values[:4] == [100, 0, 0, 100]
header, values = properties(objects[33302])
assert header[0] == 13 and header[7] & 1 and header[8] & 32768
assert all(properties(objects[v])[0][6] & 4096 for v in (33302, 33306, 33307, 33308))
assert 'search' in prep['hint'].lower()
assert 'Fht malen botch lenack lalchen uta g quedish benddy!' in objects[33302]
assert properties(objects[33314])[0][0] == 15 and properties(objects[33314])[1][:4] == [450, 5, 0, 100]
assert not any(properties(body)[0][0] in {25, 29} or re.search(r'^T\s*$', body, re.M) or '_proclib_' in body for body in objects.values())
graph = edges(rooms)
assert len(graph) == 737 and not any(edge[2] for edge in graph)
local = [x for x in graph if x[-1] in rooms]
reverse = {0: 2, 1: 3, 2: 0, 3: 1, 4: 5, 5: 4}
connections = {(a, d, b) for a, d, flags, key, b in local}
assert len(local) == 730 and all((b, reverse[d], a) in connections for a, d, b in connections)
seen = {33302}
while True:
    more = seen | {edge[-1] for edge in local if edge[0] in seen}
    if more == seen:
        break
    seen = more
assert seen == set(rooms)
assert (33302, 0, 0, 0, 585374) in graph
assert (585374, 2, 0, 0, 33302) in edges({585374: bodies('surface', 'wld')[585374]})
plains = bodies('bloody_plains', 'wld')
assert (33422, 1, 0, 0, 33597) in graph and not any(x[-1] == 33422 for x in edges({33597: plains[33597]}))
for a, b in [(33437, 33612), (33452, 33627), (33467, 33642), (33482, 33657), (33497, 33672)]:
    assert (a, 1, 0, 0, b) in graph and (b, 3, 0, 0, a) in edges({b: plains[b]})
assert sum(bool(int(body.split('~', 2)[2].strip().split()[1]) & 4) for body in rooms.values()) == 16
assert not e['special_assignments']
shop = (ROOT / 'areas/shp/killing_fields.shp').read_text()
assert shop.startswith('#33308~\nN\n33310\n33311\n33312\n33315\n0\n0.80\n1.10\n0\n')
assert '\n33308\n0\n0\n0\n28\n0\n28\nY\nN\nY\n' in shop
assert {r['arguments'][1] for r in e['reset_commands'] if r['command'] == 'G'} == {33309, 33310, 33311, 33312, 33314, 33315}
assert cat.report_for(catalog)['eligible_by_zone']['333'] == 1
ids = set(re.findall(r'ZSQ-KILLING-FIELDS-\d{2}', (ROOT / 'docs/design/zone-stories/KILLING_FIELDS.md').read_text()))
assert ids == {'ZSQ-KILLING-FIELDS-%02d' % i for i in range(1, 25)}
print('Killing Fields exact note return, typed container source, real roaming shop and boundary qualification passed')
