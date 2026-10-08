#!/usr/bin/env python3
"""Mazzolin distinct lookalike bundle, exact reset lineage and protected route contracts."""
import collections
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
import zone_story_quest_catalog as cat
import zone_story_quest_zone_inventory as inv


def bodies(kind):
    text = (ROOT / 'areas' / kind / ('mazzolin.' + kind)).read_text()
    return {int(m[1]): m[2] for m in re.finditer(r'^#(\d+)\s*\n([\s\S]*?)(?=^#\d+\s*$|\Z)', text, re.M)}


def properties(body):
    rows = body.split('~', 4)[4].strip().splitlines()
    return list(map(int, rows[0].split())), list(map(int, rows[1].split()))


e = inv.area_evidence(ROOT, 'mazzolin')
catalog = cat.production_catalog(ROOT)
mapping = next(m for m in catalog['story_mappings'] if m['source_area'] == 'mazzolin')
assert (mapping['schema_version'], mapping['revision'], mapping['coverage']) == (3, 1, 'complete')
assert len(mapping['stories']) == 1 and len(mapping['contacts']) == 39 and not mapping['exclusions']
assert len(e['requests']) == 1 and len(e['dialogue']) == 2
q = e['requests'][0]['block']
assert (q['line'], q['giver_vnum'], q['give'], q['receive'], q['disappear']) == (
    15, 21323, [('I', v) for v in range(21309, 21314)], [('I', 21316), ('I', 21320)], True)
assert not e['requests'][0]['definition']['prerequisites']
assert [(d['kind'], d['line']) for d in e['dialogue']] == [('M', 2), ('M', 10)]
story = mapping['stories'][0]
assert story['id'] == 'aeirayne-hematite-star' and story['contracts'] == [q['binding']]
assert len(story['steps']) == 6
for step, vnum in zip(story['steps'][:-1], range(21309, 21314)):
    assert (step['kind'], step['item_vnums'], step['count'], step['optional']) == ('carried_item', [vnum], 1, True)
assert story['steps'][-1]['kind'] == 'completion' and story['steps'][-1]['contracts'] == story['contracts']
assert len({step['text'] for step in story['steps'][:-1]}) == 5
assert sum(len(c['topics']) for c in mapping['contacts']) == 4
rooms, mobs, objects = (bodies(kind) for kind in ('wld', 'mob', 'obj'))
assert (len(rooms), len(mobs), len(objects)) == (136, 39, 21)
assert set(rooms) == set(range(21300, 21437)) - {21375}
assert {c['mob_vnum'] for c in mapping['contacts']} == set(mobs)
for contact in mapping['contacts']:
    assert contact['keyword'] in mobs[contact['mob_vnum']].split('~')[0].split()
giver = next(c for c in mapping['contacts'] if c['mob_vnum'] == 21323)
assert giver['keyword'] == 'aeirayne' and giver['topics'] == ['hello', 'hi', 'help', 'star']
assert (e['zone']['zone_number'], e['zone']['last_vnum'], e['zone']['reset_mode']) == (213, 21436, 1)
assert len(e['reset_commands']) == 232
assert collections.Counter(r['command'] for r in e['reset_commands']) == {'D': 16, 'O': 7, 'M': 134, 'E': 58, 'F': 11, 'G': 6}
resets = {r['line']: (r['command'], r['arguments'][:5]) for r in e['reset_commands']}
for line, command, args in [(113, 'M', [0, 21301, 1, 21308, 100]), (117, 'G', [1, 21313, 1, 0, 100]),
                            (137, 'M', [0, 21308, 14, 21331, 100]), (138, 'G', [1, 21312, 1, 0, 100]),
                            (222, 'M', [0, 21315, 1, 21410, 100]), (223, 'G', [1, 21311, 1, 0, 100]),
                            (227, 'M', [0, 21323, 1, 21411, 100]), (239, 'M', [0, 21325, 2, 21421, 100]),
                            (240, 'G', [1, 21309, 1, 0, 100]), (246, 'M', [0, 21328, 1, 21422, 100]),
                            (247, 'G', [1, 21310, 1, 0, 100]), (248, 'G', [1, 358, 1, 0, 100])]:
    assert resets[line] == (command, args), (line, resets[line])
