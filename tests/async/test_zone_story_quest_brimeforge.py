#!/usr/bin/env python3
"""BrimStone Forge joint-set, independent rune and exact access/source contracts."""
import collections
from pathlib import Path
import re
import sys
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
import zone_story_quest_catalog as cat
import zone_story_quest_zone_inventory as inv

def bodies(kind):
    return {int(m[1]): m[2] for m in re.finditer(r'^#(\d+)\s*\n([\s\S]*?)(?=^#\d+\s*$|\Z)',
            (ROOT / 'areas' / kind / ('brimeforge.' + kind)).read_text(), re.M)}

def properties(body):
    rows = body.split('~', 4)[4].strip().splitlines()
    return [list(map(int, row.split())) for row in rows[:3]]

e = inv.area_evidence(ROOT, 'brimeforge')
catalog = cat.production_catalog(ROOT)
mapping = next(m for m in catalog['story_mappings'] if m['source_area'] == 'brimeforge')
assert (mapping['schema_version'], mapping['revision'], mapping['coverage']) == (3, 1, 'complete')
assert len(mapping['stories']) == 2 and len(mapping['contacts']) == 16 and not mapping['exclusions']
assert len(e['requests']) == 2 and len(e['dialogue']) == 1
raw = [b for b in inv.native_blocks(ROOT) if b['source'] == 'areas/qst/brimeforge.qst']
assert [(b['kind'], b['line']) for b in raw] == [('M', 2), ('Q', 8), ('Q', 18)]
assert raw[0]['body'][0] == 'hi hello hey howdy~'
for story, q, giver, materials, rewards in zip(mapping['stories'], e['requests'], (131009, 131011),
        ([131002, 131003, 131004], [131019]),
        ([('I', 131005), ('E', 300000), ('I', 131006)], [('I', 131012)])):
    b = q['block']
    assert (b['giver_vnum'], b['give'], b['receive'], b['disappear']) == (giver, [('I', v) for v in materials], rewards, False)
    assert b['body'] == ['~'] and not q['definition']['prerequisites'] and q['definition']['daily_eligible']
    assert story['contracts'] == [b['binding']] and len(story['steps']) == len(materials) + 1
    for step, item in zip(story['steps'][:-1], materials):
        assert (step['kind'], step['item_vnums'], step['count'], step['optional']) == ('carried_item', [item], 1, True)
    assert story['steps'][-1]['kind'] == 'completion' and story['steps'][-1]['contracts'] == story['contracts']
rooms, mobs, objects = (bodies(kind) for kind in ('wld', 'mob', 'obj'))
assert (len(rooms), len(mobs), len(objects)) == (49, 16, 20)
assert min(rooms) == 131001 and max(rooms) == 131049
assert {c['mob_vnum'] for c in mapping['contacts']} == set(mobs)
for contact in mapping['contacts']:
    assert contact['keyword'] in mobs[contact['mob_vnum']].split('~')[0].split()
    act = int(mobs[contact['mob_vnum']].split('~', 4)[4].strip().splitlines()[0].split()[0])
    assert not act & (32768 | 2147483648)
giver = next(c for c in mapping['contacts'] if c['mob_vnum'] == 131009)
assert giver['topics'] == ['hi', 'hello', 'hey', 'howdy'] and sum(len(c['topics']) for c in mapping['contacts']) == 4
assert (e['zone']['zone_number'], e['zone']['last_vnum'], e['zone']['reset_mode']) == (1310, 131049, 2)
assert len(e['reset_commands']) == 120
assert collections.Counter(r['command'] for r in e['reset_commands']) == {'D': 24, 'O': 14, 'M': 53, 'E': 22, 'G': 4, 'F': 3}
resets = {r['line']: (r['command'], r['arguments'][:5]) for r in e['reset_commands']}
for line, command, args in [(118, 'O', [0, 131014, 1, 131048, 100]), (120, 'M', [0, 131009, 1, 131001, 100]),
                          (121, 'M', [0, 131008, 3, 131004, 100]), (122, 'E', [1, 131003, 1, 3, 100]),
                          (129, 'M', [0, 131008, 3, 131009, 100]), (130, 'E', [1, 131004, 1, 3, 100]),
                          (149, 'M', [0, 131008, 3, 131016, 100]), (150, 'E', [1, 131002, 1, 3, 100]),
                          (163, 'M', [0, 131016, 1, 131027, 100]), (164, 'G', [1, 131013, 1, 0, 100]),
                          (192, 'M', [0, 131011, 1, 131038, 100])]:
    assert resets[line] == (command, args)
