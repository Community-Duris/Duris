#!/usr/bin/env python3
"""Protect independent exact-food offerings and bounded forest integration claims."""
import collections
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
import zone_story_quest_catalog as cat
import zone_story_quest_zone_inventory as inv

def bodies(kind, area='forgotten_forest'):
    text = (ROOT / 'areas' / kind / (area + '.' + kind)).read_text()
    return {int(m[1]): m[2] for m in re.finditer(r'^#(\d+)\s*\n([\s\S]*?)(?=^#\d+\s*$|\Z)', text, re.M)}

e = inv.area_evidence(ROOT, 'forgotten_forest')
catalog = cat.production_catalog(ROOT)
m = next(m for m in catalog['story_mappings'] if m['source_area'] == 'forgotten_forest')
assert (m['schema_version'], m['revision'], m['coverage']) == (3, 1, 'complete')
assert len(m['stories']) == 4 and not m['exclusions'] and len(m['orientation']) == 12
raw = [b for b in inv.native_blocks(ROOT) if b['source'] == 'areas/qst/forgotten_forest.qst']
assert [(b['kind'], b['line'], b['give'], b['receive'], b['disappear']) for b in raw] == [
    ('Q', 2, [('I', 82703)], [('E', 17000)], False),
    ('Q', 9, [('I', 82702)], [('E', 14000)], False),
    ('Q', 16, [('I', 82701)], [('E', 18100)], False),
    ('Q', 23, [('I', 82706)], [('E', 49000)], False),
]
assert all(q['giver_vnum'] == 82702 for q in raw)
assert [s['id'] for s in m['stories']] == ['offer-blackberries', 'offer-yellow-mushroom', 'offer-small-meat', 'offer-large-meat']
for q, s in zip(raw, m['stories']):
    assert s['category'] == 'request' and s['contracts'] == [q['binding']] and len(s['steps']) == 2
    material, accepted = s['steps']
    assert (material['kind'], material['item_vnums'], material['count'], material['optional']) == ('carried_item', [q['give'][0][1]], 1, True)
    assert accepted['kind'] == 'completion' and accepted['contracts'] == [q['binding']]
assert len({tuple(s['contracts'][0].items()) for s in m['stories']}) == 4
assert all(not r['definition']['prerequisites'] and r['definition']['daily_eligible'] and r['definition']['repeatable'] for r in e['requests'])
units = [u for u in cat.story_units(catalog) if u['zone_number'] == 827]
assert len(units) == 4 and all(u['achievement'] and u['daily_candidate'] for u in units)
rooms, mobs, objects = (bodies(k) for k in ('wld', 'mob', 'obj'))
assert (len(rooms), len(mobs), len(objects)) == (92, 25, 7)
assert set(rooms) == set(range(82701, 82793))
assert {c['mob_vnum'] for c in m['contacts']} == set(mobs) == set(range(82701, 82726))
assert all(not c['topics'] and re.fullmatch(r'[a-z0-9_-]{1,64}', c['keyword']) and c['keyword'] in e['mobs'][c['mob_vnum']]['keywords'] for c in m['contacts'])
assert (e['zone']['first_vnum'], e['zone']['last_vnum'], e['zone']['reset_mode']) == (82688, 82792, 2)
assert collections.Counter(r['command'] for r in e['reset_commands']) == {'O': 4, 'M': 83, 'G': 3, 'E': 3, 'F': 3}
numeric = lambda b: [list(map(int, line.split())) for line in b.split('~', 4)[4].strip().splitlines()[:3]]
for v in (82701, 82702, 82703, 82706):
    header, values, _ = numeric(objects[v])
    assert header[0] == 19 and header[8] == 32768 and header[7] & 1
    assert header[6] == (20488 if v in (82702, 82703) else 8)
assert numeric(objects[82702])[1][:4] == [9, 6, 5, 0]
assert numeric(objects[82703])[0][7] == 16385
current = None
meat_sources = []
for r in e['reset_commands']:
    if r['command'] == 'M': current = r['arguments'][1], r['arguments'][3]
    if r['command'] == 'G': meat_sources.append((current, r['arguments'][1], r['arguments'][2]))
assert meat_sources == [((82708, 82711), 82701, 4), ((82703, 82727), 82701, 4), ((82718, 82763), 82706, 1)]
assert [(r['arguments'][1], r['arguments'][2]) for r in e['reset_commands'] if r['command'] == 'F'] == [(82720, 3)] * 3
graph = [(v, int(x[1]), int(x[4]), int(x[5]), int(x[6])) for v, body in rooms.items()
         for x in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)', body, re.S)]
assert len(graph) == 283 and all(flags == key == 0 for _, _, flags, key, _ in graph)
assert {(82701, 3, 0, 0, 516090), (82763, 4, 0, 0, 82702), (82763, 5, 0, 0, 82762), (82769, 4, 0, 0, 82770)} <= set(graph)
assert not any(v == 82702 for v, *_ in graph)
assert not any(re.search(r'^[EFC]$', b, re.M) for b in rooms.values())
assert all(int(rooms[v].split('~', 2)[2].splitlines()[1].split()[2]) == 26 for v in range(82773, 82793))
assert not e['special_assignments'] and not (ROOT / 'areas/shp/forgotten_forest.shp').exists()
tables = []
for name in ('docs/design/zone-stories/FORGOTTEN_FOREST.md', 'docs/guides/ZONE_STORY_BUILDING.md', 'docs/design/ZONE_STORY_ROADMAP_EXECUTION.md'):
    rows = [line for line in (ROOT / name).read_text(encoding='utf8').splitlines() if re.match(r'^\| ZSQ-FORGOTTEN-FOREST-\d+\s*\|', line)]
    assert len(rows) == 46 and [re.match(r'\| (ZSQ-FORGOTTEN-FOREST-\d+)', row)[1] for row in rows] == ['ZSQ-FORGOTTEN-FOREST-' + str(n).zfill(2) for n in range(1, 47)]
    tables.append(rows)
assert tables[0] == tables[1] == tables[2]
dossier = (ROOT / 'docs/design/zone-stories/FORGOTTEN_FOREST.md').read_text(encoding='utf8')
progression = dossier.split('## Progression stories', 1)[1].split('## Implementation and validation boundary', 1)[0]
assert len([line for line in progression.splitlines() if line.startswith('| ') and not line.startswith(('| Story', '| ---'))]) == 10
for phrase in ('active READY accounting', 'Daily policy remains disabled', 'Exact supplied food remains eligible',
               'nine broader', 'All10 local O/G/E', 'four independent', 'mandatory ALL meal',
               'Guidance alone does not add synthetic achievements', 'separate named fix/news commits',
               'failed or refused gloves do not automatically suppress', 'SECT_SWAMP', 'not NORENT',
               'no E/F/C tails', 'declared25..30'):
    assert phrase.lower() in dossier.lower(), phrase
print('Forgotten Forest: four independent supplied-compatible food/XP offerings, exact custody and bounded useful-outcome plans retained.')
