#!/usr/bin/env python3
"""Protect Future Ceothia's exact return, mixed reward and time-route boundaries."""
import collections
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
import zone_story_quest_catalog as cat
import zone_story_quest_zone_inventory as inv


def bodies(kind, area='ceofutur'):
    text = (ROOT / 'areas' / kind / (area + '.' + kind)).read_text()
    return {int(m[1]): m[2] for m in re.finditer(
        r'^#(\d+)\s*\n([\s\S]*?)(?=^#\d+\s*$|\Z)', text, re.M)}


def properties(body):
    return [list(map(int, x.split())) for x in body.split('~', 4)[4].strip().splitlines()[:3]]


e = inv.area_evidence(ROOT, 'ceofutur')
catalog = cat.production_catalog(ROOT)
mapping = next(m for m in catalog['story_mappings'] if m['source_area'] == 'ceofutur')
assert (mapping['schema_version'], mapping['revision'], mapping['coverage']) == (3, 1, 'complete')
assert len(mapping['stories']) == 1 and not mapping['exclusions']
raw = [b for b in inv.native_blocks(ROOT) if b['source'] == 'areas/qst/ceofutur.qst']
assert [(b['kind'], b['line']) for b in raw] == [('M', 2), ('Q', 27)]
topics = raw[0]['body'][0].split('~')[0].split()
assert topics == ['hello', 'hi', 'howdy']
q = raw[1]
assert q['give'] == [('I', 81407)]
assert q['receive'] == [('I', 81406), ('C', 500000)] and q['disappear']
story = mapping['stories'][0]
assert story['id'] == 'thief-leader-bluestone-vial'
assert story['category'] == 'request' and story['contracts'] == [q['binding']]
assert len(story['steps']) == 2
material, accepted = story['steps']
assert material['kind'] == 'carried_item' and material['item_vnums'] == [81407]
assert material['count'] == 1 and material['optional']
assert accepted['kind'] == 'completion' and accepted['contracts'] == story['contracts']
definition = e['requests'][0]['definition']
assert definition['eligible_for_zone_completion'] and definition['daily_eligible']
assert not definition['prerequisites']
units = [u for u in cat.story_units(catalog) if u['zone_number'] == 814]
assert len(units) == 1 and units[0]['achievement'] and units[0]['daily_candidate']
assert e['zone']['reset_mode'] == 1 and e['zone']['first_vnum'] == 81393
assert (ROOT / 'areas/zon/ceofutur.zon').read_text().splitlines()[2] == '81677 1 0 40 50 2'
assert not (ROOT / 'areas/shp/ceofutur.shp').exists()
rooms, mobs, objects = (bodies(k) for k in ('wld', 'mob', 'obj'))
assert (len(rooms), len(mobs), len(objects)) == (278, 57, 26)
assert {c['mob_vnum'] for c in mapping['contacts']} == set(mobs)
assert next(c for c in mapping['contacts'] if c['mob_vnum'] == 81404)['topics'] == topics
assert sum(len(c['topics']) for c in mapping['contacts']) == 3
stocked = {r['arguments'][1] for r in e['reset_commands'] if r['command'] in ('M', 'F')}
assert set(mobs) - stocked == {81407}
assert 'no local stock declaration' in next(c for c in mapping['contacts'] if c['mob_vnum'] == 81407)['description']
assert collections.Counter(r['command'] for r in e['reset_commands']) == {
    'D': 6, 'O': 9, 'M': 203, 'E': 54, 'G': 9, 'F': 6}
resets = {r['line']: (r['command'], r['arguments'][:5]) for r in e['reset_commands']}
assert resets[53] == ('M', [0, 81414, 1, 81407, 100])  # This is a room, not an item producer.
assert not any(r['arguments'][1] == 81407 for r in e['reset_commands']
               if r['command'] in ('O', 'G', 'E', 'P'))
assert resets[178] == ('M', [0, 81404, 1, 81572, 100])
vial = properties(objects[81407])
assert vial[0][0] == 10 and vial[0][6:9] == [8400896, 16385, 32832]
assert vial[0][7] & 1  # TAKE: exact loose supplied custody is accepted.
assert vial[1][:4] == [60, 1, 28, 36]  # Armor, heal and stone skin; no escape spell.
for key in (81406, 81411, 81412):
    assert properties(objects[key])[0][0] == 18
    assert properties(objects[key])[0][6] & (1 << 23)  # NORENT is not durable quest history.
    assert properties(objects[key])[1][1] == 100
for line, item in ((61, 81406), (256, 81412), (300, 81411), (309, 81410), (311, 81423)):
    assert resets[line] == ('G', [1, item, 1, 0, 100])
graph = [(v, int(m[1]), int(m[4]), int(m[5]), int(m[6])) for v, body in rooms.items()
         for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)', body, re.S)]
assert len(graph) == 1016
assert all(81400 <= target <= 81677 for _, _, _, _, target in graph)
for edge in [(81472, 5, 7, 81406, 81660), (81660, 4, 3, 81406, 81472),
             (81662, 1, 3, 81412, 81663), (81663, 3, 3, 81412, 81662),
             (81669, 1, 7, 81411, 81670), (81670, 3, 3, 81411, 81669)]:
    assert edge in graph
assert not any(target in (81671, 81674) and source not in (81671, 81674)
               for source, _, _, _, target in graph)
assert not any(source == 81675 for source, _, _, _, _ in graph)
commands = (ROOT / 'src/cmd/interp.h').read_text()
assert '#define CMD_ENTER 7' in commands and '#define CMD_DOWN 6' in commands
assert properties(objects[81422])[1][:3] == [81676, 7, -1]
assert properties(objects[81413])[1][:3] == [80980, 7, -1]
assert properties(bodies('obj', 'ceopast')[81111])[1][:3] == [81400, 7, -1]
present = [b for b in inv.native_blocks(ROOT) if b['source'] == 'areas/qst/ceothia.qst']
assert next(b for b in present if b['line'] == 185)['give'] == [('I', 81410)]
assert next(b for b in present if b['line'] == 202)['give'] == [('I', 81423)]
assert 81410 not in [v for kind, v in q['give']] and 81423 not in [v for kind, v in q['give']]
orientation = ' '.join(mapping['orientation'])
assert 'supplied by another player' in orientation and 'different destinations' in orientation
assert 'does not establish a safe destination' in orientation
tables = []
for name in ('docs/design/zone-stories/FUTURE_CEOTHIA.md', 'docs/guides/ZONE_STORY_BUILDING.md',
             'docs/design/ZONE_STORY_ROADMAP_EXECUTION.md'):
    rows = [line for line in (ROOT / name).read_text().splitlines()
            if line.startswith('| ZSQ-CEOFUTUR-')]
    assert len(rows) == 42
    tables.append(rows)
assert tables[0] == tables[1] == tables[2]
print('Future Ceothia exact vial, mixed reward, key/time routes and independent receipt boundaries passed')
