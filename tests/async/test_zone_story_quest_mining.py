#!/usr/bin/env python3
"""Protect the native whip exchange and distinct mine prerequisite/outcome stories."""
import collections
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
import zone_story_quest_catalog as cat
import zone_story_quest_zone_inventory as inv


def bodies(kind):
    text = (ROOT / 'areas' / kind / ('mining.' + kind)).read_text()
    return {int(m[1]): m[2] for m in re.finditer(r'^#(\d+)\s*\n([\s\S]*?)(?=^#\d+\s*$|\Z)', text, re.M)}


def properties(body):
    return [list(map(int, x.split())) for x in body.split('~', 4)[4].strip().splitlines()[:3]]


e = inv.area_evidence(ROOT, 'mining')
catalog = cat.production_catalog(ROOT)
m = next(m for m in catalog['story_mappings'] if m['source_area'] == 'mining')
assert (m['schema_version'], m['revision'], m['coverage']) == (3, 1, 'complete')
assert len(m['stories']) == 1 and not m['exclusions'] and len(m['orientation']) == 16
raw = [b for b in inv.native_blocks(ROOT) if b['source'] == 'areas/qst/mining.qst']
assert [(b['kind'], b['line']) for b in raw] == [('M', 2), ('Q', 8)]
topics = raw[0]['body'][0].split('~')[0].split()
assert topics == ['worried', 'worry', 'hi']
q = raw[1]
assert q['give'] == [('I', 4911)] and q['receive'] == [('I', 4912)] and not q['disappear']
s = m['stories'][0]
assert s['id'] == 'balor-whip-for-priest' and s['category'] == 'request' and s['contracts'] == [q['binding']]
material, accepted = s['steps']
assert (material['kind'], material['item_vnums'], material['count'], material['optional']) == ('carried_item', [4911], 1, True)
assert accepted['kind'] == 'completion' and accepted['contracts'] == s['contracts']
definition = e['requests'][0]['definition']
assert not definition['prerequisites'] and definition['daily_eligible'] and definition['eligible_for_zone_completion']
units = [u for u in cat.story_units(catalog) if u['zone_number'] == 49]
assert len(units) == 1 and units[0]['achievement'] and units[0]['daily_candidate']
rooms, mobs, objects = (bodies(k) for k in ('wld', 'mob', 'obj'))
assert (len(rooms), len(mobs), len(objects)) == (69, 26, 20)
assert {c['mob_vnum'] for c in m['contacts']} == set(mobs)
assert next(c for c in m['contacts'] if c['mob_vnum'] == 4921)['topics'] == topics
assert sum(len(c['topics']) for c in m['contacts']) == 3
assert set(mobs) == {r['arguments'][1] for r in e['reset_commands'] if r['command'] == 'M'}
assert not {4812, 4830}.intersection(mobs)  # Foreign special rows in the broad registry range.
assert e['zone']['first_vnum'] == 4810 and e['zone']['reset_mode'] == 2
assert (ROOT / 'areas/zon/mining.zon').read_text().splitlines()[2] == '4968 2 0 20 30 3'
assert collections.Counter(r['command'] for r in e['reset_commands']) == {'D': 22, 'O': 13, 'P': 3, 'M': 53, 'E': 13, 'G': 2}
resets = {r['line']: (r['command'], r['arguments'][:5]) for r in e['reset_commands']}
assert resets[123] == ('M', [0, 4921, 1, 4907, 100])
assert resets[169] == ('M', [0, 4922, 1, 4947, 100]) and resets[170] == ('E', [1, 4911, 1, 16, 100])
assert properties(objects[4911])[0][6:9] == [8193, 8193, 32832]  # GLOW/FLOAT; TAKE/WIELD; MAGIC/QUESTITEM.
assert properties(objects[4912])[0][6:9] == [1, 2097153, 2]  # GLOW; TAKE/GUILD_INSIGNIA; BLESS.
assert resets[166] == ('M', [0, 4914, 1, 4942, 100]) and resets[168] == ('G', [1, 4904, 1, 0, 100])
assert properties(objects[4904])[0][6:9] == [4096, 16385, 0]  # SECRET/TAKE/HOLD.
assert properties(objects[4902])[1][:4] == [100, 15, 4904, 100]
assert resets[105] == ('O', [0, 4902, 1, 4942, 100]) and resets[106] == ('P', [1, 4903, 1, 4902, 100])
assert properties(objects[4903])[0][6:9] == [1, 16385, 0]
assert properties(objects[4903])[1][1] == properties(objects[4904])[1][1] == 0
assert properties(objects[4907])[0][6:9] == [67117056, 0, 0] and "Don't go down!" in objects[4907]
assert properties(objects[4908])[1][:4] == properties(objects[4909])[1][:4] == [1000, 5, 0, 1000]
assert not any(re.search(r'^T\s', body, re.M) for body in objects.values())
assert resets[109] == ('O', [0, 4910, 1, 4962, 100]) and resets[110] == ('P', [1, 4915, 1, 4910, 100])
assert resets[112] == ('O', [0, 4906, 10, 4967, 100]) and resets[177] == ('G', [1, 4906, 10, 0, 100])
assert resets[111] == ('O', [0, 4917, 1, 4964, 100])
assert properties(objects[4913])[0][6:9] == [4096, 2049, 0] and resets[136] == ('E', [1, 4913, 1, 13, 100])
assert resets[151] == ('E', [1, 4914, 1, 16, 100])
assert properties(objects[4919])[0][6:9] == [0, 0, 1024] and properties(objects[4919])[1][:4] == [1000, 0, 0, 1000]
graph = [(v, int(x[1]), int(x[4]), int(x[5]), int(x[6])) for v, body in rooms.items()
         for x in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)', body, re.S)]