stocked = {r['arguments'][1] for r in e['reset_commands'] if r['command'] in {'M', 'F'}}
assert set(mobs) - stocked == {21329, 21334, 21335}
producers = []
for path in (ROOT / 'areas/zon').glob('*.zon'):
    for line in path.read_text().splitlines():
        if re.match(r'[OPGE]\s+-?\d+\s+213(?:09|10|11|12|13)\s', line):
            producers.append((path.stem, line.split()[:6]))
assert producers == [('mazzolin', ['G', '1', str(v), '1', '0', '100']) for v in [21313, 21312, 21311, 21309, 21310]]
assert len({objects[v] for v in range(21309, 21314)}) == 1
for v in range(21309, 21314):
    props, values = properties(objects[v])
    assert props[0] == 13 and props[6:9] == [33587200, 16385, 32768]
assert properties(objects[21316])[0][0] == 9 and properties(objects[21316])[0][7] == 257
assert properties(objects[21320])[0][0] == 3 and properties(objects[21320])[0][6:8] == [8192, 256]
assert properties(objects[21320])[1][:4] == [45, 4, 4, 167]
teleports = {v: properties(body)[1][:4] for v, body in objects.items() if properties(body)[0][0] == 25}
assert teleports == {21308: [21300, 150, 1, 0], 21317: [21300, 7, -1, 0],
                     21318: [21421, 7, -1, 0], 21319: [21302, 259, -1, 0]}
assert properties(objects[21319])[0][6] & 4096 and properties(objects[21319])[0][7] == 0
orb_rooms = {r['arguments'][3] for r in e['reset_commands'] if r['command'] == 'O' and r['arguments'][1] == 21319}
assert orb_rooms == {21381, 21385, 21388, 21410}
graph = [(v, int(m[1]), int(m[4]), int(m[5]), int(m[6])) for v, body in rooms.items()
         for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)', body, re.S)]
assert len(graph) == 288
assert {(v, d, target) for v, d, flags, key, target in graph if target not in rooms} == {(21300, 2, 803053), (21300, 3, 802652)}
for edge in [(21308, 5, 7, -2, 21309), (21309, 4, 3, 0, 21308),
             (21309, 5, 2, 0, 21311), (21311, 4, 2, -1, 21309),
             (21359, 4, 3, 0, 21362), (21362, 5, 3, 0, 21359),
             (21310, 0, 0, 0, 21411), (21411, 2, 0, 0, 21412)]:
    assert edge in graph, edge
assert not any(v == 21410 for v, d, flags, key, target in graph)
assert not (ROOT / 'areas/shp/mazzolin.shp').exists() and not e['special_assignments']
assert cat.report_for(catalog)['eligible_by_zone']['213'] == 1
ids = set(re.findall(r'ZSQ-MAZZOLIN-\d{2}', (ROOT / 'docs/design/zone-stories/MAZZOLIN.md').read_text()))
assert ids == {'ZSQ-MAZZOLIN-%02d' % i for i in range(1, 29)}
followups = []
for name in ('docs/design/zone-stories/MAZZOLIN.md', 'docs/guides/ZONE_STORY_BUILDING.md',
             'docs/design/ZONE_STORY_ROADMAP_EXECUTION.md'):
    rows = [line for line in (ROOT / name).read_text().splitlines() if line.startswith('| ZSQ-MAZZOLIN-')]
    assert len(rows) == 28 and 'M227@21411' in rows[13]
    followups.append(rows)
assert followups[0] == followups[1] == followups[2]
print('Mazzolin five distinct lookalike inputs, exact stock lineage, two-output exchange and protected return routes passed')
