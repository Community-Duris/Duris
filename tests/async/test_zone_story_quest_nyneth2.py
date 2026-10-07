#!/usr/bin/env python3
"""Protect distinct ore preparation and the stronghold's integration boundaries."""
import collections
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
import zone_story_quest_catalog as cat
import zone_story_quest_zone_inventory as inv


def bodies(kind, area='nyneth2'):
    text = (ROOT / 'areas' / kind / (area + '.' + kind)).read_text()
    return {int(m[1]): m[2] for m in re.finditer(r'^#(\d+)\s*\n([\s\S]*?)(?=^#\d+\s*$|\Z)', text, re.M)}


e = inv.area_evidence(ROOT, 'nyneth2')
catalog = cat.production_catalog(ROOT)
m = next(m for m in catalog['story_mappings'] if m['source_area'] == 'nyneth2')
assert (m['schema_version'], m['revision'], m['coverage']) == (3, 1, 'complete')
assert len(m['stories']) == 1 and not m['exclusions'] and len(m['orientation']) == 13
raw = [b for b in inv.native_blocks(ROOT) if b['source'] == 'areas/qst/nyneth2.qst']
assert [(b['kind'], b['line']) for b in raw] == [('MA', 2), ('QA', 8)]
topics = raw[0]['body'][0].split('~')[0].split()
assert topics == ['tiloxi']
q = raw[1]
assert q['give'] == [('I', v) for v in range(23016, 23021)]
assert q['receive'] == [('I', 23021)] and not q['disappear']
s = m['stories'][0]
assert s['id'] == 'five-ores-for-the-foreman' and s['category'] == 'request'
assert s['contracts'] == [q['binding']] and len(s['steps']) == 6
materials, accepted = s['steps'][:5], s['steps'][5]
assert [(r['kind'], r['item_vnums'], r['count'], r['optional']) for r in materials] == [
    ('carried_item', [v], 1, True) for v in range(23016, 23021)]
assert len({r['id'] for r in s['steps']}) == 6
assert accepted['kind'] == 'completion' and accepted['contracts'] == s['contracts']
definition = e['requests'][0]['definition']
assert not definition['prerequisites'] and definition['daily_eligible']
assert definition['eligible_for_zone_completion'] and definition['repeatable']
units = [u for u in cat.story_units(catalog) if u['zone_number'] == 230]
assert len(units) == 1 and units[0]['achievement'] and units[0]['daily_candidate']
rooms, mobs, objects = (bodies(k) for k in ('wld', 'mob', 'obj'))
assert (len(rooms), len(mobs), len(objects)) == (101, 35, 72)
assert set(rooms) == set(range(23000, 23101))
assert {c['mob_vnum'] for c in m['contacts']} == set(mobs) == set(range(23000, 23035))
assert next(c for c in m['contacts'] if c['mob_vnum'] == 23002)['topics'] == topics
assert sum(len(c['topics']) for c in m['contacts']) == 1
assert (e['zone']['first_vnum'], e['zone']['last_vnum'], e['zone']['reset_mode']) == (22997, 23100, 0)
assert (ROOT / 'areas/zon/nyneth2.zon').read_text().splitlines()[2] == '23100 0 0 40 50 2'
assert collections.Counter(r['command'] for r in e['reset_commands']) == {'M': 111, 'F': 7, 'O': 42, 'E': 82, 'G': 63, 'D': 24}
ore_rows = [r['arguments'][1:4] for r in e['reset_commands'] if r['command'] == 'O' and r['arguments'][1] in range(23016, 23021)]
assert ore_rows == [[v, 1, room] for v, room in zip(range(23016, 23021), (23017, 23020, 23024, 23029, 23031))]
assert any(r['command'] == 'M' and r['arguments'][1:4] == [23002, 1, 23006] for r in e['reset_commands'])
numeric = lambda body: [list(map(int, x.split())) for x in body.split('~', 4)[4].strip().splitlines()[:3]]
assert all(numeric(objects[v])[0][0] == 13 for v in range(23016, 23021))
assert numeric(objects[23021])[0][0] == 18 and numeric(objects[23021])[1][:4] == [0, 0, 0, 0]
cart_ids = set(range(23000, 23016)) | {23022}
assert {v for v, body in objects.items() if numeric(body)[0][0] == 25} == cart_ids | {23029}
assert all(numeric(objects[v])[1][1:3] == [7, -1] for v in cart_ids)
assert numeric(objects[23029])[1][:3] == [23083, 7, 12]
assert numeric(objects[23071])[0][0] == 12 and numeric(objects[23071])[1][1] == 100
assert numeric(bodies('obj', 'nyneth')[22964])[0][0] == 12
graph = [(v, int(x[1]), int(x[4]), int(x[5]), int(x[6])) for v, body in rooms.items()
         for x in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)', body, re.S)]
assert len(graph) == 210 and all(target in rooms for _, _, _, _, target in graph)
assert {(23002, 3, 3, 22964, 23003), (23003, 1, 3, 0, 23002),
        (23029, 1, 7, 23021, 23032), (23032, 3, 7, 23021, 23029),
        (23034, 1, 3, 23021, 23037), (23037, 3, 3, 0, 23034)} <= set(graph)
assert {(23099, d, 0, 0, target) for d, target in ((0, 23081), (1, 23081), (2, 23081), (3, 23100))} <= set(graph)
assert not any(re.search(r'^[EFC]$', b, re.M) for b in rooms.values())
assert len({tuple(map(int, b.split('~', 2)[2].strip().splitlines()[0].split())) for b in rooms.values()}) == 9
assert not (ROOT / 'areas/shp/nyneth2.shp').exists()
tables = []
for name in ('docs/design/zone-stories/NYNETH2.md', 'docs/guides/ZONE_STORY_BUILDING.md', 'docs/design/ZONE_STORY_ROADMAP_EXECUTION.md'):
    text = (ROOT / name).read_text(encoding='utf8')
    rows = [line for line in text.splitlines() if re.match(r'^\| ZSQ-NYNETH2-\d+\s*\|', line)]
    assert len(rows) == 48 and [re.match(r'\| (ZSQ-NYNETH2-\d+)', row)[1] for row in rows] == ['ZSQ-NYNETH2-' + str(n).zfill(2) for n in range(1, 49)]
    tables.append(rows)
assert tables[0] == tables[1] == tables[2]
dossier = (ROOT / 'docs/design/zone-stories/NYNETH2.md').read_text(encoding='utf8')
for phrase in ('active READY accounting', 'Daily policy remains disabled', 'Exact supplied chunks remain eligible',
               'eight broader', 'no departure', 'separate named fix/news commits', '187 local O/G/E',
               'last_mob_followable', 'durable reset-generation', 'zero-charge', 'VNUM-based', 'twelve charges'):
    assert phrase in dossier, phrase
print('Ny’Neth Stronghold: five distinct supplied ores, native acceptance, access and wider accounting/story boundaries retained.')
