#!/usr/bin/env python3
"""Protect Cinder's exact quantities and separate source, purchase and foreign return."""
import collections
from pathlib import Path
import re
import sys

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts'))
import zone_story_quest_catalog as cat
import zone_story_quest_zone_inventory as inv


def bodies(kind,area='lava'):
    text=(ROOT/'areas'/kind/(area+'.'+kind)).read_text()
    return {int(m[1]):m[2] for m in re.finditer(r'^#(\d+)\s*\n([\s\S]*?)(?=^#\d+\s*$|\Z)',text,re.M)}


def properties(body):
    return [list(map(int,x.split())) for x in body.split('~',4)[4].strip().splitlines()[:3]]


e=inv.area_evidence(ROOT,'lava')
catalog=cat.production_catalog(ROOT)
m=next(m for m in catalog['story_mappings'] if m['source_area']=='lava')
assert (m['schema_version'],m['revision'],m['coverage'])==(3,1,'complete')
assert len(m['stories'])==1 and not m['exclusions']
raw=[b for b in inv.native_blocks(ROOT) if b['source']=='areas/qst/lava.qst']
assert [(b['kind'],b['line']) for b in raw]==[('MA',2),('QA',14)]
topics=raw[0]['body'][0].split('~')[0].split()
assert topics==['hi','lava','rock','weapon','forge','need','want','armor']
q=raw[1]
assert q['give']==[('I',99701)]*5+[('I',99706)] and q['receive']==[('I',99700)] and not q['disappear']
s=m['stories'][0]
assert s['id']=='drembel-cinder-commission' and s['category']=='request' and s['contracts']==[q['binding']]
rocks,voucher,accepted=s['steps']
assert [(x['kind'],x['item_vnums'],x['count'],x['optional']) for x in (rocks,voucher)]==[
    ('carried_item',[99701],5,True),('carried_item',[99706],1,True)]
assert accepted['kind']=='completion' and accepted['contracts']==s['contracts']
definition=e['requests'][0]['definition']
assert not definition['prerequisites'] and definition['daily_eligible'] and definition['eligible_for_zone_completion']
units=[u for u in cat.story_units(catalog) if u['zone_number']==997]
assert len(units)==1 and units[0]['achievement'] and units[0]['daily_candidate']
rooms,mobs,objects=(bodies(k) for k in ('wld','mob','obj'))
assert (len(rooms),len(mobs),len(objects))==(65,20,7)
assert {c['mob_vnum'] for c in m['contacts']}==set(mobs)
assert next(c for c in m['contacts'] if c['mob_vnum']==99705)['topics']==topics
assert sum(len(c['topics']) for c in m['contacts'])==8
assert set(mobs)=={r['arguments'][1] for r in e['reset_commands'] if r['command'] in ('M','F')}
assert e['zone']['first_vnum']==99681 and e['zone']['reset_mode']==1
assert (ROOT/'areas/zon/lava.zon').read_text().splitlines()[2]=='99764 1 0 20 30 2'
assert collections.Counter(r['command'] for r in e['reset_commands'])=={'D':24,'O':1,'M':65,'E':3,'F':3,'G':13}
resets={r['line']:(r['command'],r['arguments'][:5]) for r in e['reset_commands']}
assert resets[127]==('M',[0,99705,1,99723,100])
assert resets[131]==('M',[0,99711,1,99725,100]) and resets[132]==('G',[1,99706,1,0,100])
for ml,gl,room in ((139,140,99731),(141,142,99732),(144,145,99734),(146,147,99735),(149,150,99738)):
    assert resets[ml]==('M',[0,99714,5,room,100]) and resets[gl]==('G',[1,99701,5,0,100])
for v in (99701,99706):
    p=properties(objects[v]);assert p[0][0]==8 and p[0][6:9]==[0,1,32768]
assert properties(objects[99706])[2][1]==500000
shop=(ROOT/'areas/shp/lava.shp').read_text()
assert re.findall(r'^#(\d+)~\s*$',shop,re.M)==['99711'] and shop.splitlines()[2:6]==['99706','0','0.80','1.00']
assert resets[178]==('M',[0,99713,1,99759,100]) and resets[180]==('G',[1,31318,1,0,100])
skull=properties(bodies('obj','dream')[31318])
assert skull[0][6:9]==[41971848,1,32832]  # SECRET/NODROP/NOSELL/FLOAT/NORESET/NORENT/HUM; MAGIC/QUESTITEM.
foreign=next(b for b in inv.native_blocks(ROOT) if b['source']=='areas/qst/dream.qst' and b['line']==46)
assert foreign['give']==[('I',v) for v in range(31316,31321)] and foreign['receive']==[('I',31315)]
assert foreign['binding'] not in s['contracts'] and all(foreign['binding'] not in x.get('contracts',[]) for x in s['steps'])
assert resets[105]==('O',[0,3097,1,99725,100])
assert properties(objects[99705])[1][:4]==[41,75,0,0]
graph=[(v,int(x[1]),int(x[4]),int(x[5]),int(x[6])) for v,body in rooms.items()
       for x in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',body,re.S)]
assert len(graph)==147 and all(key==0 for _,_,_,key,_ in graph)
assert {target for _,_,_,_,target in graph if target not in rooms}=={565027}
assert (99735,5,1,0,99740) in graph and (99740,4,1,0,99735) in graph
assert not any(re.search(r'^[EF](?:\s|$)',body,re.M) for body in rooms.values())
orientation=' '.join(m['orientation'])
for phrase in ('five exact lava rocks','incomplete set stays with you','room-wide response','supplied voucher','currently unavailable','five distinct skulls','cannot be dropped','aging spell','active READY accounting','Daily replay remains disabled'):
    assert phrase in orientation,phrase
tables=[]
for name in ('docs/design/zone-stories/LAVA_SPRINGS.md','docs/guides/ZONE_STORY_BUILDING.md','docs/design/ZONE_STORY_ROADMAP_EXECUTION.md'):
    rows=[x for x in (ROOT/name).read_text().splitlines() if x.startswith('| ZSQ-LAVA-')]
    assert len(rows)==40;tables.append(rows)
assert tables[0]==tables[1]==tables[2]
print('Lava Springs exact five-rock/voucher batch, supplied readiness, purchase and foreign-receipt boundaries passed')
