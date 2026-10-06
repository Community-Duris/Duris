#!/usr/bin/env python3

import copy
import importlib.util
import json
import pathlib
import subprocess
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("catalog_tool", ROOT / "scripts/zone_story_quest_catalog.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
catalog = module.production_catalog(ROOT)
mapping = next(m for m in catalog["story_mappings"] if m["source_area"] == "twin_towers_forest")
report = module.report_for(catalog)
assert report["valid"] and report["eligible_by_zone"]["135"] == 10
assert report["daily_unit_count"] == 1408
assert report['mapped_area_count'] == 162 and report['eligible_by_zone']['162'] == 4
depths = next(m for m in catalog['story_mappings'] if m['source_area'] == 'surfacekeeps')
assert (depths['schema_version'], depths['revision'], depths['coverage']) == (3, 1, 'complete')
assert len(depths['stories']) == 12 and len(depths['contacts']) == 17 and not depths['exclusions']
assert report['eligible_by_zone']['1200'] == 9
assert sum(s['category'] == 'service' for s in depths['stories']) == 3
assert sum(len(c['topics']) for c in depths['contacts']) == 109
assert sum(len(s['contracts']) for s in depths['stories']) == 15
assert sum(t.get('optional', False) for s in depths['stories'] for t in s['steps']) == 15
depths_stories = {s['id']: s for s in depths['stories']}
assert len(depths_stories['ungalen-ale']['contracts']) == 4
assert depths_stories['ungalen-ale']['steps'][0]['item_vnums'] == [83118, 83172, 83428, 83417]
assert depths_stories['gulranor-ten-body-parts']['steps'][0]['count'] == 10
assert depths_stories['gulranor-ten-body-parts']['steps'][0]['item_vnums'] == [8]
assert depths_stories['mystardala-paired-trophies']['steps'][0]['contracts'] == depths_stories['mystardala-crystal-ball']['contracts']
assert depths_stories['mystardala-paired-trophies']['steps'][0]['optional']
assert all(t.get('optional') for s in depths['stories'] for t in s['steps'][:-1])
assert all(s['steps'][-1]['contracts'] == s['contracts'] for s in depths['stories'])

icecrag = next(m for m in catalog['story_mappings'] if m['source_area'] == 'icecrag')
assert (icecrag['schema_version'], icecrag['revision'], icecrag['coverage']) == (3, 1, 'complete')
assert len(icecrag['stories']) == 10 and len(icecrag['contacts']) == 25 and len(icecrag['exclusions']) == 1
assert report['eligible_by_zone']['970'] == 8
ice_units = [u for u in module.story_units(catalog) if u['zone_number'] == 970]
assert len(ice_units) == 10 and sum(u['daily_candidate'] for u in ice_units) == 7
assert sum(s['category'] == 'service' for s in icecrag['stories']) == 2
assert sum(len(c['topics']) for c in icecrag['contacts']) == 129
assert sum(t.get('optional', False) for s in icecrag['stories'] for t in s['steps']) == 16
assert all(t.get('optional') for s in icecrag['stories'] for t in s['steps'][:-1])

cloister = next(m for m in catalog['story_mappings'] if m['source_area'] == 'cloister')
assert (cloister['schema_version'], cloister['revision'], cloister['coverage']) == (3, 1, 'complete')
assert len(cloister['stories']) == 8 and len(cloister['contacts']) == 16 and not cloister['exclusions']
assert report['eligible_by_zone']['671'] == 7
cloister_units = [u for u in module.story_units(catalog) if u['zone_number'] == 671]
assert len(cloister_units) == 8 and sum(u['daily_candidate'] for u in cloister_units) == 7
assert sum(s['category'] == 'service' for s in cloister['stories']) == 1
assert sum(len(c['topics']) for c in cloister['contacts']) == 28
assert sum(t.get('optional', False) for s in cloister['stories'] for t in s['steps']) == 11
assert all(t.get('optional') for s in cloister['stories'] for t in s['steps'][:-1])
cloister_stories = {s['id']: s for s in cloister['stories']}
assert cloister_stories['tel-rejected-recommendation']['category'] == 'service'
assert cloister_stories['disciple-recommendation']['steps'][0]['contracts'] == cloister_stories['mahr-intruder-tablet']['contracts']
assert cloister_stories['advisor-ring-and-poison']['steps'][0]['contracts'] == cloister_stories['priest-troggahns-egg']['contracts']
assert [t['item_vnums'] for t in cloister_stories['advisor-ring-and-poison']['steps'][1:-1]] == [[67113], [67103]]

willem = next(m for m in catalog['story_mappings'] if m['source_area'] == 'willem')
assert (willem['schema_version'], willem['revision'], willem['coverage']) == (3, 1, 'complete')
assert len(willem['stories']) == 6 and len(willem['contacts']) == 26 and not willem['exclusions']
assert report['eligible_by_zone']['71'] == 6
assert all(s['category'] == 'story' for s in willem['stories'])
assert sum(len(c['topics']) for c in willem['contacts']) == 39
assert sum(t.get('optional', False) for s in willem['stories'] for t in s['steps']) == 12
willem_stories = {s['id']: s for s in willem['stories']}
assert [t['item_vnums'] for t in willem_stories['lothrell-five-badges']['steps'][:-1]] == [[7124], [7116], [7113], [7107], [7110]]
assert all(t['count'] == 1 for t in willem_stories['lothrell-five-badges']['steps'][:-1])
assert willem_stories['kurtukr-bloodsaber-upgrade']['steps'][0]['contracts'] == willem_stories['lothrell-five-badges']['contracts']
assert [t['item_vnums'] for t in willem_stories['kurtukr-bloodsaber-upgrade']['steps'][1:-1]] == [[7131], [7139]]
assert all(t.get('optional') for s in willem['stories'] for t in s['steps'][:-1])

ixarkon = next(m for m in catalog['story_mappings'] if m['source_area'] == 'ixarkon')
assert (ixarkon['schema_version'], ixarkon['revision'], ixarkon['coverage']) == (3, 2, 'complete')
assert [s['id'] for s in ixarkon['stories']] == ['request-96419-68463578ae17', 'request-96423-9ef90d0b74d4', 'request-96436-719ce450900e']
assert [s['category'] for s in ixarkon['stories']] == ['story', 'story', 'service']
assert report['eligible_by_zone']['964'] == 2 and len(ixarkon['contacts']) == 16 and not ixarkon['exclusions']
assert sum(len(c['topics']) for c in ixarkon['contacts']) == 22
assert sum(t.get('optional', False) for s in ixarkon['stories'] for t in s['steps']) == 4
assert ixarkon['stories'][1]['steps'][0]['contracts'] == ixarkon['stories'][2]['contracts']
assert [t['item_vnums'] for s in ixarkon['stories'] for t in s['steps'] if t['kind'] == 'carried_item'] == [[96431], [96434], [96414]]
assert all(t.get('optional') for s in ixarkon['stories'] for t in s['steps'][:-1])

mntcastl = next(m for m in catalog['story_mappings'] if m['source_area'] == 'mntcastl')
assert (mntcastl['schema_version'], mntcastl['revision'], mntcastl['coverage']) == (3, 1, 'complete')
assert len(mntcastl['stories']) == 8 and len(mntcastl['contacts']) == 15 and not mntcastl['exclusions']
assert [s['category'] for s in mntcastl['stories']] == ['story'] * 4 + ['service'] * 4
assert report['eligible_by_zone']['371'] == 4
assert sum(len(c['topics']) for c in mntcastl['contacts']) == 15
assert sum(t.get('optional', False) for s in mntcastl['stories'] for t in s['steps']) == 14
assert mntcastl['stories'][1]['steps'][0]['contracts'] == mntcastl['stories'][4]['contracts']
assert mntcastl['stories'][3]['steps'][0]['contracts'] == mntcastl['stories'][2]['contracts']
assert mntcastl['stories'][4]['steps'][0]['contracts'][0]['giver_vnum'] == 97901
assert [t['item_vnums'] for t in mntcastl['stories'][4]['steps'] if t['kind'] == 'carried_item'] == [[37106], [97903]]
assert all(t.get('optional') for s in mntcastl['stories'] for t in s['steps'][:-1])

tundra = next(m for m in catalog['story_mappings'] if m['source_area'] == 'tundra')
assert (tundra['schema_version'], tundra['revision'], tundra['coverage']) == (3, 1, 'complete')
assert len(tundra['stories']) == 7 and len(tundra['contacts']) == 16 and not tundra['exclusions']
assert [s['category'] for s in tundra['stories']] == ['story'] * 6 + ['service']
assert report['eligible_by_zone']['137'] == 6
assert sum(len(c['topics']) for c in tundra['contacts']) == 16
assert sum(t.get('optional', False) for s in tundra['stories'] for t in s['steps']) == 14
assert tundra['stories'][1]['steps'][0]['contracts'] == tundra['stories'][0]['contracts']
assert tundra['stories'][5]['steps'][0]['contracts'][0]['giver_vnum'] == 29444
assert [t['item_vnums'] for t in tundra['stories'][0]['steps'] if t['kind'] == 'carried_item'] == [[13708], [13709], [13710], [13711]]
assert [t['item_vnums'] for t in tundra['stories'][5]['steps'] if t['kind'] == 'carried_item'] == [[334], [318], [319]]
assert all(t.get('optional') for s in tundra['stories'] for t in s['steps'][:-1])

fields_between=next(m for m in catalog['story_mappings'] if m['source_area']=='fields_between')
assert (fields_between['schema_version'],fields_between['revision'],fields_between['coverage'])==(3,2,'complete')
assert len(fields_between['stories'])==7 and len(fields_between['contacts'])==22 and not fields_between['exclusions']
assert all(s['category']=='story' for s in fields_between['stories'])
assert report['eligible_by_zone']['710']==7
assert sum(len(c['topics']) for c in fields_between['contacts'])==81
assert sum(t.get('optional',False) for s in fields_between['stories'] for t in s['steps'])==17
assert fields_between['stories'][6]['steps'][0]['contracts']==fields_between['stories'][5]['contracts']
assert [t['item_vnums'] for t in fields_between['stories'][2]['steps'] if t['kind']=='carried_item']==[[71003],[71026],[71010],[71011],[71012],[71013],[71014]]
assert all(t.get('optional') for s in fields_between['stories'] for t in s['steps'][:-1])

goblinht=next(m for m in catalog['story_mappings'] if m['source_area']=='goblinht')
assert (goblinht['schema_version'],goblinht['revision'],goblinht['coverage'])==(3,2,'complete')
assert len(goblinht['stories'])==7 and len(goblinht['contacts'])==21 and len(goblinht['exclusions'])==2
assert sum(s['category']=='story' for s in goblinht['stories'])==5
assert sum(s['category']=='service' for s in goblinht['stories'])==2
assert report['eligible_by_zone']['700']==5 and sum(len(c['topics']) for c in goblinht['contacts'])==22
assert sum(t.get('optional',False) for s in goblinht['stories'] for t in s['steps'])==15
assert goblinht['stories'][3]['steps'][0]['contracts']==goblinht['stories'][4]['contracts']
assert goblinht['stories'][6]['steps'][0]['count']==5

ceothia=next(m for m in catalog['story_mappings'] if m['source_area']=='ceothia')
assert (ceothia['schema_version'],ceothia['revision'],ceothia['coverage'])==(3,1,'complete')
assert len(ceothia['stories'])==6 and len(ceothia['contacts'])==17 and not ceothia['exclusions']
assert report['eligible_by_zone']['808']==6 and all(s['category']=='story' for s in ceothia['stories'])
assert sum(len(c['topics']) for c in ceothia['contacts'])==22
assert sum(t.get('optional',False) for s in ceothia['stories'] for t in s['steps'])==17
assert len(ceothia['stories'][0]['contracts'])==4 and len({c['giver_vnum'] for c in ceothia['stories'][0]['contracts']})==4
assert ceothia['stories'][1]['steps'][0]['contracts']==ceothia['stories'][0]['contracts']
assert ceothia['stories'][4]['steps'][1]['count']==2
assert all(t.get('optional') for s in ceothia['stories'] for t in s['steps'][:-1])

brad=next(m for m in catalog['story_mappings'] if m['source_area']=='brad')
assert (brad['schema_version'],brad['revision'],brad['coverage'])==(3,2,'complete')
assert len(brad['stories'])==5 and len(brad['contacts'])==16 and not brad['exclusions']
assert report['eligible_by_zone']['1350']==5 and all(s['category']=='story' for s in brad['stories'])
assert sum(len(c['topics']) for c in brad['contacts'])==32
assert sum(t.get('optional',False) for s in brad['stories'] for t in s['steps'])==10
assert brad['stories'][3]['steps'][0]['contracts']==brad['stories'][4]['contracts']
assert [t['item_vnums'] for t in brad['stories'][2]['steps'][:-1]]==[[134131],[134132],[134133],[134134],[134135]]
assert all(t.get('optional') for s in brad['stories'] for t in s['steps'][:-1])

desert=next(m for m in catalog['story_mappings'] if m['source_area']=='desert')
assert (desert['schema_version'],desert['revision'],desert['coverage'])==(3,1,'complete')
assert len(desert['stories'])==8 and len(desert['contacts'])==22 and not desert['exclusions']
assert report['eligible_by_zone']['490']==8 and sum(len(c['topics']) for c in desert['contacts'])==46
assert sum(t.get('optional',False) for s in desert['stories'] for t in s['steps'])==11
assert all(t.get('optional') for s in desert['stories'] for t in s['steps'][:-1])
assert desert['stories'][5]['steps'][0]['contracts']==desert['stories'][7]['contracts']

ceopast=next(m for m in catalog['story_mappings'] if m['source_area']=='ceopast')
assert (ceopast['schema_version'],ceopast['revision'],ceopast['coverage'])==(3,1,'complete')
assert len(ceopast['stories'])==4 and len(ceopast['contacts'])==13 and not ceopast['exclusions']
assert report['eligible_by_zone']['811']==4 and sum(len(c['topics']) for c in ceopast['contacts'])==21
assert sum(t.get('optional',False) for s in ceopast['stories'] for t in s['steps'])==11
assert len(ceopast['stories'][2]['contracts'])==3 and ceopast['stories'][2]['steps'][0]['item_vnums']==[81108,81114,81115]
assert ceopast['stories'][1]['steps'][0]['contracts']==ceopast['stories'][0]['contracts']
assert ceopast['stories'][3]['steps'][0]['item_vnums']==[81108] and ceopast['stories'][3]['steps'][1]['item_vnums']==[81119]

basin=next(m for m in catalog['story_mappings'] if m['source_area']=='basin_wa')
assert (basin['schema_version'],basin['revision'],basin['coverage'])==(3,1,'complete')
assert len(basin['stories'])==7 and len(basin['contacts'])==8 and not basin['exclusions']
assert report['eligible_by_zone']['340']==6 and sum(len(c['topics']) for c in basin['contacts'])==14
assert sum(t.get('optional',False) for s in basin['stories'] for t in s['steps'])==14
assert len(basin['stories'][5]['contracts'])==4 and basin['stories'][5]['steps'][0]['item_vnums']==[34000,34001,34002,34003]
assert basin['stories'][0]['steps'][0]['contracts']==basin['stories'][1]['contracts']
assert basin['stories'][6]['category']=='service'
assert [s['steps'][1]['item_vnums'] for s in basin['stories'][1:5]]==[[34002],[34001],[34000],[34003]]

crypt=next(m for m in catalog['story_mappings'] if m['source_area']=='crypt')
assert (crypt['schema_version'],crypt['revision'],crypt['coverage'])==(3,1,'complete')
assert len(crypt['stories'])==5 and len(crypt['contacts'])==9 and not crypt['exclusions']
assert report['eligible_by_zone']['143']==5 and sum(len(c['topics']) for c in crypt['contacts'])==21
assert sum(t.get('optional',False) for s in crypt['stories'] for t in s['steps'])==14
assert [t['count'] for t in crypt['stories'][0]['steps'][:-1]]==[4,2]
assert [t['item_vnums'] for t in crypt['stories'][1]['steps'][:-1]]==[[14494],[14501],[14521],[14536],[14540]]
assert crypt['stories'][3]['steps'][0]['contracts']==crypt['stories'][2]['contracts']
assert crypt['stories'][4]['steps'][0]['item_vnums']==[14556]

val=next(m for m in catalog['story_mappings'] if m['source_area']=='val')
assert (val['schema_version'],val['revision'],val['coverage'])==(3,1,'complete')
assert len(val['stories'])==8 and len(val['contacts'])==18 and not val['exclusions']
assert report['eligible_by_zone']['384']==8 and sum(len(c['topics']) for c in val['contacts'])==26
assert sum(t.get('optional',False) for s in val['stories'] for t in s['steps'])==15
assert all(s['category']=='story' for s in val['stories'])
assert val['stories'][4]['steps'][0]['contracts']==val['stories'][3]['contracts']
assert val['stories'][5]['steps'][0]['item_vnums']==[38442] and val['stories'][6]['steps'][0]['item_vnums']==[38451]

harrow=next(m for m in catalog['story_mappings'] if m['source_area']=='harrow')
assert (harrow['schema_version'],harrow['revision'],harrow['coverage'])==(3,1,'complete')
assert len(harrow['stories'])==8 and len(harrow['contacts'])==14 and not harrow['exclusions']
assert report['eligible_by_zone']['294']==8 and sum(len(c['topics']) for c in harrow['contacts'])==12
assert sum(t.get('optional',False) for s in harrow['stories'] for t in s['steps'])==20
assert all(s['category']=='story' for s in harrow['stories'])
assert all(harrow['stories'][i]['steps'][0]['contracts']==harrow['stories'][0]['contracts'] for i in range(1,5))
assert harrow['stories'][5]['steps'][0]['item_vnums']==[29440] and harrow['stories'][7]['steps'][0]['item_vnums']==[29444]

mountaintracks=next(m for m in catalog['story_mappings'] if m['source_area']=='mountaintracks')
assert (mountaintracks['schema_version'],mountaintracks['revision'],mountaintracks['coverage'])==(3,1,'complete')
assert len(mountaintracks['stories'])==4 and len(mountaintracks['contacts'])==8 and not mountaintracks['exclusions']
assert report['eligible_by_zone']['209']==4 and sum(len(c['topics']) for c in mountaintracks['contacts'])==19
assert sum(t.get('optional',False) for s in mountaintracks['stories'] for t in s['steps'])==6
assert all(s['category']=='story' for s in mountaintracks['stories'])
assert mountaintracks['stories'][3]['steps'][0]['contracts']==mountaintracks['stories'][2]['contracts']
assert [t['item_vnums'][0] for t in mountaintracks['stories'][2]['steps'][:-1]]==[20947,20948]
assert mountaintracks['stories'][0]['steps'][0]['item_vnums']==[20923] and mountaintracks['stories'][3]['steps'][1]['item_vnums']==[20949]

shortc=next(m for m in catalog['story_mappings'] if m['source_area']=='shortc')
assert (shortc['schema_version'],shortc['revision'],shortc['coverage'])==(3,1,'complete')
assert len(shortc['stories'])==2 and len(shortc['contacts'])==5 and len(shortc['exclusions'])==1
assert report['eligible_by_zone']['532']==2 and sum(len(c['topics']) for c in shortc['contacts'])==13
assert sum(t.get('optional',False) for s in shortc['stories'] for t in s['steps'])==4
assert all(s['category']=='story' for s in shortc['stories'])
assert shortc['stories'][1]['steps'][0]['contracts']==shortc['stories'][0]['contracts']
assert [t['item_vnums'][0] for t in shortc['stories'][1]['steps'][1:-1]]==[53200,53201]
assert shortc['exclusions'][0]['contracts']==[{'giver_vnum':53201,'completion_key':'give=T:19;receive=;disappear=0'}]

lavcav=next(m for m in catalog['story_mappings'] if m['source_area']=='lavcav')
assert (lavcav['schema_version'],lavcav['revision'],lavcav['coverage'])==(3,1,'complete')
assert len(lavcav['stories'])==1 and len(lavcav['contacts'])==7 and len(lavcav['exclusions'])==1
assert report['eligible_by_zone']['355']==1 and sum(len(c['topics']) for c in lavcav['contacts'])==6
assert sum(t.get('optional',False) for s in lavcav['stories'] for t in s['steps'])==3
assert lavcav['stories'][0]['category']=='story'
assert lavcav['stories'][0]['steps'][0]['contracts']==lavcav['exclusions'][0]['contracts']
assert [t['item_vnums'][0] for t in lavcav['stories'][0]['steps'][1:-1]]==[35505,35515]
assert lavcav['exclusions'][0]['contracts']==[{'giver_vnum':35535,'completion_key':'give=C:100000;receive=I:35515;disappear=1'}]

nomads=next(m for m in catalog['story_mappings'] if m['source_area']=='nomads')
assert (nomads['schema_version'],nomads['revision'],nomads['coverage'])==(3,1,'complete')
assert len(nomads['stories'])==2 and len(nomads['contacts'])==7 and not nomads['exclusions']
assert report['eligible_by_zone']['62']==2 and sum(len(c['topics']) for c in nomads['contacts'])==5
assert sum(t.get('optional',False) for s in nomads['stories'] for t in s['steps'])==6
assert all(s['category']=='story' for s in nomads['stories'])
assert nomads['stories'][1]['steps'][0]['contracts']==nomads['stories'][0]['contracts']
assert [t['item_vnums'][0] for t in nomads['stories'][0]['steps'][:-1]]==[6217,6218]
assert [t['item_vnums'][0] for t in nomads['stories'][1]['steps'][1:-1]]==[6219,6220,6221]

undermountain=next(m for m in catalog['story_mappings'] if m['source_area']=='undermountain')
assert (undermountain['schema_version'],undermountain['revision'],undermountain['coverage'])==(3,1,'complete')
assert len(undermountain['stories'])==2 and len(undermountain['contacts'])==3 and not undermountain['exclusions']
assert report['eligible_by_zone']['920']==2 and sum(len(c['topics']) for c in undermountain['contacts'])==8
assert sum(t.get('optional',False) for s in undermountain['stories'] for t in s['steps'])==3
assert undermountain['stories'][1]['steps'][0]['contracts']==undermountain['stories'][0]['contracts']
assert undermountain['stories'][0]['steps'][0]['item_vnums']==[92133] and undermountain['stories'][1]['steps'][1]['item_vnums']==[92134]

desolateinv=next(m for m in catalog['story_mappings'] if m['source_area']=='desolateinv')
assert len(desolateinv['stories'])==17 and len(desolateinv['contacts'])==16 and report['eligible_by_zone']['773']==16
assert sum(t.get('optional',False) for s in desolateinv['stories'] for t in s['steps'])==27
assert sum(s['category']=='service' for s in desolateinv['stories'])==1

spshold=next(m for m in catalog['story_mappings'] if m['source_area']=='spshold')
assert len(spshold['stories'])==4 and len(spshold['contacts'])==6 and report['eligible_by_zone']['226']==4
assert sum(t.get('optional',False) for s in spshold['stories'] for t in s['steps'])==8
assert not spshold['exclusions'] and not any(c['topics'] for c in spshold['contacts'])

harpyht=next(m for m in catalog['story_mappings'] if m['source_area']=='harpyht')
assert (harpyht['schema_version'],harpyht['revision'])==(3,2)
assert len(harpyht['stories'])==2 and len(harpyht['contacts'])==5 and len(harpyht['exclusions'])==1 and report['eligible_by_zone']['311']==2
assert sum(t.get('optional',False) for s in harpyht['stories'] for t in s['steps'])==3
assert [(c['mob_vnum'],c['topics']) for c in harpyht['contacts'] if c['topics']]==[(31108,['undead'])]

herders=next(m for m in catalog['story_mappings'] if m['source_area']=='herders')
assert (herders['schema_version'],herders['revision'],herders['coverage'])==(3,1,'complete')
assert len(herders['stories'])==12 and len(herders['contacts'])==15 and not herders['exclusions'] and report['eligible_by_zone']['943']==6
assert sum(s['category']=='service' for s in herders['stories'])==6
assert sum(len(c['topics']) for c in herders['contacts'])==52
assert sum(t.get('optional',False) for s in herders['stories'] for t in s['steps'])==19
hstories={s['id']:s for s in herders['stories']}
assert [t['item_vnums'] for t in hstories['two-farseer-mind-halves']['steps'][:-1]]==[[94344],[94345]]
assert hstories['five-beholder-eyes']['steps'][0]['count']==5
assert [t['count'] for t in hstories['diorite-boots']['steps'][:-1]]==[1,2,3]
assert hstories['diorite-boots']['steps'][0]['item_vnums']==hstories['diorite-warlord-statue']['steps'][0]['item_vnums']==[94378]
assert all(t.get('optional') for s in herders['stories'] for t in s['steps'][:-1])
assert all(s['steps'][-1]['contracts']==s['contracts'] for s in herders['stories'])

jotun=next(m for m in catalog['story_mappings'] if m['source_area']=='jotun')
assert (jotun['schema_version'],jotun['revision'],jotun['coverage'])==(3,1,'complete')
assert len(jotun['stories'])==15 and len(jotun['contacts'])==10 and not jotun['exclusions'] and report['eligible_by_zone']['960']==11
assert sum(s['category']=='service' for s in jotun['stories'])==4
assert sum(len(c['topics']) for c in jotun['contacts'])==47
assert sum(t.get('optional',False) for s in jotun['stories'] for t in s['steps'])==19
jstories={s['id']:s for s in jotun['stories']}
assert [t['item_vnums'] for t in jstories['mimir-five-proofs']['steps'][:-1]]==[[96036],[96038],[96039],[96037],[96081]]
assert jstories['mimir-five-proofs']['steps'][1]['item_vnums']==jstories['quelranor-balor-sword']['steps'][0]['item_vnums']==[96038]
assert jstories['grishnar-rashnik-totem']['steps'][0]['item_vnums']==[96061] and jstories['rashnik-grishnar-standard']['steps'][0]['item_vnums']==[96060]
assert jstories['smith-remorhaz-armor']['steps'][0]['item_vnums']==[96069] and jstories['smith-ancient-remorhaz-armor']['steps'][0]['item_vnums']==[96071]
assert all(t.get('optional') and t['count']==1 for s in jotun['stories'] for t in s['steps'][:-1])
assert all(s['steps'][-1]['contracts']==s['contracts'] for s in jotun['stories'])

temple=next(m for m in catalog['story_mappings'] if m['source_area']=='temple')
assert (temple['schema_version'],temple['revision'],temple['coverage'])==(3,1,'complete')
assert len(temple['stories'])==6 and len(temple['contacts'])==9 and not temple['exclusions'] and report['eligible_by_zone']['183']==6
assert sum(len(c['topics']) for c in temple['contacts'])==42
assert sum(t.get('optional',False) for s in temple['stories'] for t in s['steps'])==8
tstories={s['id']:s for s in temple['stories']}
assert [t['item_vnums'] for t in tstories['sage-four-proofs']['steps'][:-1]]==[[18322],[18324],[18325]]
assert [t['count'] for t in tstories['sage-four-proofs']['steps'][:-1]]==[2,1,1]
assert tstories['master-lost-locket']['steps'][0]['item_vnums']==[18307] and tstories['angel-wedding-ring']['steps'][0]['item_vnums']==[18336]
assert all(t.get('optional') for s in temple['stories'] for t in s['steps'][:-1])
assert all(s['category']=='story' and len(s['contracts'])==1 and s['steps'][-1]['contracts']==s['contracts'] for s in temple['stories'])

pods=next(m for m in catalog['story_mappings'] if m['source_area']=='pods')
assert (pods['schema_version'],pods['revision'],pods['coverage'])==(3,1,'complete')
assert len(pods['stories'])==4 and len(pods['contacts'])==5 and not pods['exclusions'] and report['eligible_by_zone']['285']==1
assert sum(len(c['topics']) for c in pods['contacts'])==52
assert sum(t.get('optional',False) for s in pods['stories'] for t in s['steps'])==4
pstories={s['id']:s for s in pods['stories']}
assert [t['count'] for t in pstories['podaling-birdfeather-headdress']['steps'][:-1]]==[6,1]
assert [t['item_vnums'] for t in pstories['podaling-birdfeather-headdress']['steps'][:-1]]==[[28554],[28555]]
assert pstories['podaling-neberihide-necklace']['steps'][0]['count']==3
assert len(pstories['chamberlain-tongue-key']['steps'])==1
assert all(t.get('optional') for s in pods['stories'] for t in s['steps'][:-1])
assert sum(s['category']=='service' for s in pods['stories'])==3
assert all(s['steps'][-1]['contracts']==s['contracts'] and len(s['contracts'])==1 for s in pods['stories'])

citadel=next(m for m in catalog['story_mappings'] if m['source_area']=='citadel')
assert (citadel['schema_version'],citadel['revision'],citadel['coverage'])==(3,1,'complete')
assert len(citadel['stories'])==2 and len(citadel['contacts'])==20 and len(citadel['exclusions'])==1
assert sum(len(c['topics']) for c in citadel['contacts'])==63 and report['eligible_by_zone']['130']==2
assert all(s['category']=='story' and len(s['steps'])==2 and s['steps'][0]['optional'] and s['steps'][-1]['contracts']==s['contracts'] for s in citadel['stories'])
assert [s['steps'][0]['item_vnums'] for s in citadel['stories']]==[[13006],[13033]]
assert citadel['exclusions'][0]['contracts']==[{'giver_vnum':13030,'completion_key':'give=I:0;receive=;disappear=0'}]

element=next(m for m in catalog['story_mappings'] if m['source_area']=='element')
assert (element['schema_version'],element['revision'],element['coverage'])==(3,1,'complete')
assert len(element['stories'])==3 and len(element['contacts'])==20 and not element['exclusions']
assert sum(len(c['topics']) for c in element['contacts'])==84 and report['eligible_by_zone']['38']==3
assert all(s['category']=='story' and len(s['steps'])==2 and s['steps'][0]['optional'] and s['steps'][-1]['contracts']==s['contracts'] for s in element['stories'])
assert [s['steps'][0]['item_vnums'] for s in element['stories']]==[[3808],[3809],[3831]]

earth=next(m for m in catalog['story_mappings'] if m['source_area']=='earth')
assert (earth['schema_version'],earth['revision'],earth['coverage'])==(3,1,'complete')
assert len(earth['stories'])==2 and len(earth['contacts'])==12 and not earth['exclusions']
assert sum(len(c['topics']) for c in earth['contacts'])==46 and report['eligible_by_zone']['435']==2
assert [s['steps'][0]['item_vnums'] for s in earth['stories']]==[[43525],[43539]]
assert all(len(s['steps'])==2 and s['steps'][0]['optional'] and s['steps'][-1]['contracts']==s['contracts'] for s in earth['stories'])

githzer=next(m for m in catalog['story_mappings'] if m['source_area']=='githzer')
assert (githzer['schema_version'],githzer['revision'],githzer['coverage'])==(3,1,'complete')
assert len(githzer['stories'])==13 and len(githzer['contacts'])==8 and not githzer['exclusions']
assert sum(len(c['topics']) for c in githzer['contacts'])==35 and report['eligible_by_zone']['444']==11
assert sum(t.get('optional',False) for s in githzer['stories'] for t in s['steps'])==20
gs={s['id']:s for s in githzer['stories']}
assert gs['prazyz-red-hide']['steps'][1]['count']==gs['prazyz-green-hide']['steps'][1]['count']==2
assert gs['prophet-five-signet-rings']['steps'][0]['item_vnums']==[44563] and gs['prophet-five-signet-rings']['steps'][0]['count']==5
assert len(gs['zangzk-paid-key']['steps'])==1 and gs['zangzk-adamantite-information']['contracts'][0]['completion_key']=='give=I:44509;receive=;disappear=0'
assert next(c for c in githzer['contacts'] if c['mob_vnum']==44437)['topics']==[]
assert all(t.get('optional') for s in githzer['stories'] for t in s['steps'][:-1])
assert all(s['steps'][-1]['contracts']==s['contracts'] for s in githzer['stories'])

assert sum(s["category"]=="service" for s in githzer["stories"])==2

worms = next(m for m in catalog['story_mappings'] if m['source_area'] == 'worms')
assert (worms['schema_version'], worms['revision'], worms['coverage']) == (3, 1, 'complete')
assert len(worms['stories']) == 16 and len(worms['contacts']) == 1 and not worms['exclusions']
assert sum(len(c['topics']) for c in worms['contacts']) == 20 and report['eligible_by_zone'].get('69', 0) == 0
assert all(s['category'] == 'service' and len(s['contracts']) == 1 and s['steps'][-1]['contracts'] == s['contracts'] for s in worms['stories'])
assert sum(len(s['steps']) - 1 for s in worms['stories']) == 34
assert sum(t['count'] for s in worms['stories'] for t in s['steps'][:-1]) == 40
ws = {s['id']: s for s in worms['stories']}
assert {t['item_vnums'][0]: t['count'] for t in ws['wilms-shield']['steps'][:-1]} == {6914: 1, 6905: 1, 6907: 2, 6913: 1}
assert ws['wilms-cloak']['steps'][0]['count'] == ws['wilms-gloves']['steps'][0]['count'] == ws['wilms-mask']['steps'][0]['count'] == 2
assert ws['wilms-armor']['steps'][2]['count'] == 3
assert ws['wilms-helmet']['steps'][0]['item_vnums'] == [6900] and ws['wilms-eyepatch']['steps'][0]['item_vnums'] == [6901]
assert all(t['optional'] and t['kind'] == 'carried_item' for s in worms['stories'] for t in s['steps'][:-1])


ravenloft = next(m for m in catalog['story_mappings'] if m['source_area'] == 'ravenloft')
assert (ravenloft['schema_version'],ravenloft['revision'],ravenloft['coverage']) == (3,1,'complete')
assert len(ravenloft['stories']) == 4 and len(ravenloft['contacts']) == 5 and len(ravenloft['exclusions']) == 1
assert report['eligible_by_zone']['583'] == 4 and sum(len(c['topics']) for c in ravenloft['contacts']) == 42
rv = {s['id']:s for s in ravenloft['stories']}
assert [t['item_vnums'] for t in rv['vey-temporal-essences']['steps'][:-1]] == [[58393],[58407]]
assert [t['item_vnums'] for t in rv['megosh-holy-relics']['steps'][:-1]] == [[58346],[58410]]
assert rv['perganan-lenience']['steps'][0]['item_vnums'] == [58370] and rv['wizard-indulgence']['steps'][0]['item_vnums'] == [58369]
assert all(t['optional'] and t['kind'] == 'carried_item' and t['count'] == 1 for s in ravenloft['stories'] for t in s['steps'][:-1])
assert all(s['category'] == 'story' and s['steps'][-1]['contracts'] == s['contracts'] for s in ravenloft['stories'])
assert ravenloft['exclusions'][0]['contracts'] == [{'giver_vnum':58343,'completion_key':'give=I:58427;receive=I:58427;disappear=0'}]
assert sum(u['daily_candidate'] for u in module.story_units(catalog) if u['zone_number'] == 583) == 1

barovia2=next(m for m in catalog['story_mappings'] if m['source_area']=='barovia2')
assert (barovia2['schema_version'],barovia2['revision'],barovia2['coverage'])==(3,1,'complete')
assert len(barovia2['stories'])==5 and len(barovia2['contacts'])==4 and not barovia2['exclusions']
assert report['eligible_by_zone']['588']==5 and sum(len(c['topics']) for c in barovia2['contacts'])==32
bv={s['id']:s for s in barovia2['stories']}
assert [t['item_vnums'] for t in bv['eva-witch-relics']['steps'][:-1]]==[[58817],[58824]]
assert [t['item_vnums'] for t in bv['eva-vistani-marks']['steps'][:-1]]==[[58845],[58844]]
assert bv['urik-perganan-note']['steps'][0]['item_vnums']==[58416]
assert bv['mirkodesiuska-dragon-egg']['steps'][0]['item_vnums']==[58809] and bv['megosh-devil-heart']['steps'][0]['item_vnums']==[58834]
assert all(t['optional'] and t['kind']=='carried_item' and t['count']==1 for s in barovia2['stories'] for t in s['steps'][:-1])
assert all(s['category']=='story' and s['steps'][-1]['contracts']==s['contracts'] for s in barovia2['stories'])
assert sum(u['daily_candidate'] for u in module.story_units(catalog) if u['zone_number']==588)==1


werrun=next(m for m in catalog['story_mappings'] if m['source_area']=='werrun')
assert (werrun['schema_version'],werrun['revision'],werrun['coverage'])==(3,1,'complete')
assert len(werrun['stories'])==4 and len(werrun['contacts'])==5 and not werrun['exclusions']
assert report['eligible_by_zone']['383']==4 and sum(len(c['topics']) for c in werrun['contacts'])==25
ws={s['id']:s for s in werrun['stories']}
for id,item in (('malfun-revan-book',38311),('malfun-sklera-braid',38312),('githzerai-vewon-book',38326)):
 assert [(t['item_vnums'],t['count']) for t in ws[id]['steps'][:-1]]==[([item],1)]
 assert ws[id]['steps'][0]['optional'] and ws[id]['steps'][0]['kind']=='carried_item'
assert len(ws['mage-debt']['steps'])==1 and 'blocks' in ws['mage-debt']['summary'] and '350000' in ws['mage-debt']['summary']
assert all(s['category']=='story' and s['steps'][-1]['contracts']==s['contracts'] for s in werrun['stories'])
assert sum(u['daily_candidate'] for u in module.story_units(catalog) if u['zone_number']==383)==3
assert not any(d['giver_vnum']==38309 for d in catalog['definitions'])


newhope=next(m for m in catalog['story_mappings'] if m['source_area']=='newhope')
assert (newhope['schema_version'],newhope['revision'],newhope['coverage'])==(3,1,'complete')
assert len(newhope['stories'])==4 and len(newhope['contacts'])==3 and not newhope['exclusions']
assert report['eligible_by_zone'].get('890',0)==0 and sum(len(c['topics']) for c in newhope['contacts'])==21
nh={s['id']:s for s in newhope['stories']}
for id,weapon in (('vitrius-short-sword',89141),('vitrius-long-sword',89142),('vitrius-two-handed-sword',89142),('vitrius-dagger',89140)):
 s=nh[id];assert s['category']=='service' and [(t['item_vnums'],t['count']) for t in s['steps'][:-1]]==[([89117],1),([weapon],1)]
 assert all(t['kind']=='carried_item' and t['optional'] for t in s['steps'][:-1])
 assert 'Accounting currently blocks' in s['summary'] and s['steps'][-1]['contracts']==s['contracts']
u=[u for u in module.story_units(catalog) if u['zone_number']==890]
assert len(u)==4 and not any(x['achievement'] or x['daily_candidate'] for x in u)


mount=next(m for m in catalog['story_mappings'] if m['source_area']=='mount')
assert (mount['schema_version'],mount['revision'],mount['coverage'])==(3,1,'complete') and not mount['exclusions']
assert len(mount['stories'])==3 and len(mount['contacts'])==14 and sum(len(c['topics']) for c in mount['contacts'])==29
assert sum(len(s['steps']) for s in mount['stories'])==9 and sum(t.get('optional',False) for s in mount['stories'] for t in s['steps'])==6
assert mount['stories'][2]['contracts'][0]['completion_key']=='give=I:9117,I:9118,I:9119,I:9120;receive=I:9132,I:9135;disappear=0' and report['eligible_by_zone']['91']==3


cloud=next(m for m in catalog['story_mappings'] if m['source_area']=='cldgt')
assert (cloud['schema_version'],cloud['revision'],cloud['coverage'])==(3,1,'complete') and not cloud['exclusions']
assert len(cloud['stories'])==3 and len(cloud['contacts'])==17 and sum(len(c['topics']) for c in cloud['contacts'])==13
assert sum(len(s['steps']) for s in cloud['stories'])==16 and sum(t.get('optional',False) for s in cloud['stories'] for t in s['steps'])==13
assert cloud['stories'][0]['steps'][0]['count']==5 and report['eligible_by_zone']['995']==3


myrloch=next(m for m in catalog['story_mappings'] if m['source_area']=='myrloch_vale')
assert (myrloch['schema_version'],myrloch['revision'],myrloch['coverage'])==(3,1,'complete') and not myrloch['exclusions']
assert len(myrloch['stories'])==4 and len(myrloch['contacts'])==11 and sum(len(c['topics']) for c in myrloch['contacts'])==6
assert sum(len(s['steps']) for s in myrloch['stories'])==11 and sum(t.get('optional',False) for s in myrloch['stories'] for t in s['steps'])==7
assert report['eligible_by_zone']['264']==4


bandit=next(m for m in catalog['story_mappings'] if m['source_area']=='banditca')
assert (bandit['schema_version'],bandit['revision'],bandit['coverage'])==(3,1,'complete') and not bandit['exclusions']
assert len(bandit['stories'])==4 and len(bandit['contacts'])==14 and sum(len(c['topics']) for c in bandit['contacts'])==8
assert sum(len(s['steps']) for s in bandit['stories'])==10 and sum(t.get('optional',False) for s in bandit['stories'] for t in s['steps'])==6
assert report['eligible_by_zone']['148']==4


maze=next(m for m in catalog['story_mappings'] if m['source_area']=='maze_are')
assert (maze['schema_version'],maze['revision'],maze['coverage'])==(3,1,'complete') and not maze['exclusions']
assert len(maze['stories'])==7 and len(maze['contacts'])==10 and sum(len(c['topics']) for c in maze['contacts'])==9
assert sum(len(s['steps']) for s in maze['stories'])==21 and sum(t.get('optional',False) for s in maze['stories'] for t in s['steps'])==14
assert report['eligible_by_zone']['940']==7


vargan=next(m for m in catalog['story_mappings'] if m['source_area']=='vargan')
assert (vargan['schema_version'],vargan['revision'],vargan['coverage'])==(3,1,'complete') and not vargan['exclusions']
assert len(vargan['stories'])==1 and len(vargan['contacts'])==11 and sum(len(c['topics']) for c in vargan['contacts'])==6
assert len(vargan['stories'][0]['steps'])==3 and sum(t.get('optional',False) for t in vargan['stories'][0]['steps'])==2
assert report['eligible_by_zone']['21']==1


pworm=next(m for m in catalog['story_mappings'] if m['source_area']=='pworm')
assert (pworm['schema_version'],pworm['revision'],pworm['coverage'])==(3,1,'complete') and not pworm['exclusions']
assert len(pworm['stories'])==1 and len(pworm['contacts'])==11 and sum(len(c['topics']) for c in pworm['contacts'])==13
assert len(pworm['stories'][0]['steps'])==3 and sum(t.get('optional',False) for t in pworm['stories'][0]['steps'])==2
assert report['eligible_by_zone']['425']==1


prisonb=next(m for m in catalog['story_mappings'] if m['source_area']=='prisonb')
assert (prisonb['schema_version'],prisonb['revision'],prisonb['coverage'])==(3,1,'complete') and not prisonb['exclusions']
assert len(prisonb['stories'])==1 and len(prisonb['contacts'])==16 and sum(len(c['topics']) for c in prisonb['contacts'])==9
assert len(prisonb['stories'][0]['steps'])==6 and sum(t.get('optional',False) for t in prisonb['stories'][0]['steps'])==5
assert report['eligible_by_zone']['430']==1


moonshae=next(m for m in catalog['story_mappings'] if m['source_area']=='moonshae')
assert (moonshae['schema_version'],moonshae['revision'],moonshae['coverage'])==(3,1,'complete') and not moonshae['exclusions']
assert len(moonshae['stories'])==2 and len(moonshae['contacts'])==12 and sum(len(c['topics']) for c in moonshae['contacts'])==14
assert [len(s['steps']) for s in moonshae['stories']]==[2,3] and sum(t.get('optional',False) for s in moonshae['stories'] for t in s['steps'])==3
assert report['eligible_by_zone']['262']==2


ixxillikor=next(m for m in catalog['story_mappings'] if m['source_area']=='ixxillikor')
assert (ixxillikor['schema_version'],ixxillikor['revision'],ixxillikor['coverage'])==(3,1,'complete') and not ixxillikor['exclusions']
assert len(ixxillikor['stories'])==2 and len(ixxillikor['contacts'])==12 and sum(len(c['topics']) for c in ixxillikor['contacts'])==7
assert [len(s['steps']) for s in ixxillikor['stories']]==[4,1] and sum(t.get('optional',False) for s in ixxillikor['stories'] for t in s['steps'])==3
assert report['eligible_by_zone']['42']==2


apocalypse=next(m for m in catalog['story_mappings'] if m['source_area']=='4horse')
assert (apocalypse['schema_version'],apocalypse['revision'],apocalypse['coverage'])==(3,1,'complete') and not apocalypse['exclusions']
assert len(apocalypse['stories'])==2 and len(apocalypse['contacts'])==12 and sum(len(c['topics']) for c in apocalypse['contacts'])==7
assert [len(s['steps']) for s in apocalypse['stories']]==[6,2] and sum(t.get('optional',False) for s in apocalypse['stories'] for t in s['steps'])==6
assert report['eligible_by_zone']['345']==2


shady=next(m for m in catalog['story_mappings'] if m['source_area']=='shady')
assert (shady['schema_version'],shady['revision'],shady['coverage'])==(3,2,'complete') and not shady['exclusions']
assert len(shady['stories'])==3 and len(shady['contacts'])==14 and sum(len(c['topics']) for c in shady['contacts'])==11
assert [len(s['steps']) for s in shady['stories']]==[2,2,2] and sum(t.get('optional',False) for s in shady['stories'] for t in s['steps'])==3
assert report['eligible_by_zone']['975']==3

kimordril=next(m for m in catalog['story_mappings'] if m['source_area']=='kimordril')
assert (kimordril['schema_version'],kimordril['revision'],kimordril['coverage'])==(3,2,'complete') and not kimordril['exclusions']
assert len(kimordril['stories'])==4 and len(kimordril['contacts'])==14 and sum(len(c['topics']) for c in kimordril['contacts'])==14
assert [len(s['steps']) for s in kimordril['stories']]==[4,2,2,2] and sum(t.get('optional',False) for s in kimordril['stories'] for t in s['steps'])==6
assert report['eligible_by_zone']['955']==4

cerebusp=next(m for m in catalog['story_mappings'] if m['source_area']=='cerebusp')
assert (cerebusp['schema_version'],cerebusp['revision'],cerebusp['coverage'])==(3,1,'complete') and not cerebusp['exclusions']
assert len(cerebusp['stories'])==9 and len(cerebusp['contacts'])==17 and sum(len(c['topics']) for c in cerebusp['contacts'])==5
assert [len(s['steps']) for s in cerebusp['stories']]==[4,3,5,2,2,2,4,3,5] and sum(t.get('optional',False) for s in cerebusp['stories'] for t in s['steps'])==21
assert report['eligible_by_zone']['220']==9

prison=next(m for m in catalog['story_mappings'] if m['source_area']=='prison')
assert (prison['schema_version'],prison['revision'],prison['coverage'])==(3,1,'complete') and not prison['exclusions']
assert len(prison['stories'])==2 and len(prison['contacts'])==9 and sum(len(c['topics']) for c in prison['contacts'])==8
assert [len(s['steps']) for s in prison['stories']]==[2,4] and sum(t.get('optional',False) for s in prison['stories'] for t in s['steps'])==4
assert report['eligible_by_zone']['73']==2


negplane=next(m for m in catalog['story_mappings'] if m['source_area']=='negplane')
assert (negplane['schema_version'],negplane['revision'],negplane['coverage'])==(3,1,'complete') and not negplane['exclusions']
assert len(negplane['stories'])==2 and len(negplane['contacts'])==10 and sum(len(c['topics']) for c in negplane['contacts'])==9
assert [len(s['steps']) for s in negplane['stories']]==[7,2] and sum(t.get('optional',False) for s in negplane['stories'] for t in s['steps'])==7
assert report['eligible_by_zone']['266']==2


voluntown=next(m for m in catalog['story_mappings'] if m['source_area']=='Voluntown')
assert (voluntown['schema_version'],voluntown['revision'],voluntown['coverage'])==(3,1,'complete') and not voluntown['exclusions']
assert len(voluntown['stories'])==2 and len(voluntown['contacts'])==10 and sum(len(c['topics']) for c in voluntown['contacts'])==9
assert [len(s['steps']) for s in voluntown['stories']]==[7,2] and sum(t.get('optional',False) for s in voluntown['stories'] for t in s['steps'])==7
assert report['eligible_by_zone']['1424']==2


firesworn=next(m for m in catalog['story_mappings'] if m['source_area']=='firesworn_altar')
assert (firesworn['schema_version'],firesworn['revision'],firesworn['coverage'])==(3,1,'complete') and not firesworn['exclusions']
assert len(firesworn['stories'])==1 and len(firesworn['contacts'])==8 and sum(len(c['topics']) for c in firesworn['contacts'])==12
assert len(firesworn['stories'][0]['contracts'])==1 and len(firesworn['stories'][0]['steps'])==5
assert sum(t.get('optional',False) for t in firesworn['stories'][0]['steps'])==4
assert report['eligible_by_zone']['1351']==1

shabo=next(m for m in catalog['story_mappings'] if m['source_area']=='shabo')
assert (shabo['schema_version'],shabo['revision'],shabo['coverage'])==(3,1,'complete') and not shabo['exclusions']
assert len(shabo['stories'])==2 and len(shabo['contacts'])==10 and sum(len(c['topics']) for c in shabo['contacts'])==18
assert [len(s['contracts']) for s in shabo['stories']]==[1,1]
assert sum(len(s['steps']) for s in shabo['stories'])==7 and sum(t.get('optional',False) for s in shabo['stories'] for t in s['steps'])==5
assert report['eligible_by_zone']['328']==2

aravne=next(m for m in catalog['story_mappings'] if m['source_area']=='clfhaven')
assert (aravne['schema_version'],aravne['revision'],aravne['coverage'])==(3,1,'complete') and not aravne['exclusions']
assert len(aravne['stories'])==3 and len(aravne['contacts'])==8 and sum(len(c['topics']) for c in aravne['contacts'])==15
assert [len(s['contracts']) for s in aravne['stories']]==[1,1,1]
assert sum(len(s['steps']) for s in aravne['stories'])==18 and sum(t.get('optional',False) for s in aravne['stories'] for t in s['steps'])==15
assert report['eligible_by_zone']['215']==3

tharn=next(m for m in catalog['story_mappings'] if m['source_area']=='tharnadian_ruin')
assert (tharn['schema_version'],tharn['revision'],tharn['coverage'])==(3,1,'complete') and not tharn['exclusions']
assert len(tharn['stories'])==3 and len(tharn['contacts'])==5 and sum(len(c['topics']) for c in tharn['contacts'])==16
assert [len(s['contracts']) for s in tharn['stories']]==[2,2,1]
assert sum(len(s['steps']) for s in tharn['stories'])==10 and sum(t.get('optional',False) for s in tharn['stories'] for t in s['steps'])==7
assert report['eligible_by_zone']['55']==3

goblincave=next(m for m in catalog['story_mappings'] if m['source_area']=='goblincave')
assert (goblincave['schema_version'],goblincave['revision'],goblincave['coverage'])==(3,1,'complete')
assert len(goblincave['stories'])==6 and len(goblincave['contacts'])==4 and sum(len(c['topics']) for c in goblincave['contacts'])==11 and not goblincave['exclusions']
assert [s['category'] for s in goblincave['stories']]==['service','service','request','service','service','request']
assert sum(len(s['steps']) for s in goblincave['stories'])==14 and sum(t.get('optional',False) for s in goblincave['stories'] for t in s['steps'])==8
assert report['eligible_by_zone']['190']==2 and next(c for c in goblincave['contacts'] if c['mob_vnum']==19005)['keyword']=='leather'

stormht=next(m for m in catalog['story_mappings'] if m['source_area']=='stormht')
assert (stormht['schema_version'],stormht['revision'],stormht['coverage'])==(3,1,'complete')
assert len(stormht['stories'])==1 and len(stormht['contacts'])==8 and sum(len(c['topics']) for c in stormht['contacts'])==16 and not stormht['exclusions']
assert not next(c for c in stormht['contacts'] if c['mob_vnum']==30868)['topics']
king=stormht['stories'][0];assert king['id']=='sultan-proof-for-the-crown' and king['contracts'][0]['giver_vnum']==30871
assert [(t['item_vnums'],t['count'],t.get('optional',False)) for t in king['steps'] if t['kind']=='carried_item']==[([40073],1,True)]
assert len(king['steps'])==2 and king['steps'][-1]['contracts']==king['contracts'] and report['eligible_by_zone']['308']==1

khildarak=next(m for m in catalog['story_mappings'] if m['source_area']=='khildarak')
assert (khildarak['schema_version'],khildarak['revision'],khildarak['coverage'])==(3,2,'complete')
assert len(khildarak['stories'])==2 and len(khildarak['contacts'])==6 and sum(len(c['topics']) for c in khildarak['contacts'])==22 and not khildarak['exclusions']
assert [s['id'] for s in khildarak['stories']]==['request-17118-e5eeeb25247d','request-17248-effe52f75b66']
assert [s['contracts'][0]['giver_vnum'] for s in khildarak['stories']]==[17118,17248]
assert [[t['item_vnums'][0] for t in s['steps'] if t['kind']=='carried_item'] for s in khildarak['stories']]==[[17074],[17022]]
assert sum(t.get('optional',False) for s in khildarak['stories'] for t in s['steps'])==2 and report['eligible_by_zone']['170']==2

labyrinth=next(m for m in catalog['story_mappings'] if m['source_area']=='labyrinth')
assert (labyrinth['schema_version'],labyrinth['revision'],labyrinth['coverage'])==(3,1,'complete')
assert len(labyrinth['stories'])==3 and len(labyrinth['contacts'])==8 and sum(len(c['topics']) for c in labyrinth['contacts'])==31 and not labyrinth['exclusions']
assert [s['contracts'][0]['giver_vnum'] for s in labyrinth['stories']]==[5024,5028,5055]
assert [[t['item_vnums'][0] for t in s['steps'] if t['kind']=='carried_item'] for s in labyrinth['stories']]==[[5014],[5001,5002,5003,5005,5006,5007,5008,5009,5020],[5061,5053,5058]]
assert sum(len(s['steps']) for s in labyrinth['stories'])==16 and report['eligible_by_zone']['50']==3

highway=next(m for m in catalog['story_mappings'] if m['source_area']=='highway')
assert (highway['schema_version'],highway['revision'],highway['coverage'])==(3,1,'complete')
assert len(highway['stories'])==3 and len(highway['contacts'])==10 and sum(len(c['topics']) for c in highway['contacts'])==25 and not highway['exclusions']
assert [s['contracts'][0]['giver_vnum'] for s in highway['stories']]==[41315,41360,41360]
assert [[t['item_vnums'][0] for t in s['steps'] if t['kind']=='carried_item'] for s in highway['stories']]==[[41398],[41348],[41415,41416]]
assert highway['stories'][0]['steps'][0]['contracts'][0]['giver_vnum']==7603 and highway['stories'][0]['steps'][0]['optional']
assert sum(len(s['steps']) for s in highway['stories'])==8 and report['eligible_by_zone']['413']==3

caves_skelenak=next(m for m in catalog['story_mappings'] if m['source_area']=='caves_skelenak')
assert (caves_skelenak['schema_version'],caves_skelenak['revision'],caves_skelenak['coverage'])==(3,1,'complete')
assert len(caves_skelenak['stories'])==9 and len(caves_skelenak['contacts'])==8 and sum(len(c['topics']) for c in caves_skelenak['contacts'])==17 and not caves_skelenak['exclusions']
assert [s['contracts'][0]['giver_vnum'] for s in caves_skelenak['stories']]==[4027]*3+[4038]*6
assert [[t['item_vnums'][0] for t in s['steps'][:-1]] for s in caves_skelenak['stories']]==[[4021],[4022],[4023],[4005,4025],[26438],[16071],[20604],[4016,4017],[26013]]
assert all(t['count']==1 and t['optional'] for s in caves_skelenak['stories'] for t in s['steps'][:-1]) and report['eligible_by_zone']['40']==9

yuan_ti=next(m for m in catalog['story_mappings'] if m['source_area']=='yuan_ti')
assert (yuan_ti['schema_version'],yuan_ti['revision'],yuan_ti['coverage'])==(3,1,'complete')
assert len(yuan_ti['stories'])==1 and len(yuan_ti['contacts'])==8 and sum(len(c['topics']) for c in yuan_ti['contacts'])==14 and not yuan_ti['exclusions']
assert yuan_ti['stories'][0]['category']=='story' and len(yuan_ti['stories'][0]['contracts'])==1
assert [(t['item_vnums'],t['count']) for t in yuan_ti['stories'][0]['steps'][:-1]]==[([80570],1)]
assert yuan_ti['stories'][0]['steps'][0]['optional'] and report['eligible_by_zone']['805']==1

earthp=next(m for m in catalog['story_mappings'] if m['source_area']=='earthp')
assert (earthp['schema_version'],earthp['revision'],earthp['coverage'])==(3,1,'complete')
assert len(earthp['stories'])==2 and len(earthp['contacts'])==6 and sum(len(c['topics']) for c in earthp['contacts'])==28 and not earthp['exclusions']
assert [s['category'] for s in earthp['stories']]==['story','story'] and all(len(s['contracts'])==1 for s in earthp['stories'])
assert [[(t['item_vnums'],t['count']) for t in s['steps'][:-1]] for s in earthp['stories']]==[[([400104],10)],[([131227],1)]]
assert all(t['optional'] for s in earthp['stories'] for t in s['steps'][:-1])
assert report['eligible_by_zone']['1312']==2

pyramid=next(m for m in catalog['story_mappings'] if m['source_area']=='pyramid')
assert (pyramid['schema_version'],pyramid['revision'],pyramid['coverage'])==(3,1,'complete')
assert len(pyramid['stories'])==3 and len(pyramid['contacts'])==4 and sum(len(c['topics']) for c in pyramid['contacts'])==11 and not pyramid['exclusions']
assert [s['category'] for s in pyramid['stories']]==['story','story','story']
assert [len(s['contracts']) for s in pyramid['stories']]==[1,1,1]
assert [[(t['item_vnums'],t['count']) for t in s['steps'][:-1]] for s in pyramid['stories']]==[[([20400],3)],[([20419],1)],[([20403],1),([20404],1),([20405],1),([20406],1)]]
assert all(t['optional'] for s in pyramid['stories'] for t in s['steps'][:-1])
assert report['eligible_by_zone']['204']==3

minopass=next(m for m in catalog['story_mappings'] if m['source_area']=='minopass')
assert (minopass['schema_version'],minopass['revision'],minopass['coverage'])==(3,1,'complete')
assert len(minopass['stories'])==4 and len(minopass['contacts'])==7 and sum(len(c['topics']) for c in minopass['contacts'])==23 and not minopass['exclusions']
assert [s['category'] for s in minopass['stories']]==['story','story','story','service']
assert [len(s['contracts']) for s in minopass['stories']]==[1,1,1,1]
assert [[t['item_vnums'] for t in s['steps'][:-1]] for s in minopass['stories']]==[[[94723]],[[94715]],[[88807],[4402]],[]]
assert all(t['optional'] and t['count']==1 for s in minopass['stories'] for t in s['steps'][:-1])
assert report['eligible_by_zone']['947']==3

ttowers=next(m for m in catalog['story_mappings'] if m['source_area']=='ttowers')
assert (ttowers['schema_version'],ttowers['revision'],ttowers['coverage'])==(3,1,'complete')
assert len(ttowers['stories'])==5 and len(ttowers['contacts'])==9 and sum(len(c['topics']) for c in ttowers['contacts'])==17 and not ttowers['exclusions']
assert [len(s['contracts']) for s in ttowers['stories']]==[1,1,1,3,1]
assert [[t['item_vnums'] for t in s['steps'][:-1]] for s in ttowers['stories']]==[[[13223]],[[13223]],[[13221],[13222]],[[13221,13223,13222]],[[13221],[13222],[13223]]]
assert all(t['optional'] and t['count']==1 for s in ttowers['stories'] for t in s['steps'][:-1])
assert report['eligible_by_zone']['132']==5

ruins=next(m for m in catalog['story_mappings'] if m['source_area']=='ruins')
assert (ruins['schema_version'],ruins['revision'],ruins['coverage'])==(3,1,'complete')
assert len(ruins['contacts'])==3 and sum(len(c['topics']) for c in ruins['contacts'])==17 and not ruins['exclusions']
assert [s['category'] for s in ruins['stories']]==['service','story']
assert [t['item_vnums'] for t in ruins['stories'][0]['steps'][:-1]]==[[98601],[98602],[98603],[98604]]
assert [t['item_vnums'] for t in ruins['stories'][1]['steps'][:-1]]==[[98642]]
assert all(t['kind']=='carried_item' and t['optional'] and t['count']==1 for s in ruins['stories'] for t in s['steps'][:-1])
assert all(s['contracts']==s['steps'][-1]['contracts'] for s in ruins['stories'])
u=[u for u in module.story_units(catalog) if u['zone_number']==986]
assert len(u)==2 and sum(x['achievement'] for x in u)==sum(x['daily_candidate'] for x in u)==1
assert report['eligible_by_zone']['986']==1
nh=next(m for m in catalog['story_mappings'] if m['source_area']=='newhaven')
assert nh['revision']==2 and 'PULL TABLE in the ruined shack' in next(s for s in nh['stories'] if s['id']=='vulgaris-veldian-collar')['steps'][0]['hint']

woodseer=next(m for m in catalog['story_mappings'] if m['source_area']=='woodseer')
assert (woodseer['schema_version'],woodseer['revision'],woodseer['coverage'])==(3,2,'complete')
assert len(woodseer['stories'])==len(woodseer['contacts'])==7 and not woodseer['exclusions']
assert report['eligible_by_zone']['165']==7 and sum(len(c['topics']) for c in woodseer['contacts'])==22
assert all(s['category']=='story' and len(s['steps'])==2 and s['steps'][0]['optional'] and s['contracts']==s['steps'][-1]['contracts'] for s in woodseer['stories'])
units=[u for u in module.story_units(catalog) if u['zone_number']==165];assert len(units)==7 and sum(u['achievement'] for u in units)==sum(u['daily_candidate'] for u in units)==7

forgotten=next(m for m in catalog['story_mappings'] if m['source_area']=='mansion')
assert (forgotten['schema_version'],forgotten['revision'],forgotten['coverage'])==(3,1,'complete')
assert len(forgotten['stories'])==3 and len(forgotten['contacts'])==4 and not forgotten['exclusions']
assert report['eligible_by_zone']['35']==3 and sum(len(c['topics']) for c in forgotten['contacts'])==25
assert all(s['category']=='story' for s in forgotten['stories']) and [len(s['steps']) for s in forgotten['stories']]==[2,2,3]
assert all(len(s['contracts'])==len(s['steps'][-1]['contracts'])==1 for s in forgotten['stories'])
units=[u for u in module.story_units(catalog) if u['zone_number']==35];assert len(units)==3 and sum(u['achievement'] for u in units)==sum(u['daily_candidate'] for u in units)==3

battle=next(m for m in catalog['story_mappings'] if m['source_area']=='battlefi')
assert (battle['schema_version'],battle['revision'],battle['coverage'])==(3,1,'complete')
assert len(battle['stories'])==7 and len(battle['contacts'])==6 and not battle['exclusions']
assert report['eligible_by_zone']['664']==6 and sum(len(c['topics']) for c in battle['contacts'])==36
assert [s['category'] for s in battle['stories']]==['story','service','story','story','story','story','story']
assert [len(s['steps']) for s in battle['stories']]==[2,2,5,2,2,2,2] and battle['stories'][1]['steps'][0]['count']==2
assert all(len(s['contracts'])==len(s['steps'][-1]['contracts'])==1 for s in battle['stories'])
units=[u for u in module.story_units(catalog) if u['zone_number']==664];assert len(units)==7 and sum(u['achievement'] for u in units)==sum(u['daily_candidate'] for u in units)==6

realm=next(m for m in catalog['story_mappings'] if m['source_area']=='connectorzones')
assert (realm['schema_version'],realm['revision'],realm['coverage'])==(3,1,'complete')
assert len(realm['stories'])==6 and len(realm['contacts'])==8 and not realm['exclusions']
assert report['eligible_by_zone']['536']==4 and sum(len(c['topics']) for c in realm['contacts'])==28
assert [s['category'] for s in realm['stories']]==['story','service','story','story','story','service']
assert [len(s['steps']) for s in realm['stories']]==[2,2,4,3,10,3] and sum(len(s['steps'])-1 for s in realm['stories'])==18
assert realm['stories'][1]['steps'][0]['count']==4 and len(realm['stories'][4]['contracts'])==3
assert len(realm['stories'][4]['steps'][-1]['contracts'])==3
assert all(len(s['contracts'])==1 for i,s in enumerate(realm['stories']) if i!=4)
units=[u for u in module.story_units(catalog) if u['zone_number']==536];assert len(units)==6 and sum(u['achievement'] for u in units)==sum(u['daily_candidate'] for u in units)==4

oasis=next(m for m in catalog['story_mappings'] if m['source_area']=='oasis')
assert (oasis['schema_version'],oasis['revision'],oasis['coverage'])==(3,1,'complete')
assert len(oasis['stories'])==9 and len(oasis['contacts'])==9 and not oasis['exclusions']
assert report['eligible_by_zone']['780']==3 and sum(len(c['topics']) for c in oasis['contacts'])==34
assert [s['category'] for s in oasis['stories']]==['story','story','service','service','service','service','service','service','story']
assert [len(s['steps']) for s in oasis['stories']]==[4,3,1,2,4,3,4,3,2]
assert sum(len(s['steps'])-1 for s in oasis['stories'])==17
for story in oasis['stories']:
 assert len(story['contracts'])==1 and story['steps'][-1]['contracts']==story['contracts']
 assert all(t['kind']=='carried_item' and t['optional'] and t['count']==1 for t in story['steps'][:-1])
units=[u for u in module.story_units(catalog) if u['zone_number']==780];assert len(units)==9 and sum(u['achievement'] for u in units)==sum(u['daily_candidate'] for u in units)==3

pharrvly=next(m for m in catalog['story_mappings'] if m['source_area']=='pharrvly')
assert (pharrvly['schema_version'],pharrvly['revision'],pharrvly['coverage'])==(3,1,'complete')
assert len(pharrvly['stories'])==2 and len(pharrvly['contacts'])==2 and not pharrvly['exclusions']
assert report['eligible_by_zone']['402']==2 and sum(len(c['topics']) for c in pharrvly['contacts'])==22
assert [len(s['steps']) for s in pharrvly['stories']]==[4,2]
assert [[t['item_vnums'] for t in s['steps'][:-1]] for s in pharrvly['stories']]==[[[40208],[40209],[40210]],[[40213]]]
for story in pharrvly['stories']:
 assert story['category']=='story' and len(story['contracts'])==1 and story['steps'][-1]['contracts']==story['contracts']
 assert all(t['kind']=='carried_item' and t['optional'] and t['count']==1 for t in story['steps'][:-1])
units=[u for u in module.story_units(catalog) if u['zone_number']==402];assert len(units)==2 and all(u['achievement'] and u['daily_candidate'] for u in units)

mistywood=next(m for m in catalog['story_mappings'] if m['source_area']=='mistywood')
assert (mistywood['schema_version'],mistywood['revision'],mistywood['coverage'])==(3,1,'complete')
assert len(mistywood['stories'])==3 and len(mistywood['contacts'])==5 and not mistywood['exclusions']
assert report['eligible_by_zone']['950']==3 and sum(len(c['topics']) for c in mistywood['contacts'])==37
for story in mistywood['stories']:
 assert story['category']=='story' and len(story['contracts'])==1 and len(story['steps'])==2
 assert story['steps'][0]['kind']=='carried_item' and story['steps'][0]['optional'] and story['steps'][0]['count']==1
 assert story['steps'][-1]['contracts']==story['contracts']
assert [s['steps'][0]['item_vnums'] for s in mistywood['stories']]==[[95000],[95001],[95003]]
units=[u for u in module.story_units(catalog) if u['zone_number']==950];assert len(units)==3 and all(u['achievement'] and u['daily_candidate'] for u in units)


library=next(m for m in catalog['story_mappings'] if m['source_area']=='library')
assert (library['schema_version'],library['revision'],library['coverage'])==(3,1,'complete')
assert len(library['stories'])==6 and len(library['contacts'])==11 and not library['exclusions']
assert report['eligible_by_zone']['4020']==5 and sum(len(c['topics']) for c in library['contacts'])==31
assert sum(len(s['steps'])-1 for s in library['stories'])==32
assert library['stories'][0]['category']=='service' and len(library['stories'][0]['steps'])==1
dream=library['stories'][4]
assert [s['item_vnums'] for s in dream['steps'][:-1]]==[[v] for v in range(402041,402049)]
assert len({s['text'] for s in dream['steps'][:-1]})==8
for story in library['stories']:
 assert story['steps'][-1]['contracts']==story['contracts'] and len(story['contracts'])==1
 assert all(s['kind']=='carried_item' and s['optional'] for s in story['steps'][:-1])
units=[u for u in module.story_units(catalog) if u['zone_number']==4020]
assert len(units)==6 and sum(u['achievement'] for u in units)==5 and sum(u['daily_candidate'] for u in units)==5


raxthan=next(m for m in catalog['story_mappings'] if m['source_area']=='raxthan')
assert (raxthan['schema_version'],raxthan['revision'],raxthan['coverage'])==(3,1,'complete')
assert len(raxthan['stories'])==10 and len(raxthan['contacts'])==9 and not raxthan['exclusions']
assert report['eligible_by_zone']['429']==10 and sum(len(c['topics']) for c in raxthan['contacts'])==25
assert sum(len(s['steps'])-1 for s in raxthan['stories'])==13
yr={s['id']:s for s in raxthan['stories']}
assert yr['grobklarn-three-shrooms']['steps'][0]['count']==3 and yr['grobklarn-three-shrooms']['steps'][0]['item_vnums']==[42927]
assert yr['trosat-strange-mushrooms']['steps'][0]['item_vnums']==[42926]
assert yr['dravkult-paired-proofs']['steps'][1]['item_vnums']==yr['trin-paired-hearts']['steps'][0]['item_vnums']==[42940]
assert yr['drustl-raxthan-head']['contracts'][0]['giver_vnum']==yr['drustl-lost-arrow']['contracts'][0]['giver_vnum']==42912
assert yr['drustl-raxthan-head']['contracts']!=yr['drustl-lost-arrow']['contracts']
for s in raxthan['stories']:
 assert s['category']=='story' and s['steps'][-1]['contracts']==s['contracts']
 assert all(t['kind']=='carried_item' and t['optional'] for t in s['steps'][:-1])
units=[u for u in module.story_units(catalog) if u['zone_number']==429]
assert len(units)==10 and all(u['achievement'] and u['daily_candidate'] for u in units)


tower = next(m for m in catalog['story_mappings'] if m['source_area'] == 'trnsptow')
assert tower['schema_version'] == 3 and tower['coverage'] == 'complete'
assert len(tower['stories']) == 4 and len(tower['contacts']) == 14 and not tower['exclusions']
assert sum(t.get('optional', False) for s in tower['stories'] for t in s['steps']) == 11
assert all(s['category'] == 'story' for s in tower['stories'])
assert mapping["schema_version"] == 3 and mapping["stories"][0]["steps"][0]["optional"]
assert sum(s["category"] == "service" for s in mapping["stories"]) == 12
assert len(mapping["exclusions"]) == 1 and len(mapping["exclusions"][0]["contracts"]) == 40
assert sum(len(s["contracts"]) for s in mapping["stories"]) == 44
assert all(s["category"] == "service" for s in mapping["stories"] if s["id"].startswith(("prepare-animal-", "archer-trade-")))
assert "arrows" in next(c for c in mapping["contacts"] if c["mob_vnum"] == 13501)["topics"]
assert "backpack" in next(c for c in mapping["contacts"] if c["mob_vnum"] == 13503)["topics"]

new_areas = {"breale", "elvish", "krimman", "bastine", "pineholl", "quietus", "torg", "solonar"}
smokev = next(m for m in catalog["story_mappings"] if m["source_area"] == "smokev")
assert smokev["schema_version"] == 3 and smokev["revision"] == 1
assert report["eligible_by_zone"]["202"] == 10 and len(smokev["stories"]) == 11
assert sum(s["category"] == "service" for s in smokev["stories"]) == 1
assert not smokev["exclusions"] and all(len(s["contracts"]) == 1 for s in smokev["stories"])
smokev_stories = {s["id"]: s for s in smokev["stories"]}
helm = smokev_stories["raltrons-helm-delivery"]
assert helm["steps"][0]["optional"] and helm["steps"][0]["contracts"] == smokev_stories["tarlators-humanity-request"]["contracts"]
assert helm["steps"][1]["item_vnums"] == [20252]
assert smokev_stories["forvos-bottle-exchange"]["category"] == "service"
assert all(smokev_stories[s]["category"] == "request" for s in ("dargast-scales-for-boots", "dolgars-scale-for-a-warvisor"))
assert [t["item_vnums"] for t in smokev_stories["the-two-dragon-hearts"]["steps"][:-1]] == [[20200], [20209]]
assert [t["item_vnums"] for t in smokev_stories["a-meal-for-azcatlipoca"]["steps"][:-1]] == [[20211]]
assert all(s["steps"][-1]["contracts"] == s["contracts"] for s in smokev["stories"])
keeps = next(m for m in catalog["story_mappings"] if m["source_area"] == "caertannad")
assert keeps["schema_version"] == 3 and keeps["revision"] == 1
assert report["eligible_by_zone"]["784"] == 29 and len(keeps["stories"]) == 30
assert not keeps["exclusions"] and sum(s["category"] == "service" for s in keeps["stories"]) == 1
keeps_stories = {s["id"]: s for s in keeps["stories"]}
shards = keeps_stories["four-distinct-life-shards"]
assert [t["item_vnums"] for t in shards["steps"] if t["kind"] == "carried_item"] == [[78499], [78513], [78514], [78515]]
assert all(t["count"] == 1 for t in shards["steps"] if t["kind"] == "carried_item")
endurium = keeps_stories["marnys-endurium-commission"]
assert endurium["steps"][0]["optional"]
assert [t["item_vnums"] for t in endurium["steps"] if t["kind"] == "carried_item"] == [[78424], [78463]]
staff = keeps_stories["the-staff-of-twin-worlds"]
assert [t["item_vnums"] for t in staff["steps"] if t["kind"] == "carried_item"] == [[78465], [78477], [78480], [78486], [78492]]
assert len(staff["contracts"]) == 1 and all(t.get("optional") for t in staff["steps"][:4])
assert keeps_stories["mungirs-silverleaf-remedy"]["category"] == "service"
assert len(keeps_stories["gremlin-claws-for-the-tower-key"]["steps"][0]["contracts"]) == 2
assert keeps_stories["gremlin-claws-for-the-tower-key"]["steps"][0]["optional"]
assert keeps_stories["hindiss-figurine-exchange"]["contracts"] != keeps_stories["hindiss-thel-samar-proof"]["contracts"]
assert keeps_stories["crowfoots-nether-ore"]["contracts"] != keeps_stories["crowfoots-basilisk-egg"]["contracts"]
assert all(s["steps"][-1]["contracts"] == s["contracts"] for s in keeps["stories"])
bloodstone = next(m for m in catalog["story_mappings"] if m["source_area"] == "bs")
assert bloodstone["schema_version"] == 3 and bloodstone["revision"] == 1
assert report["eligible_by_zone"]["740"] == 31
assert len(bloodstone["stories"]) == 63 and len(bloodstone["exclusions"]) == 2
assert sum(s["category"] == "service" for s in bloodstone["stories"]) == 32
assert sum(t.get("optional", False) for s in bloodstone["stories"] for t in s["steps"]) == 35
bs_stories = {s["id"]: s for s in bloodstone["stories"]}
quarters = bs_stories["the-four-bloodstone-quarters"]
assert [t["item_vnums"] for t in quarters["steps"] if t["kind"] == "carried_item"] == [[74259], [74260], [74261], [74262]]
wife = bs_stories["the-numbaca-ingredients"]
assert [t["item_vnums"][0] for t in wife["steps"] if t["kind"] == "carried_item"] == [74057, 74240, 74243, 74246, 74293]
assert wife["steps"][0]["optional"] and wife["steps"][0]["contracts"] == bs_stories["the-captains-missionary-proof"]["contracts"]
bs_storm = bs_stories["navift-commission-55315"]
assert [t["item_vnums"][0] for t in bs_storm["steps"] if t["kind"] == "carried_item"] == [55166, 55209, 55284, 55316, 55317]
assert len(bs_storm["steps"][0]["contracts"]) == 2 and bs_storm["steps"][0]["optional"]
bs_elixir = bs_stories["fibblefingers-planar-elixirs"]
assert len([t for t in bs_elixir["steps"] if t["kind"] == "carried_item"]) == 11
assert sum(t.get("optional", False) for t in bs_elixir["steps"]) == 5
earrings = [s for s in bloodstone["stories"] if s["id"].startswith("hedvig-commission-")]
assert len(earrings) == 9 and all(s["category"] == "service" for s in earrings)
assert [(s["steps"][1]["item_vnums"], s["steps"][1]["count"]) for s in earrings] == [([n], 2) for n in range(55352, 55361)]
assert all(s["steps"][-1]["contracts"] == s["contracts"] for s in bloodstone["stories"])
moria = next(m for m in catalog["story_mappings"] if m["source_area"] == "moria")
assert moria["schema_version"] == 3 and moria["revision"] == 1
assert report["eligible_by_zone"]["990"] == 1 and len(moria["stories"]) == 1
runes = moria["stories"][0]
assert runes["category"] == "story" and len(runes["contracts"]) == 5
assert runes["steps"][0]["optional"] and runes["steps"][0]["item_vnums"] == [99072]
assert [t["item_vnums"] for t in runes["steps"][1:-1]] == [[n] for n in range(99002, 99007)]
assert all(t["count"] == 1 for t in runes["steps"][1:-1])
assert runes["steps"][-1]["contracts"] == runes["contracts"] and not moria["exclusions"]
claw = next(m for m in catalog["story_mappings"] if m["source_area"] == "clwcvrn")
assert claw["schema_version"] == 3 and claw["revision"] == 1
assert report["eligible_by_zone"]["807"] == 1 and len(claw["stories"]) == 7
assert sum(s["category"] == "service" for s in claw["stories"]) == 6
assert len(claw["exclusions"]) == 1 and len(claw["exclusions"][0]["contracts"]) == 13
claw_final = claw["stories"][0]
assert [t["item_vnums"] for t in claw_final["steps"][:-1]] == [[80700], [80733], [80734]]
assert [t.get("optional", False) for t in claw_final["steps"][:-1]] == [True, True, False]
assert all(s["steps"][-1]["contracts"] == s["contracts"] for s in claw["stories"])
long = next(m for m in catalog["story_mappings"] if m["source_area"] == "long")
assert long["schema_version"] == 3 and long["revision"] == 1
assert report["eligible_by_zone"]["344"] == 9 and len(long["stories"]) == 14
assert sum(s["category"] == "story" for s in long["stories"]) == 4
assert sum(s["category"] == "request" for s in long["stories"]) == 5
assert sum(s["category"] == "service" for s in long["stories"]) == 5
assert sum(t.get("optional", False) for s in long["stories"] for t in s["steps"]) == 6
long_stories = {s["id"]: s for s in long["stories"]}
assert [t["item_vnums"] for t in long_stories["proof-against-the-siege-leaders"]["steps"][:-1]] == [[n] for n in range(34427, 34432)]
solar = long_stories["recognition-by-selunes-solar"]
assert solar["steps"][0]["optional"] and solar["steps"][1]["item_vnums"] == [34452]
assert long_stories["four-skins-for-snakeskin-boots"]["steps"][0]["count"] == 4
assert long_stories["two-furs-for-a-fox-scarf"]["steps"][0]["count"] == 2
assert long_stories["vipers-delight"]["steps"][0]["count"] == 2
mist = long_stories["the-shadowy-mist"]
assert all(t["optional"] for t in mist["steps"][:3])
assert [t["item_vnums"] for t in mist["steps"][3:-1]] == [[34443], [34444], [34445], [34439]]
blend = long_stories["the-kiss-of-talona-blend"]
assert all(t["optional"] for t in blend["steps"][:2])
assert [t["item_vnums"] for t in blend["steps"][2:-1]] == [[34449], [34447], [34438]]
assert all(s["steps"][-1]["contracts"] == s["contracts"] for s in long["stories"])
pearl = next(m for m in catalog["story_mappings"] if m["source_area"] == "blackpearl")
assert pearl["schema_version"] == 3 and pearl["revision"] == 1
assert len(pearl["stories"]) == 31 and not pearl["exclusions"]
assert {category: sum(s["category"] == category for s in pearl["stories"]) for category in ("story", "request", "service")} == {"story": 2, "request": 12, "service": 17}
assert report["eligible_by_zone"]["1422"] == 14
assert sum(t.get("optional", False) for s in pearl["stories"] for t in s["steps"]) == 25
pearl_stories = {s["id"]: s for s in pearl["stories"]}
reconstruction = pearl_stories["reconstruct-warthehrs-dragonslayer"]
assert all(t["optional"] for t in reconstruction["steps"][:7])
assert [t["item_vnums"] for t in reconstruction["steps"][7:-1]] == [[n] for n in (142204, 142205, 142206, 142208, 142209, 142224, 142226, 142227, 142228)]
assert [t["item_vnums"] for t in pearl_stories["four-distinct-horns-for-derimous"]["steps"][:-1]] == [[n] for n in range(142220, 142224)]
assert pearl_stories["warthehrs-letter-to-lyle"]["steps"][1]["item_vnums"] == [142213]
assert pearl_stories["abals-reply-for-lyles-fragment"]["steps"][1]["item_vnums"] == [142219]
assert all(s["steps"][-1]["contracts"] == s["contracts"] for s in pearl["stories"])
assert all(s["category"] == "service" for s in pearl["stories"] if s["contracts"][0]["giver_vnum"] in (142232, 142233, 142234))
raven = next(m for m in catalog["story_mappings"] if m["source_area"] == "ravenloft2")
assert raven["schema_version"] == 3 and raven["revision"] == 1
assert len(raven["stories"]) == 33 and not raven["exclusions"]
assert {category: sum(s["category"] == category for s in raven["stories"]) for category in ("story", "request", "service")} == {"story": 12, "request": 13, "service": 8}
assert report["eligible_by_zone"]["590"] == 25
assert sum(t.get("optional", False) for s in raven["stories"] for t in s["steps"]) == 14
raven_stories = {s["id"]: s for s in raven["stories"]}
roles = [raven_stories[id] for id in ("the-chaplains-favor", "the-whispering-blades-favor", "the-archmages-favor", "the-dread-guards-favor")]
assert [s["steps"][2]["item_vnums"] for s in roles] == [[59281], [59289], [59300], [59282]]
assert all(s["steps"][0]["optional"] and len(s["steps"][0]["contracts"]) == 8 for s in roles)
assert all(s["steps"][1]["item_vnums"] == [59202] and s["steps"][1]["count"] == 5 for s in roles)
blinsky = raven_stories["blinskys-clockwork-recovery"]
assert len(blinsky["contracts"]) == 5 and blinsky["steps"][0]["item_vnums"] == [59093, 59126, 59255, 59254, 59283]
assert blinsky["steps"][0]["count"] == 1
assert all(s["steps"][-1]["contracts"] == s["contracts"] for s in raven["stories"])
assert all(raven_stories[id]["category"] == "service" for id in ("ezmereldas-paid-reading", "izeks-paid-dues", "janders-ant-meat", "rahadins-spectral-key"))
assert sum(u["daily_candidate"] for u in module.story_units(catalog) if u["zone_number"] == 590) == 18
new_mappings = {m["source_area"]: m for m in catalog["story_mappings"] if m["source_area"] in new_areas}
assert set(new_mappings) == new_areas and all(m["coverage"] == "complete" for m in new_mappings.values())
assert sum(d["source_area"] in new_areas for d in catalog["definitions"]) == 86
assert sum(len(m["stories"]) for m in new_mappings.values()) == 80
drider = next(s for s in new_mappings["elvish"]["stories"] if s["title"] == "Release the Cursed Drider")
assert len(drider["contracts"]) == 1 and drider["steps"][-2]["item_vnums"] == [35813]
assert len(drider["steps"]) == 6 and all(t["optional"] for t in drider["steps"][:-2])
assert drider["steps"][0]["item_vnums"] == [35824]  # Exact access key, distinct from the reward key.
breale = new_mappings["breale"]
assert breale["schema_version"] == new_mappings["elvish"]["schema_version"] == 3
assert breale["revision"] == new_mappings["elvish"]["revision"] == 2
triad = next(s for s in breale["stories"] if s["id"] == "finish-the-triad-mixture-2602")
assert len(triad["contracts"]) == 1 and sum(t.get("optional", False) for t in triad["steps"]) == 5
assert all(len(t["contracts"]) == 1 for t in triad["steps"] if t["kind"] == "completion")
assert report["eligible_by_zone"]["26"] == 6 and report["eligible_by_zone"]["358"] == 2
promotions = [s for s in new_mappings["bastine"]["stories"] if s["title"].startswith("The Bastine Road:")]
assert len(promotions) == 12 and all(len(s["contracts"]) == 1 for s in promotions)
assert new_mappings["bastine"]["schema_version"] == new_mappings["krimman"]["schema_version"] == 3
assert new_mappings["bastine"]["revision"] == new_mappings["krimman"]["revision"] == 2
assert [next(t["item_vnums"][0] for t in s["steps"] if t["kind"] == "carried_item" and not t.get("optional")) for s in promotions] == [
    41388, 41407, 12802, 41327, 41924, 2607, 41408, 41922, 41920, 41375, 41411, 70970]
assert sum(t.get("optional", False) for t in promotions[-1]["steps"]) == 11
assert report["eligible_by_zone"]["164"] == 8 and report["eligible_by_zone"]["76"] == 14
pineholl = new_mappings["pineholl"]
assert pineholl["schema_version"] == 2 and pineholl["revision"] == 2
assert report["eligible_by_zone"]["160"] == 7 and len(pineholl["stories"]) == 7
clothing = [s for s in pineholl["stories"] if s["contracts"][0]["giver_vnum"] == 16080]
assert [(s["steps"][0]["item_vnums"], s["steps"][0]["count"]) for s in clothing] == [
    ([16019], 3), ([16020], 3), ([16021], 2), ([16024], 3), ([16025], 3)]
assert all(s["category"] == "request" and len(s["contracts"]) == 1 and len(s["steps"]) == 2 for s in clothing)
assert all(len(s["contracts"]) == 1 and s["steps"][-1]["contracts"] == s["contracts"] for s in pineholl["stories"])
assert all(s["category"] == "service" for s in new_mappings["quietus"]["stories"] if "Credentials" in s["title"] or s["title"].startswith("Hear ") or s["title"].startswith("Obtain "))
credentials = [s for s in new_mappings["quietus"]["stories"] if s["steps"][0].get("item_vnums") == [1701, 80808]]
assert len(credentials) == 5 and all("badge" in s["steps"][0]["text"] and "longsword" in s["steps"][0]["text"] for s in credentials)
quietus = new_mappings["quietus"]
assert quietus["schema_version"] == 3 and quietus["revision"] == 2
assert report["eligible_by_zone"]["17"] == 4
assert len(quietus["stories"]) == 11 and sum(s["category"] == "service" for s in quietus["stories"]) == 7
assert sum(len(s["contracts"]) for s in quietus["stories"]) == 16 and not quietus["exclusions"]
quietus_missions = [s for s in quietus["stories"] if s["category"] == "story"]
assert len(quietus_missions) == 4 and all(len(s["contracts"]) == 1 for s in quietus_missions)
assert [sum(t.get("optional", False) for t in s["steps"]) for s in quietus_missions] == [1, 1, 1, 2]
assert [t["item_vnums"] for t in quietus_missions[0]["steps"] if t["kind"] == "carried_item"] == [[1732], [1746]]
assert all(s["steps"][-1]["contracts"] == s["contracts"] for s in quietus_missions)
secret_briefing = quietus_missions[-1]["steps"][1]
assert secret_briefing["optional"] and len(secret_briefing["contracts"]) == 2
assert {c["completion_key"] for c in secret_briefing["contracts"]} == {
    "give=I:16429;receive=;disappear=0", "give=I:1747;receive=I:1747;disappear=0"}
assert len(quietus["contacts"]) == 10
assert next(c for c in quietus["contacts"] if c["mob_vnum"] == 1709)["topics"] == ["aresliean", "quest"]
assert next(c for c in quietus["contacts"] if c["mob_vnum"] == 1751)["topics"] == ["name", "aresliean", "darvanu"]
chisel = next(s for s in new_mappings["torg"]["stories"] if s["id"] == "a-fine-chisel-for-the-craftsman")
assert {c["giver_vnum"] for c in chisel["contracts"]} == {29023, 29024}
assert len(chisel["steps"]) == 2 and chisel["steps"][0]["count"] == 1
torg = new_mappings["torg"]
assert torg["schema_version"] == 3 and torg["revision"] == 2
assert report["eligible_by_zone"]["289"] == 12 and len(torg["stories"]) == 14
assert sum(s["category"] == "service" for s in torg["stories"]) == 2
assert sum(len(s["contracts"]) for s in torg["stories"]) == 15 and not torg["exclusions"]
assert len(torg["contacts"]) == 12
assert next(c for c in torg["contacts"] if c["mob_vnum"] == 28975)["topics"] == []
assert all(s["steps"][-1]["contracts"] == s["contracts"] for s in torg["stories"])
for prefix, optional in (("a-dracolich-hide-", 1), ("a-secret-rose-delivery-", 2), ("an-obsidian-buckle-", 1)):
    story = next(s for s in torg["stories"] if s["id"].startswith(prefix))
    assert sum(t.get("optional", False) for t in story["steps"]) == optional
rings = next(s for s in torg["stories"] if s["id"] == "evidence-of-a-secret-affair-28932")
assert [t["item_vnums"] for t in rings["steps"] if t["kind"] == "carried_item"] == [[28916], [28938]]
legends = next(s for s in torg["stories"] if s["id"] == "relics-of-the-eight-legends-28964")
assert [t["item_vnums"][0] for t in legends["steps"][:-1]] == list(range(28944, 28952))
assert all(t["count"] == 1 for t in legends["steps"][:-1])
buckle = next(s for s in torg["stories"] if s["id"] == "an-obsidian-buckle-29024")
assert {c["giver_vnum"] for c in buckle["contracts"]} == {29024}
assert [t["item_vnums"] for t in buckle["steps"] if t["kind"] == "carried_item"] == [[28962], [28982]]
solonar = new_mappings["solonar"]
assert solonar["schema_version"] == 3 and solonar["revision"] == 2
assert report["eligible_by_zone"]["306"] == 5 and len(solonar["stories"]) == 15
assert sum(s["category"] == "service" for s in solonar["stories"]) == 10
assert sum(len(s["contracts"]) for s in solonar["stories"]) == 15 and not solonar["exclusions"]
assert len(solonar["contacts"]) == 14
assert all(s["steps"][-1]["contracts"] == s["contracts"] for s in solonar["stories"])
for prefix, optional in (("robes-of-", 5), ("a-piwafwi-", 2), ("forge-mage-", 2), ("prepare-an-ancient-", 2)):
    story = next(s for s in solonar["stories"] if s["id"].startswith(prefix))
    assert sum(t.get("optional", False) for t in story["steps"]) == optional
mage_bane = next(s for s in solonar["stories"] if s["id"] == "forge-mage-bane-30638")
assert [t["item_vnums"] for t in mage_bane["steps"] if t["kind"] == "carried_item"] == [[30659], [30662], [30664]]
piwafwi = next(s for s in solonar["stories"] if s["id"] == "a-piwafwi-of-power-30604")
assert [t["item_vnums"] for t in piwafwi["steps"] if t["kind"] == "carried_item"] == [[30634], [30635], [30636], [30649], [30666]]
scroll = next(s for s in solonar["stories"] if s["id"] == "prepare-an-ancient-scroll-30617")
assert scroll["contracts"][0]["completion_key"].endswith("receive=I:30666,I:30666;disappear=0")
grove_definitions = [d for d in catalog["definitions"] if d["source_area"] == "solonar"]
assert {d["giver_vnum"] for d in grove_definitions if d["daily_exclusion"] == "Unsupported durable offering"} == {30600, 30603, 30604}
assert next(c for c in solonar["contacts"] if c["mob_vnum"] == 30638)["keyword"] == "valin"
family = next(s for s in new_mappings["krimman"]["stories"] if s["title"] == "Release the Haunted Family")
assert len(family["contracts"]) == 1 and len(family["steps"]) == 9
assert all(t["optional"] for t in family["steps"][:-4])
assert [t["item_vnums"] for t in family["steps"][-4:-1]] == [[16452], [16453], [16454]]

ailvio = next(m for m in catalog["story_mappings"] if m["source_area"] == "newbie")
assert len(ailvio["stories"]) == 39 and report["eligible_by_zone"]["292"] == 22
feeding = next(s for s in ailvio["stories"] if s["id"] == "feed-the-ailing-family")
# Every valid native pair is covered, including two fish of the same kind.
import itertools
fishes = [293, 294, 295, 318, 319, 330, 332, 333, 334, 335, 355, 356]
pairs = {tuple(int(g[2:]) for g in c["completion_key"].split(";")[0][5:].split(",")) for c in feeding["contracts"]}
assert len(feeding["contracts"]) == 78 and pairs == set(itertools.combinations_with_replacement(fishes, 2))
assert feeding["steps"][0]["item_vnums"] == fishes and feeding["steps"][0]["count"] == 2
assert all(c["completion_key"].endswith("receive=I:29223;disappear=1") for c in feeding["contracts"])
assert sum(s["category"] == "service" for s in ailvio["stories"]) == 17
cleric = next(s for s in ailvio["stories"] if s["id"] == "request-29238-c2cf98d3f50e")
assert all(t["optional"] for t in cleric["steps"] if t["kind"] == "completion" and t["id"] != "turn-in")
assert next(c for c in ailvio["contacts"] if c["mob_vnum"] == 29233)["topics"] == ["quest"]
assert "item" in next(c for c in ailvio["contacts"] if c["mob_vnum"] == 29237)["topics"]
mansion = next(m for m in catalog["story_mappings"] if m["source_area"] == "braddistock")
assert report["eligible_by_zone"]["13"] == 1 and not mansion["exclusions"]
assert [(s["id"], s["category"]) for s in mansion["stories"]] == [("release-slippers", "service"), ("quiet-the-mansion", "story")]
assert all(t["optional"] for t in mansion["stories"][1]["steps"][:2])

coverage_spec = importlib.util.spec_from_file_location("home_coverage", ROOT / "scripts/zone_story_quest_home_coverage.py")
coverage = importlib.util.module_from_spec(coverage_spec)
coverage_spec.loader.exec_module(coverage)
required = coverage.required_areas(ROOT)
assert len(required["areas"]) == 27 and required["excluded_empty_town_markers"] == ["end"]
assert all((ROOT / r["mapping"]).is_file() for r in required["areas"])
assert all(next(m for m in catalog["story_mappings"] if m["source_area"] == r["source_area"])["schema_version"] in (2, 3) for r in required["areas"])

with tempfile.TemporaryDirectory(prefix="duris-authored-story-") as temporary:
    binary = pathlib.Path(temporary) / "story_test"
    subprocess.run([
        "g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-Isrc",
        "tests/async/zone_story_quest_story_harness.cpp",
        "src/world/zone_story_quest_tracking.c", "src/world/zone_story_quest_catalog.c",
        "src/world/zone_story_quest_story.c", "src/world/zone_story_quest_feature.c",
        "-lcjson", "-o", str(binary),
    ], cwd=ROOT, check=True)
    native = ROOT / "docs/reference/ZONE_STORY_QUEST_PRODUCTION_CATALOG.json"
    subprocess.run([str(binary), str(native), str(ROOT / "areas/story/twin_towers_forest.story.json")], cwd=ROOT, check=True)
    subprocess.run([str(binary), str(native), str(ROOT / "areas/story/twin_towers_forest.story.json"), "all"], cwd=ROOT, check=True)
    boundary_dir = pathlib.Path(temporary) / "sidecar-boundary"
    boundary_dir.mkdir()
    boundary_path = boundary_dir / "twin_towers_forest.story.json"
    encoded = json.dumps(mapping).encode("utf-8")
    boundary_path.write_bytes(encoded + b" " * (module.MAX_STORY_MAPPING_BYTES - len(encoded)))
    subprocess.run([str(binary), str(native), str(boundary_path), "boundary"], cwd=ROOT, check=True)
    with boundary_path.open("ab") as output:
        output.write(b" ")
    subprocess.run([str(binary), str(native), str(boundary_path), "oversized"], cwd=ROOT, check=True)
    for version in (1, 2):
        legacy_mapping = copy.deepcopy(mapping)
        legacy_mapping["schema_version"] = version
        for story in legacy_mapping["stories"]:
            for step in story["steps"]:
                step.pop("optional", None)
        if version == 1:
            for key in ("introduction", "orientation", "contacts"):
                legacy_mapping.pop(key)
        legacy_path = pathlib.Path(temporary) / f"schema-{version}.json"
        legacy_path.write_text(json.dumps(legacy_mapping), encoding="utf-8")
        subprocess.run([str(binary), str(native), str(legacy_path)], cwd=ROOT, check=True)

    partial = copy.deepcopy(mapping)
    partial["coverage"] = "partial"
    partial["stories"] = partial["stories"][:1]
    partial["exclusions"] = []
    for mode in ("partial", "service"):
        if mode == "service":
            partial["stories"][0]["category"] = "service"
        path = pathlib.Path(temporary) / f"{mode}.json"
        path.write_text(json.dumps(partial), encoding="utf-8")
        candidate = copy.deepcopy(catalog)
        candidate["story_mappings"] = [partial]
        assert module.report_for(candidate)["eligible_by_zone"]["135"] == (76 if mode == "service" else 77)
        subprocess.run([str(binary), str(native), str(path), mode], cwd=ROOT, check=True)

    def invalid(name, mutate):
        bad = copy.deepcopy(mapping)
        mutate(bad)
        candidate = copy.deepcopy(catalog)
        candidate["story_mappings"] = [bad]
        assert any(item["code"] == "invalid_story_mapping" for item in module.validate_catalog(candidate)), name
        path = pathlib.Path(temporary) / f"{name}.json"
        path.write_text(json.dumps(bad), encoding="utf-8")
        subprocess.run([str(binary), str(native), str(path), "invalid"], cwd=ROOT, check=True)

    invalid("unknown-contract", lambda m: m["stories"][0]["contracts"][0].update(giver_vnum=999999))
    invalid("duplicate-binding", lambda m: m["stories"][1]["contracts"].append(m["stories"][0]["contracts"][0]))
    invalid("incomplete-coverage", lambda m: m["exclusions"].pop())
    invalid("unknown-field", lambda m: m.update(rewards=10))
    invalid("untracked-event", lambda m: m["stories"][0]["steps"][0].update(kind="recover_item"))
    invalid("wrong-slot", lambda m: m["stories"][0]["steps"][0].update(slot=43))
    invalid("invalid-count", lambda m: m["stories"][0]["steps"][0].update(count=True))
    invalid("duplicate-item", lambda m: m["stories"][0]["steps"][0]["item_vnums"].append(13521))
    invalid("duplicate-step", lambda m: m["stories"][0]["steps"].append(m["stories"][0]["steps"][0]))
    invalid("control-text", lambda m: m["stories"][0].update(summary="bad\ntext"))
    invalid("wrong-area", lambda m: m.update(source_area="alatorin"))
    invalid("duplicate-contact", lambda m: m["contacts"].append(m["contacts"][0]))
    invalid("invalid-keyword", lambda m: m["contacts"][0].update(keyword="alvinar hello"))
    invalid("duplicate-topic", lambda m: m["contacts"][0]["topics"].append(m["contacts"][0]["topics"][0]))
    invalid("invalid-orientation", lambda m: m["orientation"].append("bad\ncommand"))
    invalid("invalid-optional", lambda m: m["stories"][0]["steps"][0].update(optional=1))
    invalid("old-schema-optional", lambda m: m.update(schema_version=2))
    unknown_item = copy.deepcopy(catalog)
    unknown_item["story_mappings"] = [copy.deepcopy(mapping)]
    unknown_item["story_mappings"][0]["stories"][0]["steps"][0]["item_vnums"] = [999999]
    try:
        module.story_units(unknown_item, {13521})
    except ValueError:
        pass
    else:
        raise AssertionError("unknown object prototype was accepted")
    duplicate = pathlib.Path(temporary) / "duplicate-field.json"
    duplicate.write_text(json.dumps(mapping).replace('"schema_version": 3', '"schema_version": 3, "schema_version": 3'), encoding="utf-8")
    subprocess.run([str(binary), str(native), str(duplicate), "invalid"], cwd=ROOT, check=True)

print("builder-authored zone story schema and projection regression passed")
