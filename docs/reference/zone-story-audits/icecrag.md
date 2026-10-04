# IceCrag Castle: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence icecrag \
  --evidence-format markdown --output docs/reference/zone-story-audits/icecrag.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 97001 | `give=C:1000;receive=I:97005;disappear=1` | service: The cleaning crew: buy a guard-walk key | [areas/qst/icecrag.qst:22](../../../areas/qst/icecrag.qst#L22) |
| 97002 | `give=I:11606,I:11607;receive=E:50000,I:97029;disappear=1` | story: The ice artist: replace his tools | [areas/qst/icecrag.qst:61](../../../areas/qst/icecrag.qst#L61) |
| 97006 | `give=I:303,I:315,I:6551,I:11519,I:15252,I:16025,I:66058;receive=I:97110,I:97136,I:97136,I:97136;disappear=0` | Excluded: Masha’s cuisine recipe requires missing item prototype 6551 and includes a fox pelt and spell parchment despite food dialogue. Keep it excluded from achievements and daily quests until a builder selects and qualifies a complete recipe; do not invent replacement ingredients. | [areas/qst/icecrag.qst:141](../../../areas/qst/icecrag.qst#L141) |
| 97008 | `give=I:97137,I:97138,I:97149;receive=I:97139;disappear=1` | story: The ice priest: recover all three speech pages | [areas/qst/icecrag.qst:180](../../../areas/qst/icecrag.qst#L180) |
| 97010 | `give=C:800;receive=I:97146,I:97146;disappear=1` | service: The servant: buy two milk casks | [areas/qst/icecrag.qst:226](../../../areas/qst/icecrag.qst#L226) |
| 97014 | `give=I:90017,I:90017,I:92048,I:92048;receive=C:75000,E:30000,I:97141;disappear=1` | story: The raucous guest: replace four wine bottles | [areas/qst/icecrag.qst:309](../../../areas/qst/icecrag.qst#L309) |
| 97020 | `give=C:25000,I:97041,I:97047,I:97048;receive=I:97143,I:97145;disappear=1` | story: The sergeant: bring winter clothing | [areas/qst/icecrag.qst:365](../../../areas/qst/icecrag.qst#L365) |
| 97021 | `give=I:97006;receive=I:97144;disappear=1` | story: The commander: return his borrowed book | [areas/qst/icecrag.qst:429](../../../areas/qst/icecrag.qst#L429) |
| 97023 | `give=I:97115;receive=I:97135;disappear=1` | story: The Viscount: bring the kitchen onion | [areas/qst/icecrag.qst:483](../../../areas/qst/icecrag.qst#L483) |
| 97029 | `give=I:97029;receive=I:97140;disappear=1` | story: The Siege Master: supply calf-skin shoes | [areas/qst/icecrag.qst:539](../../../areas/qst/icecrag.qst#L539) |
| 97039 | `give=I:97016,I:97017;receive=C:250000,I:55032;disappear=0` | story: Myrke: bring the Captain’s and Strife’s hearts | [areas/qst/icecrag.qst:589](../../../areas/qst/icecrag.qst#L589) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 97001 | crew icecrag | [areas/qst/icecrag.qst:2](../../../areas/qst/icecrag.qst#L2) |
| 97001 | responsibilities | [areas/qst/icecrag.qst:8](../../../areas/qst/icecrag.qst#L8) |
| 97001 | sell | [areas/qst/icecrag.qst:14](../../../areas/qst/icecrag.qst#L14) |
| 97002 | shoe shoes calf calfskin calfskin | [areas/qst/icecrag.qst:32](../../../areas/qst/icecrag.qst#L32) |
| 97002 | work ice sculpture girl art | [areas/qst/icecrag.qst:40](../../../areas/qst/icecrag.qst#L40) |
| 97002 | she magnificent finish | [areas/qst/icecrag.qst:45](../../../areas/qst/icecrag.qst#L45) |
| 97002 | tools | [areas/qst/icecrag.qst:56](../../../areas/qst/icecrag.qst#L56) |
| 97006 | onions onion | [areas/qst/icecrag.qst:74](../../../areas/qst/icecrag.qst#L74) |
| 97006 | castle icecrag | [areas/qst/icecrag.qst:78](../../../areas/qst/icecrag.qst#L78) |
| 97006 | strife icess | [areas/qst/icecrag.qst:83](../../../areas/qst/icecrag.qst#L83) |
| 97006 | kitchen cook chef food | [areas/qst/icecrag.qst:92](../../../areas/qst/icecrag.qst#L92) |
| 97006 | project interest | [areas/qst/icecrag.qst:99](../../../areas/qst/icecrag.qst#L99) |
| 97006 | archivist | [areas/qst/icecrag.qst:113](../../../areas/qst/icecrag.qst#L113) |
| 97006 | tar | [areas/qst/icecrag.qst:119](../../../areas/qst/icecrag.qst#L119) |
| 97006 | malice | [areas/qst/icecrag.qst:127](../../../areas/qst/icecrag.qst#L127) |
| 97008 | siege viscount strife malice archivist masha treasury castle icecrag | [areas/qst/icecrag.qst:160](../../../areas/qst/icecrag.qst#L160) |
| 97008 | speech notes | [areas/qst/icecrag.qst:165](../../../areas/qst/icecrag.qst#L165) |
| 97010 | icecrag castle servant job | [areas/qst/icecrag.qst:202](../../../areas/qst/icecrag.qst#L202) |
| 97010 | banquet | [areas/qst/icecrag.qst:208](../../../areas/qst/icecrag.qst#L208) |
| 97010 | more life somewhere | [areas/qst/icecrag.qst:213](../../../areas/qst/icecrag.qst#L213) |
| 97010 | sell milk | [areas/qst/icecrag.qst:221](../../../areas/qst/icecrag.qst#L221) |
| 97014 | guest icecrag castle | [areas/qst/icecrag.qst:237](../../../areas/qst/icecrag.qst#L237) |
| 97014 | banquet strife icess | [areas/qst/icecrag.qst:241](../../../areas/qst/icecrag.qst#L241) |
| 97014 | specialty trade | [areas/qst/icecrag.qst:254](../../../areas/qst/icecrag.qst#L254) |
| 97014 | magic enchantment | [areas/qst/icecrag.qst:262](../../../areas/qst/icecrag.qst#L262) |
| 97014 | gem stones gold ore | [areas/qst/icecrag.qst:266](../../../areas/qst/icecrag.qst#L266) |
| 97014 | wine | [areas/qst/icecrag.qst:273](../../../areas/qst/icecrag.qst#L273) |
| 97014 | stolen shipment | [areas/qst/icecrag.qst:283](../../../areas/qst/icecrag.qst#L283) |
| 97014 | caravan information | [areas/qst/icecrag.qst:293](../../../areas/qst/icecrag.qst#L293) |
| 97020 | icecrag castle | [areas/qst/icecrag.qst:332](../../../areas/qst/icecrag.qst#L332) |
| 97020 | banquet strife icess | [areas/qst/icecrag.qst:339](../../../areas/qst/icecrag.qst#L339) |
| 97020 | storm delayed guests | [areas/qst/icecrag.qst:344](../../../areas/qst/icecrag.qst#L344) |
| 97020 | freezing frost cold | [areas/qst/icecrag.qst:352](../../../areas/qst/icecrag.qst#L352) |
| 97021 | book | [areas/qst/icecrag.qst:386](../../../areas/qst/icecrag.qst#L386) |
| 97021 | lost | [areas/qst/icecrag.qst:392](../../../areas/qst/icecrag.qst#L392) |
| 97021 | mountains trek | [areas/qst/icecrag.qst:400](../../../areas/qst/icecrag.qst#L400) |
| 97021 | perch natural countryside | [areas/qst/icecrag.qst:409](../../../areas/qst/icecrag.qst#L409) |
| 97021 | prophecy fate nonsense non-sense | [areas/qst/icecrag.qst:416](../../../areas/qst/icecrag.qst#L416) |
| 97021 | man fellow strange | [areas/qst/icecrag.qst:422](../../../areas/qst/icecrag.qst#L422) |
| 97023 | strife malice archivist masha treasury castle icecrag | [areas/qst/icecrag.qst:444](../../../areas/qst/icecrag.qst#L444) |
| 97023 | financial business | [areas/qst/icecrag.qst:451](../../../areas/qst/icecrag.qst#L451) |
| 97023 | research | [areas/qst/icecrag.qst:463](../../../areas/qst/icecrag.qst#L463) |
| 97023 | alchemy | [areas/qst/icecrag.qst:469](../../../areas/qst/icecrag.qst#L469) |
| 97023 | sample | [areas/qst/icecrag.qst:476](../../../areas/qst/icecrag.qst#L476) |
| 97029 | castle strife icecrag | [areas/qst/icecrag.qst:499](../../../areas/qst/icecrag.qst#L499) |
| 97029 | pouch | [areas/qst/icecrag.qst:506](../../../areas/qst/icecrag.qst#L506) |
| 97029 | hobby hobbies | [areas/qst/icecrag.qst:518](../../../areas/qst/icecrag.qst#L518) |
| 97029 | artist calf skin calf-skin pair | [areas/qst/icecrag.qst:530](../../../areas/qst/icecrag.qst#L530) |
| 97039 | captain | [areas/qst/icecrag.qst:561](../../../areas/qst/icecrag.qst#L561) |
| 97039 | killed | [areas/qst/icecrag.qst:567](../../../areas/qst/icecrag.qst#L567) |
| 97039 | bodyrush vial | [areas/qst/icecrag.qst:583](../../../areas/qst/icecrag.qst#L583) |
| 97063 | book | [areas/qst/icecrag.qst:600](../../../areas/qst/icecrag.qst#L600) |
| 97063 | commander | [areas/qst/icecrag.qst:621](../../../areas/qst/icecrag.qst#L621) |
| 97063 | home | [areas/qst/icecrag.qst:636](../../../areas/qst/icecrag.qst#L636) |
| 97063 | hi hello | [areas/qst/icecrag.qst:669](../../../areas/qst/icecrag.qst#L669) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 97000 | `ice_snooty_wife` | [src/specs/specs.assign.c:1105](../../../src/specs/specs.assign.c#L1105) |
| mob | 97001 | `ice_cleaning_crew` | [src/specs/specs.assign.c:1106](../../../src/specs/specs.assign.c#L1106) |
| mob | 97002 | `ice_artist` | [src/specs/specs.assign.c:1107](../../../src/specs/specs.assign.c#L1107) |
| mob | 97005 | `ice_privates` | [src/specs/specs.assign.c:1108](../../../src/specs/specs.assign.c#L1108) |
| mob | 97006 | `ice_masha` | [src/specs/specs.assign.c:1109](../../../src/specs/specs.assign.c#L1109) |
| mob | 97007 | `ice_tubby_merchant` | [src/specs/specs.assign.c:1110](../../../src/specs/specs.assign.c#L1110) |
| mob | 97008 | `ice_priest` | [src/specs/specs.assign.c:1111](../../../src/specs/specs.assign.c#L1111) |
| mob | 97011 | `ice_garden_attendant` | [src/specs/specs.assign.c:1112](../../../src/specs/specs.assign.c#L1112) |
| mob | 97014 | `ice_raucous_guest` | [src/specs/specs.assign.c:1113](../../../src/specs/specs.assign.c#L1113) |
| mob | 97016 | `ice_tar` | [src/specs/specs.assign.c:1114](../../../src/specs/specs.assign.c#L1114) |
| mob | 97021 | `ice_commander` | [src/specs/specs.assign.c:1115](../../../src/specs/specs.assign.c#L1115) |
| mob | 97023 | `ice_viscount` | [src/specs/specs.assign.c:1116](../../../src/specs/specs.assign.c#L1116) |
| mob | 97028 | `ice_masonary_crew` | [src/specs/specs.assign.c:1117](../../../src/specs/specs.assign.c#L1117) |
| mob | 97033 | `ice_impatient_guest` | [src/specs/specs.assign.c:1118](../../../src/specs/specs.assign.c#L1118) |
| mob | 97040 | `ice_bodyguards` | [src/specs/specs.assign.c:1123](../../../src/specs/specs.assign.c#L1123) |
| mob | 97041 | `ice_bodyguards` | [src/specs/specs.assign.c:1124](../../../src/specs/specs.assign.c#L1124) |
| mob | 97042 | `ice_bodyguards` | [src/specs/specs.assign.c:1125](../../../src/specs/specs.assign.c#L1125) |
| mob | 97030 | `ice_wolf` | [src/specs/specs.assign.c:1126](../../../src/specs/specs.assign.c#L1126) |
| mob | 97003 | `ice_malice` | [src/specs/specs.assign.c:1127](../../../src/specs/specs.assign.c#L1127) |
| obj | 97118 | `artifact_hide` | [src/specs/specs.assign.c:1395](../../../src/specs/specs.assign.c#L1395) |

## Reset coverage

620 parsed reset commands: D: 148, E: 247, F: 4, G: 37, M: 150, O: 30, P: 4.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
