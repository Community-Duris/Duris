#!/usr/bin/env python3
"""Protect the Roper Den's four-item journal and wider integration boundaries."""
import collections
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
import zone_story_quest_catalog as cat
import zone_story_quest_zone_inventory as inv


def bodies(kind):
    text = (ROOT / 'areas' / kind / ('nexus_roper.' + kind)).read_text()
    return {int(m[1]): m[2] for m in re.finditer(r'^#(\d+)\s*\n([\s\S]*?)(?=^#\d+\s*$|\Z)', text, re.M)}


e = inv.area_evidence(ROOT, 'nexus_roper')
catalog = cat.production_catalog(ROOT)
m = next(m for m in catalog['story_mappings'] if m['source_area'] == 'nexus_roper')
assert (m['schema_version'], m['revision'], m['coverage']) == (3, 1, 'complete')
assert len(m['stories']) == 1 and not m['exclusions'] and len(m['orientation']) == 12
raw = [b for b in inv.native_blocks(ROOT) if b['source'] == 'areas/qst/nexus_roper.qst']
assert [(b['kind'], b['line']) for b in raw] == [('M', 2), ('Q', 6)]
topics = raw[0]['body'][0].split('~')[0].split()
assert topics == ['roper', 'tentacle', 'ropers', 'hello', 'hi']
q = raw[1]
assert q['give'] == [('I', 130400)]*4 and q['receive'] == [('I', 130401)] and not q['disappear']
assert q['body'] == ['~']
s = m['stories'][0]
assert s['id'] == 'tentacles-for-sebastian' and s['category'] == 'request' and s['contracts'] == [q['binding']]
material, accepted = s['steps']
assert (material['kind'], material['item_vnums'], material['count'], material['optional']) == ('carried_item', [130400], 4, True)
assert accepted['kind'] == 'completion' and accepted['contracts'] == s['contracts']
definition = e['requests'][0]['definition']
assert not definition['prerequisites'] and definition['daily_eligible'] and definition['eligible_for_zone_completion'] and definition['repeatable']
units = [u for u in cat.story_units(catalog) if u['zone_number'] == 1304]
assert len(units) == 1 and units[0]['achievement'] and units[0]['daily_candidate']
rooms, mobs, objects = (bodies(k) for k in ('wld', 'mob', 'obj'))
assert (len(rooms), len(mobs), len(objects)) == (25, 8, 2)
assert {c['mob_vnum'] for c in m['contacts']} == set(mobs) == set(range(130400, 130408))
assert next(c for c in m['contacts'] if c['mob_vnum'] == 130407)['topics'] == topics
assert sum(len(c['topics']) for c in m['contacts']) == 5
assert all(not b.split('~', 4)[3].strip() for b in mobs.values())
assert (e['zone']['first_vnum'], e['zone']['last_vnum'], e['zone']['reset_mode']) == (130400, 130424, 0)
assert (ROOT / 'areas/zon/nexus_roper.zon').read_text().splitlines()[2] == '130424 0 0 40 50 4'
assert collections.Counter(r['command'] for r in e['reset_commands']) == {'M': 57, 'G': 4, 'D': 2}
carriers = []
current = None
for r in e['reset_commands']:
    if r['command'] == 'M':
        current = r['arguments'][1], r['arguments'][3]
    if r['command'] == 'G':
        assert r['arguments'][1:5] == [130400, 4, 0, 100]
        carriers.append(current)
assert carriers == [(130406, 130409), (130406, 130411), (130406, 130415), (130406, 130416)]
assert any(r['command'] == 'M' and r['arguments'][1:4] == [130407, 1, 130424] for r in e['reset_commands'])
tentacle = [list(map(int, x.split())) for x in objects[130400].split('~', 4)[4].strip().splitlines()[:3]]
key = [list(map(int, x.split())) for x in objects[130401].split('~', 4)[4].strip().splitlines()[:3]]
assert tentacle[0][0] == 13 and tentacle[0][7:9] == [16385, 32768] and tentacle[1][:4] == [0, 0, 0, 0]
assert key[0][0] == 18 and key[0][7] == 1 and key[1][:4] == [0, 100, 0, 0]
graph = [(v, int(x[1]), int(x[4]), int(x[5]), int(x[6])) for v, body in rooms.items()
         for x in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)', body, re.S)]
assert len(graph) == 51 and {target for _, _, _, _, target in graph if target not in rooms} == {811645}
assert {(130400, 3, 5, 0, 130424), (130424, 1, 5, 0, 130400), (130422, 5, 4, 0, 130423), (130423, 4, 4, 0, 130422)} <= set(graph)
assert (130406, 4, 0, 0, 130405) in graph and (130410, 4, 0, 0, 130407) in graph
assert not any(re.search(r'^[EFC]$', b, re.M) for b in rooms.values())
controls = {tuple(map(int, b.split('~', 2)[2].strip().splitlines()[0].split())) for b in rooms.values()}
assert controls == {(1304, n, 15, 0) for n in (1208258632, 1208258636, 1208258637, 1216647240)}
assert not (ROOT / 'areas/shp/nexus_roper.shp').exists() and not e['special_assignments']
tables = []
for name in ('docs/design/zone-stories/NEXUS_ROPER.md', 'docs/guides/ZONE_STORY_BUILDING.md', 'docs/design/ZONE_STORY_ROADMAP_EXECUTION.md'):
    text = (ROOT / name).read_text(encoding='utf8')
    rows = [line for line in text.splitlines() if re.match(r'^\| ZSQ-NEXUSROPER-\d+\s*\|', line)]
    assert len(rows) == 40 and [re.match(r'\| (ZSQ-NEXUSROPER-\d+)', row)[1] for row in rows] == ['ZSQ-NEXUSROPER-' + str(n).zfill(2) for n in range(1, 41)]
    tables.append(rows)
assert tables[0] == tables[1] == tables[2]
dossier = (ROOT / 'docs/design/zone-stories/NEXUS_ROPER.md').read_text(encoding='utf8')
assert len(re.findall(r'^\| (?:Sebastian’s|Hidden approach|Roper ecology|Lawful source|Flaming key|Volcanic route|Useful nexus)', dossier, re.M)) == 7
for phrase in ('active READY accounting', 'Daily policy remains disabled', 'Exact supplied tentacles remain eligible', 'six broader', 'no departure', 'separate named fix/news commits', 'state&3', 'nexus_stone_touch', 'no_reset_zone_reset'):
    assert phrase in dossier, phrase
print('Roper Den nexus: four exact supplied tentacles/key, aliases, source/access/controller boundaries and broader follow-ups retained.')