assert len(graph) == 153 and sum(key in (4903, 4906) for _, _, _, key, _ in graph) == 3
assert (4914, 5, 2, 4903, 4943) in graph and (4943, 4, 2, 0, 4914) in graph
assert (4908, 0, 3, 4906, 4962) in graph and (4962, 2, 2, 4906, 4908) in graph
assert (4949, 0, 4, 0, 4953) in graph and (4959, 0, 4, 0, 4963) in graph
assert {target for _, _, _, _, target in graph if target not in rooms} == {566915}
for v in range(4943, 4947):
    assert list(map(int, rooms[v].split('~', 2)[2].strip().splitlines()[0].split())) == [49, 134250496, 18, 0]
for v in range(4948, 4953):
    assert list(map(int, rooms[v].split('~', 2)[2].strip().splitlines()[0].split())) == [49, 32768, 11, 0]
assert list(map(int, rooms[4947].split('~', 2)[2].strip().splitlines()[0].split())) == [49, 32768, 15, 0]
assert not any(re.search(r'^[EF]\s*$', body, re.M) for body in rooms.values())
reached = {4900}; pending = [4900]
while pending:
    room = pending.pop()
    for source, _, _, _, target in graph:
        if source == room and target in rooms and target not in reached:
            reached.add(target); pending.append(target)
assert reached == set(range(4900, 4964))
assert (4964, 2, 0, 0, 4919) in graph and (4967, 1, 0, 0, 4924) in graph and (4968, 3, 0, 0, 4968) in graph
assert not (ROOT / 'areas/shp/mining.shp').exists()
orientation = ' '.join(m['orientation'])
for phrase in ('three aliases share one response', 'exact supplied whip', 'current loose custody',
               'different key', 'mirrors to the reciprocal side', 'actual fall', 'fire sector',
               'no declared route', 'golden boomerang', 'lasting results', 'active READY accounting', 'Daily replay remains disabled'):
    assert phrase in orientation, phrase
tables = []
for name in ('docs/design/zone-stories/HALFLING_SILVER_MINE.md', 'docs/guides/ZONE_STORY_BUILDING.md', 'docs/design/ZONE_STORY_ROADMAP_EXECUTION.md'):
    rows = [x for x in (ROOT / name).read_text().splitlines() if x.startswith('| ZSQ-MINING-')]
    assert len(rows) == 44; tables.append(rows)
assert tables[0] == tables[1] == tables[2]
dossier = (ROOT / 'docs/design/zone-stories/HALFLING_SILVER_MINE.md').read_text()
for title in ("Priest's native request", 'Engineering access and accountability', 'Hazardous descent and return',
              'Planar disturbance', 'Intrusion and vault security', 'Treasury/property recovery',
              'Mine production and structural safety', 'Ventilation and shaft service',
              'Worker welfare and leadership', 'Sanitation and staff containment'):
    assert '| ' + title + ' |' in dossier
print('Halfling Silver Mine exact whip, supplied custody, key chain, hazards, vault and independent beneficiary boundaries passed')
