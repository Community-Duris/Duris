#!/usr/bin/env python3
"""Protect Rice Fields exchange identity, services, access and useful-outcome bounds."""
import collections
from pathlib import Path
import re
import sys
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
import zone_story_quest_catalog as cat
import zone_story_quest_zone_inventory as inv
def bodies(kind, area='jademini'):
    text = (ROOT / 'areas' / kind / (area + '.' + kind)).read_text()
    return {int(m[1]): m[2] for m in re.finditer(r'^#(\d+)\s*\n([\s\S]*?)(?=^#\d+\s*$|\Z)', text, re.M)}
e = inv.area_evidence(ROOT, 'jademini')
catalog = cat.production_catalog(ROOT)
m = next(m for m in catalog['story_mappings'] if m['source_area'] == 'jademini')
assert (m['schema_version'], m['revision'], m['coverage']) == (3, 1, 'complete')
assert len(m['stories']) == 4 and not m['exclusions'] and len(m['orientation']) == 12
raw = [b for b in inv.native_blocks(ROOT) if b['source'] == 'areas/qst/jademini.qst' and b['kind'] == 'Q']
assert [(b['giver_vnum'], b['line'], b['give'], b['receive'], b['disappear']) for b in raw] == [
    (77202, 2, [('C', 50000)], [('I', 76676)], False),
    (77203, 9, [('I', 76670)], [('I', 77201)], False),
    (77214, 28, [('I', 77204)], [('I', 77205)], True),
    (77218, 43, [('I', 22622)], [], False),
]
assert [s['id'] for s in m['stories']] == ['buy-map', 'can-wood-sprite', 'release-princess', 'offer-sea-maps']
assert [s['category'] for s in m['stories']] == ['service', 'service', 'story', 'service']
for index, (q, s) in enumerate(zip(raw, m['stories'])):
    assert s['contracts'] == [q['binding']] and len(s['steps']) == (1 if index == 0 else 2)
    if index:
        material = s['steps'][0]
        assert (material['kind'], material['item_vnums'], material['count'], material['optional']) == ('carried_item', [q['give'][0][1]], 1, True)
    assert s['steps'][-1]['kind'] == 'completion' and s['steps'][-1]['contracts'] == [q['binding']]
assert 'guarded' in m['stories'][0]['summary']
assert all(not r['definition']['prerequisites'] for r in e['requests'])
units = [u for u in cat.story_units(catalog) if u['zone_number'] == 772]
assert len(units) == 4 and sum(u['achievement'] for u in units) == sum(u['daily_candidate'] for u in units) == 1
rooms, mobs, objects = (bodies(k) for k in ('wld', 'mob', 'obj'))
assert (len(rooms), len(mobs), len(objects)) == (63, 20, 15)
assert set(rooms) == set(range(77200, 77263)) and set(mobs) == set(range(77200, 77220))
assert {c['mob_vnum'] for c in m['contacts']} == set(mobs) | {38011, 38026}
prototypes = inv.prototypes(ROOT, cat.zone_registry(ROOT), 'mob')
assert all(not c['topics'] and re.fullmatch(r'[a-z0-9_-]{1,64}', c['keyword']) and c['keyword'] in prototypes[c['mob_vnum']]['keywords'] for c in m['contacts'])
assert (e['zone']['first_vnum'], e['zone']['last_vnum'], e['zone']['reset_mode']) == (76941, 77262, 2)
assert collections.Counter(r['command'] for r in e['reset_commands']) == {'D': 10, 'O': 3, 'P': 5, 'M': 53, 'G': 3, 'E': 2}
numeric = lambda b: [list(map(int, line.split())) for line in b.split('~', 4)[4].strip().splitlines()[:3]]
for v, target in ((77202, 77242), (77203, 77253)):
    header, values, _ = numeric(objects[v])
    assert header[0] == 25 and not header[7] & 1 and values[:3] == [target, 264, -1]
assert numeric(objects[77201])[0][0] == 34 and numeric(objects[77201])[1][0] == 19
assert numeric(objects[77210])[0][0] == 15 and numeric(objects[77210])[1][1:3] == [29, 22633]
current = None
sources = []
for r in e['reset_commands']:
    if r['command'] == 'M': current = r['arguments'][1], r['arguments'][3]
    if r['command'] == 'G': sources.append((current, r['arguments'][1], r['arguments'][2]))
