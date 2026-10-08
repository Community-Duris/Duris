#!/usr/bin/env python3
"""Phantasmagoric exact source, dialogue, retirement and independent service contracts."""
import collections
from pathlib import Path
import re
import sys
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
import zone_story_quest_catalog as cat
import zone_story_quest_zone_inventory as inv

def bodies(kind, area='valdrak'):
    return {int(m[1]): m[2] for m in re.finditer(r'^#(\d+)\s*\n([\s\S]*?)(?=^#\d+\s*$|\Z)',
            (ROOT / 'areas' / kind / (area + '.' + kind)).read_text(), re.M)}

def properties(body):
    rows = body.split('~', 4)[4].strip().splitlines()
    return list(map(int, rows[0].split())), list(map(int, rows[1].split()))

def graph_for(rooms):
    return [(v, int(m[1]), int(m[4]), int(m[5]), int(m[6])) for v, body in rooms.items()
            for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)', body, re.S)]

e = inv.area_evidence(ROOT, 'valdrak')
catalog = cat.production_catalog(ROOT)
mapping = next(m for m in catalog['story_mappings'] if m['source_area'] == 'valdrak')
assert (mapping['schema_version'], mapping['revision'], mapping['coverage']) == (3, 1, 'complete')
assert len(mapping['stories']) == 1 and len(mapping['contacts']) == 57 and not mapping['exclusions']
assert len(e['requests']) == 1 and len(e['dialogue']) == 2
raw = [b for b in inv.native_blocks(ROOT) if b['source'] == 'areas/qst/valdrak.qst']
assert [(b['kind'], b['line']) for b in raw] == [('M', 2), ('M', 14), ('M', 28), ('Q', 40)]
assert raw[2]['body'][0] == 'azopiltaczet, acopiltaczet~'
q = e['requests'][0]['block']
assert (q['line'], q['giver_vnum'], q['give'], q['receive'], q['disappear']) == (
    40, 53110, [('I', 53108), ('I', 53117), ('I', 53118)], [('C', 81000), ('E', 13300)], True)
assert not e['requests'][0]['definition']['prerequisites']
story = mapping['stories'][0]
assert story['id'] == 'xolot-three-spider-return' and story['contracts'] == [q['binding']]
assert len(story['steps']) == 4
for step, vnum in zip(story['steps'][:3], (53108, 53117, 53118)):
    assert (step['kind'], step['item_vnums'], step['count'], step['optional']) == ('carried_item', [vnum], 1, True)
assert story['steps'][3]['kind'] == 'completion' and story['steps'][3]['contracts'] == story['contracts']
rooms, mobs, objects = (bodies(kind) for kind in ('wld', 'mob', 'obj'))
assert (len(rooms), len(mobs), len(objects)) == (100, 57, 19)
assert set(rooms) == set(range(53100, 53200))
assert {c['mob_vnum'] for c in mapping['contacts']} == set(mobs)
for c in mapping['contacts']:
    assert c['keyword'] in mobs[c['mob_vnum']].split('~')[0].split()
giver = next(c for c in mapping['contacts'] if c['mob_vnum'] == 53110)
assert giver['keyword'] == 'xolot' and giver['topics'] == ['gravf', 'valdrak', 'acopiltaczet']
assert sum(len(c['topics']) for c in mapping['contacts']) == 3
assert (e['zone']['zone_number'], e['zone']['last_vnum'], e['zone']['reset_mode']) == (531, 53199, 2)
assert len(e['reset_commands']) == 311
assert collections.Counter(r['command'] for r in e['reset_commands']) == {'O': 29, 'M': 253, 'G': 20, 'E': 8, 'F': 1}
resets = {r['line']: (r['command'], r['arguments'][:5]) for r in e['reset_commands']}
for line, command, args in [(10, 'O', [0, 53100, 1, 53119, 100]), (202, 'M', [0, 53110, 1, 53150, 100]),
                          (205, 'G', [1, 53106, 1, 0, 100]), (206, 'G', [1, 53107, 1, 0, 100]),
                          (207, 'F', [1, 53129, 1, 53150, 100]), (305, 'M', [0, 53134, 1, 53187, 100]),
                          (306, 'G', [1, 53117, 1, 0, 100]), (309, 'M', [0, 53132, 1, 53190, 100]),
                          (310, 'G', [1, 53108, 1, 0, 100]), (318, 'M', [0, 53131, 1, 53199, 100]),
                          (319, 'G', [1, 53110, 1, 0, 100]), (320, 'G', [1, 53118, 1, 0, 100])]:
    assert resets[line] == (command, args)
