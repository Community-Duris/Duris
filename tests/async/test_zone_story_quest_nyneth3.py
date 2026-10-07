#!/usr/bin/env python3
"""Protect fourteen lookalike identities and the final stronghold's story boundaries."""
import collections
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
import zone_story_quest_catalog as cat
import zone_story_quest_zone_inventory as inv


def bodies(kind, area='nyneth3'):
    text = (ROOT / 'areas' / kind / (area + '.' + kind)).read_text()
    return {int(m[1]): m[2] for m in re.finditer(r'^#(\d+)\s*\n([\s\S]*?)(?=^#\d+\s*$|\Z)', text, re.M)}


e = inv.area_evidence(ROOT, 'nyneth3')
catalog = cat.production_catalog(ROOT)
m = next(m for m in catalog['story_mappings'] if m['source_area'] == 'nyneth3')
assert (m['schema_version'], m['revision'], m['coverage']) == (3, 1, 'complete')
assert len(m['stories']) == 1 and not m['exclusions'] and len(m['orientation']) == 15
raw = [b for b in inv.native_blocks(ROOT) if b['source'] == 'areas/qst/nyneth3.qst']
assert [(b['kind'], b['line']) for b in raw] == [('MA', 2), ('Q', 6)]
topics = raw[0]['body'][0].split('~')[0].split()
assert topics == ['soul', 'souls']
q = raw[1]
assert q['give'] == [('I', v) for v in range(38741, 38755)]
assert q['receive'] == [('I', 38755)] and not q['disappear'] and q['body'] == ['~']
s = m['stories'][0]
assert s['id'] == 'fourteen-souls-for-the-hunger' and s['category'] == 'request'
assert s['contracts'] == [q['binding']] and len(s['steps']) == 15
materials, accepted = s['steps'][:14], s['steps'][14]
assert [(r['kind'], r['item_vnums'], r['count'], r['optional']) for r in materials] == [
    ('carried_item', [v], 1, True) for v in range(38741, 38755)]
assert len({r['id'] for r in s['steps']}) == len({r['text'] for r in s['steps']}) == 15
assert accepted['kind'] == 'completion' and accepted['contracts'] == s['contracts']
definition = e['requests'][0]['definition']
assert not definition['prerequisites'] and definition['daily_eligible']
assert definition['eligible_for_zone_completion'] and definition['repeatable']
units = [u for u in cat.story_units(catalog) if u['zone_number'] == 387]
assert len(units) == 1 and units[0]['achievement'] and units[0]['daily_candidate']
rooms, mobs, objects = (bodies(k) for k in ('wld', 'mob', 'obj'))
assert (len(rooms), len(mobs), len(objects)) == (118, 43, 82)
assert set(rooms) == set(range(38700, 38818))
assert {c['mob_vnum'] for c in m['contacts']} == set(mobs) == set(range(38700, 38743))
assert next(c for c in m['contacts'] if c['mob_vnum'] == 38736)['topics'] == topics
assert sum(len(c['topics']) for c in m['contacts']) == 2
assert all(re.fullmatch(r'[a-z0-9_-]{1,64}', c['keyword']) and c['keyword'] in e['mobs'][c['mob_vnum']]['keywords'] for c in m['contacts'])
assert {c['mob_vnum']: c['keyword'] for c in m['contacts'] if c['mob_vnum'] in (38703, 38707, 38718, 38737)} == {38703: 'kethkel', 38707: 'lotaan', 38718: 'blood', 38737: 'nyneth'}
assert (e['zone']['first_vnum'], e['zone']['last_vnum'], e['zone']['reset_mode']) == (38677, 38817, 0)
assert (ROOT / 'areas/zon/nyneth3.zon').read_text().splitlines()[2] == '38817 0 0 40 50 4'
assert collections.Counter(r['command'] for r in e['reset_commands']) == {'M': 84, 'F': 18, 'O': 11, 'E': 60, 'G': 67, 'D': 6}
carriers = {}
current = None
for r in e['reset_commands']:
    if r['command'] in ('M', 'F'):
        current = r['arguments'][1], r['arguments'][3]
    if r['command'] == 'G' and r['arguments'][1] in range(38741, 38755):
        assert r['arguments'][2] == 1 and r['arguments'][1] not in carriers
        carriers[r['arguments'][1]] = current
assert [carriers[v] for v in range(38741, 38755)] == [(v, room) for v, room in (
    (38717, 38805), (38718, 38805), (38721, 38805), (38722, 38805),
    (38707, 38802), (38709, 38802), (38712, 38802), (38714, 38802),
    (38723, 38803), (38724, 38803), (38725, 38803),
    (38731, 38804), (38732, 38804), (38733, 38804))]
