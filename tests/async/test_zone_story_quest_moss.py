#!/usr/bin/env python3
"""Protect Mosswood's supplied beet trade and broader integration boundaries."""
import collections
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
import zone_story_quest_catalog as cat
import zone_story_quest_zone_inventory as inv


def bodies(kind):
    text = (ROOT / 'areas' / kind / ('moss.' + kind)).read_text()
    return {int(m[1]): m[2] for m in re.finditer(r'^#(\d+)\s*\n([\s\S]*?)(?=^#\d+\s*$|\Z)', text, re.M)}


e = inv.area_evidence(ROOT, 'moss')
catalog = cat.production_catalog(ROOT)
m = next(m for m in catalog['story_mappings'] if m['source_area'] == 'moss')
assert (m['schema_version'], m['revision'], m['coverage']) == (3, 1, 'complete')
assert len(m['stories']) == 1 and not m['exclusions'] and len(m['orientation']) == 16
raw = [b for b in inv.native_blocks(ROOT) if b['source'] == 'areas/qst/moss.qst']
assert [(b['kind'], b['line']) for b in raw] == [('M', 2), ('Q', 9)]
topics = raw[0]['body'][0].split('~')[0].split()
assert topics == ['hi', 'hello']
q = raw[1]
assert q['give'] == [('I', 23436)] and q['receive'] == [('C', 1000)] and not q['disappear']
s = m['stories'][0]
assert s['id'] == 'beet-for-ijale' and s['category'] == 'request' and s['contracts'] == [q['binding']]
material, accepted = s['steps']
assert (material['kind'], material['item_vnums'], material['count'], material['optional']) == ('carried_item', [23436], 1, True)
assert accepted['kind'] == 'completion' and accepted['contracts'] == s['contracts']
definition = e['requests'][0]['definition']
assert not definition['prerequisites'] and definition['daily_eligible'] and definition['eligible_for_zone_completion']
units = [u for u in cat.story_units(catalog) if u['zone_number'] == 234]
assert len(units) == 1 and units[0]['achievement'] and units[0]['daily_candidate']
rooms, mobs, objects = (bodies(k) for k in ('wld', 'mob', 'obj'))
assert (len(rooms), len(mobs), len(objects)) == (161, 128, 37)
assert {c['mob_vnum'] for c in m['contacts']} == set(mobs) == set(range(23400, 23528))
assert next(c for c in m['contacts'] if c['mob_vnum'] == 23518)['topics'] == topics
assert sum(len(c['topics']) for c in m['contacts']) == 2
stock = {r['arguments'][1] for r in e['reset_commands'] if r['command'] in ('M', 'F')}
assert set(mobs) - stock == {23417, 23447, 23490}
for v in set(mobs) - stock:
    assert 'no M/F reset source' in next(c for c in m['contacts'] if c['mob_vnum'] == v)['description']
assert (e['zone']['first_vnum'], e['zone']['last_vnum'], e['zone']['reset_mode']) == (23326, 23560, 2)
assert (ROOT / 'areas/zon/moss.zon').read_text().splitlines()[2] == '23560 2 0 40 80 1'
assert collections.Counter(r['command'] for r in e['reset_commands']) == {'M': 183, 'F': 2, 'G': 26, 'O': 3, 'D': 36}
produced = {r['arguments'][1] for r in e['reset_commands'] if r['command'] in ('G', 'O')}
assert set(objects) - produced == {23418, *range(23427, 23435)}
assert 'defined without a local reset source' in next(c for c in m['contacts'] if c['mob_vnum'] == 23414)['description']
beets = [r['arguments'][:5] for r in e['reset_commands'] if r['command'] == 'O' and r['arguments'][1] == 23436]
assert {(r[1], r[2], r[3]) for r in beets} == {(23436, 2, 23444), (23436, 2, 23448)}
beet = [list(map(int, x.split())) for x in objects[23436].split('~', 4)[4].strip().splitlines()[:3]]
assert beet[0][0] == 19 and beet[0][6:10] == [4096, 1, 32768, 0] and beet[1][:4] == [2, 0, 0, 0]
pond = [list(map(int, x.split())) for x in objects[23400].split('~', 4)[4].strip().splitlines()[:3]]
assert pond[0][0] == 17 and pond[0][7] == 0 and pond[1][:4] == [600, 600, 16, 0]
graph = [(v, int(x[1]), int(x[4]), int(x[5]), int(x[6])) for v, body in rooms.items()
         for x in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)', body, re.S)]
assert len(graph) == 399 and {target for _, _, _, _, target in graph if target not in rooms} == {611257}
assert (23475, 1, 2, 0, 23484) in graph and (23484, 3, 2, 0, 23475) in graph
assert (23539, 0, 0, 4, 23540) in graph
assert not any(re.search(r'^[EFC]$', b, re.M) for b in rooms.values())
reached = {23400}; pending = [23400]
while pending:
    room = pending.pop()
    for source, _, _, _, target in graph:
        if source == room and target in rooms and target not in reached:
            reached.add(target); pending.append(target)
assert len(reached) == 157 and set(rooms) - reached == {23434, 23558, 23559, 23560}
shop = (ROOT / 'areas/shp/moss.shp').read_text()
assert re.findall(r'^#(\d+)~', shop, re.M) == ['23408', '23518', '23525']
assert not e['special_assignments']
tables = []
for name in ('docs/design/zone-stories/MOSSWOOD.md', 'docs/guides/ZONE_STORY_BUILDING.md', 'docs/design/ZONE_STORY_ROADMAP_EXECUTION.md'):
    text = (ROOT / name).read_text(encoding='utf8')
    rows = [line for line in text.splitlines() if re.match(r'^\| ZSQ-MOSS-\d+\s*\|', line)]
    assert len(rows) == 42 and [re.match(r'\| (ZSQ-MOSS-\d+)', row)[1] for row in rows] == ['ZSQ-MOSS-' + str(n).zfill(2) for n in range(1, 43)]
    tables.append(rows)
assert tables[0] == tables[1] == tables[2]
dossier = (ROOT / 'docs/design/zone-stories/MOSSWOOD.md').read_text(encoding='utf8')
assert len(re.findall(r'^\| (?:Ijale|Field work|Livestock,|Village crafts|Wizard household|Captain,|Pond,|Swamp |Rocky routes|Households,)', dossier, re.M)) == 10
for phrase in ('active READY accounting', 'Daily policy remains disabled', 'An exact supplied beet remains eligible', 'nine broader', 'no departure', 'separate named fix/news commits'):
    assert phrase in dossier, phrase
print('Mosswood: exact supplied beet/C1000, alias/food/source/shop boundaries and comprehensive wider follow-ups retained.')
