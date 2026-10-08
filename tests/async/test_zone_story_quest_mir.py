#!/usr/bin/env python3
"""Protect Mir's native exchange, custom-source boundaries and comprehensive plan."""
import collections
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
import zone_story_quest_catalog as cat
import zone_story_quest_zone_inventory as inv


def bodies(kind, area='mir'):
    text = (ROOT / 'areas' / kind / (area + '.' + kind)).read_text()
    return {int(m[1]): m[2] for m in re.finditer(r'^#(\d+)\s*\n([\s\S]*?)(?=^#\d+\s*$|\Z)', text, re.M)}


def properties(body):
    return [list(map(int, x.split())) for x in body.split('~', 4)[4].strip().splitlines()[:3]]


e = inv.area_evidence(ROOT, 'mir')
catalog = cat.production_catalog(ROOT)
m = next(m for m in catalog['story_mappings'] if m['source_area'] == 'mir')
assert (m['schema_version'], m['revision'], m['coverage']) == (3, 1, 'complete')
assert len(m['stories']) == 1 and not m['exclusions'] and len(m['orientation']) == 16
raw = [b for b in inv.native_blocks(ROOT) if b['source'] == 'areas/qst/mir.qst']
assert [(b['kind'], b['line']) for b in raw] == [('M', 2), ('Q', 10)]
topics = raw[0]['body'][0].split('~')[0].split()
assert topics == ['hello', 'greetings']
q = raw[1]
assert q['give'] == [('I', 41929)] and q['receive'] == [('E', 100000), ('I', 41930)] and not q['disappear']
s = m['stories'][0]
assert s['id'] == 'light-dark-scroll-for-priest' and s['category'] == 'request' and s['contracts'] == [q['binding']]
material, accepted = s['steps']
assert (material['kind'], material['item_vnums'], material['count'], material['optional']) == ('carried_item', [41929], 1, True)
assert accepted['kind'] == 'completion' and accepted['contracts'] == s['contracts']
definition = e['requests'][0]['definition']
assert not definition['prerequisites'] and definition['daily_eligible'] and definition['eligible_for_zone_completion']
units = [u for u in cat.story_units(catalog) if u['zone_number'] == 419]
assert len(units) == 1 and units[0]['achievement'] and units[0]['daily_candidate']
rooms, mobs, objects = (bodies(k) for k in ('wld', 'mob', 'obj'))
assert (len(rooms), len(mobs), len(objects)) == (281, 26, 46)
assert {c['mob_vnum'] for c in m['contacts']} == set(mobs)
assert next(c for c in m['contacts'] if c['mob_vnum'] == 41916)['topics'] == topics
assert sum(len(c['topics']) for c in m['contacts']) == 2
assert set(mobs) == {r['arguments'][1] for r in e['reset_commands'] if r['command'] in ('M', 'F')}
assert (e['zone']['first_vnum'], e['zone']['last_vnum'], e['zone']['reset_mode']) == (41839, 42180, 1)
assert (ROOT / 'areas/zon/mir.zon').read_text().splitlines()[2] == '42180 1 0 20 25 3'
assert collections.Counter(r['command'] for r in e['reset_commands']) == {'D': 6, 'O': 24, 'P': 4, 'M': 54, 'G': 28, 'E': 8, 'F': 4}
resets = {r['line']: (r['command'], r['arguments'][:5]) for r in e['reset_commands']}
assert resets[74] == ('M', [0, 41916, 1, 41925, 100])
assert resets[153] == ('M', [0, 41914, 1, 42180, 100]) and resets[154] == ('G', [1, 41929, 1, 0, 100])
assert resets[155] == ('M', [0, 41915, 1, 42180, 100])
assert properties(objects[41929])[0][6:9] == [12288, 8388609, 32768]
assert properties(objects[41929])[1][:4] == [60, 123, 122, -1]
assert properties(objects[41930])[0][6:9] == [3284992, 8404997, 64]
assert properties(objects[42166])[0][6] == 8392704 and properties(objects[42166])[1][1] == 100
assert resets[125] == ('G', [1, 42166, 1, 0, 100])
assert properties(objects[42167])[1][:4] == [15, 13, 42166, 15]
assert resets[50] == ('P', [1, 42178, 1, 42167, 100]) and resets[51] == ('P', [1, 41916, 1, 42167, 100])
assert resets[53] == ('P', [1, 42175, 1, 42174, 75]) and resets[54] == ('P', [1, 42168, 1, 42174, 75])
assert resets[55] == ('O', [0, 42169, 1, 42174, 95]) and resets[59] == ('O', [0, 359, 1, 42174, 100])
assert properties(objects[42172])[1][:4] == [0, 0, 0, 256] and properties(objects[42173])[1][:4] == [0, 0, 1610, 0]
assert properties(objects[41925])[1][:4] == [42166, 7, -1, 0] and properties(objects[41926])[1][:4] == [42179, 7, -1, 0]
assert resets[48] == ('O', [0, 41926, 1, 42166, 100]) and resets[60] == ('O', [0, 41925, 1, 42179, 100])
for v, trap in ((41910, '508 1 1 100'), (41926, '2304 9 2 100'), (41927, '6 11 2 100'), (41928, '6 15 1 100'), (42175, '6 12 1 50')):
    assert re.search(r'^T\s*\n' + re.escape(trap) + r'\s*$', objects[v], re.M)
