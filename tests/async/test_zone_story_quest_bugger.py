#!/usr/bin/env python3
"""Bugger Caves exact eggs, refusal, hidden sources and parent/arrival contracts."""
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

e = inv.area_evidence(ROOT, 'bugger')
catalog = cat.production_catalog(ROOT)
mapping = next(m for m in catalog['story_mappings'] if m['source_area'] == 'bugger')
assert (mapping['schema_version'], mapping['revision'], mapping['coverage']) == (3, 1, 'complete')
assert len(mapping['stories']) == 1 and len(mapping['exclusions']) == 1 and len(mapping['contacts']) == 22
raw = [b for b in inv.native_blocks(ROOT) if b['source'] == 'areas/qst/bugger.qst']
assert [(b['kind'], b['line']) for b in raw] == [('MA', 2), ('QA', 7), ('QA', 14)]
assert raw[0]['body'][0] == 'hi hello agitated~'
request, refusal = raw[1:]
assert request['give'] == [('I', 6403), ('I', 6404), ('I', 6405)] and request['receive'] == [('I', 6406)]
assert refusal['give'] == refusal['receive'] == [('I', 6402)] and 'wrong kind of egg' in '\n'.join(refusal['body'])
assert not request['disappear'] and not refusal['disappear']
story = mapping['stories'][0]
assert story['category'] == 'request' and story['contracts'] == [request['binding']]
assert mapping['exclusions'][0]['contracts'] == [refusal['binding']]
assert len(story['steps']) == 4
for step, vnum in zip(story['steps'][:-1], (6403, 6404, 6405)):
    assert (step['kind'], step['item_vnums'], step['count'], step['optional']) == ('carried_item', [vnum], 1, True)
assert story['steps'][-1]['kind'] == 'completion' and story['steps'][-1]['contracts'] == story['contracts']
definitions = {tuple(r['block']['give']): r['definition'] for r in e['requests']}
assert all(not d['prerequisites'] and d['active'] and d['eligible_for_zone_completion'] for d in definitions.values())
assert definitions[tuple(request['give'])]['daily_eligible']
assert not definitions[tuple(refusal['give'])]['daily_eligible'] and definitions[tuple(refusal['give'])]['daily_exclusion'] == 'Item exchange'
units = [u for u in cat.story_units(catalog) if u['zone_number'] == 64]
assert len(units) == 1 and units[0]['achievement'] and units[0]['daily_candidate']
assert cat.report_for(catalog)['eligible_by_zone']['64'] == 1
rooms, mobs, objects = (bodies('bugger', k) for k in ('wld', 'mob', 'obj'))
assert (len(rooms), len(mobs), len(objects)) == (28, 22, 7) and (min(rooms), max(rooms)) == (6400, 6427)
assert {c['mob_vnum'] for c in mapping['contacts']} == set(mobs)
for contact in mapping['contacts']:
    assert contact['keyword'] in mobs[contact['mob_vnum']].split('~')[0].split()
    act = int(mobs[contact['mob_vnum']].split('~', 4)[4].strip().splitlines()[0].split()[0])
    assert not act & (32768 | 2147483648)
assert sum(len(c['topics']) for c in mapping['contacts']) == 3
assert next(c for c in mapping['contacts'] if c['mob_vnum'] == 6402)['topics'] == ['hi', 'hello', 'agitated']
assert (e['zone']['zone_number'], e['zone']['last_vnum'], e['zone']['reset_mode']) == (64, 6427, 2)
assert collections.Counter(r['command'] for r in e['reset_commands']) == {'D': 2, 'O': 4, 'P': 3, 'M': 81, 'E': 1}
resets = {r['line']: (r['command'], r['arguments'][:5]) for r in e['reset_commands']}
for line, command, args in [(17, 'O', [0, 6400, 1, 6413, 100]), (18, 'P', [1, 6405, 1, 6400, 100]),
    (19, 'O', [0, 6404, 2, 6416, 100]), (20, 'O', [0, 6403, 1, 6419, 100]),
    (21, 'O', [0, 6404, 2, 6423, 100]), (30, 'M', [0, 6409, 1, 6402, 100]),
    (31, 'E', [1, 6401, 1, 13, 100]), (32, 'P', [1, 6402, 2, 6401, 100]),
    (33, 'P', [1, 6402, 2, 6401, 100]), (55, 'M', [0, 6402, 1, 6412, 100])]:
    assert resets[line] == (command, args)
