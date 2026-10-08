#!/usr/bin/env python3
"""Arachdrathos paid-service policy, key access and journal receipt boundaries."""
import collections
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
import zone_story_quest_catalog as cat
import zone_story_quest_zone_inventory as inv

def bodies(kind):
    return {int(m[1]): m[2] for m in re.finditer(r'^#(\d+)\s*\n([\s\S]*?)(?=^#\d+\s*$|\Z)',
        (ROOT / 'areas' / kind / ('arac-web.' + kind)).read_text(), re.M)}

def properties(body):
    return [list(map(int, x.split())) for x in body.split('~', 4)[4].strip().splitlines()[:3]]

e = inv.area_evidence(ROOT, 'arac-web')
catalog = cat.production_catalog(ROOT)
mapping = next(m for m in catalog['story_mappings'] if m['source_area'] == 'arac-web')
assert (mapping['schema_version'], mapping['revision'], mapping['coverage']) == (3, 2, 'complete')
assert len(mapping['stories']) == 1 and not mapping['exclusions'] and len(mapping['contacts']) == 44
raw = [b for b in inv.native_blocks(ROOT) if b['source'] == 'areas/qst/arac-web.qst']
assert [(b['kind'], b['line']) for b in raw] == [('M', 2), ('Q', 6)]
assert raw[0]['body'][0] == 'hi hello door~'
q = raw[1]
assert q['give'] == [('C', 10000)] and q['receive'] == [('I', 36400)] and q['disappear']
story = mapping['stories'][0]
assert story['id'] == 'gatekeeper-private-key' and story['category'] == 'service'
assert story['contracts'] == [q['binding']] and len(story['steps']) == 1
assert story['steps'][0]['kind'] == 'completion' and story['steps'][0]['contracts'] == story['contracts']
definition = e['requests'][0]['definition']
assert definition['eligible_for_zone_completion'] and definition['repeatable']
assert not definition['daily_eligible'] and definition['daily_exclusion'] == 'No repeatable item offering'
units = [u for u in cat.story_units(catalog) if u['zone_number'] == 364]
assert len(units) == 1 and not units[0]['achievement'] and not units[0]['daily_candidate']
rooms, mobs, objects = (bodies(k) for k in ('wld', 'mob', 'obj'))
assert (len(rooms), len(mobs), len(objects)) == (190, 44, 44)
assert (min(rooms), max(rooms)) == (36400, 36589)
assert {c['mob_vnum'] for c in mapping['contacts']} == set(mobs)
for contact in mapping['contacts']:
    assert contact['keyword'] in mobs[contact['mob_vnum']].split('~')[0].split()
gatekeeper = next(c for c in mapping['contacts'] if c['mob_vnum'] == 36423)
assert gatekeeper['topics'] == ['hi', 'hello', 'door']
assert sum(len(c['topics']) for c in mapping['contacts']) == 3
assert collections.Counter(r['command'] for r in e['reset_commands']) == {'D': 6, 'O': 7, 'M': 126, 'E': 3, 'G': 46, 'F': 3}
resets = {r['line']: (r['command'], r['arguments'][:5]) for r in e['reset_commands']}
assert resets[190] == ('M', [0, 36423, 1, 36570, 100])
assert resets[191] == ('G', [1, 36400, 1, 0, 100])
assert not any(r['command'] in ('O', 'P', 'G', 'E') and r['arguments'][1] == 36401 for r in e['reset_commands'])
assert properties(objects[36400])[0][0] == 18
assert properties(objects[36400])[0][6:8] == [8388608, 16385]
assert properties(objects[36401])[0][6:8] == [8912896, 16385]
assert properties(objects[36401])[1][1] == 50
for v in range(36409, 36415):
    assert properties(objects[v])[0][6:8] == [524288, 2097153]
    assert not any(properties(objects[v])[1])
graph = [(v, int(m[1]), int(m[4]), int(m[5]), int(m[6])) for v, b in rooms.items()
    for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)', b, re.S)]
assert len(graph) == 457 and len([x for x in graph if x[-1] not in rooms]) == 11
for edge in [(36570, 2, 3, 36400, 36569), (36569, 0, 3, 36400, 36570),
    (36567, 1, 0, 0, 36570), (36588, 3, 0, 0, 36197)]:
    assert edge in graph
assert 'Pleasure Room' in rooms[36569] and 'Slaver' in rooms[36570]
assert not any(re.search(r'^F\s*$', b, re.M) for b in rooms.values())
doors = {(r['arguments'][1], r['arguments'][2]): r['arguments'][3] for r in e['reset_commands'] if r['command'] == 'D'}
assert doors == {(36545, 2): 0, (36546, 0): 0, (36569, 0): 2, (36570, 2): 2, (36572, 2): 0, (36573, 0): 0}
quest = (ROOT / 'src/world/quest.c').read_text()
give = quest[quest.index('const bool giving_coins = isdigit(*temparg);'):quest.index('int quest_sort_comp')]
assert give.index('economic_gameplay_authority::active()') < give.index('do_give(pl, arg, -4)')
assert 'This quest cannot accept offerings right now.' in give
pet = (ROOT / 'src/specs/specs.room.c').read_text()
pet = pet[pet.index('int pet_shops('):pet.index('int pray_for_items(')]
assert 'pet_room = ch->in_room + 1' in pet and 'Pet purchases are unavailable' in pet
assert pet.index('economic_gameplay_authority::active()') < pet.index('SUB_MONEY')
world_quest = (ROOT / 'src/world/world_quest.c').read_text()
assert 'world_quest_reward_source_id' in world_quest and 'context.source_id != world_quest_reward_source_id(ch)' in world_quest
assert 'quest_started = 0' not in world_quest[world_quest.index('void resetQuest('):world_quest.index('static int world_quest_share_limit')]
payment = (ROOT / 'src/specs/specs.world_quest.c').read_text()
context = payment[payment.index('struct world_quest_payment_context'):payment.index('static_assert')]
assert 'giver_vnum' in context and 'quest_started' not in context
guidance = ' '.join(mapping['orientation'] + [c['description'] for c in mapping['contacts']] + [story['summary'], story['steps'][0]['hint']])
assert all(word in guidance.lower() for word in ('accounting', 'unavailable', 'supplied', 'north', 'south', 'norent', 'retire', 'daily', 'allegiance', 'receipt'))
tables = []
for name in ('docs/design/zone-stories/ARAC-WEB.md', 'docs/guides/ZONE_STORY_BUILDING.md', 'docs/design/ZONE_STORY_ROADMAP_EXECUTION.md'):
    rows = [line for line in (ROOT / name).read_text().splitlines() if line.startswith('| ZSQ-ARAC-WEB-')]
    assert len(rows) == 37
    tables.append(rows)
assert tables[0] == tables[1] == tables[2]
print('Arachdrathos paid service policy, key custody/access, native accounting refusal and deeper journal boundaries passed')
