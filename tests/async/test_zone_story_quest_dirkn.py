#!/usr/bin/env python3
"""Dirk'nspire independent intelligence receipts, exact custody and source guidance."""
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
    rows = body.split('~', 4)[4].strip().splitlines()
    return [list(map(int, row.split())) for row in rows[:3]]

e = inv.area_evidence(ROOT, 'dirkn')
catalog = cat.production_catalog(ROOT)
mapping = next(m for m in catalog['story_mappings'] if m['source_area'] == 'dirkn')
assert (mapping['schema_version'], mapping['revision'], mapping['coverage']) == (3, 1, 'complete')
assert len(mapping['stories']) == 2 and not mapping['exclusions'] and len(mapping['contacts']) == 39
raw = [b for b in inv.native_blocks(ROOT) if b['source'] == 'areas/qst/dirkn.qst']
assert [(b['kind'], b['line']) for b in raw] == [('M', 2), ('Q', 10), ('Q', 19)]
assert raw[0]['body'][0] == 'secrets maps secret map~'
document, paper = raw[1:]
assert document['give'] == [('I', 96845)] and document['receive'] == [('I', 96843)]
assert paper['give'] == [('I', 96802)] and paper['receive'] == [('C', 2570)]
assert 'not what I had hoped for' in '\n'.join(paper['body'])
assert not document['disappear'] and not paper['disappear']
for story, request, vnum in zip(mapping['stories'], (document, paper), (96845, 96802)):
    assert story['category'] == 'request' and story['contracts'] == [request['binding']]
    assert len(story['steps']) == 2
    step = story['steps'][0]
    assert (step['kind'], step['item_vnums'], step['count'], step['optional']) == ('carried_item', [vnum], 1, True)
    assert story['steps'][1]['kind'] == 'completion' and story['steps'][1]['contracts'] == story['contracts']
assert all(not r['definition']['prerequisites'] and r['definition']['daily_eligible'] for r in e['requests'])
units = [u for u in cat.story_units(catalog) if u['zone_number'] == 968]
assert len(units) == 2 and all(u['achievement'] and u['daily_candidate'] for u in units)
rooms, mobs, objects = (bodies('dirkn', k) for k in ('wld', 'mob', 'obj'))
assert (len(rooms), len(mobs), len(objects)) == (86, 39, 46) and (min(rooms), max(rooms)) == (96800, 96885)
assert {c['mob_vnum'] for c in mapping['contacts']} == set(mobs)
for contact in mapping['contacts']:
    assert contact['keyword'] in mobs[contact['mob_vnum']].split('~')[0].split()
assert sum(len(c['topics']) for c in mapping['contacts']) == 4
assert next(c for c in mapping['contacts'] if c['mob_vnum'] == 96829)['topics'] == ['maps', 'secrets', 'map', 'secret']
assert int(mobs[96820].split('~', 4)[4].strip().split()[0]) & 32768
assert (e['zone']['zone_number'], e['zone']['last_vnum'], e['zone']['reset_mode']) == (968, 96885, 2)
assert collections.Counter(r['command'] for r in e['reset_commands']) == {'D': 44, 'O': 27, 'P': 11, 'M': 69, 'E': 30, 'G': 6}
resets = {r['line']: (r['command'], r['arguments'][:5]) for r in e['reset_commands']}
for line, command, args in [(185, 'O', [0, 96800, 1, 96802, 100]), (187, 'P', [1, 96802, 1, 96800, 100]),
    (206, 'O', [0, 96815, 3, 96851, 100]), (215, 'O', [0, 96815, 3, 96868, 100]),
    (216, 'P', [1, 96845, 1, 96815, 100]), (219, 'O', [0, 96815, 3, 96884, 100]),
    (236, 'M', [0, 96829, 1, 96813, 100]), (237, 'E', [1, 96843, 1, 24, 100]),
    (283, 'M', [0, 96804, 1, 96848, 100]), (284, 'G', [1, 96810, 1, 0, 100]),
    (311, 'M', [0, 96816, 1, 96868, 100]), (314, 'E', [1, 96835, 1, 16, 100])]:
    assert resets[line] == (command, args)
