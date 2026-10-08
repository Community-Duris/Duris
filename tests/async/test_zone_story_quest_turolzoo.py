#!/usr/bin/env python3
"""Protect the zoo's exact native tooth pair and deeper integration boundaries."""
import collections
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
import zone_story_quest_catalog as cat
import zone_story_quest_zone_inventory as inv


def bodies(kind):
    text = (ROOT / 'areas' / kind / ('turolzoo.' + kind)).read_text()
    return {int(m[1]): m[2] for m in re.finditer(r'^#(\d+)\s*\n([\s\S]*?)(?=^#\d+\s*$|\Z)', text, re.M)}


e = inv.area_evidence(ROOT, 'turolzoo')
catalog = cat.production_catalog(ROOT)
m = next(m for m in catalog['story_mappings'] if m['source_area'] == 'turolzoo')
assert (m['schema_version'], m['revision'], m['coverage']) == (3, 1, 'complete')
assert len(m['stories']) == 1 and not m['exclusions'] and len(m['orientation']) == 10
raw = [b for b in inv.native_blocks(ROOT) if b['source'] == 'areas/qst/turolzoo.qst']
assert [(b['kind'], b['line']) for b in raw] == [('M', 2), ('Q', 11)]
assert raw[0]['giver_vnum'] == 53003 and raw[0]['body'][0] == 'hunt prey hunting help~'
q = raw[1]
assert q['giver_vnum'] == 53003 and q['give'] == [('I', 53001), ('I', 53003)]
assert q['receive'] == [('I', 53004)] and q['disappear']
s = m['stories'][0]
assert s['id'] == 'collect-the-hunters-teeth' and s['category'] == 'request'
assert s['contracts'] == [q['binding']] and len(s['steps']) == 3
saber, gorilla, accepted = s['steps']
assert (saber['kind'], saber['item_vnums'], saber['count'], saber['optional']) == ('carried_item', [53001], 1, True)
assert (gorilla['kind'], gorilla['item_vnums'], gorilla['count'], gorilla['optional']) == ('carried_item', [53003], 1, True)
assert accepted['kind'] == 'completion' and accepted['contracts'] == s['contracts']
definition = e['requests'][0]['definition']
assert not definition['prerequisites'] and definition['daily_eligible'] and definition['repeatable']
units = [u for u in cat.story_units(catalog) if u['zone_number'] == 530]
assert len(units) == 1 and units[0]['achievement'] and units[0]['daily_candidate']
rooms, mobs, objects = (bodies(k) for k in ('wld', 'mob', 'obj'))
assert (len(rooms), len(mobs), len(objects)) == (62, 13, 6)
assert set(rooms) == set(range(53000, 53062))
assert {c['mob_vnum'] for c in m['contacts']} == set(mobs) == set(range(53000, 53013))
assert next(c for c in m['contacts'] if c['mob_vnum'] == 53003)['topics'] == ['hunt', 'prey', 'hunting', 'help']
assert sum(len(c['topics']) for c in m['contacts']) == 4
assert all(re.fullmatch(r'[a-z0-9_-]{1,64}', c['keyword']) and c['keyword'] in e['mobs'][c['mob_vnum']]['keywords'] for c in m['contacts'])
assert (e['zone']['first_vnum'], e['zone']['last_vnum'], e['zone']['reset_mode']) == (52921, 53061, 2)
assert (ROOT / 'areas/zon/turolzoo.zon').read_text().splitlines()[2] == '53061 2 0 20 30 1'
assert collections.Counter(r['command'] for r in e['reset_commands']) == {'D': 12, 'O': 1, 'M': 56, 'E': 5, 'G': 2}
current = None
source = []
for r in e['reset_commands']:
    if r['command'] == 'M':
        current = r['arguments'][1], r['arguments'][3]
    if r['command'] == 'G':
        source.append((r['arguments'][1], current, r['arguments'][2]))
