#!/usr/bin/env python3
"""Protect exact librarian identity, control meanings and separate useful outcomes."""
import collections
from pathlib import Path
import re
import sys

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts'))
import zone_story_quest_catalog as cat
import zone_story_quest_zone_inventory as inv

def bodies(kind,area='evkeep'):
    text=(ROOT/'areas'/kind/(area+'.'+kind)).read_text()
    return {int(m[1]):m[2] for m in re.finditer(r'^#(\d+)\s*\n([\s\S]*?)(?=^#\d+\s*$|\Z)',text,re.M)}

def numeric(body):
    return [list(map(int,line.split())) for line in body.split('~',4)[4].strip().splitlines()[:3]]

e=inv.area_evidence(ROOT,'evkeep')
catalog=cat.production_catalog(ROOT)
m=next(m for m in catalog['story_mappings'] if m['source_area']=='evkeep')
assert (m['schema_version'],m['revision'],m['coverage'])==(3,1,'complete')
assert len(m['stories'])==1 and not m['exclusions'] and len(m['orientation'])==15
all_native=inv.native_blocks(ROOT)
raw=[b for b in all_native if b['source']=='areas/qst/evkeep.qst' and b['kind']=='Q']
assert len(raw)==1
q=raw[0]
assert (q['giver_vnum'],q['line'],q['give'],q['receive'],q['disappear'])==(44858,2,[('I',44880)],[('E',150000)],False)
s=m['stories'][0]
assert (s['id'],s['category'],s['contracts'])==('librarian-desk-key','story',[q['binding']])
assert len(s['steps'])==2
assert (s['steps'][0]['kind'],s['steps'][0]['item_vnums'],s['steps'][0]['count'],s['steps'][0]['optional'])==('carried_item',[44880],1,True)
assert s['steps'][1]['kind']=='completion' and s['steps'][1]['contracts']==[q['binding']]
assert not e['dialogue'] and not e['special_assignments'] and not e['requests'][0]['definition']['prerequisites']
units=[u for u in cat.story_units(catalog) if u['zone_number']==448]
assert len(units)==1 and units[0]['achievement'] and units[0]['daily_candidate']
rooms,mobs,objects=(bodies(k) for k in ('wld','mob','obj'))
assert (len(rooms),len(mobs),len(objects))==(147,59,85)
assert set(rooms)==set(range(44801,44948)) and set(mobs)==set(range(44801,44860))
assert {c['mob_vnum'] for c in m['contacts']}==set(mobs)
assert all(not c['topics'] and re.fullmatch(r'[a-z0-9_-]{1,64}',c['keyword']) and c['keyword'] in e['mobs'][c['mob_vnum']]['keywords'] for c in m['contacts'])
assert next(c for c in m['contacts'] if c['mob_vnum']==44858)['keyword']=='marcinr'
assert [v for v,b in mobs.items() if int(b.split('~',4)[4].strip().split()[0])&(1<<15)]==[44818]
assert (e['zone']['first_vnum'],e['zone']['last_vnum'],e['zone']['reset_mode'])==(44786,44947,0)
assert collections.Counter(r['command'] for r in e['reset_commands'])=={'D':62,'O':9,'P':3,'M':84,'G':15,'F':9,'E':90}
stock=[r for r in e['reset_commands'] if r['command'] in ('O','P','G','E')]
assert len(stock)==117
keys=[r for r in stock if r['command']=='G' and r['arguments'][1]==44880]
assert len(keys)==2 and all(r['arguments'][2]==2 for r in keys)
assert numeric(objects[44880])[0][0]==8 and numeric(objects[44880])[1]==[0]*8
assert numeric(objects[44801])[0][0]==11 and numeric(objects[44802])[0][0]==18
assert numeric(objects[44803])[1][1]==40
assert all(numeric(objects[v])[1][1]==100 for v in (44832,44833))
assert numeric(objects[44885])[0][0]==29 and numeric(objects[44885])[1][:3]==[270,44831,2]
assert numeric(objects[44830])[0][0]==25 and numeric(objects[44830])[1][:3]==[44892,320,600]
assert numeric(objects[44831])[0][0]==15 and numeric(objects[44831])[1][:3]==[900,29,44833]
assert '\nT\n512 0 1 100\n' in objects[44831]
assert numeric(objects[44879])[0][0]==10 and numeric(objects[44879])[1][:4]==[51,285,549,-1]
assert numeric(objects[44845])[0][0]==4 and numeric(objects[44845])[1][:4]==[50,9,9,165]
assert numeric(objects[44826])[0][0]==5 and numeric(objects[44824])[0][0]==13
graph=[(v,int(x[1]),int(x[4]),int(x[5]),int(x[6])) for v,body in rooms.items() for x in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',body,re.S)]
assert len(graph)==401
assert (44831,2,9,0,44890) in graph
assert not any(v==44947 for v,_,_,_,_ in graph)
door_resets={(r['arguments'][1],r['arguments'][2]):r['arguments'][3] for r in e['reset_commands'] if r['command']=='D'}
assert door_resets[(44831,2)]==9
assert door_resets[(44870,1)]==6 and door_resets[(44878,4)]==2
assert all(not re.search(r'^(E|F|C)\s*$',b,re.M) for b in rooms.values())
foreign=[b for b in all_native if b['source']=='areas/qst/wh.qst' and b['kind']=='Q' and b['giver_vnum']==55222 and b['line']==3155]
assert len(foreign)==1 and foreign[0]['give']==[('I',55186)] and set(foreign[0]['receive'])=={('I',55362),('C',1000000),('I',55033)} and not foreign[0]['disappear']
assert foreign[0]['binding'] not in s['contracts']
assert (ROOT/'areas/shp/evkeep.shp').exists()
switch=(ROOT/'src/specs/specs.object.c').read_text()
travel=(ROOT/'src/magic/spell_travel.c').read_text()
assert 'obj->type != ITEM_SWITCH || obj->value[0] != cmd' in switch
assert 'REMOVE_BIT(world[in_room].dir_option[door]->exit_info, EX_BLOCKED)' in switch
assert 'if ((obj->type != ITEM_TELEPORT) || (obj->value[1] != cmd))' in travel
assert 'if (!--obj->value[2])' in travel
assert '#define CMD_TOUCH 320' in (ROOT/'src/cmd/interp.h').read_text()
assert 'Paid skill practice is unavailable while active accounting is enabled.' in (ROOT/'src/guild/guild.c').read_text()
assert 'Shop trades and services are unavailable while economic accounting is active.' in (ROOT/'src/economy/shop.c').read_text()
paths=['docs/design/zone-stories/KEEP_OF_EVIL.md','docs/guides/ZONE_STORY_BUILDING.md','docs/design/ZONE_STORY_ROADMAP_EXECUTION.md']
tables=[]
for path in paths:
    text=(ROOT/path).read_text(encoding='utf8')
    rows=[l for l in text.splitlines() if l.startswith('| ZSQ-EVKEEP-')]
    assert len(rows)==45
    assert [re.search(r'ZSQ-EVKEEP-(\d+)',l)[1] for l in rows]==[str(i).zfill(2) for i in range(1,46)]
    tables.append(rows)
assert tables[0]==tables[1]==tables[2]
dossier=(ROOT/paths[0]).read_text(encoding='utf8')
assert len(re.findall(r'^\| (?!ID|Story|---|ZSQ-EVKEEP-)[^|]+ \|',dossier,re.M))==15
for phrase in ('supplied','noD','first global','active READY','daily policy disabled','600 charges','ROOM_LOCKER','Paid PRACTICE','separately named','without a duplicate'):
    assert phrase in dossier,phrase
print('Keep of Evil exact desk-key/XP, real control meanings and fifteen-story follow-up regression passed')
