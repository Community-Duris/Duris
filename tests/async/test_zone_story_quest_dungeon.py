#!/usr/bin/env python3
"""Protect Treasure Caves' exact return and source/control/history boundaries."""
import collections
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
import zone_story_quest_catalog as cat
import zone_story_quest_zone_inventory as inv


def bodies(kind, area='dungeon'):
    text = (ROOT / 'areas' / kind / (area + '.' + kind)).read_text()
    return {int(m[1]): m[2] for m in re.finditer(
        r'^#(\d+)\s*\n([\s\S]*?)(?=^#\d+\s*$|\Z)', text, re.M)}


def properties(body):
    return [list(map(int, x.split())) for x in body.split('~', 4)[4].strip().splitlines()[:3]]


e = inv.area_evidence(ROOT, 'dungeon')
catalog = cat.production_catalog(ROOT)
mapping = next(m for m in catalog['story_mappings'] if m['source_area'] == 'dungeon')
assert (mapping['schema_version'], mapping['revision'], mapping['coverage']) == (3, 1, 'complete')
assert len(mapping['stories']) == 1 and not mapping['exclusions']
raw = [b for b in inv.native_blocks(ROOT) if b['source'] == 'areas/qst/dungeon.qst']
assert [(b['kind'], b['line']) for b in raw] == [('M', 2), ('Q', 13)]
topics = raw[0]['body'][0].split('~')[0].split()
assert topics == ['hi', 'hello', 'hail', 'help', 'quest']
q = raw[1]
assert q['give'] == [('I', 93006)] and q['receive'] == [('C', 10000)] and not q['disappear']
story = mapping['stories'][0]
assert story['id'] == 'thief-brass-figurine' and story['category'] == 'request'
assert story['contracts'] == [q['binding']] and len(story['steps']) == 2
material, accepted = story['steps']
assert material['kind'] == 'carried_item' and material['item_vnums'] == [93006]
assert material['count'] == 1 and material['optional']
assert accepted['kind'] == 'completion' and accepted['contracts'] == story['contracts']
definition = e['requests'][0]['definition']
assert definition['eligible_for_zone_completion'] and definition['daily_eligible']
assert not definition['prerequisites']
units = [u for u in cat.story_units(catalog) if u['zone_number'] == 930]
assert len(units) == 1 and units[0]['achievement'] and units[0]['daily_candidate']
assert e['zone']['reset_mode'] == 2 and e['zone']['first_vnum'] == 92520
assert (ROOT / 'areas/zon/dungeon.zon').read_text().splitlines()[2] == '93115 2 0 10 12 1'
assert not (ROOT / 'areas/shp/dungeon.shp').exists()
rooms, mobs, objects = (bodies(k) for k in ('wld', 'mob', 'obj'))
assert (len(rooms), len(mobs), len(objects)) == (116, 36, 24)
assert {c['mob_vnum'] for c in mapping['contacts']} == set(mobs)
assert next(c for c in mapping['contacts'] if c['mob_vnum'] == 93001)['topics'] == topics
assert sum(len(c['topics']) for c in mapping['contacts']) == 5
stocked = {r['arguments'][1] for r in e['reset_commands'] if r['command'] in ('M', 'R')}
assert set(mobs) == stocked
assert collections.Counter(r['command'] for r in e['reset_commands']) == {
    'D': 8, 'O': 4, 'P': 4, 'M': 139, 'E': 34, 'R': 2, 'G': 3}
