#!/usr/bin/env python3
"""Protect the Barrow's exact exchange, real access and independent story outcomes."""
import collections
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
import zone_story_quest_catalog as cat
import zone_story_quest_zone_inventory as inv


def bodies(kind, area='elftomb'):
    text = (ROOT / 'areas' / kind / (area + '.' + kind)).read_text()
    return {int(m[1]): m[2] for m in re.finditer(r'^#(\d+)\s*\n([\s\S]*?)(?=^#\d+\s*$|\Z)', text, re.M)}


def numeric(body):
    return [list(map(int, line.split())) for line in body.split('~', 4)[4].strip().splitlines()[:3]]


e = inv.area_evidence(ROOT, 'elftomb')
catalog = cat.production_catalog(ROOT)
m = next(m for m in catalog['story_mappings'] if m['source_area'] == 'elftomb')
assert (m['schema_version'], m['revision'], m['coverage']) == (3, 1, 'complete')
assert len(m['stories']) == 1 and not m['exclusions'] and len(m['orientation']) == 10
all_native = inv.native_blocks(ROOT)
raw = [b for b in all_native if b['source'] == 'areas/qst/elftomb.qst' and b['kind'] == 'Q']
assert len(raw) == 1
q = raw[0]
assert (q['giver_vnum'], q['line'], q['give'], q['receive'], q['disappear']) == (20607, 2, [('I', 20619)], [('I', 20615)], False)
s = m['stories'][0]
assert (s['id'], s['category'], s['contracts']) == ('old-warrior-king-head', 'story', [q['binding']])
assert len(s['steps']) == 2
assert (s['steps'][0]['kind'], s['steps'][0]['item_vnums'], s['steps'][0]['count'], s['steps'][0]['optional']) == ('carried_item', [20619], 1, True)
assert s['steps'][1]['kind'] == 'completion' and s['steps'][1]['contracts'] == [q['binding']]
assert not e['dialogue'] and not e['special_assignments'] and not e['requests'][0]['definition']['prerequisites']
units = [u for u in cat.story_units(catalog) if u['zone_number'] == 206]
assert len(units) == 1 and units[0]['achievement'] and units[0]['daily_candidate']
rooms, mobs, objects = (bodies(k) for k in ('wld', 'mob', 'obj'))
assert (len(rooms), len(mobs), len(objects)) == (207, 15, 28)
assert set(rooms) == set(range(20600, 20807)) and set(mobs) == set(range(20600, 20615))
assert {c['mob_vnum'] for c in m['contacts']} == set(mobs)
assert all(not c['topics'] and re.fullmatch(r'[a-z0-9_-]{1,64}', c['keyword']) and c['keyword'] in e['mobs'][c['mob_vnum']]['keywords'] for c in m['contacts'])
assert all(not (int(b.split('~', 4)[4].strip().split()[0]) & (1 << 15)) for b in mobs.values())
assert (e['zone']['first_vnum'], e['zone']['last_vnum'], e['zone']['reset_mode']) == (20599, 20806, 2)
assert collections.Counter(r['command'] for r in e['reset_commands']) == {'D': 12, 'O': 17, 'P': 24, 'M': 88, 'G': 30, 'E': 7}
stock = [r for r in e['reset_commands'] if r['command'] in ('O', 'P', 'G', 'E')]
assert len(stock) == 78 and all(20600 <= r['arguments'][1] <= 20627 for r in stock)
assert len([r for r in stock if r['command'] == 'O' and r['arguments'][1] == 20617]) == 14
heads = [r for r in stock if r['command'] == 'P' and r['arguments'][1] == 20619]
assert len(heads) == 2 and all(r['arguments'][2:4] == [2, 20617] for r in heads)
assert len([r for r in stock if r['command'] == 'P' and r['arguments'][1] == 20618]) == 13
for v in (20618, 20619, 20620):
    assert numeric(objects[v])[0][0] == 13  # These named remains are not ITEM_CORPSE24.