assert {r['arguments'][1] for r in e['reset_commands'] if r['command'] in ('M', 'F')} == set(mobs)
producers = []
for path in (ROOT / 'areas/zon').glob('*.zon'):
    for line in path.read_text().splitlines():
        if re.match(r'[OPGE]\s+-?\d+\s+(?:53108|53117|53118)\s', line):
            producers.append((path.stem, line.split()[:6]))
assert sorted(producers) == [('valdrak', ['G', '1', str(v), '1', '0', '100']) for v in (53108, 53117, 53118)]
for v in (53108, 53117, 53118):
    flags, values = properties(objects[v])
    assert flags[7] & 1 and flags[8] & 32768
assert properties(objects[53108])[0][0] == 5 and not properties(objects[53108])[0][6] & 4096
assert properties(objects[53117])[0][0] == 5 and not properties(objects[53117])[0][6] & 4096
assert properties(objects[53118])[0][0] == 8 and properties(objects[53118])[0][6] & 4096
assert not any(properties(objects[v])[0][6] & 8388608 for v in (53108, 53117, 53118))
assert properties(objects[53110])[0][0] == 5
assert properties(objects[53100])[0][0] == 17 and properties(objects[53100])[1][:4] == [9000, 9000, 0, 0]
for v, duration in ((53101, 33), (53105, 43)):
    assert properties(objects[v])[0][0] == 19 and properties(objects[v])[1][:6] == [duration, 0, 0, 0, 0, 0]
assert properties(objects[53106])[0][6] & 4096 and properties(objects[53107])[0][6] & 4096
graph = graph_for(rooms)
assert len(graph) == 234
assert {(v, d, target) for v, d, f, k, target in graph if target not in rooms} == {
    (53100, 2, 832131), (53100, 3, 831730)}
assert {(v, d, f, target) for v, d, f, k, target in graph if f} == {
    (53157, 5, 4, 53177), (53177, 4, 4, 53157), (53194, 1, 4, 53195)}
for v in (53128, 53130):
    assert sum(edge[0] == v for edge in graph) == 2
    assert int(rooms[v].split('~', 2)[2].strip().splitlines()[0].split()[1]) & 8192
assert int(rooms[53119].split('~', 2)[2].strip().splitlines()[0].split()[2]) == 16
assert not any(int(b.split('~', 2)[2].strip().splitlines()[0].split()[1]) & 32 for b in rooms.values())
shop = (ROOT / 'areas/shp/valdrak.shp').read_text()
assert '#53156' in shop and '53119' in shop and not e['special_assignments']
assert e['requests'][0]['definition']['daily_eligible'] and cat.report_for(catalog)['eligible_by_zone']['531'] == 1
tables = []
for name in ('docs/design/zone-stories/VALDRAK.md', 'docs/guides/ZONE_STORY_BUILDING.md',
             'docs/design/ZONE_STORY_ROADMAP_EXECUTION.md'):
    rows = [line for line in (ROOT / name).read_text().splitlines() if line.startswith('| ZSQ-VALDRAK-')]
    assert len(rows) == 30
    tables.append(rows)
assert tables[0] == tables[1] == tables[2]
print('Phantasmagoric exact joint return, three dialogue families, hidden heart, retirement and independent service contracts passed')
