#!/usr/bin/env python3
"""Protect Jindon's native arms return and separate payment/travel/aid outcomes."""
import collections
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
import zone_story_quest_catalog as cat
import zone_story_quest_zone_inventory as inv


def bodies(kind, area='jin'):
    text = (ROOT / 'areas' / kind / (area + '.' + kind)).read_text()
    return {int(m[1]): m[2] for m in re.finditer(
        r'^#(\d+)\s*\n([\s\S]*?)(?=^#\d+\s*$|\Z)', text, re.M)}


def properties(body):
    return [list(map(int, x.split())) for x in body.split('~', 4)[4].strip().splitlines()[:3]]


e = inv.area_evidence(ROOT, 'jin')
catalog = cat.production_catalog(ROOT)
mapping = next(m for m in catalog['story_mappings'] if m['source_area'] == 'jin')
assert (mapping['schema_version'], mapping['revision'], mapping['coverage']) == (3, 1, 'complete')
assert len(mapping['stories']) == 1 and not mapping['exclusions']
raw = [b for b in inv.native_blocks(ROOT) if b['source'] == 'areas/qst/jin.qst']
assert [(b['kind'], b['line']) for b in raw] == [('M', 2), ('Q', 8)]
topics = raw[0]['body'][0].split('~')[0].split()
assert topics == ['thri-kreen', 'two', 'limb', 'limbs', 'ogre', 'missing', 'battle']
q = raw[1]
assert q['give'] == [('I', 82004)] and q['receive'] == [('I', 82003)] and not q['disappear']
story = mapping['stories'][0]
assert story['id'] == 'sirax-lost-arms' and story['category'] == 'request'
assert story['contracts'] == [q['binding']] and len(story['steps']) == 2
material, accepted = story['steps']
assert material['kind'] == 'carried_item' and material['item_vnums'] == [82004]
assert material['count'] == 1 and material['optional']
assert accepted['kind'] == 'completion' and accepted['contracts'] == story['contracts']
definition = e['requests'][0]['definition']
assert definition['eligible_for_zone_completion'] and definition['daily_eligible']
assert not definition['prerequisites']
units = [u for u in cat.story_units(catalog) if u['zone_number'] == 820]
assert len(units) == 1 and units[0]['achievement'] and units[0]['daily_candidate']
assert e['zone']['reset_mode'] == 2 and e['zone']['first_vnum'] == 81678
assert (ROOT / 'areas/zon/jin.zon').read_text().splitlines()[2] == '82089 2 0 40 50 1'
rooms, mobs, objects = (bodies(k) for k in ('wld', 'mob', 'obj'))
assert (len(rooms), len(mobs), len(objects)) == (90, 48, 37)
assert {c['mob_vnum'] for c in mapping['contacts']} == set(mobs)
assert next(c for c in mapping['contacts'] if c['mob_vnum'] == 82007)['topics'] == topics
assert sum(len(c['topics']) for c in mapping['contacts']) == 7
stocked = {r['arguments'][1] for r in e['reset_commands'] if r['command'] == 'M'}
assert set(mobs) - stocked == {82015}
assert 'no local stock declaration' in next(c for c in mapping['contacts'] if c['mob_vnum'] == 82015)['description']
assert collections.Counter(r['command'] for r in e['reset_commands']) == {
    'M': 86, 'E': 25, 'G': 9, 'O': 4, 'D': 8}
resets = {r['line']: (r['command'], r['arguments'][:5]) for r in e['reset_commands']}
for line, command, args in (
    (58, 'M', [0, 82006, 1, 82018, 100]),
    (59, 'G', [1, 82004, 1, 0, 100]),
    (60, 'M', [0, 82007, 1, 82019, 100]),
    (128, 'M', [0, 82000, 1, 82063, 100]),
    (129, 'G', [1, 82019, 1, 0, 100]),
    (43, 'O', [0, 82030, 1, 82080, 100]),
    (44, 'O', [0, 82024, 1, 82082, 100]),
    (145, 'M', [0, 82045, 1, 82080, 100]),
):
    assert resets[line] == (command, args), (line, resets[line])
arms, ticket, whirlpool, altar = (properties(objects[v]) for v in (82004, 82019, 82030, 82024))
assert arms[0][0] == 8 and arms[0][6:9] == [0, 1, 32768]
assert not arms[0][6] & ((1 << 12) | (1 << 23))  # Neither SECRET nor NORENT.
assert ticket[0][0] == 25 and ticket[0][7] == 0 and ticket[1][:3] == [82062, 10, 1]
assert whirlpool[0][0] == 25 and whirlpool[0][7] == 0 and whirlpool[1][:3] == [82065, 7, -1]
assert altar[0][0] == 15 and altar[0][7] == 0 and altar[1][:4] == [50, 15, 0, 50]
assert re.search(r'^T\s+516\s+15\s+-1\s+35\s*$', objects[82024], re.M)
graph = [(v, int(m[1]), int(m[4]), int(m[5]), int(m[6])) for v, body in rooms.items()
         for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)', body, re.S)]
assert len(graph) == 193 and all(key == 0 for _, _, _, key, _ in graph)
assert (82062, 5, 4, 0, 82064) in graph and (82064, 4, 0, 0, 82062) in graph
assert {target for _, _, _, _, target in graph if target not in rooms} == {619653}
assert not any(re.search(r'^[EF](?:\s|$)', body, re.M) for body in rooms.values())
assert properties(objects[82032])[1][:4] == [30, 1, 1, 239]
assert properties(objects[82007])[1][:4] == [26, 141, 1, 192]
shop = (ROOT / 'areas/shp/jin.shp').read_text()
assert re.findall(r'^#(\d+)~\s*$', shop, re.M) == ['82028']
orientation = ' '.join(mapping['orientation'])
assert 'supplied by another player' in orientation and 'seven aliases share one' in orientation
assert 'does not by itself record healing, reattachment, rescue or safe departure' in orientation
assert 'currently unavailable while accounting is active' in orientation
assert 'rather than rescuing' in orientation and 'Daily replay remains disabled' in orientation
tables = []
for name in ('docs/design/zone-stories/JINDON_DEATHWOOD.md', 'docs/guides/ZONE_STORY_BUILDING.md',
             'docs/design/ZONE_STORY_ROADMAP_EXECUTION.md'):
    rows = [line for line in (ROOT / name).read_text().splitlines() if line.startswith('| ZSQ-JIN-')]
    assert len(rows) == 44
    tables.append(rows)
assert tables[0] == tables[1] == tables[2]
print('Jindon exact arms, source/payment/travel, altar and independent receipt boundaries passed')