resets = {r['line']: (r['command'], r['arguments'][:5]) for r in e['reset_commands']}
assert resets[54] == ('M', [0, 93001, 1, 93006, 100])
assert resets[43] == ('O', [0, 93009, 1, 93107, 100])
assert resets[44] == ('P', [1, 93006, 1, 93009, 100])
assert resets[197] == ('M', [0, 93020, 1, 93107, 100])
assert resets[201] == ('G', [1, 93016, 1, 0, 100])
figurine, chest, key = (properties(objects[v]) for v in (93006, 93009, 93016))
assert figurine[0][0] == 8 and figurine[0][6:9] == [1, 1, 32768]
assert not figurine[0][6] & ((1 << 12) | (1 << 23))  # Prose invisibility is not SECRET/NORENT.
assert chest[0][0] == 15 and chest[0][7] == 0
assert chest[1][:4] == [50, 29, 93016, 250]  # CLOSEABLE/CLOSED/LOCKED/PICKPROOF.
assert key[0][0] == 18 and key[0][6] & (1 << 23) and key[0][7] & 1
assert key[1][1] == 0  # No native key-break roll.
vines = properties(objects[93023])
assert vines[0][0] == 29 and vines[0][6:8] == [2, 0]  # NOSHOW, nonTAKE.
assert vines[1][:4] == [340, 93002, 5, 1]
assert resets[41] == ('O', [0, 93023, 1, 93002, 100])
graph = [(v, int(m[1]), int(m[4]), int(m[5]), int(m[6])) for v, body in rooms.items()
         for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)', body, re.S)]
assert len(graph) == 236 and all(key == 0 for _, _, _, key, _ in graph)
assert (93002, 5, 1, 0, 93003) in graph and (93003, 4, 1, 0, 93002) in graph
assert {target for _, _, _, _, target in graph if target not in rooms} == {36004, 810962}
door_states = {(r['arguments'][1], r['arguments'][2]): r['arguments'][3]
               for r in e['reset_commands'] if r['command'] == 'D'}
assert door_states[(93002, 5)] == 9 and door_states[(93003, 4)] == 1
falls = {v: int(m[1]) for v, body in rooms.items()
         for m in re.finditer(r'^F[ \t]*(?:\n)?(-?\d+)[ \t]*$', body, re.M)}
assert falls == {93022: 20, 93074: 5, 93081: 100, 93082: 100, 93083: 100}
assert sum(len(re.findall(r'^E\s*$', body, re.M)) for body in rooms.values()) == 7
for area, room, target in (('arac_wild', 36004, 93115), ('underdark', 810962, 93069)):
    assert re.search(r'\bD1\s+([^~]*)~([^~]*)~\s*0\s+0\s+' + str(target), bodies('wld', area)[room], re.S)
assert resets[107] == ('M', [0, 93016, 1, 93028, 100])
assert resets[108] == ('E', [1, 93007, 1, 16, 100])
assert resets[151] == ('M', [0, 93017, 1, 93063, 100])
assert resets[153] == ('M', [0, 93034, 1, 93063, 100])
hoard = properties(objects[93008])
assert hoard[0][0] == 8 and hoard[0][7] == 0 and not any(hoard[1])
assert properties(objects[93017])[0][0] == 15
assert properties(objects[93018])[1][:2] == [30, 43]
assert resets[45] == ('O', [0, 93017, 1, 93107, 100])
for line in (46, 47, 48):
    assert resets[line] == ('P', [1, 93018, 3, 93017, 100])
orientation = ' '.join(mapping['orientation'])
assert 'supplied by another player' in orientation and 'five aliases share one' in orientation
assert 'Opening the door and crossing are separate' in orientation
assert 'intended location hint needs builder review' in orientation
assert 'configured to remove poison' in orientation
tables = []
for name in ('docs/design/zone-stories/TREASURE_CAVES.md', 'docs/guides/ZONE_STORY_BUILDING.md',
             'docs/design/ZONE_STORY_ROADMAP_EXECUTION.md'):
    rows = [line for line in (ROOT / name).read_text().splitlines()
            if line.startswith('| ZSQ-DUNGEON-')]
    assert len(rows) == 40
    tables.append(rows)
assert tables[0] == tables[1] == tables[2]
print('Treasure Caves exact figurine, source/key/chest, vine and independent receipt boundaries passed')
