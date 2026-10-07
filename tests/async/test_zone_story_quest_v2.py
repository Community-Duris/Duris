#!/usr/bin/env python3
"""Vargan II exact three-piece return, hidden sources and independent access contracts."""
import collections
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
import zone_story_quest_catalog as cat
import zone_story_quest_zone_inventory as inv


def bodies(kind, area='v2'):
    text = (ROOT / 'areas' / kind / (area + '.' + kind)).read_text()
    return {int(m[1]): m[2] for m in re.finditer(r'^#(\d+)\s*\n([\s\S]*?)(?=^#\d+\s*$|\Z)', text, re.M)}


def properties(body):
    rows = body.split('~', 4)[4].strip().splitlines()
    return list(map(int, rows[0].split())), list(map(int, rows[1].split()))


def graph_for(rooms):
    return [(v, int(m[1]), int(m[4]), int(m[5]), int(m[6])) for v, body in rooms.items()
            for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)', body, re.S)]


e = inv.area_evidence(ROOT, 'v2')
catalog = cat.production_catalog(ROOT)
mapping = next(m for m in catalog['story_mappings'] if m['source_area'] == 'v2')
assert (mapping['schema_version'], mapping['revision'], mapping['coverage']) == (3, 1, 'complete')
assert len(mapping['stories']) == 1 and len(mapping['contacts']) == 23 and not mapping['exclusions']
assert len(e['requests']) == 1 and len(e['dialogue']) == 2
q = e['requests'][0]['block']
assert (q['line'], q['giver_vnum'], q['give'], q['receive'], q['disappear']) == (
    13, 7822, [('I', 7844), ('I', 7845), ('I', 7846)], [('I', 7847)], False)
assert not e['requests'][0]['definition']['prerequisites']
assert [(d['kind'], d['line']) for d in e['dialogue']] == [('M', 2), ('M', 6)]
story = mapping['stories'][0]
assert story['id'] == 'vurlok-shattered-sword' and story['contracts'] == [q['binding']]
assert len(story['steps']) == 4
for step, vnum in zip(story['steps'][:3], (7844, 7845, 7846)):
    assert (step['kind'], step['item_vnums'], step['count'], step['optional']) == ('carried_item', [vnum], 1, True)
assert story['steps'][3]['kind'] == 'completion' and story['steps'][3]['contracts'] == story['contracts']
assert sum(len(c['topics']) for c in mapping['contacts']) == 2
rooms, mobs, objects = (bodies(kind) for kind in ('wld', 'mob', 'obj'))
assert (len(rooms), len(mobs), len(objects)) == (119, 23, 53)
assert set(rooms) == set(range(7800, 7919))
assert {c['mob_vnum'] for c in mapping['contacts']} == set(mobs)
for contact in mapping['contacts']:
    assert contact['keyword'] in mobs[contact['mob_vnum']].split('~')[0].split()
giver = next(c for c in mapping['contacts'] if c['mob_vnum'] == 7822)
assert giver['keyword'] == 'vurlok' and giver['topics'] == ['hi', 'champion']
assert (e['zone']['zone_number'], e['zone']['last_vnum'], e['zone']['reset_mode']) == (78, 7918, 2)
assert len(e['reset_commands']) == 139
assert collections.Counter(r['command'] for r in e['reset_commands']) == {
    'D': 30, 'O': 1, 'P': 1, 'M': 52, 'E': 51, 'G': 4}
assert all(r['arguments'][3] == 0 for r in e['reset_commands'] if r['command'] == 'D')
resets = {r['line']: (r['command'], r['arguments'][:5]) for r in e['reset_commands']}
for line, command, args in [(129, 'O', [0, 7851, 1, 7917, 100]), (130, 'P', [1, 7852, 1, 7851, 100]),
                            (144, 'M', [0, 7820, 2, 7826, 100]), (145, 'M', [0, 7820, 2, 7826, 100]),
                            (146, 'E', [1, 7800, 1, 18, 100]), (199, 'M', [0, 7814, 1, 7887, 100]),
                            (201, 'G', [1, 7844, 1, 0, 100]), (202, 'M', [0, 7803, 1, 7888, 100]),
                            (206, 'M', [0, 7812, 1, 7889, 100]), (209, 'G', [1, 7846, 1, 0, 100]),
                            (210, 'M', [0, 7813, 1, 7890, 100]), (213, 'G', [1, 7845, 1, 0, 100]),
                            (237, 'M', [0, 7822, 1, 7918, 100])]:
    assert resets[line] == (command, args), (line, resets[line])
