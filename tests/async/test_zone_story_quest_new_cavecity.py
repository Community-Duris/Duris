#!/usr/bin/env python3
"""Protect atomic material preparation, real teacher placement and separate outcomes."""
import collections
from pathlib import Path
import re
import sys

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts'))
import zone_story_quest_catalog as cat
import zone_story_quest_zone_inventory as inv

def bodies(kind,area='new_cavecity'):
 text=(ROOT/'areas'/kind/(area+'.'+kind)).read_text()
 return {int(m[1]):m[2] for m in re.finditer(r'^#(\d+)\s*\n([\s\S]*?)(?=^#\d+\s*$|\Z)',text,re.M)}

def numeric(body):
 return [list(map(int,line.split())) for line in body.split('~',4)[4].strip().splitlines()[:3]]

e=inv.area_evidence(ROOT,'new_cavecity')
catalog=cat.production_catalog(ROOT)
m=next(m for m in catalog['story_mappings'] if m['source_area']=='new_cavecity')
assert (m['schema_version'],m['revision'],m['coverage'])==(3,1,'complete')
assert len(m['stories'])==1 and not m['exclusions'] and len(m['orientation'])==15
all_native=inv.native_blocks(ROOT)
raw=[b for b in all_native if b['source']=='areas/qst/new_cavecity.qst' and b['kind']=='Q']
assert len(raw)==1
q=raw[0]
assert (q['giver_vnum'],q['line'],q['give'],q['receive'],q['disappear'])==(15120,16,[('I',402),('I',32490),('I',26614)],[('I',411)],False)
s=m['stories'][0]
assert (s['id'],s['category'],s['contracts'])==('wizard-intelligence-scroll','story',[q['binding']])
assert len(s['steps'])==4
assert [(r['kind'],r['item_vnums'],r['count'],r['optional']) for r in s['steps'][:3]]==[('carried_item',[402],1,True),('carried_item',[32490],1,True),('carried_item',[26614],1,True)]
assert s['steps'][3]['kind']=='completion' and s['steps'][3]['contracts']==[q['binding']]
assert not e['dialogue'] and not e['requests'][0]['definition']['prerequisites']
units=[u for u in cat.story_units(catalog) if u['zone_number']==151]
assert len(units)==1 and units[0]['achievement'] and units[0]['daily_candidate']
rooms,mobs,objects=(bodies(k) for k in ('wld','mob','obj'))
assert (len(rooms),len(mobs),len(objects))==(73,25,34)
assert {c['mob_vnum'] for c in m['contacts']}==set(mobs)
assert all(not c['topics'] and re.fullmatch(r'[a-z0-9_-]{1,64}',c['keyword']) and c['keyword'] in e['mobs'][c['mob_vnum']]['keywords'] for c in m['contacts'])
assert next(c for c in m['contacts'] if c['mob_vnum']==15120)['keyword']=='valhingen'
assert [v for v,b in mobs.items() if int(b.split('~',4)[4].strip().split()[0])&(1<<15)]==[15122]
assert (e['zone']['first_vnum'],e['zone']['last_vnum'],e['zone']['reset_mode'])==(15100,15186,0)
assert collections.Counter(r['command'] for r in e['reset_commands'])=={'D':24,'O':3,'P':1,'M':62,'G':9,'F':7,'E':23}
assert len([r for r in e['reset_commands'] if r['command'] in ('O','P','G','E')])==36
assert not any(r['command']=='M' and r['arguments'][1]==15120 for r in e['reset_commands'])
outside=(ROOT/'areas/zon/twin_towers_forest.zon').read_text()
assert re.search(r'^M 0 15120 1 15141 100 0 0 0\s+\*',outside,re.M)
assert len(re.findall(r'^G 1 15119 100 0 100 0 0 0\s+\*',outside,re.M))==4
assert numeric(objects[15142])[0][0]==22 and numeric(objects[15142])[1]==[0]*8
assert numeric(objects[15130])[0][0]==15 and numeric(objects[15135])[0][0]==12
assert numeric(objects[15122])[0][0]==15 and numeric(objects[15122])[2][0]==-30
assert all(numeric(objects[v])[1][1]==100 for v in (15100,15101,15102))
assert numeric(objects[15104])[1][1]==0
assert numeric(bodies('obj','heavens')[411])[0][0]==13 and numeric(bodies('obj','heavens')[411])[1]==[0]*8
graph=[(v,int(x[1]),int(x[4]),int(x[5]),int(x[6])) for v,body in rooms.items() for x in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',body,re.S)]
assert len(graph)==155 and (15116,0,2,15102,15121) in graph
assert (15138,4,5,-1,15147) in graph and (15161,1,0,0,584184) in graph
door_resets={(r['arguments'][1],r['arguments'][2]):r['arguments'][3] for r in e['reset_commands'] if r['command']=='D'}
assert door_resets[(15150,2)]==0 and door_resets[(15158,1)]==2
assert door_resets[(15133,2)]==1 and door_resets[(15138,4)]==1
assert '\nM\n0 30\n' in rooms[15115] and '\nM\n0 20\n' in rooms[15143]
assert all(not re.search(r'^(F|C)\s*$',b,re.M) for b in rooms.values())
foreign=[b for b in all_native if b['source']=='areas/qst/wh.qst' and b['kind']=='Q' and b['giver_vnum']==55218 and b['line']==3107]
assert len(foreign)==1 and foreign[0]['give']==[('I',55182)] and set(foreign[0]['receive'])=={('I',55362),('C',1000000),('I',55033)} and not foreign[0]['disappear']
assert foreign[0]['binding'] not in s['contracts']
wh=next(x for x in catalog['story_mappings'] if x['source_area']=='wh')
assert next(x for x in wh['stories'] if x['id']=='request-55218-3272a80db2dc')['contracts']==[foreign[0]['binding']]
epic=(ROOT/'src/classes/epic_skills.c').read_text()
assert '{ 15120, SKILL_EPIC_INTELLIGENCE, 0, 100, 0, 0, 0 }' in epic
assert 'Epic skill purchases are unavailable while economic accounting is active.' in epic
assert 'mob_index[real_mobile(epic_teachers[i].vnum)].func.mob = epic_teacher;' in (ROOT/'src/world/epic.c').read_text()
assert 'obj_index[real_object0(67272)].func.obj = dranum_mask;' in (ROOT/'src/specs/specs.assign.c').read_text()
assert not (ROOT/'areas/shp/new_cavecity.shp').exists()
paths=['docs/design/zone-stories/NEW_CAVE_CITY.md','docs/guides/ZONE_STORY_BUILDING.md','docs/design/ZONE_STORY_ROADMAP_EXECUTION.md']
tables=[]
for path in paths:
 text=(ROOT/path).read_text(encoding='utf8')
 rows=[l for l in text.splitlines() if l.startswith('| ZSQ-NEW-CAVECITY-')]
 assert len(rows)==45
 assert [re.search(r'ZSQ-NEW-CAVECITY-(\d+)',l)[1] for l in rows]==[str(i).zfill(2) for i in range(1,46)]
 tables.append(rows)
assert tables[0]==tables[1]==tables[2]
dossier=(ROOT/paths[0]).read_text(encoding='utf8')
assert len(re.findall(r'^\| (?!ID|Story|---|ZSQ-NEW-CAVECITY-)[^|]+ \|',dossier,re.M))==15
for phrase in ('Supplied','noD','first global','active READY','daily policy disabled','mode0','ACT_TEACHER','separately named','without a duplicate','three optional','TRASH13'):
 assert phrase in dossier,phrase
print('New Cave city exact atomic wizard exchange, real teacher placement and fifteen-story follow-up regression passed')
