#!/usr/bin/env python3
"""Protect consequence, custody, actual access and independent foreign ownership."""
import collections
from pathlib import Path
import re
import sys
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts'))
import zone_story_quest_catalog as cat
import zone_story_quest_zone_inventory as inv
def bodies(kind,area='tikit'):
 text=(ROOT/'areas'/kind/(area+'.'+kind)).read_text()
 return {int(m[1]):m[2] for m in re.finditer(r'^#(\d+)\s*\n([\s\S]*?)(?=^#\d+\s*$|\Z)',text,re.M)}
def numeric(body):return [list(map(int,l.split())) for l in body.split('~',4)[4].strip().splitlines()[:3]]
e=inv.area_evidence(ROOT,'tikit');catalog=cat.production_catalog(ROOT)
m=next(m for m in catalog['story_mappings'] if m['source_area']=='tikit')
assert (m['schema_version'],m['revision'],m['coverage'])==(3,1,'complete')
assert len(m['stories'])==1 and not m['exclusions'] and len(m['orientation'])==15
native=inv.native_blocks(ROOT)
q=next(b for b in native if b['source']=='areas/qst/tikit.qst' and b['kind']=='Q')
assert (q['giver_vnum'],q['line'],q['give'],q['receive'],q['disappear'])==(43763,2,[('I',43742)],[('I',43743)],False)
assert 'tosses the scared little kitty-cat' in '\n'.join(q['body'])
s=m['stories'][0]
assert (s['id'],s['category'],s['contracts'])==('sacrifice-for-temple-key','story',[q['binding']])
assert len(s['steps'])==2
assert (s['steps'][0]['kind'],s['steps'][0]['item_vnums'],s['steps'][0]['count'],s['steps'][0]['optional'])==('carried_item',[43742],1,True)
assert s['steps'][1]['kind']=='completion' and s['steps'][1]['contracts']==[q['binding']]
assert all('sacrific' in text.lower() for text in (s['title'],s['summary'],s['steps'][0]['hint'],s['steps'][1]['hint']))
assert not e['dialogue'] and not e['special_assignments'] and not e['requests'][0]['definition']['prerequisites']
units=[u for u in cat.story_units(catalog) if u['zone_number']==437]
assert len(units)==1 and units[0]['achievement'] and units[0]['daily_candidate']
rooms,mobs,objects=(bodies(k) for k in ('wld','mob','obj'))
assert (len(rooms),len(mobs),len(objects))==(340,71,55)
assert set(rooms)==set(range(43700,44040))
assert {c['mob_vnum'] for c in m['contacts']}==set(mobs)
assert all(not c['topics'] and re.fullmatch(r'[a-z0-9_-]{1,64}',c['keyword']) and c['keyword'] in e['mobs'][c['mob_vnum']]['keywords'] for c in m['contacts'])
assert next(c for c in m['contacts'] if c['mob_vnum']==43763)['keyword']=='sacrificial'
assert next(c for c in m['contacts'] if c['mob_vnum']==43750)['keyword']=='kahtkihlr'
assert (e['zone']['first_vnum'],e['zone']['last_vnum'],e['zone']['reset_mode'])==(43698,44039,2)
assert collections.Counter(r['command'] for r in e['reset_commands'])=={'D':30,'O':6,'P':2,'M':232,'G':46,'E':35,'F':38}
assert len([r for r in e['reset_commands'] if r['command'] in ('O','P','G','E')])==89
assert len([r for r in e['reset_commands'] if r['command']=='G' and r['arguments'][1]==43742 and r['arguments'][2]==999])==2
assert numeric(objects[43742])[0][0]==13 and numeric(objects[43743])[0][0]==18
assert e['mobs'][43742]['keywords']==['faerie'] and 'scared kitty cat' in objects[43742]
assert numeric(objects[43743])[1]==[0]*8
assert numeric(objects[43727])[0][0]==15 and numeric(objects[43727])[1][:3]==[500,29,43726]
assert numeric(objects[43729])[0][0]==9 and numeric(objects[43740])[0][0]==13
assert 'cant be used for currency anywhere' in objects[43740]
graph=[(v,int(x[1]),int(x[4]),int(x[5]),int(x[6])) for v,b in rooms.items() for x in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)]
assert len(graph)==855
assert (44038,3,3,43743,44039) in graph and (44039,5,0,0,44101) in graph
assert (43901,3,0,0,561015) in graph and (43703,2,7,43754,43704) in graph
doors={(r['arguments'][1],r['arguments'][2]):r['arguments'][3] for r in e['reset_commands'] if r['command']=='D'}
assert doors[(43703,2)]==6 and doors[(43704,0)]==2
assert doors[(43903,0)]==5 and doors[(44038,3)]==2 and doors[(44039,1)]==2
assert all(not re.search(r'^(E|F|C|M)\s*$',b,re.M) for b in rooms.values())
array=numeric(bodies('obj','tikitt')[44190])
assert array[0][0]==25 and array[1][:3]==[43704,7,-1]
assert re.search(r'^O 0 44190 1 44333 100 0 0 0\s+\*',(ROOT/'areas/zon/tikitt.zon').read_text(),re.M)
foreign=[b for b in native if b['source']=='areas/qst/tikitt.qst' and b['kind']=='Q' and b['line'] in (227,234,241,251,258,271)]
assert len(foreign)==6 and all(b['binding'] not in s['contracts'] for b in foreign)
temple=next(x for x in catalog['story_mappings'] if x['source_area']=='tikitt')
assert all(any(b['binding'] in story['contracts'] for story in temple['stories']) for b in foreign)
assert {(k,v) for b in foreign for k,v in b['give'] if k=='I' and v in (43703,43752,43753)}=={('I',43703),('I',43752),('I',43753)}
shops=(ROOT/'areas/shp/tikit.shp').read_text()
assert set(map(int,re.findall(r'^#(\d+)~',shops,re.M)))=={43700,43703,43704,43705,43706,43722,43732,43750}
paths=['docs/design/zone-stories/LOST_CITY_OF_TIKITZOPL.md','docs/guides/ZONE_STORY_BUILDING.md','docs/design/ZONE_STORY_ROADMAP_EXECUTION.md']
tables=[]
for path in paths:
 text=(ROOT/path).read_text(encoding='utf8');rows=[l for l in text.splitlines() if l.startswith('| ZSQ-TIKIT-')]
 assert len(rows)==45
 assert [re.search(r'ZSQ-TIKIT-(\d+)',l)[1] for l in rows]==[str(i).zfill(2) for i in range(1,46)]
 tables.append(rows)
assert tables[0]==tables[1]==tables[2]
dossier=(ROOT/paths[0]).read_text(encoding='utf8')
assert len(re.findall(r'^\| (?!ID|Story|---|ZSQ-TIKIT-)[^|]+ \|',dossier,re.M))==15
for phrase in ('Supplied','noD','first global','active READY','daily policy disabled','mode2','separately named','without a duplicate','sacrifice','TRASH13','ENTER7'):
 assert phrase in dossier,phrase
print('Tikitzopl native consequence,exact custody,real access and fifteen-story ownership regression passed')
