#!/usr/bin/env python3
"""Protect Orrak's supplied staff return and deeper source/outcome boundaries."""
import collections
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
import zone_story_quest_catalog as cat
import zone_story_quest_zone_inventory as inv


def bodies(kind):
    text = (ROOT / 'areas' / kind / ('orrak.' + kind)).read_text()
    return {int(m[1]): m[2] for m in re.finditer(r'^#(\d+)\s*\n([\s\S]*?)(?=^#\d+\s*$|\Z)', text, re.M)}


e = inv.area_evidence(ROOT, 'orrak')
catalog = cat.production_catalog(ROOT)
m = next(m for m in catalog['story_mappings'] if m['source_area'] == 'orrak')
assert (m['schema_version'], m['revision'], m['coverage']) == (3, 1, 'complete')
assert len(m['stories']) == 1 and not m['exclusions'] and len(m['orientation']) == 11
raw = [b for b in inv.native_blocks(ROOT) if b['source'] == 'areas/qst/orrak.qst']
assert [(b['kind'], b['line']) for b in raw] == [('M', 2), ('Q', 8)]
assert raw[0]['giver_vnum'] == 9001 and raw[0]['body'][0] == 'orrak~'
q = raw[1]
assert q['giver_vnum'] == 9016 and q['give'] == [('I', 9011)]
assert q['receive'] == [('I', 9012)] and q['disappear']
s = m['stories'][0]
assert s['id'] == 'return-the-prisoners-staff' and s['category'] == 'request'
assert s['contracts'] == [q['binding']] and len(s['steps']) == 2
staff, accepted = s['steps']
assert (staff['kind'], staff['item_vnums'], staff['count'], staff['optional']) == ('carried_item', [9011], 1, True)
assert accepted['kind'] == 'completion' and accepted['contracts'] == s['contracts']
definition = e['requests'][0]['definition']
assert not definition['prerequisites'] and definition['daily_eligible'] and definition['repeatable']
units = [u for u in cat.story_units(catalog) if u['zone_number'] == 90]
assert len(units) == 1 and units[0]['achievement'] and units[0]['daily_candidate']
rooms, mobs, objects = (bodies(k) for k in ('wld', 'mob', 'obj'))
assert (len(rooms), len(mobs), len(objects)) == (39, 18, 18)
assert set(rooms) == set(range(9000, 9039))
assert {c['mob_vnum'] for c in m['contacts']} == set(mobs) == set(range(9000, 9018))
assert next(c for c in m['contacts'] if c['mob_vnum'] == 9001)['topics'] == ['orrak']
assert sum(len(c['topics']) for c in m['contacts']) == 1
assert all(re.fullmatch(r'[a-z0-9_-]{1,64}', c['keyword']) and c['keyword'] in e['mobs'][c['mob_vnum']]['keywords'] for c in m['contacts'])
assert (e['zone']['first_vnum'], e['zone']['last_vnum'], e['zone']['reset_mode']) == (8921, 9038, 1)
assert (ROOT / 'areas/zon/orrak.zon').read_text().splitlines()[2] == '9038 1 0 50 60 1'
assert collections.Counter(r['command'] for r in e['reset_commands']) == {'M': 18, 'F': 7, 'O': 3, 'E': 11, 'G': 4, 'D': 12}
assert not any(r['command'] == 'R' for r in e['reset_commands'])
current = None
source = []
for r in e['reset_commands']:
    if r['command'] in ('M', 'F'):
        current = r['arguments'][1], r['arguments'][3]
    if r['command'] == 'E' and r['arguments'][1] == 9011:
        source.append((current, r['arguments'][2:4]))
assert source == [((9012, 9028), [1, 16])]
assert any(r['command'] == 'M' and r['arguments'][1:4] == [9016, 1, 9038] for r in e['reset_commands'])
numeric = lambda body: [list(map(int, x.split())) for x in body.split('~', 4)[4].strip().splitlines()[:3]]
assert numeric(objects[9011])[0][0] == 5 and numeric(objects[9011])[1][5:8] == [0, 0, 0]
assert numeric(objects[9012])[0][0] == 9
assert all(numeric(objects[v])[0][0] == 18 and numeric(objects[v])[1][1] == 0 for v in (9002, 9010))
assert numeric(objects[9009])[0][0] == 17 and numeric(objects[9009])[1][:4] == [-1, -1, 9, 1]
assert not any(numeric(b)[0][0] == 25 for b in objects.values())
graph = [(v, int(x[1]), int(x[4]), int(x[5]), int(x[6])) for v, body in rooms.items()
         for x in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)', body, re.S)]
assert len(graph) == 77 and [(v, d, target) for v, d, _, _, target in graph if target not in rooms] == [(9000, 2, 610629)]
assert {(9018, 0, 3, 9002, 9019), (9019, 2, 2, 0, 9018),
        (9036, 0, 3, 9010, 9038), (9038, 2, 2, 9010, 9036),
        (9035, 5, 0, 0, 9037), (9037, 4, 0, 0, 9035)} <= set(graph)
assert len({tuple(map(int, b.split('~', 2)[2].strip().splitlines()[0].split())) for b in rooms.values()}) == 5
assert len(re.findall(r'^F\s*\n100\s*$', rooms[9035], re.M)) == 1
assert sum(len(re.findall(r'^F\s*$', b, re.M)) for b in rooms.values()) == 1
assert not any(re.search(r'^[EC]$', b, re.M) for b in rooms.values())
assert not e['special_assignments'] and not (ROOT / 'areas/shp/orrak.shp').exists()
tables = []
for name in ('docs/design/zone-stories/ORRAK.md', 'docs/guides/ZONE_STORY_BUILDING.md', 'docs/design/ZONE_STORY_ROADMAP_EXECUTION.md'):
    rows = [line for line in (ROOT / name).read_text(encoding='utf8').splitlines() if re.match(r'^\| ZSQ-ORRAK-\d+\s*\|', line)]
    assert len(rows) == 44 and [re.match(r'\| (ZSQ-ORRAK-\d+)', row)[1] for row in rows] == ['ZSQ-ORRAK-' + str(n).zfill(2) for n in range(1, 45)]
    tables.append(rows)
assert tables[0] == tables[1] == tables[2]
dossier = (ROOT / 'docs/design/zone-stories/ORRAK.md').read_text(encoding='utf8')
progression = dossier.split('## Progression stories', 1)[1].split('## Implementation and validation boundary', 1)[0]
assert len([line for line in progression.splitlines() if line.startswith('| ') and not line.startswith(('| Story', '| ---'))]) == 9
for phrase in ('active READY accounting', 'Daily policy remains disabled', 'Exact supplied staff remains eligible',
               'eight broader', '18 O/G/E', 'safe return or restored home', 'first source recovery',
               'last_mob_followable', 'SLIME9 and poison1', 'zero-charge', 'terminal hazard',
               'separate named fix/news commits', 'no named living destination', 'surf is absent from AREA'):
    assert phrase.lower() in dossier.lower(), phrase
print('Orrak: supplied exact staff/bracelet/departure, access/source, warning/fall/pool and broader useful outcomes retained.')
