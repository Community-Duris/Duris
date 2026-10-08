#!/usr/bin/env python3
"""Myconid exact external spores, independent cache and admitted passage contracts."""
import collections
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
import zone_story_quest_catalog as cat
import zone_story_quest_zone_inventory as inv


def bodies(kind, area='myconid'):
    text = (ROOT / 'areas' / kind / (area + '.' + kind)).read_text()
    return {int(m[1]): m[2] for m in re.finditer(r'^#(\d+)\s*\n([\s\S]*?)(?=^#\d+\s*$|\Z)', text, re.M)}


def properties(body):
    rows = body.split('~', 4)[4].strip().splitlines()
    return list(map(int, rows[0].split())), list(map(int, rows[1].split()))


def graph_for(rooms):
    return [(v, int(m[1]), int(m[4]), int(m[5]), int(m[6])) for v, body in rooms.items()
            for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)', body, re.S)]


e = inv.area_evidence(ROOT, 'myconid')
catalog = cat.production_catalog(ROOT)
mapping = next(m for m in catalog['story_mappings'] if m['source_area'] == 'myconid')
assert (mapping['schema_version'], mapping['revision'], mapping['coverage']) == (3, 1, 'complete')
assert len(mapping['stories']) == 1 and len(mapping['contacts']) == 21 and not mapping['exclusions']
assert len(e['requests']) == 1 and len(e['dialogue']) == 2
q = e['requests'][0]['block']
assert (q['line'], q['giver_vnum'], q['give'], q['receive'], q['disappear']) == (
    18, 2320, [('I', 88502)], [('C', 50000)], False)
assert not e['requests'][0]['definition']['prerequisites']
assert [(d['kind'], d['line']) for d in e['dialogue']] == [('M', 2), ('M', 7)]
story = mapping['stories'][0]
assert story['id'] == 'alchemist-giant-spores' and story['contracts'] == [q['binding']]
assert len(story['steps']) == 2
step = story['steps'][0]
assert (step['kind'], step['item_vnums'], step['count'], step['optional']) == ('carried_item', [88502], 1, True)
assert story['steps'][1]['kind'] == 'completion' and story['steps'][1]['contracts'] == story['contracts']
assert sum(len(c['topics']) for c in mapping['contacts']) == 4
rooms, mobs, objects = (bodies(kind) for kind in ('wld', 'mob', 'obj'))
assert (len(rooms), len(mobs), len(objects)) == (93, 21, 15)
assert set(rooms) == set(range(2300, 2393))
assert {c['mob_vnum'] for c in mapping['contacts']} == set(mobs)
for contact in mapping['contacts']:
    assert contact['keyword'] in mobs[contact['mob_vnum']].split('~')[0].split()
giver = next(c for c in mapping['contacts'] if c['mob_vnum'] == 2320)
assert giver['keyword'] == 'alchemist' and giver['topics'] == ['specific', 'giant', 'mushrooms', 'mushroom']
assert (e['zone']['zone_number'], e['zone']['last_vnum'], e['zone']['reset_mode']) == (23, 2392, 2)
assert len(e['reset_commands']) == 103
assert collections.Counter(r['command'] for r in e['reset_commands']) == {'D': 2, 'O': 20, 'P': 2, 'M': 74, 'G': 2, 'E': 3}
resets = {r['line']: (r['command'], r['arguments'][:5]) for r in e['reset_commands']}
for line, command, args in [(8, 'D', [0, 2374, 1, 8, 100]), (12, 'D', [0, 2392, 3, 8, 100]),
                            (29, 'O', [0, 2302, 1, 2368, 100]), (30, 'P', [1, 2311, 1, 2302, 100]),
                            (31, 'P', [1, 2310, 1, 2302, 100]), (32, 'O', [0, 2314, 1, 2374, 100]),
                            (34, 'O', [0, 2303, 1, 2383, 100]), (36, 'O', [0, 2312, 1, 2388, 100]),
                            (38, 'O', [0, 2313, 1, 2392, 100]), (91, 'M', [0, 2303, 1, 2368, 100]),
                            (92, 'G', [1, 2300, 1, 0, 100]), (93, 'G', [1, 2301, 1, 0, 100]),
                            (113, 'M', [0, 2314, 1, 2383, 100]), (114, 'M', [0, 2320, 1, 2384, 100])]:
    assert resets[line] == (command, args), (line, resets[line])
