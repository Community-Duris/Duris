#!/usr/bin/env python3
"""Storm Port native bundle, foreign giver, hidden custody and travel boundaries."""
import collections
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
import zone_story_quest_catalog as cat
import zone_story_quest_zone_inventory as inv

def bodies(area, kind):
    return {int(m[1]): m[2] for m in re.finditer(r'^#(\d+)\s*\n([\s\S]*?)(?=^#\d+\s*$|\Z)',
        (ROOT / 'areas' / kind / (area + '.' + kind)).read_text(), re.M)}

def properties(body):
    return [list(map(int, x.split())) for x in body.split('~', 4)[4].strip().splitlines()[:3]]

e = inv.area_evidence(ROOT, 'stormport')
catalog = cat.production_catalog(ROOT)
mapping = next(m for m in catalog['story_mappings'] if m['source_area'] == 'stormport')
assert (mapping['schema_version'], mapping['revision'], mapping['coverage']) == (3, 1, 'complete')
assert len(mapping['stories']) == 2 and not mapping['exclusions'] and len(mapping['contacts']) == 71
raw = [b for b in inv.native_blocks(ROOT) if b['source'] == 'areas/qst/stormport.qst']
assert [(b['kind'], b['line']) for b in raw] == [('M', 2), ('Q', 12), ('Q', 21), ('M', 30)]
assert raw[0]['body'][0] == 'hi hello~' and raw[3]['body'][0] == 'qc_action 30~'
tchan, smith = raw[1:3]
assert tchan['body'] == ['~'] and tchan['receive'] == [('I', 22435)]
assert tchan['give'] == [('I', 22431), ('I', 22433), ('I', 22432), ('I', 22434)]
assert smith['give'] == [('I', 22406), ('I', 22405)] and smith['receive'] == [('I', 22407)]
assert not tchan['disappear'] and not smith['disappear']
for story, request in zip(mapping['stories'], (tchan, smith)):
    assert story['category'] == 'request' and story['contracts'] == [request['binding']]
    assert len(story['steps']) == len(request['give']) + 1
    for step, (_, value) in zip(story['steps'][:-1], request['give']):
        assert (step['kind'], step['item_vnums'], step['count'], step['optional']) == ('carried_item', [value], 1, True)
    assert story['steps'][-1]['kind'] == 'completion' and story['steps'][-1]['contracts'] == story['contracts']
assert all(not r['definition']['prerequisites'] and r['definition']['daily_eligible'] for r in e['requests'])
units = [u for u in cat.story_units(catalog) if u['zone_number'] == 224]
assert len(units) == 2 and all(u['achievement'] and u['daily_candidate'] for u in units)
rooms, mobs, objects = (bodies('stormport', k) for k in ('wld', 'mob', 'obj'))
assert (len(rooms), len(mobs), len(objects)) == (147, 71, 43)
assert (min(rooms), max(rooms)) == (22400, 22546)
assert {c['mob_vnum'] for c in mapping['contacts']} == set(mobs)
for contact in mapping['contacts']:
    assert contact['keyword'] in mobs[contact['mob_vnum']].split('~')[0].split()
assert sum(len(c['topics']) for c in mapping['contacts']) == 2
assert next(c for c in mapping['contacts'] if c['mob_vnum'] == 22428)['topics'] == ['hi', 'hello']
assert not next(c for c in mapping['contacts'] if c['mob_vnum'] == 22469)['topics']
assert collections.Counter(r['command'] for r in e['reset_commands']) == {'D': 64, 'O': 8, 'P': 1, 'M': 148, 'E': 23, 'G': 28, 'F': 3}
resets = {r['line']: (r['command'], r['arguments'][:5]) for r in e['reset_commands']}
for line, command, args in [(268, 'O', [0, 22404, 1, 22503, 100]), (269, 'P', [1, 22405, 1, 22404, 100]),
    (367, 'G', [1, 22434, 1, 0, 100]), (371, 'G', [1, 22431, 1, 0, 100]),
    (394, 'G', [1, 22406, 1, 0, 100]), (420, 'G', [1, 22433, 1, 0, 100]), (426, 'G', [1, 22432, 1, 0, 100])]:
    assert resets[line] == (command, args)