assert {r['arguments'][1] for r in e['reset_commands'] if r['command'] == 'M'} == set(mobs)
assert len({objects[v].split('~', 2)[0] for v in (6402, 6403, 6404)}) == 1
assert len({objects[v].split('~', 2)[1] for v in (6402, 6403, 6404)}) == 1
for vnum, extra, kind in [(6402, 0, 19), (6403, 8491016, 12), (6404, 8392712, 13)]:
    flags, values, tail = properties(objects[vnum])
    assert flags[0] == kind and flags[6] == extra and flags[7] == 16385 and flags[8] == 32768
assert properties(objects[6400])[0][0] == 15 and properties(objects[6400])[0][7] == 0
assert properties(objects[6400])[1][:4] == [100, 0, 0, 100]
assert properties(objects[6405])[0][7] == 1 and not properties(objects[6405])[0][6] & 8388608
assert properties(objects[6406])[0][6:8] == [536871232, 9]
assert '\nA\n13 5\nA\n19 1' in objects[6406]
assert all('\nT\n' not in b and properties(b)[0][0] not in (25, 29) for b in objects.values())
graph = [(v, int(m[1]), int(m[4]), int(m[5]), int(m[6])) for v, b in rooms.items()
    for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)', b, re.S)]
assert len(graph) == 56 and len([x for x in graph if x[-1] not in rooms]) == 2
for edge in [(6400, 0, 0, 0, 807099), (6426, 4, 0, 0, 4380), (6425, 0, 5, 0, 6426), (6426, 2, 5, 0, 6425)]:
    assert edge in graph
assert re.search(r'0 0 6400\s', bodies('underdark', 'wld')[807099])
assert re.search(r'0 0 6426\s', bodies('ixxillikor', 'wld')[4380])
controls = {v: list(map(int, b.split('~', 2)[2].strip().splitlines()[0].split())) for v, b in rooms.items()}
assert len({tuple(x) for x in controls.values()}) == 5 and all(x[1] & 1 for x in controls.values())
assert controls[6417][1] == 4361 and all(controls[v][1:3] == [1, 28] for v in (6420, 6421, 6422))
db = (ROOT / 'src/world/db.c').read_text()
issuance = db[db.index('static bool reset_command_issues_item('):db.index('void reset_zone(')]
assert all("case '" + c + "':" in issuance for c in ('O', 'P', 'E'))
p_reset = db[db.index("case 'P': /* object to object */"):db.index("case 'G': /* obj_to_char */")]
assert 'get_obj_num(ZCMD.arg3)' in p_reset
search = (ROOT / 'src/cmd/actobj.c').read_text().split('void do_search(', 1)[1].split('void ', 1)[0]
assert 'ITEM_SECRET' in search and 'REMOVE_BIT(k->extra_flags, ITEM_SECRET)' in search and 'invoke_object_special(k, ch, CMD_FOUND, NULL)' in search
innates = (ROOT / 'src/classes/innates.c').read_text()
prime = innates[innates.index('void do_shift_prime(P_char ch,'):innates.index('void do_blast(P_char ch,')]
assert '6405' in prime and 'INNATE_SHIFT_PRIME' in prime and 'char_to_room(ch, r_room, -1)' in prime
assert not (ROOT / 'areas/shp/bugger.shp').exists() and not e['special_assignments']
guidance = ' '.join(mapping['orientation'] + [s.get('hint', '') for s in story['steps']])
assert all(word in guidance.lower() for word in ('search', 'storage', 'unfinished', 'lost', 'misplaced', 'tasty', 'loose', 'supplied', 'carve'))
tables = []
for name in ('docs/design/zone-stories/BUGGER.md', 'docs/guides/ZONE_STORY_BUILDING.md', 'docs/design/ZONE_STORY_ROADMAP_EXECUTION.md'):
    rows = [line for line in (ROOT / name).read_text().splitlines() if line.startswith('| ZSQ-BUGGER-')]
    assert len(rows) == 28
    tables.append(rows)
assert tables[0] == tables[1] == tables[2]
print('Bugger Caves exact joint eggs, wrong-food exclusion, hidden source, parent, arrival and fair repair contracts passed')