stocked = {r['arguments'][1] for r in e['reset_commands'] if r['command'] in ('M', 'F')}
assert set(mobs) == stocked
producers = []
for path in (ROOT / 'areas/zon').glob('*.zon'):
    for row in path.read_text().splitlines():
        if re.match(r'[OPGE]\s+-?\d+\s+(?:131002|131003|131004|131019|131020)\s', row):
            producers.append((path.stem, row.split()[:6]))
assert sorted(producers) == [('brimeforge', ['E', '1', str(v), '1', '3', '100']) for v in (131002, 131003, 131004)]
for v in (131002, 131003, 131004, 131019):
    flags, values, tail = properties(objects[v])
    assert flags[7] & 1 and flags[8] & 32768 and flags[6] & 8388608
assert properties(objects[131002])[2] == [0, 0, 100, 0, 0, 0, 32]
defines = (ROOT / 'src/core/defines.h').read_text()
assert '#define AFF4_STORNOGS_GREATER_SPHERES BIT_6' in defines
assert properties(objects[131005])[0][0] == 3 and properties(objects[131005])[1][:4] == [41, 4, 4, 277]
assert '#define SPELL_IMMOLATE 277' in (ROOT / 'src/magic/spells.h').read_text()
for v in (131006, 131012, 131020):
    assert properties(objects[v])[0][0] == 18 and properties(objects[v])[1][1] == 100
flags, values, tail = properties(objects[131014])
assert flags[0] == 15 and flags[7] == 0 and flags[6] & 8388608 and values[:4] == [0, 29, 131020, 0]
assert not any(r['command'] == 'P' for r in e['reset_commands'])
assert all('\nT\n' not in body for body in objects.values())
graph = [(v, int(m[1]), int(m[4]), int(m[5]), int(m[6])) for v, b in rooms.items()
         for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)', b, re.S)]
assert len(graph) == 98 and len([x for x in graph if x[-1] not in rooms]) == 2
for edge in [(131018, 1, 3, 131006, 131027), (131027, 0, 3, 0, 131042),
             (131038, 0, 3, 131012, 131039), (131038, 1, 3, 131013, 131040),
             (131038, 2, 3, 131014, 131041), (131036, 3, 0, 0, 53802), (131037, 1, 0, 0, 5879)]:
    assert edge in graph
db = (ROOT / 'src/world/db.c').read_text()
loader = db[db.index('void setup_dir('):db.index('void renum_world(')]
assert 'if (state == 3)' in loader and 'exit_info |= EX_PICKPROOF' in loader
knock = (ROOT / 'src/magic/spell_item_enhancement.c').read_text()
knock = knock[knock.index('void spell_knock('):knock.index('void spell_create_water(')]
assert 'CONT_PICKPROOF' in knock and 'FIND_OBJ_INV | FIND_OBJ_ROOM' in knock and 'find_door(' not in knock
control = list(map(int, rooms[131026].split('~', 2)[2].strip().splitlines()[0].split()))
assert control[1] & 131072 and not control[1] & 2048
assert len({int(b.split('~', 2)[2].strip().splitlines()[0].split()[1]) for b in rooms.values()}) == 6
assert not (ROOT / 'areas/shp/brimeforge.shp').exists() and not e['special_assignments']
assert cat.report_for(catalog)['eligible_by_zone']['1310'] == 2
tables = []
for name in ('docs/design/zone-stories/BRIMEFORGE.md', 'docs/guides/ZONE_STORY_BUILDING.md', 'docs/design/ZONE_STORY_ROADMAP_EXECUTION.md'):
    rows = [line for line in (ROOT / name).read_text().splitlines() if line.startswith('| ZSQ-BRIMEFORGE-')]
    assert len(rows) == 32
    tables.append(rows)
assert tables[0] == tables[1] == tables[2]
print('BrimStone Forge exact joint set, independent rune, source identity, pickproof access, device and fair repair contracts passed')