assert {r['arguments'][1] for r in e['reset_commands'] if r['command'] == 'M'} == set(mobs)
producers, root_producers = [], []
for path in (ROOT / 'areas/zon').glob('*.zon'):
    for line in path.read_text().splitlines():
        if re.match(r'[OPGE]\s+-?\d+\s+88502\s', line):
            producers.append((path.stem, line.split()[:6]))
        if re.match(r'[OPGE]\s+-?\d+\s+88500\s', line):
            root_producers.append((path.stem, line.split()[:6]))
assert sorted(producers) == [('newbie', ['G', '1', '88502', '1', '0', '100']),
                             ('udmini', ['P', '1', '88502', '1', '88501', '100'])]
assert not root_producers
foreign = bodies('obj', 'udmini')
assert properties(foreign[88502])[0][0] == 13 and properties(foreign[88502])[0][7] == 1
assert properties(foreign[88501])[0][0] == 15 and properties(foreign[88501])[0][6:8] == [32770, 0]
assert properties(foreign[88501])[1][:4] == [0, 5, 0, 0]
assert re.search(r'\bT\s+512\s+0\s+10\s+60\b', foreign[88501])
assert properties(foreign[88500])[0][0] == 25 and properties(foreign[88500])[0][7] == 0
assert properties(foreign[88500])[1][:4] == [88500, 7, -1, 0]
assert properties(objects[2301])[0][0] == 18 and properties(objects[2301])[1][:2] == [0, 0]
assert properties(objects[2302])[0][0] == 15 and properties(objects[2302])[1][:4] == [10, 13, 2301, 100]
for v, values in [(2313, [340, 2392, 3, 0]), (2314, [340, 2374, 1, 0])]:
    assert properties(objects[v])[0][0] == 29 and properties(objects[v])[0][7] == 0
    assert properties(objects[v])[1][:4] == values
for v, values in [(2300, [15, 44, 65, -1]), (2310, [1, 1, -1, -1]), (2311, [1, 3, 141, -1])]:
    assert properties(objects[v])[0][0] == 10 and properties(objects[v])[1][:4] == values
for v, poison in [(2304, 1), (2305, 0), (2306, 0)]:
    assert properties(objects[v])[0][0] == 19 and properties(objects[v])[1][3] == poison
assert properties(objects[2312])[0][0] == 17 and properties(objects[2312])[1][:4] == [-1, -1, 0, 0]
graph = graph_for(rooms)
assert len(graph) == 224
assert {(v, d, target) for v, d, flags, key, target in graph if target not in rooms} == {(2300, 4, 836064), (2392, 5, 811016)}
assert {(v, d, flags, target) for v, d, flags, key, target in graph if flags & 8} == {
    (2374, 1, 8, 2392), (2392, 3, 8, 2374)}
under = graph_for(bodies('wld', 'underdark'))
assert any(v == 836064 and d == 5 and target == 2300 for v, d, f, k, target in under)
assert any(v == 811016 and d == 4 and target == 2392 for v, d, f, k, target in under)
giant = graph_for({v: b for v, b in bodies('wld', 'udmini').items() if v in {88500, 88501}})
assert {(v, d, target) for v, d, f, k, target in giant} == {(88500, 4, 88501), (88501, 5, 88500)}
maerg = next(r['block'] for r in inv.area_evidence(ROOT, 'newbie')['requests'] if r['block']['line'] == 240)
assert (maerg['giver_vnum'], maerg['give'], maerg['receive']) == (29218, [('I', 88502)], [('I', 29291)])
assert not (ROOT / 'areas/shp/myconid.shp').exists() and not e['special_assignments']
assert cat.report_for(catalog)['eligible_by_zone']['23'] == 1
followups = []
for name in ('docs/design/zone-stories/MYCONID.md', 'docs/guides/ZONE_STORY_BUILDING.md',
             'docs/design/ZONE_STORY_ROADMAP_EXECUTION.md'):
    rows = [line for line in (ROOT / name).read_text().splitlines() if line.startswith('| ZSQ-MYCONID-')]
    assert len(rows) == 28 and 'M114@2384' in rows[16] and 'M91@2368' in rows[21]
    followups.append(rows)
assert followups[0] == followups[1] == followups[2]
assert set(re.findall(r'ZSQ-MYCONID-\d{2}', (ROOT / 'docs/design/zone-stories/MYCONID.md').read_text())) == {
    'ZSQ-MYCONID-%02d' % i for i in range(1, 29)}
print('Myconid exact external spores, independent key/cache, source ownership and selected passage contracts passed')
