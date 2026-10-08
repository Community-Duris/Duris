#!/usr/bin/env python3
"""Protect Shadow Forest acceptance, source/ward boundaries and full progression."""
import collections
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
import zone_story_quest_catalog as cat
import zone_story_quest_zone_inventory as inv


def bodies(kind):
    text = (ROOT / 'areas' / kind / ('mist.' + kind)).read_text()
    return {int(m[1]): m[2] for m in re.finditer(r'^#(\d+)\s*\n([\s\S]*?)(?=^#\d+\s*$|\Z)', text, re.M)}


def properties(body):
    return [list(map(int, x.split())) for x in body.split('~', 4)[4].strip().splitlines()[:3]]


e = inv.area_evidence(ROOT, 'mist')
catalog = cat.production_catalog(ROOT)
m = next(m for m in catalog['story_mappings'] if m['source_area'] == 'mist')
assert (m['schema_version'], m['revision'], m['coverage']) == (3, 1, 'complete')
assert len(m['stories']) == 1 and not m['exclusions'] and len(m['orientation']) == 12
raw = [b for b in inv.native_blocks(ROOT) if b['source'] == 'areas/qst/mist.qst']
assert [(b['kind'], b['line']) for b in raw] == [('M', 2), ('Q', 6)]
topics = raw[0]['body'][0].split('~')[0].split()
assert topics == ['darnac']
q = raw[1]
assert q['give'] == [('I', 6305)] and q['receive'] == [('I', 6306)] and q['disappear']
s = m['stories'][0]
assert s['id'] == 'darnac-head-for-palon' and s['category'] == 'request' and s['contracts'] == [q['binding']]
material, accepted = s['steps']
assert (material['kind'], material['item_vnums'], material['count'], material['optional']) == ('carried_item', [6305], 1, True)
assert accepted['kind'] == 'completion' and accepted['contracts'] == s['contracts']
definition = e['requests'][0]['definition']
assert not definition['prerequisites'] and definition['daily_eligible'] and definition['eligible_for_zone_completion']
units = [u for u in cat.story_units(catalog) if u['zone_number'] == 63]
assert len(units) == 1 and units[0]['achievement'] and units[0]['daily_candidate']
rooms, mobs, objects = (bodies(k) for k in ('wld', 'mob', 'obj'))
assert (len(rooms), len(mobs), len(objects)) == (73, 9, 7)
assert {c['mob_vnum'] for c in m['contacts']} == set(mobs) == set(range(6300, 6309))
assert next(c for c in m['contacts'] if c['mob_vnum'] == 6306)['topics'] == topics
assert sum(len(c['topics']) for c in m['contacts']) == 1
assert set(mobs) == {r['arguments'][1] for r in e['reset_commands'] if r['command'] == 'M'}
assert (e['zone']['first_vnum'], e['zone']['last_vnum'], e['zone']['reset_mode']) == (6249, 6372, 2)
assert (ROOT / 'areas/zon/mist.zon').read_text().splitlines()[2] == '6372 2 0 25 35 1'
assert collections.Counter(r['command'] for r in e['reset_commands']) == {'M': 55, 'E': 5, 'G': 1}
resets = {r['line']: (r['command'], r['arguments'][:5]) for r in e['reset_commands']}
assert resets[10] == ('M', [0, 6307, 1, 6300, 100])
assert resets[66] == ('M', [0, 6305, 1, 6370, 100]) and resets[68] == ('G', [1, 6305, 1, 0, 100])
assert resets[69] == ('M', [0, 6306, 1, 6372, 100])
assert [resets[n][1][1] for n in (12, 13, 65, 67, 70)] == [6301, 6300, 6302, 6303, 6304]
assert properties(objects[6305])[0][0] == 13 and properties(objects[6305])[0][6:10] == [0, 16385, 32768, 0]
assert properties(objects[6306])[0][0] == 9 and properties(objects[6306])[0][7:10] == [17, 0, 57374]
assert properties(objects[6303])[0][9] == 130590 and properties(objects[6301])[0][6] == 169926656
assert not any(re.search(r'^[TCEF]$', b, re.M) for b in objects.values())
graph = [(v, int(x[1]), int(x[4]), int(x[5]), int(x[6])) for v, body in rooms.items()
         for x in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)', body, re.S)]
assert len(graph) == 199 and all(flag == key == 0 for _, _, flag, key, _ in graph)
assert {target for _, _, _, _, target in graph if target not in rooms} == {628063, 628462}
assert all(list(map(int, b.split('~', 2)[2].strip().splitlines()[0].split()))[1] in (8421376, 8421380, 8421440) for b in rooms.values())
assert list(map(int, rooms[6300].split('~', 2)[2].strip().splitlines()[0].split())) == [63, 8421380, 3]
assert list(map(int, rooms[6367].split('~', 2)[2].strip().splitlines()[0].split())) == [63, 8421440, 0]
assert not any(re.search(r'^[EFC]$', b, re.M) for b in rooms.values())
assert not [x for x in graph if x[0] == 6371]
for source in (6370, 6372):
    assert {direction for room, direction, _, _, target in graph if room == source and target == 6371} == {0, 1, 2, 3}
for start, count, missing in ((6300, 67, {6367, 6368, 6369, 6370, 6371, 6372}), (6366, 67, {6367, 6368, 6369, 6370, 6371, 6372}), (6368, 69, {6367, 6370, 6371, 6372})):
    reached = {start}; pending = [start]
    while pending:
        room = pending.pop()
        for source, _, _, _, target in graph:
            if source == room and target in rooms and target not in reached:
                reached.add(target); pending.append(target)
    assert len(reached) == count and set(rooms) - reached == missing
assert len(e['special_assignments']) == 1 and not (ROOT / 'areas/shp/mist.shp').exists()
guard = (ROOT / 'src/specs/specs.mist.c').read_text()
for condition in ('cmd == CMD_SET_PERIODIC', 'IS_TRUSTED(ch)', 'ch->in_room == real_room(6300)', 'cmd == CMD_NORTH', 'GET_LEVEL(pl) > 36 && !IS_TRUSTED(pl)'):
    assert condition in guard
tables = []
for name in ('docs/design/zone-stories/SHADOW_FOREST.md', 'docs/guides/ZONE_STORY_BUILDING.md', 'docs/design/ZONE_STORY_ROADMAP_EXECUTION.md'):
    text = (ROOT / name).read_text(encoding='utf8')
    rows = [line for line in text.splitlines() if re.match(r'^\| ZSQ-MIST-\d+\s*\|', line)]
    assert len(rows) == 34 and [re.match(r'\| (ZSQ-MIST-\d+)', row)[1] for row in rows] == ['ZSQ-MIST-' + str(n).zfill(2) for n in range(1, 35)]
    tables.append(rows)
assert tables[0] == tables[1] == tables[2]
dossier = (ROOT / 'docs/design/zone-stories/SHADOW_FOREST.md').read_text(encoding='utf8')
assert len(re.findall(r'^\| (?:Palon|Guardian,|Grolen,|Treant |Travelers,|Forest equipment)', dossier, re.M)) == 7
for phrase in ('active READY accounting', 'Daily policy remains disabled', 'exact supplied head', 'six broader', 'no local R mount stock', 'separate named fix/news commits'):
    assert phrase in dossier, phrase
print('The Shadow Forest: exact head/crown/departure, source versus supplied custody, guardian/roaming boundaries and comprehensive follow-ups retained.')