assert {r['arguments'][1] for r in e['reset_commands'] if r['command'] == 'M'} == set(mobs)
assert properties(objects[96800])[0][0] == 15 and properties(objects[96800])[0][7] == 0
assert properties(objects[96815])[1][1:3] == [13, 0]
assert properties(objects[96802])[0][6:9] == [4096, 16385, 32768]
assert properties(objects[96845])[0][6:9] == [0, 1, 32768]
assert properties(objects[96835])[1][5:8] == [27188, 40, 35]
assert '_noquest_' in objects[96835].split('~')[0]
for vnum, liquid, poison in [(96804, 16, 2), (96807, 14, 2), (96808, 5, 0),
    (96821, 17, 0), (96824, 9, 0), (96839, 13, 0), (96842, 16, 0)]:
    assert properties(objects[vnum])[1][2:4] == [liquid, poison]
graph = [(v, int(m[1]), int(m[4]), int(m[5]), int(m[6])) for v, b in rooms.items()
    for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)', b, re.S)]
assert len(graph) == 177 and len([x for x in graph if x[-1] not in rooms]) == 1
for edge in [(96800, 2, 0, 0, 90810), (96808, 3, 4, 0, 96814),
    (96821, 1, 5, 0, 96820), (96846, 1, 3, 96810, 96848)]:
    assert edge in graph
assert re.search(r'0 0 96800\s', bodies('valinhav', 'wld')[90810])
assert not any(r['command'] == 'D' and r['arguments'][:3] == [0, 96808, 3] for r in e['reset_commands'])
assert '\nF\n5\n' in rooms[96824]
controls = {v: list(map(int, b.split('~', 2)[2].strip().splitlines()[0].split())) for v, b in rooms.items()}
assert len({tuple(x) for x in controls.values()}) == 14
assert all(x[1] & 1 for x in controls.values())
db = (ROOT / 'src/world/db.c').read_text()
p_reset = db[db.index("case 'P': /* object to object */"):db.index("case 'G': /* obj_to_char */")]
assert 'get_obj_num(ZCMD.arg3)' in p_reset
assert re.search(r'state\s*&=\s*3;', db)
foreign = [b for b in inv.native_blocks(ROOT) if b['giver_vnum'] in (83336, 83439) and ('I', 96835) in b['give']]
assert len(foreign) == 2 and len({b['binding']['giver_vnum'] for b in foreign}) == 2
assert all(b['give'] == [('I', 78419), ('I', 95526), ('I', 96835), ('I', 34226), ('I', 4505)] and b['disappear'] for b in foreign)
assert 'P 1 96820 1 83330' in (ROOT / 'areas/zon/alatorin.zon').read_text()
assert 'obj_index[real_object0(96402)].func.obj = illithid_teleport_veil' in (ROOT / 'src/specs/specs.assign.c').read_text()
assert not e['special_assignments'] and (ROOT / 'areas/shp/dirkn.shp').exists()
guidance = ' '.join(mapping['orientation'] + [c['description'] for c in mapping['contacts']] + [step.get('hint', '') for s in mapping['stories'] for step in s['steps']])
assert all(word in guidance.lower() for word in ('search', 'guano', 'moneybox', 'loose', 'supplied', 'independent', 'platinum', 'alatorin', 'teacher'))
tables = []
for name in ('docs/design/zone-stories/DIRKN.md', 'docs/guides/ZONE_STORY_BUILDING.md', 'docs/design/ZONE_STORY_ROADMAP_EXECUTION.md'):
    rows = [line for line in (ROOT / name).read_text().splitlines() if line.startswith('| ZSQ-DIRKN-')]
    assert len(rows) == 34
    tables.append(rows)
assert tables[0] == tables[1] == tables[2]
print('Dirk’nspire independent intelligence, meaningful coins, exact custody, access and foreign-consumer contracts passed')