assert all(e['mobs'][carriers[v][0]]['name'] in materials[v - 38741]['text'] for v in carriers)
assert len({e['items'][v]['name'] for v in carriers}) == 1
assert any(r['command'] == 'M' and r['arguments'][1:4] == [38736, 1, 38807] for r in e['reset_commands'])
numeric = lambda body: [list(map(int, x.split())) for x in body.split('~', 4)[4].strip().splitlines()[:3]]
assert all(numeric(objects[v])[0][0] == 13 for v in range(38741, 38755))
assert all(numeric(objects[v])[0][0] == 18 and numeric(objects[v])[1][1] == 100 for v in (38755, 38757))
portals = {v: numeric(body)[1][:3] for v, body in objects.items() if numeric(body)[0][0] == 25}
assert portals == {38701: [38700, 7, -1], 38704: [38711, 7, -1], 38705: [38710, 7, -1], 38756: [38809, 7, -1], 38780: [38700, 7, -1]}
assert not any(r['command'] in ('O', 'G', 'E', 'P') and r['arguments'][1] == 38705 for r in e['reset_commands'])
assert numeric(objects[38781])[1][5:8] == [62, 56, 18]
assert numeric(bodies('obj', 'nyneth2')[23071])[0][0] == 12
graph = [(v, int(x[1]), int(x[4]), int(x[5]), int(x[6])) for v, body in rooms.items()
         for x in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)', body, re.S)]
assert len(graph) == 232 and all(target in rooms for _, _, _, _, target in graph)
assert {(38700, 0, 3, 23071, 38701), (38701, 2, 3, 0, 38700),
        (38807, 0, 3, 38755, 38808), (38808, 2, 3, 0, 38807),
        (38809, 1, 7, 38757, 38811), (38811, 3, 3, 38757, 38809)} <= set(graph)
assert {(38812, d, 0, 0, target) for d, target in ((0, 38813), (1, 38811), (2, 38811), (3, 38814))} <= set(graph)
assert not any(re.search(r'^[EFC]$', b, re.M) for b in rooms.values())
assert len({tuple(map(int, b.split('~', 2)[2].strip().splitlines()[0].split())) for b in rooms.values()}) == 6
assert not (ROOT / 'areas/shp/nyneth3.shp').exists()
assert {(r['kind'], r['vnum'], r['function']) for r in e['special_assignments']} == {
    ('mob', 38737, 'nyneth'), ('obj', 38725, 'stormbringer'),
    ('obj', 38763, 'ring_of_regeneration'), ('obj', 38772, 'platemail_of_defense'),
    ('obj', 38761, 'generic_shield_block_proc')}
tables = []
for name in ('docs/design/zone-stories/NYNETH3.md', 'docs/guides/ZONE_STORY_BUILDING.md', 'docs/design/ZONE_STORY_ROADMAP_EXECUTION.md'):
    text = (ROOT / name).read_text(encoding='utf8')
    rows = [line for line in text.splitlines() if re.match(r'^\| ZSQ-NYNETH3-\d+\s*\|', line)]
    assert len(rows) == 56 and [re.match(r'\| (ZSQ-NYNETH3-\d+)', row)[1] for row in rows] == ['ZSQ-NYNETH3-' + str(n).zfill(2) for n in range(1, 57)]
    tables.append(rows)
assert tables[0] == tables[1] == tables[2]
dossier = (ROOT / 'docs/design/zone-stories/NYNETH3.md').read_text(encoding='utf8')
progression = dossier.split('## Progression stories', 1)[1].split('## Implementation and validation boundary', 1)[0]
assert len([line for line in progression.splitlines() if line.startswith('| ') and not line.startswith(('| Story', '| ---'))]) == 12
for phrase in ('active READY accounting', 'Daily policy remains disabled', 'Exact supplied soul types remain eligible',
               'eleven broader', 'no departure', 'separate named fix/news commits', '138 local O/G/E',
               'last_mob_followable', 'nine winning integers', 'identical visible name', 'effective ordinary `barb`',
               'first lawful recovery', 'held/packed', 'actual25 nor50percent', 'zero-charge', 'fourteen materials plus one completion'):
    assert phrase.lower() in dossier.lower(), phrase
print('Ny’Neth Continued: fourteen distinct lookalike/supplied souls, native aliases, source/access/custom-effect and broader story boundaries retained.')
