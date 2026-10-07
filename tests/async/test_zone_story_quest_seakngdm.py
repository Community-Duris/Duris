#!/usr/bin/env python3
"""Protect Sea Kingdom's supplied return and custom story integration boundaries."""
import collections
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
import zone_story_quest_catalog as cat
import zone_story_quest_zone_inventory as inv


def bodies(kind):
    text = (ROOT / 'areas' / kind / ('seakngdm.' + kind)).read_text()
    return {int(m[1]): m[2] for m in re.finditer(r'^#(\d+)\s*\n([\s\S]*?)(?=^#\d+\s*$|\Z)', text, re.M)}


e = inv.area_evidence(ROOT, 'seakngdm')
catalog = cat.production_catalog(ROOT)
m = next(m for m in catalog['story_mappings'] if m['source_area'] == 'seakngdm')
assert (m['schema_version'], m['revision'], m['coverage']) == (3, 1, 'complete')
assert len(m['stories']) == 1 and not m['exclusions'] and len(m['orientation']) == 14
raw = [b for b in inv.native_blocks(ROOT) if b['source'] == 'areas/qst/seakngdm.qst']
assert [(b['kind'], b['line']) for b in raw] == [('M', 2), ('Q', 9)]
assert raw[0]['giver_vnum'] == 31531 and raw[0]['body'][0] == 'hello hi poseidon help fish fisherman~'
q = raw[1]
assert q['giver_vnum'] == 31531 and q['give'] == [('I', 31529)]
assert q['receive'] == [('I', 31531)] and q['disappear']
s = m['stories'][0]
assert s['id'] == 'return-the-fishermans-amulet' and s['category'] == 'request'
assert s['contracts'] == [q['binding']] and len(s['steps']) == 2
half, accepted = s['steps']
assert (half['kind'], half['item_vnums'], half['count'], half['optional']) == ('carried_item', [31529], 1, True)
assert accepted['kind'] == 'completion' and accepted['contracts'] == s['contracts']
definition = e['requests'][0]['definition']
assert not definition['prerequisites'] and definition['daily_eligible'] and definition['repeatable']
units = [u for u in cat.story_units(catalog) if u['zone_number'] == 315]
assert len(units) == 1 and units[0]['achievement'] and units[0]['daily_candidate']
rooms, mobs, objects = (bodies(k) for k in ('wld', 'mob', 'obj'))
assert (len(rooms), len(mobs), len(objects)) == (225, 50, 58)
assert set(rooms) == set(range(31500, 31725))
assert {c['mob_vnum'] for c in m['contacts']} == set(mobs) == set(range(31500, 31550))
assert next(c for c in m['contacts'] if c['mob_vnum'] == 31531)['topics'] == ['hello', 'hi', 'poseidon', 'help', 'fish', 'fisherman']
assert sum(len(c['topics']) for c in m['contacts']) == 6
assert all(re.fullmatch(r'[a-z0-9_-]{1,64}', c['keyword']) and c['keyword'] in e['mobs'][c['mob_vnum']]['keywords'] for c in m['contacts'])
assert (e['zone']['first_vnum'], e['zone']['last_vnum'], e['zone']['reset_mode']) == (31425, 31724, 1)
assert (ROOT / 'areas/zon/seakngdm.zon').read_text().splitlines()[2] == '31724 1 0 40 50 1'
assert collections.Counter(r['command'] for r in e['reset_commands']) == {'D': 20, 'O': 27, 'M': 137, 'G': 10, 'F': 21, 'E': 38}
current = None
source = []
for r in e['reset_commands']:
    if r['command'] in ('M', 'F'):
        current = r['arguments'][1], r['arguments'][3]
    if r['command'] == 'G' and r['arguments'][1] == 31529:
        source.append((current, r['arguments'][2]))