assert not 508 & 1 and 508 & 4  # ROOM/all directions without MOVE.
assert resets[97] == ('G', [1, 41922, 1, 0, 100]) and resets[116] == ('G', [1, 41920, 1, 0, 100])
assert [resets[n][1][1] for n in (141, 143, 145)] == [42170, 42177, 42171]
graph = [(v, int(x[1]), int(x[4]), int(x[5]), int(x[6])) for v, body in rooms.items()
         for x in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)', body, re.S)]
assert len(graph) == 558 and {target for _, _, _, _, target in graph if target not in rooms} == {41316, 41585}
for edge in ((41950, 5, 5, 0, 42180), (42180, 4, 5, 0, 41950), (42166, 5, 5, 0, 42167), (42167, 4, 1, 0, 42166), (42173, 5, 7, 42178, 42174), (42174, 4, 3, 42178, 42173), (42051, 2, 4, 0, 42150), (42168, 1, 4, 0, 42170)):
    assert edge in graph, edge
unreachable = {41943, 41944, 41947, 41948, 41949, 41963, 41964, 41967, 41977, 41978, 41979, 41982, 41983, 41993, 41994, 41995, 41996, 41997, 41998, 41999, 42013, 42014, 42015, 42016, 42017, 42018, 42019, 42033, 42034, 42035, 42036, 42037, 42038, 42052}
for start in (42137, 42152):
    reached = {start}; pending = [start]
    while pending:
        room = pending.pop()
        for source, _, _, _, target in graph:
            if source == room and target in rooms and target not in reached:
                reached.add(target); pending.append(target)
    assert len(reached) == 237 and set(rooms) - reached == unreachable | set(range(42166, 42176))
currents = [(v, tuple(map(int, re.search(r'^C\n([^\n]+)', body, re.M)[1].split()))) for v, body in rooms.items() if re.search(r'^C$', body, re.M)]
assert currents == list(zip(range(42152, 42165), ((75, 3), (75, 3), (70, 3), (65, 0), (60, 3), (55, 3), (50, 3), (45, 3), (40, 3), (35, 3), (30, 0), (25, 3), (20, 3))))
assert re.search(r'^F\n10$', rooms[42167], re.M) and re.search(r'^F\n50$', rooms[42168], re.M)
assert list(map(int, rooms[42174].split('~', 2)[2].strip().splitlines()[0].split())) == [419, 167809096, 11]
assert not any(re.search(r'^E$', body, re.M) for body in rooms.values())
assert not (ROOT / 'areas/shp/mir.shp').exists()
assert len(e['special_assignments']) == 15
foreign = (ROOT / 'areas/zon/highway.zon').read_text().splitlines()
assert foreign[163].startswith('O 0 41915 1 41540') and foreign[175].startswith('P 1 41917 1 41353')
assert properties(bodies('obj', 'highway')[41353])[1][:4] == [2000, 5, 0, 250]
tables = []
for name in ('docs/design/zone-stories/FOREST_OF_MIR.md', 'docs/guides/ZONE_STORY_BUILDING.md', 'docs/design/ZONE_STORY_ROADMAP_EXECUTION.md'):
    text = (ROOT / name).read_text(encoding='utf8')
    rows = [line for line in text.splitlines() if line.startswith('| ZSQ-MIR-')]
    assert len(rows) == 61 and [re.match(r'\| (ZSQ-MIR-\d+)', row)[1] for row in rows] == ['ZSQ-MIR-' + str(n).zfill(2) for n in range(1, 62)]
    tables.append(rows)
assert tables[0] == tables[1] == tables[2]
dossier = (ROOT / 'docs/design/zone-stories/FOREST_OF_MIR.md').read_text(encoding='utf8')
assert len(re.findall(r'^\| (?:The fallen|Webs,|Johan |Mythra,|Orc |Maze,|River |Mercury |Wyrms,|Relics |The rune)', dossier, re.M)) == 11
for phrase in ('active READY accounting', 'Daily policy remains disabled', 'exact supplied scroll', '34 forest rooms', 'without explicit extraction', 'separate named fix/news commits'):
    assert phrase in dossier, phrase
print('Forest of Mir: exact scroll/experience/token exchange, sources, routes, custom effects and comprehensive follow-ups retained.')
