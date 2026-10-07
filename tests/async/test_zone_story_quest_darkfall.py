#!/usr/bin/env python3
"""Protect Darkfall's exact exchange, current routes and useful-outcome boundaries."""
import collections
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
import zone_story_quest_catalog as cat
import zone_story_quest_zone_inventory as inv


def bodies(kind, area='darkfall'):
    text = (ROOT / 'areas' / kind / (area + '.' + kind)).read_text()
    return {int(m[1]): m[2] for m in re.finditer(r'^#(\d+)\s*\n([\s\S]*?)(?=^#\d+\s*$|\Z)', text, re.M)}


def numeric(body):
    return [list(map(int, line.split())) for line in body.split('~', 4)[4].strip().splitlines()[:3]]


e = inv.area_evidence(ROOT, 'darkfall')
catalog = cat.production_catalog(ROOT)
m = next(m for m in catalog['story_mappings'] if m['source_area'] == 'darkfall')
assert (m['schema_version'], m['revision'], m['coverage']) == (3, 1, 'complete')
assert len(m['stories']) == 1 and not m['exclusions'] and len(m['orientation']) == 10
all_native = inv.native_blocks(ROOT)
raw = [b for b in all_native if b['source'] == 'areas/qst/darkfall.qst' and b['kind'] == 'Q']
assert len(raw) == 1
q = raw[0]
assert (q['giver_vnum'], q['line'], q['give'], q['receive'], q['disappear']) == (95304, 37, [('I', 32490), ('I', 26614), ('I', 402)], [('I', 406)], False)
s = m['stories'][0]
assert (s['id'], s['category'], s['contracts']) == ('grellinar-agility-scroll', 'story', [q['binding']])
assert len(s['steps']) == 4
for material, v in zip(s['steps'][:-1], (32490, 26614, 402)):
    assert (material['kind'], material['item_vnums'], material['count'], material['optional']) == ('carried_item', [v], 1, True)
assert s['steps'][-1]['kind'] == 'completion' and s['steps'][-1]['contracts'] == [q['binding']]
assert not e['dialogue'] and not e['special_assignments'] and not e['requests'][0]['definition']['prerequisites']
units = [u for u in cat.story_units(catalog) if u['zone_number'] == 953]
assert len(units) == 1 and units[0]['achievement'] and units[0]['daily_candidate']
rooms, mobs, objects = (bodies(k) for k in ('wld', 'mob', 'obj'))
assert (len(rooms), len(mobs), len(objects)) == (49, 5, 4)
assert set(rooms) == set(range(95300, 95349)) and set(mobs) == set(range(95300, 95305))
assert {c['mob_vnum'] for c in m['contacts']} == set(mobs)
assert all(not c['topics'] and re.fullmatch(r'[a-z0-9_-]{1,64}', c['keyword']) and c['keyword'] in e['mobs'][c['mob_vnum']]['keywords'] for c in m['contacts'])
assert (e['zone']['first_vnum'], e['zone']['last_vnum'], e['zone']['reset_mode']) == (95203, 95348, 2)
assert collections.Counter(r['command'] for r in e['reset_commands']) == {'M': 29, 'E': 4, 'G': 1}
su = [r for r in e['reset_commands'] if r['command'] == 'M' and r['arguments'][1] == 95300]
assert len(su) == 25 and all(r['arguments'][2] == 25 for r in su)
parent, item_rows = None, []
for reset in e['reset_commands']:
    args = reset['arguments']
    if reset['command'] == 'M':
        parent = (args[1], args[3])
    elif reset['command'] in ('E', 'G'):
        item_rows.append((parent, reset['command'], args[:5]))
assert item_rows == [
    ((95301, 95339), 'E', [1, 95300, 1, 9, 100]),
    ((95301, 95339), 'G', [1, 67259, 1, 0, 100]),
    ((95303, 95340), 'E', [1, 95302, 1, 15, 100]),
    ((95302, 95341), 'E', [1, 95301, 1, 18, 100]),
    ((95304, 95346), 'E', [1, 95303, 1, 8, 100]),
]
for v in (95302, 95304):
    flags = int(mobs[v].split('~', 4)[4].strip().split()[0])
    assert flags & (1 << 15)  # ACT_TEACHER is BIT16.