assert sources == [((77201, 77215), 77200, 1), ((77211, 77235), 77204, 1), ((77219, 77248), 77209, 1)]
assert [(r['arguments'][1], r['arguments'][2]) for r in e['reset_commands'] if r['command'] == 'P'] == [(77212, 2), (77212, 2), (77211, 1), (77213, 1), (77214, 1)]
assert sum(r['command'] == 'M' and r['arguments'][1] == 38026 and r['arguments'][3] == 77258 for r in e['reset_commands']) == 2
assert sum(r['command'] == 'M' and r['arguments'][1] == 38011 and r['arguments'][3] == 77259 for r in e['reset_commands']) == 2
graph = [(v, int(x[1]), int(x[4]), int(x[5]), int(x[6])) for v, body in rooms.items()
         for x in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)', body, re.S)]
assert len(graph) == 153
assert {(77239, 1, 3, 76710, 77240), (77240, 3, 3, 76710, 77239), (77261, 5, 6, 38037, 77262), (77262, 4, 7, 38037, 77261), (77262, 2, 0, 0, -1)} <= set(graph)
assert not any(key == 77204 for _, _, _, key, _ in graph)
assert not any(re.search(r'^[EFC]$', b, re.M) for b in rooms.values())
assert [(a['vnum'], a['function']) for a in e['special_assignments']] == [(77216, 'archer')]
assert not (ROOT / 'areas/shp/jademini.shp').exists()
all_native = inv.native_blocks(ROOT)
foreign = [(b['giver_vnum'], b['line'], b['give'], b['receive'], b['disappear']) for b in all_native if (b['source'], b['line']) in {('areas/qst/jade.qst', 326), ('areas/qst/jade.qst', 337), ('areas/qst/jade.qst', 353), ('areas/qst/spshold.qst', 10), ('areas/qst/spshold.qst', 18), ('areas/qst/thetis.qst', 37)}]
assert len(foreign) == 6
assert (38037, 37, [('I', 77209)], [('I', 38037), ('I', 77209)], False) in foreign
assert (22626, 18, [('I', 77209)], [('I', 22633)], False) in foreign
assert (76711, 337, [('I', 77205)], [('I', 76722), ('I', 55372)], False) in foreign
tables = []
for name in ('docs/design/zone-stories/RICE_FIELDS.md', 'docs/guides/ZONE_STORY_BUILDING.md', 'docs/design/ZONE_STORY_ROADMAP_EXECUTION.md'):
    rows = [line for line in (ROOT / name).read_text(encoding='utf8').splitlines() if re.match(r'^\| ZSQ-JADEMINI-\d+\s*\|', line)]
    assert len(rows) == 46 and [re.match(r'\| (ZSQ-JADEMINI-\d+)', row)[1] for row in rows] == ['ZSQ-JADEMINI-' + str(n).zfill(2) for n in range(1, 47)]
    tables.append(rows)
assert tables[0] == tables[1] == tables[2]
dossier = (ROOT / 'docs/design/zone-stories/RICE_FIELDS.md').read_text(encoding='utf8')
progression = dossier.split('## Progression stories', 1)[1].split('## Implementation and validation boundary', 1)[0]
assert len([line for line in progression.splitlines() if line.startswith('| ') and not line.startswith(('| Story', '| ---'))]) == 9
for phrase in ('active READY accounting', 'Daily policy remains disabled', 'Exact supplied materials remain eligible', 'nine broader', 'All13 local O/P/G/E', 'Guidance alone does not add synthetic achievements', 'separate named fix/news commits', 'documentation-only commit', 'no E/F/C tails', 'CMD_JUMP', 'KEY18', 'TOTEM34'):
    assert phrase.lower() in dossier.lower(), phrase
guide = (ROOT / 'docs/design/zone-stories/THE_JADE_EMPIRE.md').read_text(encoding='utf8')
assert 'The planks use JUMP 264 teleports' in guide and 'The planks use WALK 264 teleports' not in guide
assert '#define CMD_JUMP 264' in (ROOT / 'src/cmd/interp.h').read_text()
print('Rice Fields: four independent exchanges, three recorded services, exact supplied materials and bounded access/beneficiary plans retained.')
