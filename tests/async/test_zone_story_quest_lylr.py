#!/usr/bin/env python3
"""Protect Venmar's exact exchange and independent access, captive and service outcomes."""
import collections
from pathlib import Path
import re
import sys

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts'))
import zone_story_quest_catalog as cat
import zone_story_quest_zone_inventory as inv


def bodies(kind):
    text=(ROOT/'areas'/kind/('lylr.'+kind)).read_text()
    return {int(m[1]):m[2] for m in re.finditer(r'^#(\d+)\s*\n([\s\S]*?)(?=^#\d+\s*$|\Z)',text,re.M)}


def properties(body):
    return [list(map(int,x.split())) for x in body.split('~',4)[4].strip().splitlines()[:3]]


e=inv.area_evidence(ROOT,'lylr')
catalog=cat.production_catalog(ROOT)
m=next(m for m in catalog['story_mappings'] if m['source_area']=='lylr')
assert (m['schema_version'],m['revision'],m['coverage'])==(3,1,'complete')
assert len(m['stories'])==1 and not m['exclusions'] and len(m['orientation'])<=16
raw=[b for b in inv.native_blocks(ROOT) if b['source']=='areas/qst/lylr.qst']
assert [(b['kind'],b['line']) for b in raw]==[('M',2),('Q',11)]
topics=raw[0]['body'][0].split('~')[0].split();assert topics==['evil','ogre']
q=raw[1]
assert q['give']==[('I',82217)] and q['receive']==[('I',82200)] and q['disappear']
s=m['stories'][0]
assert s['id']=='venmar-ogre-scalp' and s['category']=='request' and s['contracts']==[q['binding']]
material,accepted=s['steps']
assert (material['kind'],material['item_vnums'],material['count'],material['optional'])==('carried_item',[82217],1,True)
assert accepted['kind']=='completion' and accepted['contracts']==s['contracts']
definition=e['requests'][0]['definition']
assert not definition['prerequisites'] and definition['daily_eligible'] and definition['eligible_for_zone_completion']
units=[u for u in cat.story_units(catalog) if u['zone_number']==822]
assert len(units)==1 and units[0]['achievement'] and units[0]['daily_candidate']
rooms,mobs,objects=(bodies(k) for k in ('wld','mob','obj'))
assert (len(rooms),len(mobs),len(objects))==(64,36,31)
assert {c['mob_vnum'] for c in m['contacts']}==set(mobs)
assert next(c for c in m['contacts'] if c['mob_vnum']==82201)['topics']==topics
assert sum(len(c['topics']) for c in m['contacts'])==2
assert set(mobs)=={r['arguments'][1] for r in e['reset_commands'] if r['command'] in ('M','F')}
assert e['zone']['first_vnum']==82140 and e['zone']['reset_mode']==2
assert (ROOT/'areas/zon/lylr.zon').read_text().splitlines()[2]=='82263 2 0 40 50 1'
assert collections.Counter(r['command'] for r in e['reset_commands'])=={'D':18,'O':4,'P':2,'M':72,'E':25,'G':21,'F':2}
resets={r['line']:(r['command'],r['arguments'][:5]) for r in e['reset_commands']}
assert resets[93]==('M',[0,82201,4,82203,100])
assert resets[98]==('M',[0,82202,1,82211,100]) and resets[100]==('G',[1,82217,1,0,100])
assert resets[99]==('E',[1,82201,1,16,20])
assert properties(objects[82217])[0][6:9]==[8433672,1,32768]  # SECRET/NOSELL/FLOAT/NOLOCATE/NORENT; TAKE/QUESTITEM.
assert properties(objects[82223])[0][6:9]==[8388608,1,0] and properties(objects[82223])[1][1]==0
assert resets[148]==('M',[0,82223,1,82234,100]) and resets[150]==('G',[1,82223,1,0,100])
assert properties(objects[82215])[1][:4]==[500,15,0,500] and re.search(r'^T\s+512 15 3 50$',objects[82215],re.M)
assert properties(objects[82216])[1][0]==9999 and properties(objects[82221])[1][:4]==[40,1,1,23]
assert properties(objects[82212])[1][:4]==[30,1,65,141]
graph=[(v,int(x[1]),int(x[4]),int(x[5]),int(x[6])) for v,body in rooms.items()
       for x in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',body,re.S)]
assert len(graph)==134 and sum(key==82223 for _,_,_,key,_ in graph)==9
assert (82232,2,2,82223,82238) in graph and (82238,0,2,0,82232) in graph
assert {target for _,_,_,_,target in graph if target not in rooms}=={539281,540876}
assert re.search(r'^F\s*\n20$',rooms[82210],re.M) and (82210,5,0,0,82209) in graph
unvisited=set(rooms);components=[]
while unvisited:
    reached={min(unvisited)};pending=list(reached)
    while pending:
        room=pending.pop()
        for source,_,_,_,target in graph:
            neighbor=target if source==room else source if target==room else None
            if neighbor in rooms and neighbor not in reached:
                reached.add(neighbor);pending.append(neighbor)
    components.append(reached);unvisited-=reached
assert components==[set(range(82200,82240)),set(range(82240,82264))]
shop=(ROOT/'areas/shp/lylr.shp').read_text()
assert re.findall(r'^#(\d+)~\s*$',shop,re.M)==['82204','82208','82224'] and '#82229~' not in shop
orientation=' '.join(m['orientation'])
for phrase in ('two aliases share one response','exact supplied scalp','hidden','cannot be sold','current loose inventory',
               'lack reciprocal entry exits','generated world quests','no local drink shop','rent handler',
               'serious vigor','earthquake','active READY accounting','Daily replay remains disabled'):
    assert phrase in orientation,phrase
tables=[]
for name in ('docs/design/zone-stories/LYLR_MEOP.md','docs/guides/ZONE_STORY_BUILDING.md','docs/design/ZONE_STORY_ROADMAP_EXECUTION.md'):
    rows=[x for x in (ROOT/name).read_text().splitlines() if x.startswith('| ZSQ-LYLR-')]
    assert len(rows)==40;tables.append(rows)
assert tables[0]==tables[1]==tables[2]
print('Lylr-Meop exact scalp, supplied custody, retirement, access, captive and independent service boundaries passed')