assert numeric(objects[95301])[0][0] == 32 and numeric(objects[95301])[1][0] == 185
assert '\nA\n14 25\n' in objects[95303]
tablet, scroll = bodies('obj', 'heavens')[402], bodies('obj', 'heavens')[406]
assert 'artifact' in tablet.lower() and 'Agility' in scroll and 'purported' in scroll.lower()
assert numeric(scroll)[0][0] == 13 and numeric(scroll)[1] == [0] * 8
assert 'Halgeous' in rooms[95346] and 'wreck' in rooms[95340].lower()
for v, body in rooms.items():
    controls = list(map(int, body.split('~', 2)[2].strip().splitlines()[0].split()))
    assert controls == [953, 4 if v in (95300, 95348) else 0, 3]
    assert not re.search(r'^[EFC]\s*$', body, re.M)
graph = [(v, int(x[1]), int(x[4]), int(x[5]), int(x[6])) for v, body in rooms.items()
         for x in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)', body, re.S)]
assert len(graph) == 97 and (95312, 0, 4, 0, 82679) in graph
assert not any(v == 95310 or target == 95310 for v, _, _, _, target in graph)
mira = bodies('wld', 'mira')[82679]
assert re.search(r'\bD2\s+[^~]*~[^~]*~\s*4\s+0\s+95312', mira)
for zone in (ROOT / 'areas/zon').glob('*.zon'):
    for row in zone.read_text().splitlines():
        if re.match(r'^D\s+', row):
            assert int(row.split()[2]) not in (95312, 82679), (zone, row)
trios = [b for b in all_native if b['kind'] in ('Q', 'QA') and sorted(b['give']) == [('I', 402), ('I', 26614), ('I', 32490)]]
assert {b['giver_vnum'] for b in trios} == {95304, 4203, 15120, 21535, 53658, 78006, 80907, 98534, 99548}
foreign = [b for b in all_native if b['kind'] in ('Q', 'QA') and (b['giver_vnum'], b['line']) in {(32448, 23), (26644, 98)}]
assert len(foreign) == 2 and all(b['disappear'] for b in foreign)
assert not (ROOT / 'areas/shp/darkfall.shp').exists()
tables = []
for name in ('docs/design/zone-stories/DARKFALL_FOREST.md', 'docs/guides/ZONE_STORY_BUILDING.md', 'docs/design/ZONE_STORY_ROADMAP_EXECUTION.md'):
    rows = [line for line in (ROOT / name).read_text(encoding='utf8').splitlines() if re.match(r'^\| ZSQ-DARKFALL-\d+\s*\|', line)]
    assert len(rows) == 46 and [re.match(r'\| (ZSQ-DARKFALL-\d+)', row)[1] for row in rows] == ['ZSQ-DARKFALL-' + str(n).zfill(2) for n in range(1, 47)]
    tables.append(rows)
assert tables[0] == tables[1] == tables[2]
dossier = (ROOT / 'docs/design/zone-stories/DARKFALL_FOREST.md').read_text(encoding='utf8')
progression = dossier.split('## Progression stories', 1)[1].split('## Implementation and validation boundary', 1)[0]
assert len([line for line in progression.splitlines() if line.startswith('| ') and not line.startswith(('| Story', '| ---'))]) == 8
for phrase in ('active READY accounting', 'Daily policy remains disabled', 'Exact supplied materials remain eligible', 'eight broader', 'Allfive local E/G', 'Guidance alone does not add synthetic achievements', 'separate named fix/news commits', 'TRASH13', 'ordinary open/nonsecret', 'ACT_TEACHER', 'INSTRUMENT_LYRE', 'SONG_ALLIES', 'actual clamped heal'):
    assert phrase.lower() in dossier.lower(), phrase
print('Darkfall Forest: exact supplied trio, current open route and distinct teaching/music/work/beneficiary outcomes retained.')