assert source == [(53003, (53004, 53021), 1), (53001, (53002, 53037), 1)]
assert any(r['command'] == 'M' and r['arguments'][1:5] == [53003, 1, 53036, 100] for r in e['reset_commands'])
numeric = lambda body: [list(map(int, x.split())) for x in body.split('~', 4)[4].strip().splitlines()[:3]]
assert numeric(objects[53000])[0][0] == 29 and numeric(objects[53000])[1][:4] == [340, 53019, 5, 0]
for v in (53001, 53003):
    header = numeric(objects[v])[0]
    assert header[0] == 13 and header[6:9] == [4104, 16385, 32768]
assert numeric(objects[53004])[0][0] == 9
assert [(r['arguments'][2], r['arguments'][3]) for r in e['reset_commands'] if r['command'] == 'E' and r['arguments'][1] == 53002] == [(4, 16), (4, 17), (4, 25), (4, 26)]
graph = [(v, int(x[1]), int(x[4]), int(x[5]), int(x[6])) for v, body in rooms.items()
         for x in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)', body, re.S)]
assert len(graph) == 141 and [(v, d, target) for v, d, _, _, target in graph if target not in rooms] == [(53000, 1, 7123)]
assert {(53019, 5, 13, 0, 53021), (53021, 4, 5, 0, 53019), (53021, 3, 0, 0, 53061),
        (53061, 1, 0, 0, 53021), (53037, 0, 1, 0, 53035)} <= set(graph)
assert all(key == 0 for _, _, _, key, _ in graph)
assert len({tuple(map(int, b.split('~', 2)[2].strip().splitlines()[0].split())) for b in rooms.values()}) == 10
assert sum(len(re.findall(r'^E$', b, re.M)) for b in rooms.values()) == 3
assert not any(re.search(r'^[FC]$', b, re.M) for b in rooms.values())
assert not e['special_assignments'] and not (ROOT / 'areas/shp/turolzoo.shp').exists()
headers = {v: list(map(int, b.split('~', 4)[4].strip().splitlines()[0].split()[:-1])) for v, b in mobs.items()}
assert all(h[0] & 1073741824 and not h[0] & 536870912 for h in headers.values())
assert headers[53012][1] == 896 and headers[53003][0] & 4 and not headers[53003][0] & 2
tables = []
for name in ('docs/design/zone-stories/TUROLZOO.md', 'docs/guides/ZONE_STORY_BUILDING.md', 'docs/design/ZONE_STORY_ROADMAP_EXECUTION.md'):
    rows = [line for line in (ROOT / name).read_text(encoding='utf8').splitlines() if re.match(r'^\| ZSQ-TUROLZOO-\d+\s*\|', line)]
    assert len(rows) == 46 and [re.match(r'\| (ZSQ-TUROLZOO-\d+)', row)[1] for row in rows] == ['ZSQ-TUROLZOO-' + str(n).zfill(2) for n in range(1, 47)]
    tables.append(rows)
assert tables[0] == tables[1] == tables[2]
dossier = (ROOT / 'docs/design/zone-stories/TUROLZOO.md').read_text(encoding='utf8')
progression = dossier.split('## Progression stories', 1)[1].split('## Implementation and validation boundary', 1)[0]
assert len([line for line in progression.splitlines() if line.startswith('| ') and not line.startswith(('| Story', '| ---'))]) == 10
for phrase in ('active READY accounting', 'Daily policy remains disabled', 'Exact supplied pair remains eligible',
               'nine broader', 'eight O/G/E', 'first source custody', 'no named saved lasting collection outcome',
               'Guidance alone does not add synthetic achievements', 'separate named fix/news commits',
               'ACT_HUNTER', 'not an ACT2 cleaning bit', 'NOSELL|SECRET', 'no F/C tails'):
    assert phrase.lower() in dossier.lower(), phrase
print('Turolopolis Zoo: supplied exact tooth pair/helmet/departure, four shared aliases, hidden route and useful-outcome boundaries retained.')
