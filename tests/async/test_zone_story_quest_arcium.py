#!/usr/bin/env python3
"""Protect Arcium's joint offering, source identities and truthful access guidance."""
import collections
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
import zone_story_quest_catalog as cat
import zone_story_quest_zone_inventory as inv


def bodies(kind, area='arcium'):
    text = (ROOT / 'areas' / kind / (area + '.' + kind)).read_text()
    return {int(m[1]): m[2] for m in re.finditer(
        r'^#(\d+)\s*\n([\s\S]*?)(?=^#\d+\s*$|\Z)', text, re.M)}


def properties(body):
    return [list(map(int, x.split())) for x in body.split('~', 4)[4].strip().splitlines()[:3]]


e = inv.area_evidence(ROOT, 'arcium')
catalog = cat.production_catalog(ROOT)
mapping = next(m for m in catalog['story_mappings'] if m['source_area'] == 'arcium')
assert (mapping['schema_version'], mapping['revision'], mapping['coverage']) == (3, 1, 'complete')
assert len(mapping['stories']) == 1 and not mapping['exclusions']
raw = [b for b in inv.native_blocks(ROOT) if b['source'] == 'areas/qst/arcium.qst']
assert [(b['kind'], b['line']) for b in raw] == [('MA', 2), ('QA', 11)]
topics = raw[0]['body'][0].split('~')[0].split()
assert topics == ['hi', 'king', 'kovii', 'granra', 'kelien', 'slican', 'kurlon', 'fragnox', 'lord', 'lords']
q = raw[1]
assert q['give'] == [('I', i) for i in range(85715, 85721)]
assert q['receive'] == [('I', 85721)] and q['disappear']
story = mapping['stories'][0]
assert story['category'] == 'request' and story['contracts'] == [q['binding']]
assert len(story['steps']) == 7
assert [s['item_vnums'] for s in story['steps'][:-1]] == [[i] for i in range(85715, 85721)]
assert all(s['kind'] == 'carried_item' and s['count'] == 1 and s['optional']
           for s in story['steps'][:-1])
assert story['steps'][-1]['kind'] == 'completion'
assert story['steps'][-1]['contracts'] == story['contracts']
definition = e['requests'][0]['definition']
assert definition['eligible_for_zone_completion'] and definition['daily_eligible']
assert not definition['prerequisites']
units = [u for u in cat.story_units(catalog) if u['zone_number'] == 857]
assert len(units) == 1 and units[0]['achievement'] and units[0]['daily_candidate']
rooms, mobs, objects = (bodies(k) for k in ('wld', 'mob', 'obj'))
assert (len(rooms), len(mobs), len(objects)) == (115, 51, 33)
assert {c['mob_vnum'] for c in mapping['contacts']} == set(mobs)
assert next(c for c in mapping['contacts'] if c['mob_vnum'] == 85700)['topics'] == topics
assert sum(len(c['topics']) for c in mapping['contacts']) == 10
assert collections.Counter(r['command'] for r in e['reset_commands']) == {
    'D': 22, 'O': 9, 'P': 4, 'M': 121, 'E': 22, 'G': 19, 'F': 8}
resets = {r['line']: (r['command'], r['arguments'][:5]) for r in e['reset_commands']}
for material, carrier, room, mline, gline in [
        (85715, 85747, 85811, 268, 271), (85716, 85721, 85746, 167, 169),
        (85717, 85724, 85806, 259, 263), (85718, 85723, 85716, 131, 132),
        (85719, 85722, 85757, 184, 186), (85720, 85725, 85752, 174, 178)]:
    assert resets[mline] == ('M', [0, carrier, 1, room, 100])
    assert resets[gline] == ('G', [1, material, 1, 0, 100])
    props = properties(objects[material])
    assert props[0][0] == 8 and props[0][6:9] == [20480, 1, 32768]
    assert props[0][7] & 1  # TAKE: current loose custody can satisfy the return.
for key in (85703, 85705):
    assert properties(objects[key])[0][6] & (1 << 23)  # NORENT differs from accepted history.
    assert properties(objects[key])[1][1] == 100
graph = [(v, int(m[1]), int(m[4]), int(m[5]), int(m[6])) for v, body in rooms.items()
         for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)', body, re.S)]
assert len(graph) == 268
for edge in [(85799, 6, 3, 85703, 85805), (85800, 8, 3, 85703, 85811),
             (85802, 9, 3, 85703, 85806), (85729, 0, 2, 85705, 85733),
             (85803, 5, 5, 0, 85764)]:
    assert edge in graph
commands = (ROOT / 'src/cmd/interp.h').read_text()
assert '#define CMD_ENTER 7' in commands and '#define CMD_DOWN 6' in commands
assert properties(objects[85700])[1][:3] == [85700, 7, -1]
assert properties(objects[85725])[1][:3] == [31300, 7, -1]
dream = bodies('obj', 'dream')
assert properties(dream[31301])[1][:3] == [85805, 15, -1]
assert properties(dream[31303])[1][:3] == [85805, 264, -1]
assert resets[272] == ('G', [1, 31319, 1, 0, 100])
assert 31319 not in [v for kind, v in q['give']]
epic = (ROOT / 'src/classes/epic_skills.c').read_text()
assert '{ 85724, SKILL_SHIELD_COMBAT, 0, 100, 0, 0, 0 }' in epic
assert 'Epic skill purchases are unavailable while economic accounting is active.' in epic
orientation = ' '.join(mapping['orientation'])
assert 'ENTER the fixed zipline cart' in orientation and 'ENTER the fluffy bed' in orientation
assert 'supplied exact hearts' in orientation and 'first recovery' in orientation
tables = []
for name in ('docs/design/zone-stories/ARCIUM.md', 'docs/guides/ZONE_STORY_BUILDING.md',
             'docs/design/ZONE_STORY_ROADMAP_EXECUTION.md'):
    rows = [line for line in (ROOT / name).read_text().splitlines()
            if line.startswith('| ZSQ-ARCIUM-')]
    assert len(rows) == 37
    tables.append(rows)
assert tables[0] == tables[1] == tables[2]
print('Arcium joint offering, exact materials, ENTER travel, foreign ownership and journal boundaries passed')