assert numeric(objects[20617])[1][:3] == [200, 5, 0]
assert numeric(objects[20603])[1][:3] == [100, 1, 0]
assert numeric(objects[20605])[1][1:3] == [13, 20606]
assert all(numeric(objects[v])[1][1] == 0 for v in (20606, 20616))
assert numeric(objects[20600])[0][0] == 2 and numeric(objects[20600])[1][:4] == [50, 3, -1, -1]
assert numeric(objects[20601])[0][0] == 11 and numeric(objects[20601])[1] == [0] * 8
assert numeric(objects[20626])[0][0] == 10 and numeric(objects[20626])[1][:5] == [25, 65, -1, -1, 20]
assert '\nA\n14 30\n' in objects[20615] and '\nA\n1 10\n' in objects[20615]
graph = [(v, int(x[1]), int(x[4]), int(x[5]), int(x[6])) for v, body in rooms.items()
         for x in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)', body, re.S)]
assert len(graph) == 667
assert (20771, 0, 3, 20616, 20772) in graph and (20772, 2, 3, 20616, 20771) in graph
assert (20685, 3, 1, 0, 20686) in graph and 'east' in rooms[20685].lower()
assert not any(r == 20804 or target == 20804 for r, _, _, _, target in graph)
door_resets = {(r['arguments'][1], r['arguments'][2]): r['arguments'][3] for r in e['reset_commands'] if r['command'] == 'D'}
assert all(door_resets[(r, d)] == 5 for r, d in ((20602, 4), (20650, 2), (20802, 5)))
assert all(door_resets[(r, d)] == 2 for r, d in ((20771, 0), (20772, 2)))
assert sum(bool(re.search(r'^F\s*$', b, re.M)) for b in rooms.values()) == 3
assert sum(bool(re.search(r'^C\s*$', b, re.M)) for b in rooms.values()) == 4
assert not any(re.search(r'^E\s*$', b, re.M) for b in rooms.values())
monk = [b for b in all_native if b['source'] == 'areas/qst/caves_skelenak.qst' and b['kind'] == 'Q' and b['giver_vnum'] == 4038 and b['line'] == 172]
assert len(monk) == 1 and monk[0]['give'] == [('I', 20604)] and monk[0]['receive'] == [('C', 150000)] and not monk[0]['disappear']
assert monk[0]['binding'] not in s['contracts'] and not (ROOT / 'areas/shp/elftomb.shp').exists()

# Source boundaries protect the planned fixes without changing native behavior.
db = (ROOT / 'src/world/db.c').read_text()
handler = (ROOT / 'src/world/handler.c').read_text()
device = (ROOT / 'src/item/device_actions.c').read_text()
assert 'obj_to = get_obj_num(ZCMD.arg3)' in db
assert 'for (i = object_list; i; i = i->next)' in handler
assert 'if (is_scroll(kind) && !spell)' in device and 'if (!device_spell(spell))' in device
assert 'scroll->value[i] >= 1' in (ROOT / 'src/cmd/actoth.c').read_text()

paths = ['docs/design/zone-stories/BARROW_OF_THE_QUIOSHO.md', 'docs/guides/ZONE_STORY_BUILDING.md', 'docs/design/ZONE_STORY_ROADMAP_EXECUTION.md']
tables = []
for path in paths:
    text = (ROOT / path).read_text(encoding='utf8')
    rows = [l for l in text.splitlines() if l.startswith('| ZSQ-ELFTOMB-')]
    assert len(rows) == 40
    assert [re.search(r'ZSQ-ELFTOMB-(\d+)', l)[1] for l in rows] == [str(i).zfill(2) for i in range(1, 41)]
    tables.append(rows)
assert tables[0] == tables[1] == tables[2]
dossier = (ROOT / paths[0]).read_text(encoding='utf8')
assert len(re.findall(r'^\| (?!ID|Story|---|ZSQ-ELFTOMB-)[^|]+ \|', dossier, re.M)) == 10
for phrase in ('supplied','noD','first global','non-takeable','active READY','daily policy disabled','66percent','negative spell IDs','separately named','no installed'):
    if phrase == 'no installed':
        assert 'do not claim installed or played' in dossier
    else:
        assert phrase in dossier, phrase
assert 'Barrow of the Quiosho' in (ROOT / 'docs/design/ZONE_STORY_ZONE_PRIORITIES.md').read_text()
print('Barrow exact head exchange, source/access/effect boundaries and ten-story follow-up regression passed')