assert not any(r['command'] == 'M' and r['arguments'][1] == 22428 for r in e['reset_commands'])
assert 'M 0 22428 1 4809 100' in (ROOT / 'areas/zon/tchan.zon').read_text()
assert properties(objects[22404])[1][1:3] == [5, 0]
assert properties(objects[22405])[0][6:9] == [4096, 1, 32768]
assert properties(objects[22406])[0][6:9] == [0, 1, 32768]
for value in (22431, 22433):
    assert properties(objects[value])[0][6:9] == [8388608, 16385, 32768]
for value in (22432, 22434):
    assert properties(objects[value])[0][6:9] == [8392704, 16385, 32768]
assert properties(objects[22401])[1][:3] == [22530, 7, -1]
assert properties(objects[22424])[1][:3] == [22439, 6, -1]
assert not any(r['command'] in ('O', 'P', 'G', 'E') and r['arguments'][1] in (22423, 22424, 22440, 22441, 22442) for r in e['reset_commands'])
graph = [(v, int(m[1]), int(m[4]), int(m[5]), int(m[6])) for v, b in rooms.items()
    for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)', b, re.S)]
assert len(graph) == 320 and len([x for x in graph if x[-1] not in rooms]) == 6
old_ship = {22508, 22509, 22510, 22511, 22538, 22539, 22540}
assert not any(v not in old_ship and dest in old_ship for v, _, _, _, dest in graph)
assert all(key == 0 for _, _, _, key, _ in graph)
assert not any(re.search(r'^E\s*$|^F\s*$', b, re.M) for b in rooms.values())
for edge in [(22422, 5, 5, 0, 22512), (22527, 3, 5, 0, 22458), (22455, 5, 5, 0, 22537)]:
    assert edge in graph
spec = (ROOT / 'src/specs/specs.stormport.c').read_text()
guard = spec.index('economic_gameplay_authority::active()')
assert guard < spec.index('SUB_MONEY') and 'unavailable' in spec[guard:guard + 550]
ferry = (ROOT / 'src/world/ferryact.c').read_text()
automat = ferry[ferry.index('int ferry_automat_proc('):] if 'int ferry_automat_proc(' in ferry else ferry[ferry.index('ferry_automat_proc(P_obj'):]
assert automat.index('economic_gameplay_authority::active()') < automat.index('SUB_MONEY')
assert 'FERRY_TICKET_VNUM' in automat and '47014' in ferry and '47018' in ferry
runtime = (ROOT / 'src/world/zone_story_quest_runtime.c').read_text()
assert 'zone_number = definition.zone_number' in runtime and 'event.transaction.room_vnum = room_vnum' in runtime
assert 'new quest completion tracking requires active economic accounting' in runtime
guidance = ' '.join(mapping['orientation'] + [c['description'] for c in mapping['contacts']] + [step.get('hint', '') for s in mapping['stories'] for step in s['steps']])
assert all(word in guidance.lower() for word in ('loose', 'supplied', 'independent', 'monestary', 'accounting', 'badge', 'search', 'norent', 'seaspray'))
tables = []
for name in ('docs/design/zone-stories/STORMPORT.md', 'docs/guides/ZONE_STORY_BUILDING.md', 'docs/design/ZONE_STORY_ROADMAP_EXECUTION.md'):
    rows = [line for line in (ROOT / name).read_text().splitlines() if line.startswith('| ZSQ-STORMPORT-')]
    assert len(rows) == 34
    tables.append(rows)
assert tables[0] == tables[1] == tables[2]
print('Storm Port native bundles, foreign giver, hidden custody, meaningful empty reply and accounting travel boundaries passed')
