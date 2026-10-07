#!/usr/bin/env python3
"""Bronze Citadel independent returns and source/access/hazard evidence contracts."""
import collections
from pathlib import Path
import re
import sys
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
import zone_story_quest_catalog as cat
import zone_story_quest_zone_inventory as inv

def bodies(kind, area='bctdl'):
    return {int(m[1]): m[2] for m in re.finditer(r'^#(\d+)\s*\n([\s\S]*?)(?=^#\d+\s*$|\Z)',
            (ROOT / 'areas' / kind / (area + '.' + kind)).read_text(), re.M)}

def properties(body):
    rows = body.split('~', 4)[4].strip().splitlines()
    return list(map(int, rows[0].split())), list(map(int, rows[1].split()))

e = inv.area_evidence(ROOT, 'bctdl')
catalog = cat.production_catalog(ROOT)
mapping = next(m for m in catalog['story_mappings'] if m['source_area'] == 'bctdl')
assert (mapping['schema_version'], mapping['revision'], mapping['coverage']) == (3, 1, 'complete')
assert len(mapping['stories']) == 2 and len(mapping['contacts']) == 35 and not mapping['exclusions']
assert len(e['requests']) == 2 and len(e['dialogue']) == 1
raw = [b for b in inv.native_blocks(ROOT) if b['source'] == 'areas/qst/bctdl.qst']
assert [(b['kind'], b['line']) for b in raw] == [('MA', 2), ('QA', 11), ('QA', 23)]
assert raw[0]['body'][0] == 'hello~'
for s, q, item, reward, departure in zip(mapping['stories'], e['requests'], (32483, 32490), (32422, 32029), (False, True)):
    b = q['block']
    assert (b['giver_vnum'], b['give'], b['receive'], b['disappear']) == (32448, [('I', item)], [('I', reward)], departure)
    assert not q['definition']['prerequisites']
    assert s['contracts'] == [b['binding']] and len(s['steps']) == 2
    material, completion = s['steps']
    assert (material['kind'], material['item_vnums'], material['count'], material['optional']) == ('carried_item', [item], 1, True)
    assert completion['kind'] == 'completion' and completion['contracts'] == s['contracts']
rooms, mobs, objects = (bodies(kind) for kind in ('wld', 'mob', 'obj'))
assert (len(rooms), len(mobs), len(objects)) == (77, 35, 71)
assert min(rooms) == 32420 and max(rooms) == 32501 and len(rooms) < 82
assert {c['mob_vnum'] for c in mapping['contacts']} == set(mobs)
for c in mapping['contacts']:
    assert c['keyword'] in mobs[c['mob_vnum']].split('~')[0].split()
giver = next(c for c in mapping['contacts'] if c['mob_vnum'] == 32448)
assert giver['topics'] == ['hello'] and sum(len(c['topics']) for c in mapping['contacts']) == 1
assert (e['zone']['zone_number'], e['zone']['last_vnum'], e['zone']['reset_mode']) == (324, 32501, 0)
assert len(e['reset_commands']) == 302
assert collections.Counter(r['command'] for r in e['reset_commands']) == {'D': 32, 'O': 2, 'M': 86, 'E': 123, 'G': 46, 'F': 13}
resets = {r['line']: (r['command'], r['arguments'][:5]) for r in e['reset_commands']}
for line, command, args in [(32, 'D', [0, 32471, 4, 8, 100]), (96, 'D', [0, 32482, 0, 6, 100]),
                          (137, 'O', [0, 32436, 1, 32420, 100]), (138, 'O', [0, 32421, 1, 32480, 100]),
                          (281, 'M', [0, 32420, 1, 32469, 100]), (284, 'G', [1, 32490, 1, 0, 100]),
                          (348, 'M', [0, 32439, 1, 32482, 100]), (350, 'G', [1, 32482, 1, 0, 100]),
                          (351, 'G', [1, 32483, 1, 0, 100]), (353, 'M', [0, 32448, 1, 32483, 100])]:
    assert resets[line] == (command, args)
