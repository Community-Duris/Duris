#!/usr/bin/env python3
"""Protect Crushk's exact foreign-shiv bounty and useful-outcome boundaries."""
import collections
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
import zone_story_quest_catalog as cat
import zone_story_quest_zone_inventory as inv


def bodies(kind, area='valley_crushk'):
    text = (ROOT / 'areas' / kind / (area + '.' + kind)).read_text()
    return {int(m[1]): m[2] for m in re.finditer(r'^#(\d+)\s*\n([\s\S]*?)(?=^#\d+\s*$|\Z)', text, re.M)}


e = inv.area_evidence(ROOT, 'valley_crushk')
catalog = cat.production_catalog(ROOT)
m = next(m for m in catalog['story_mappings'] if m['source_area'] == 'valley_crushk')
assert (m['schema_version'], m['revision'], m['coverage']) == (3, 1, 'complete')
assert len(m['stories']) == 1 and not m['exclusions'] and len(m['orientation']) == 12
raw = [b for b in inv.native_blocks(ROOT) if b['source'] == 'areas/qst/valley_crushk.qst']
assert [(b['kind'], b['line']) for b in raw] == [('M', 2), ('MA', 6), ('QA', 16)]
assert raw[0]['body'][0] == 'qc_action 55~' and raw[1]['body'][0] == 'hi hello~'
q = raw[2]
assert q['giver_vnum'] == 2038 and q['give'] == [('I', 43143)]
assert q['receive'] == [('C', 4000)] and not q['disappear']
s = m['stories'][0]
assert s['id'] == 'claim-flazohs-bandit-bounty' and s['category'] == 'request'
assert s['contracts'] == [q['binding']] and len(s['steps']) == 2
material, accepted = s['steps']
assert (material['kind'], material['item_vnums'], material['count'], material['optional']) == ('carried_item', [43143], 1, True)
assert accepted['kind'] == 'completion' and accepted['contracts'] == s['contracts']
definition = e['requests'][0]['definition']
assert not definition['prerequisites'] and definition['daily_eligible'] and definition['repeatable']
units = [u for u in cat.story_units(catalog) if u['zone_number'] == 20]
assert len(units) == 1 and units[0]['achievement'] and units[0]['daily_candidate']
rooms, mobs, objects = (bodies(k) for k in ('wld', 'mob', 'obj'))
assert (len(rooms), len(mobs), len(objects)) == (85, 38, 22)
assert set(rooms) == set(range(2001, 2089)) - {2013, 2069, 2071}
assert {c['mob_vnum'] for c in m['contacts']} == set(mobs) == set(range(2001, 2039))
assert next(c for c in m['contacts'] if c['mob_vnum'] == 2038)['topics'] == ['hi', 'hello']
assert sum(len(c['topics']) for c in m['contacts']) == 2
assert all(re.fullmatch(r'[a-z0-9_-]{1,64}', c['keyword']) and c['keyword'] in e['mobs'][c['mob_vnum']]['keywords'] for c in m['contacts'])
assert (e['zone']['first_vnum'], e['zone']['last_vnum'], e['zone']['reset_mode']) == (1948, 2088, 2)
assert collections.Counter(r['command'] for r in e['reset_commands']) == {'D': 20, 'O': 2, 'M': 128, 'E': 6, 'G': 15}
assert not any(r['command'] == 'G' and r['arguments'][1] == 43143 for r in e['reset_commands'])
foreign = inv.area_evidence(ROOT, 'shipy')
current = None
shiv_sources = []
for r in foreign['reset_commands']:
    if r['command'] == 'M':
        current = r['arguments'][1], r['arguments'][3]
    if r['command'] == 'G' and r['arguments'][1] == 43143:
        shiv_sources.append((current, r['arguments'][2]))
assert shiv_sources == [((43198, 43328), 11)] * 11
numeric = lambda body: [list(map(int, x.split())) for x in body.split('~', 4)[4].strip().splitlines()[:3]]
for v, target in ((2001, 2044), (2005, 2088)):
    header, values, _ = numeric(objects[v])
    assert header[0] == 18 and header[6] == 8388608 and values[:2] == [target, 0]
shiv_header = numeric(bodies('obj', 'shipy')[43143])[0]
assert shiv_header[0] == 5 and shiv_header[7:9] == [8193, 32768]
graph = [(v, int(x[1]), int(x[4]), int(x[5]), int(x[6])) for v, body in rooms.items()
         for x in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)', body, re.S)]
assert len(graph) == 182
assert {(2018, 1, 2, -2, 2019), (2019, 3, 2, -2, 2018),
        (2044, 0, 2, 2001, 2051), (2051, 2, 2, 2001, 2044),
        (2087, 2, 2, 2005, 2088), (2088, 0, 2, 2005, 2087)} <= set(graph)
assert sum(len(re.findall(r'^E$', b, re.M)) for b in rooms.values()) == 3
assert not any(re.search(r'^[FC]$', b, re.M) for b in rooms.values())
assert not e['special_assignments']
shops = (ROOT / 'areas/shp/valley_crushk.shp').read_text()
assert re.findall(r'^#(\d+)~', shops, re.M) == ['2031', '2032', '2033']
assert '#2032~\nN\n399\n606\n2020\n2026\n0' in shops
assert '#2033~\nN\n2021\n2025\n0' in shops
tables = []
for name in ('docs/design/zone-stories/VALLEY_CRUSHK.md', 'docs/guides/ZONE_STORY_BUILDING.md', 'docs/design/ZONE_STORY_ROADMAP_EXECUTION.md'):
    rows = [line for line in (ROOT / name).read_text(encoding='utf8').splitlines() if re.match(r'^\| ZSQ-VALLEY-CRUSHK-\d+\s*\|', line)]
    assert len(rows) == 46 and [re.match(r'\| (ZSQ-VALLEY-CRUSHK-\d+)', row)[1] for row in rows] == ['ZSQ-VALLEY-CRUSHK-' + str(n).zfill(2) for n in range(1, 47)]
    tables.append(rows)
assert tables[0] == tables[1] == tables[2]
dossier = (ROOT / 'docs/design/zone-stories/VALLEY_CRUSHK.md').read_text(encoding='utf8')
progression = dossier.split('## Progression stories', 1)[1].split('## Implementation and validation boundary', 1)[0]
assert len([line for line in progression.splitlines() if line.startswith('| ') and not line.startswith(('| Story', '| ---'))]) == 10
for phrase in ('active READY accounting', 'Daily policy remains disabled', 'Exact supplied shiv remains eligible',
               'nine broader', 'All23 local O/G/E', 'independently confirmed coin payment',
               'Guidance alone does not add synthetic achievements', 'separate named fix/news commits',
               'surf and surface2011 are inactive', 'no exact207 prototype', 'no F/C tails'):
    assert phrase.lower() in dossier.lower(), phrase
print('Valley of Crushk: supplied exact foreign shiv/currency bounty, shared aliases, magic gates and useful-outcome boundaries retained.')