assert source == [((31503, 31608), 1)]
assert any(r['command'] == 'M' and r['arguments'][1:5] == [31531, 1, 31671, 100] for r in e['reset_commands'])
numeric = lambda body: [list(map(int, x.split())) for x in body.split('~', 4)[4].strip().splitlines()[:3]]
assert numeric(objects[31529])[0][0] == 8 and numeric(objects[31531])[0][0] == 9
assert numeric(objects[31556])[1][5:8] == [1374076111, 60, 13]
assert [numeric(objects[v])[1][1] for v in (31535, 31508, 31553, 31557)] == [100, 0, 0, 100]
portals = {v: numeric(b)[1][:3] for v, b in objects.items() if numeric(b)[0][0] == 25}
assert portals == {31500: [31536, 11, -1], 31501: [31541, 6, -1], 31503: [31545, 7, -1], 31504: [31547, 7, -1],
                   31505: [31673, 264, -1], 31509: [31518, 122, -1], 31536: [31700, 7, -1], 31547: [31677, 7, -1]}
graph = [(v, int(x[1]), int(x[4]), int(x[5]), int(x[6])) for v, body in rooms.items()
         for x in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)', body, re.S)]
assert len(graph) == 558 and [(v, d, target) for v, d, _, _, target in graph if target not in rooms] == [(31500, 4, 23281)]
assert {(31500, 9, 3, 31535, 31507), (31507, 6, 3, 0, 31500),
        (31527, 0, 3, 31508, 31645), (31645, 2, 3, 31508, 31527),
        (31654, 2, 8, 31553, 31668), (31668, 5, 7, 31557, 31724),
        (31724, 4, 7, 31557, 31668)} <= set(graph)
assert not re.search(r'^D\d+$', rooms[31672], re.M)
assert len({tuple(map(int, b.split('~', 2)[2].strip().splitlines()[0].split())) for b in rooms.values()}) == 33
assert [(v, int(f)) for v, b in rooms.items() for f in re.findall(r'^F\s*\n(\d+)\s*$', b, re.M)] == [(31714, 40), (31715, 45), (31716, 40), (31717, 45)]
assert sum(len(re.findall(r'^E$', b, re.M)) for b in rooms.values()) == 5
assert not any(re.search(r'^C$', b, re.M) for b in rooms.values())
assert {(a['kind'], a['vnum'], a['function']) for a in e['special_assignments']} == {('obj', 31549, 'glowing_necklace'), ('obj', 31514, 'SeaKingdom_Tsunami')}
assert not (ROOT / 'areas/shp/seakngdm.shp').exists()
tables = []
for name in ('docs/design/zone-stories/SEAKNGDM.md', 'docs/guides/ZONE_STORY_BUILDING.md', 'docs/design/ZONE_STORY_ROADMAP_EXECUTION.md'):
    rows = [line for line in (ROOT / name).read_text(encoding='utf8').splitlines() if re.match(r'^\| ZSQ-SEAKNGDM-\d+\s*\|', line)]
    assert len(rows) == 62 and [re.match(r'\| (ZSQ-SEAKNGDM-\d+)', row)[1] for row in rows] == ['ZSQ-SEAKNGDM-' + str(n).zfill(2) for n in range(1, 63)]
    tables.append(rows)
assert tables[0] == tables[1] == tables[2]
dossier = (ROOT / 'docs/design/zone-stories/SEAKNGDM.md').read_text(encoding='utf8')
progression = dossier.split('## Progression stories', 1)[1].split('## Implementation and validation boundary', 1)[0]
assert len([line for line in progression.splitlines() if line.startswith('| ') and not line.startswith(('| Story', '| ---'))]) == 12
for phrase in ('active READY accounting', 'Daily policy remains disabled', 'Exact supplied half remains eligible',
               'eleven broader', '75 O/G/E', 'first source custody', 'no named saved lasting destination',
               'SUB_MONEY returns -1', '40000000 copper', 'exactly two root-carried fragments',
               'guidance alone does not add synthetic achievements', 'separate named fix/news commits', 'Winterhaven55127', 'nexus stone is394'):
    assert phrase.lower() in dossier.lower(), phrase
print('Sea Kingdom: supplied exact half/eye/departure, six shared aliases, real routes and broader custom outcomes retained.')