assert {r['arguments'][1] for r in e['reset_commands'] if r['command'] == 'M'} == set(mobs)
producers = []
for path in (ROOT / 'areas/zon').glob('*.zon'):
    for line in path.read_text().splitlines():
        if re.match(r'[OPGE]\s+-?\d+\s+(?:7844|7845|7846)\s', line):
            producers.append((path.stem, line.split()[:6]))
assert sorted(producers) == [('v2', ['G', '1', str(v), '1', '0', '100']) for v in (7844, 7845, 7846)]
for v in (7844, 7845, 7846):
    flags, values = properties(objects[v])
    assert flags[0] == 13 and flags[6] == 8392704 and flags[8] == 32768 and flags[7] == 1
assert properties(objects[7800])[0][0] == 18
assert properties(objects[7801])[0][0] == 9
assert properties(objects[7822])[0][0] == 5
assert properties(objects[7847])[0][0] == 5 and properties(objects[7847])[0][7] == 8193
assert properties(objects[7847])[1][:4] == [13, 2, 4, 3]
assert properties(objects[7851])[0][0] == 15 and properties(objects[7851])[0][6] == 4096
assert properties(objects[7851])[0][7] == 0 and properties(objects[7851])[1][:4] == [4, 0, 0, 100]
assert properties(objects[7852])[0][0] == 13 and properties(objects[7852])[0][7] == 1
assert not any(r['command'] in 'OPGE' and r['arguments'][1] == 7850 for r in e['reset_commands'])
graph = graph_for(rooms)
assert len(graph) == 249
assert {(v, d, target) for v, d, flags, key, target in graph if target not in rooms} == {(7800, 4, 2182)}
assert {(v, d, flags, target) for v, d, flags, key, target in graph if flags & 8} == {
    (7887, 2, 9, 7888), (7888, 0, 9, 7887)}
assert {(v, d, flags, key, target) for v, d, flags, key, target in graph if (v, d) in {(7824, 1), (7827, 3)}} == {
    (7824, 1, 2, 7800, 7827), (7827, 3, 2, 7800, 7824)}
assert any(v == 7824 and d == 0 and f == 5 and k == 7801 for v, d, f, k, t in graph)
foreign = graph_for(bodies('wld', 'vargan'))
assert any(v == 2182 and d == 5 and target == 7800 for v, d, f, k, target in foreign)
for v in range(7892, 7897):
    assert list(map(int, rooms[v].split('~', 2)[2].strip().splitlines()[0].split())) == [78, 9, 8]
assert {v for v, d, f, k, t in graph if v in range(7892, 7897) and d == 5} == {7892}
assert not (ROOT / 'areas/shp/v2.shp').exists() and not e['special_assignments']
assert cat.report_for(catalog)['eligible_by_zone']['78'] == 1
followups = []
for name in ('docs/design/zone-stories/V2.md', 'docs/guides/ZONE_STORY_BUILDING.md',
             'docs/design/ZONE_STORY_ROADMAP_EXECUTION.md'):
    rows = [line for line in (ROOT / name).read_text().splitlines() if line.startswith('| ZSQ-V2-')]
    assert len(rows) == 28 and 'M199@7887' in rows[3] and 'M210@7890' in rows[4] and 'M206@7889' in rows[5]
    followups.append(rows)
assert followups[0] == followups[1] == followups[2]
assert set(re.findall(r'ZSQ-V2-\d{2}', (ROOT / 'docs/design/zone-stories/V2.md').read_text())) == {
    'ZSQ-V2-%02d' % i for i in range(1, 29)}
print('Vargan II exact joint return, hidden source ownership, independent map/key and selected access contracts passed')
