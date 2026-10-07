#!/usr/bin/env python3
"""Protect Bandit Canyons exact joint exchange, access and useful-outcome bounds."""
import collections
from pathlib import Path
import re
import sys
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
import zone_story_quest_catalog as cat
import zone_story_quest_zone_inventory as inv
def bodies(kind, area='bandit'):
    text = (ROOT / 'areas' / kind / (area + '.' + kind)).read_text()
    return {int(m[1]): m[2] for m in re.finditer(r'^#(\d+)\s*\n([\s\S]*?)(?=^#\d+\s*$|\Z)', text, re.M)}
e = inv.area_evidence(ROOT, 'bandit')
catalog = cat.production_catalog(ROOT)
m = next(m for m in catalog['story_mappings'] if m['source_area'] == 'bandit')
assert (m['schema_version'], m['revision'], m['coverage']) == (3, 1, 'complete')
assert len(m['stories']) == 1 and not m['exclusions'] and len(m['orientation']) == 10
raw = [b for b in inv.native_blocks(ROOT) if b['source'] == 'areas/qst/bandit.qst' and b['kind'] == 'Q']
assert len(raw) == 1
q = raw[0]
assert (q['giver_vnum'], q['line'], q['give'], q['receive'], q['disappear']) == (98534, 13, [('I', 32490), ('I', 26614), ('I', 402)], [('I', 403)], False)
s = m['stories'][0]
assert (s['id'], s['category'], s['contracts']) == ('olat-strength-scroll', 'story', [q['binding']])
assert len(s['steps']) == 4
for material, v in zip(s['steps'][:-1], (32490, 26614, 402)):
    assert (material['kind'], material['item_vnums'], material['count'], material['optional']) == ('carried_item', [v], 1, True)
assert s['steps'][-1]['kind'] == 'completion' and s['steps'][-1]['contracts'] == [q['binding']]
assert not e['dialogue'] and not e['special_assignments'] and not e['requests'][0]['definition']['prerequisites']
units = [u for u in cat.story_units(catalog) if u['zone_number'] == 985]
assert len(units) == 1 and units[0]['achievement'] and units[0]['daily_candidate']
rooms, mobs, objects = (bodies(k) for k in ('wld', 'mob', 'obj'))
assert (len(rooms), len(mobs), len(objects)) == (76, 54, 34)
assert set(rooms) == set(range(98500, 98576)) and set(mobs) == set(range(98500, 98554))
assert {c['mob_vnum'] for c in m['contacts']} == set(mobs)
assert all(not c['topics'] and re.fullmatch(r'[a-z0-9_-]{1,64}', c['keyword']) and c['keyword'] in e['mobs'][c['mob_vnum']]['keywords'] for c in m['contacts'])
assert (e['zone']['first_vnum'], e['zone']['last_vnum'], e['zone']['reset_mode']) == (98019, 98575, 2)
assert collections.Counter(r['command'] for r in e['reset_commands']) == {'D': 32, 'O': 6, 'P': 6, 'M': 172, 'E': 64, 'F': 7, 'G': 9}
assert sum(r['command'] in ('O', 'P', 'G', 'E') for r in e['reset_commands']) == 85
assert [(r['arguments'][2], r['arguments'][3]) for r in e['reset_commands'] if r['command'] == 'M' and r['arguments'][1] == 98539] == [(4, 98530)] * 4
numeric = lambda b: [list(map(int, line.split())) for line in b.split('~', 4)[4].strip().splitlines()[:3]]
for v, command, room, direction in ((98516, 340, 98528, 1), (98517, 270, 98531, 3)):
    header, values, _ = numeric(objects[v])
    assert header[0] == 29 and not header[7] & 1 and values[:4] == [command, room, direction, 1]
assert numeric(objects[98524])[1][3] == 0 and numeric(objects[98525])[1][3] == 1
assert 'b o sh lf' in objects[98514] and 'In ba kro m p ll' in objects[98515]
assert 'Lanngh' in objects[98507] and 'High-Magi of Corontonn' in objects[98507]
tablet, scroll = bodies('obj', 'heavens')[402], bodies('obj', 'heavens')[403]
assert 'artifact' in tablet.lower() and numeric(scroll)[0][0] == 13 and numeric(scroll)[1] == [0] * 8
graph = [(v, int(x[1]), int(x[4]), int(x[5]), int(x[6])) for v, body in rooms.items()
         for x in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)', body, re.S)]
assert len(graph) == 163
assert {(98524, 1, 2, -2, 98525), (98525, 3, 2, 0, 98524), (98528, 1, 0, 0, 98531), (98531, 3, 0, 0, 98528)} <= set(graph)
assert all(target != 98575 for _, _, _, _, target in graph)
assert any(r['command'] == 'D' and r['arguments'][1:4] == [98528, 1, 8] for r in e['reset_commands'])
assert any(r['command'] == 'D' and r['arguments'][1:4] == [98531, 3, 8] for r in e['reset_commands'])
assert '\nF\n10\nC\n20 5\n' in rooms[98539] and '\nF\n20\n' in rooms[98542]
assert '\n985 134250496 8\n' in rooms[98549] and '\nF\n10\nC\n50 5\n' in rooms[98549]
assert 'traitors within the caravan guards' in rooms[98514]
assert not (ROOT / 'areas/shp/bandit.shp').exists()
all_native = inv.native_blocks(ROOT)
trios = [b for b in all_native if b['kind'] in ('Q', 'QA') and sorted(b['give']) == [('I', 402), ('I', 26614), ('I', 32490)]]
assert {b['giver_vnum'] for b in trios} == {98534, 4203, 15120, 21535, 53658, 78006, 80907, 95304, 99548}
foreign = [b for b in all_native if b['kind'] in ('Q', 'QA') and (b['giver_vnum'], b['line']) in {(32448, 23), (26644, 98), (83302, 5328)}]
assert len(foreign) == 3
tables = []
for name in ('docs/design/zone-stories/BANDIT_CANYONS.md', 'docs/guides/ZONE_STORY_BUILDING.md', 'docs/design/ZONE_STORY_ROADMAP_EXECUTION.md'):
    rows = [line for line in (ROOT / name).read_text(encoding='utf8').splitlines() if re.match(r'^\| ZSQ-BANDIT-\d+\s*\|', line)]
    assert len(rows) == 46 and [re.match(r'\| (ZSQ-BANDIT-\d+)', row)[1] for row in rows] == ['ZSQ-BANDIT-' + str(n).zfill(2) for n in range(1, 47)]
    tables.append(rows)
assert tables[0] == tables[1] == tables[2]
dossier = (ROOT / 'docs/design/zone-stories/BANDIT_CANYONS.md').read_text(encoding='utf8')
progression = dossier.split('## Progression stories', 1)[1].split('## Implementation and validation boundary', 1)[0]
assert len([line for line in progression.splitlines() if line.startswith('| ') and not line.startswith(('| Story', '| ---'))]) == 10
for phrase in ('active READY accounting', 'Daily policy remains disabled', 'Exact supplied materials remain eligible', 'ten broader', 'All85 local O/P/G/E', 'Guidance alone does not add synthetic achievements', 'separate named fix/news commits', 'TRASH13', 'SWITCH29', 'NO_GROUND8', 'not CLOSED', 'handled no-op'):
    assert phrase.lower() in dossier.lower(), phrase
print('Bandit Canyons: exact joint supplied trio, distinct scroll/control/beneficiary outcomes and comprehensive fair follow-ups retained.')