stocked = {r['arguments'][1] for r in e['reset_commands'] if r['command'] in ('M', 'F')}
assert set(mobs) - stocked == {32429, 32430}
producers = []
for path in (ROOT / 'areas/zon').glob('*.zon'):
    for line in path.read_text().splitlines():
        if re.match(r'[OPGE]\s+-?\d+\s+(?:32483|32490)\s', line):
            producers.append((path.stem, line.split()[:6]))
assert sorted(producers) == [('bctdl', ['G', '1', str(v), '1', '0', '100']) for v in (32483, 32490)]
for v in (32483, 32490):
    flags, values = properties(objects[v])
    assert flags[7] & 1 and flags[8] & 32768 and not flags[6] & (4096 | 8388608)
assert properties(objects[32422])[0][6] & 8388608
assert properties(objects[32421])[0][0] == 29 and properties(objects[32421])[0][7] == 0
assert properties(objects[32421])[0][6] == 2 and properties(objects[32421])[1][:4] == [42, 32471, 4, 0]
defines = (ROOT / 'src/core/defines.h').read_text()
assert '#define ITEM_NOSHOW BIT_2' in defines and '#define CMD_STAND 42' in (ROOT / 'src/cmd/interp.h').read_text()
handler = (ROOT / 'src/world/handler.c').read_text()
lookup = handler[handler.index('P_obj get_obj_in_list_vis('):handler.index('P_obj get_obj_vis(')]
assert '(IS_NOSHOW(i) && OBJ_VNUM(i) != VNUM_TRACKS)' in lookup
switch = (ROOT / 'src/specs/specs.object.c').read_text()
switch = switch[switch.index('int item_switch('):switch.index('int item_switch(') + 6500]
assert 'FIND_NO_TRACKS' in switch and 'obj->value[0] != cmd' in switch
assert 'only place' in rooms[32480] and 'to stand' in rooms[32480]

assert 'T\n6 2 1 50' in objects[32490] and 'T\n4 5 1 60' in objects[32421]
assert properties(objects[32436])[0][0] == 25 and properties(objects[32436])[1][:3] == [32057, 7, -1]
foreign = bodies('obj', 'avernus')
assert properties(foreign[32013])[1][:3] == [32420, 7, -1]
assert properties(foreign[32029])[0][0] == 9 and 'platemail' in foreign[32029]
assert 'potions' in '\n'.join(e['requests'][1]['block']['body'])
graph = [(v, int(m[1]), int(m[4]), int(m[5]), int(m[6])) for v, b in rooms.items()
         for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)', b, re.S)]
assert len(graph) == 160 and all(target in rooms for v, d, f, k, target in graph)
assert (32471, 4, 8, 0, 32481) in graph
assert (32471, 1, 3, 32422, 32472) in graph and (32472, 3, 3, 32422, 32471) in graph
assert int(rooms[32498].split('~', 2)[2].strip().splitlines()[0].split()[2]) == 25
assert not (ROOT / 'areas/shp/bctdl.shp').exists()
assert {(a['kind'], a['vnum'], a['function']) for a in e['special_assignments']} == {('obj', 32428, 'artifact_invisible'), ('obj', 32486, 'bel_sword')}
assert e['requests'][0]['definition']['daily_eligible'] and not e['requests'][1]['definition']['daily_eligible']
assert cat.report_for(catalog)['eligible_by_zone']['324'] == 2
tables = []
for name in ('docs/design/zone-stories/BCTDL.md', 'docs/guides/ZONE_STORY_BUILDING.md', 'docs/design/ZONE_STORY_ROADMAP_EXECUTION.md'):
    rows = [line for line in (ROOT / name).read_text().splitlines() if line.startswith('| ZSQ-BCTDL-')]
    assert len(rows) == 32
    tables.append(rows)
assert tables[0] == tables[1] == tables[2]
print('Bronze Citadel independent returns, exact sources, hazard interruption, keys/cube/access and fair reward repair contracts passed')
