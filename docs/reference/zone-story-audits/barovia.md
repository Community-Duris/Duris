# The Realm of Barovia: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence barovia \
  --evidence-format markdown --output docs/reference/zone-story-audits/barovia.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 91006 | `give=I:91021;receive=I:91020;disappear=0` | story: Ashlyn's Lost Companions | [areas/qst/barovia.qst:114](../../../areas/qst/barovia.qst#L114) |
| 91007 | `give=I:91015,I:91018,I:91026,I:91027,I:91042,I:91043,I:91044,I:91045,I:91046;receive=I:91047;disappear=0` | story: Bildrath's Nine Trinkets | [areas/qst/barovia.qst:169](../../../areas/qst/barovia.qst#L169) |
| 91011 | `give=I:91022;receive=C:200000;disappear=0` | request: Kolyan's Forged Letter | [areas/qst/barovia.qst:313](../../../areas/qst/barovia.qst#L313) |
| 91011 | `give=I:91038;receive=I:91039;disappear=0` | story: Hossa's Ambush Plan | [areas/qst/barovia.qst:332](../../../areas/qst/barovia.qst#L332) |
| 91012 | `give=I:91022;receive=I:91022;disappear=0` | service: Ireena's First Letter Guidance | [areas/qst/barovia.qst:411](../../../areas/qst/barovia.qst#L411) |
| 91012 | `give=I:91038;receive=I:91038;disappear=0` | service: Ireena's Ambush Plan Guidance | [areas/qst/barovia.qst:418](../../../areas/qst/barovia.qst#L418) |
| 91018 | `give=C:5000;receive=;disappear=0` | service: Parriwimple's Collection Clue | [areas/qst/barovia.qst:465](../../../areas/qst/barovia.qst#L465) |
| 91050 | `give=I:91036;receive=I:91037;disappear=1` | story: Ephon's Gate Key | [areas/qst/barovia.qst:597](../../../areas/qst/barovia.qst#L597) |
| 91052 | `give=I:58412;receive=I:91065;disappear=1` | story: Gertruda and Mad Mary | [areas/qst/barovia.qst:638](../../../areas/qst/barovia.qst#L638) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 91000 | hi hello | [areas/qst/barovia.qst:20](../../../areas/qst/barovia.qst#L20) |
| 91000 | barovia | [areas/qst/barovia.qst:25](../../../areas/qst/barovia.qst#L25) |
| 91000 | strahd vistani ravenloft burgomaster heroes help why | [areas/qst/barovia.qst:35](../../../areas/qst/barovia.qst#L35) |
| 91001 | hi hello | [areas/qst/barovia.qst:42](../../../areas/qst/barovia.qst#L42) |
| 91001 | vistani | [areas/qst/barovia.qst:46](../../../areas/qst/barovia.qst#L46) |
| 91002 | hi hello | [areas/qst/barovia.qst:58](../../../areas/qst/barovia.qst#L58) |
| 91006 | hi hello | [areas/qst/barovia.qst:72](../../../areas/qst/barovia.qst#L72) |
| 91006 | companions | [areas/qst/barovia.qst:78](../../../areas/qst/barovia.qst#L78) |
| 91006 | zombies undead zombie | [areas/qst/barovia.qst:94](../../../areas/qst/barovia.qst#L94) |
| 91006 | strahd | [areas/qst/barovia.qst:100](../../../areas/qst/barovia.qst#L100) |
| 91006 | sunsword treasure madam eva | [areas/qst/barovia.qst:105](../../../areas/qst/barovia.qst#L105) |
| 91007 | vistani | [areas/qst/barovia.qst:138](../../../areas/qst/barovia.qst#L138) |
| 91007 | hi hello | [areas/qst/barovia.qst:149](../../../areas/qst/barovia.qst#L149) |
| 91007 | items buy sell store open close | [areas/qst/barovia.qst:156](../../../areas/qst/barovia.qst#L156) |
| 91010 | hi hello | [areas/qst/barovia.qst:187](../../../areas/qst/barovia.qst#L187) |
| 91010 | drink | [areas/qst/barovia.qst:194](../../../areas/qst/barovia.qst#L194) |
| 91010 | vistani | [areas/qst/barovia.qst:199](../../../areas/qst/barovia.qst#L199) |
| 91010 | strahd | [areas/qst/barovia.qst:211](../../../areas/qst/barovia.qst#L211) |
| 91010 | ismark | [areas/qst/barovia.qst:219](../../../areas/qst/barovia.qst#L219) |
| 91011 | hi hello | [areas/qst/barovia.qst:234](../../../areas/qst/barovia.qst#L234) |
| 91011 | vistani strahd | [areas/qst/barovia.qst:244](../../../areas/qst/barovia.qst#L244) |
| 91011 | zombies zombie incursion | [areas/qst/barovia.qst:255](../../../areas/qst/barovia.qst#L255) |
| 91011 | madam eva | [areas/qst/barovia.qst:272](../../../areas/qst/barovia.qst#L272) |
| 91011 | symbol ravenkind | [areas/qst/barovia.qst:277](../../../areas/qst/barovia.qst#L277) |
| 91011 | sister ireena | [areas/qst/barovia.qst:284](../../../areas/qst/barovia.qst#L284) |
| 91012 | hi hello | [areas/qst/barovia.qst:349](../../../areas/qst/barovia.qst#L349) |
| 91012 | father dead kolyan burgomaster body help cry crying | [areas/qst/barovia.qst:354](../../../areas/qst/barovia.qst#L354) |
| 91012 | strahd henchmen vistani | [areas/qst/barovia.qst:363](../../../areas/qst/barovia.qst#L363) |
| 91012 | symbol ravenkind | [areas/qst/barovia.qst:377](../../../areas/qst/barovia.qst#L377) |
| 91012 | wise woman madam eva | [areas/qst/barovia.qst:392](../../../areas/qst/barovia.qst#L392) |
| 91018 | hi hello | [areas/qst/barovia.qst:437](../../../areas/qst/barovia.qst#L437) |
| 91018 | strahd | [areas/qst/barovia.qst:442](../../../areas/qst/barovia.qst#L442) |
| 91018 | vistani | [areas/qst/barovia.qst:447](../../../areas/qst/barovia.qst#L447) |
| 91018 | collection bildrath | [areas/qst/barovia.qst:459](../../../areas/qst/barovia.qst#L459) |
| 91021 | hi hello zombies zombie undead strahd vistani | [areas/qst/barovia.qst:496](../../../areas/qst/barovia.qst#L496) |
| 91030 | hi hello strahd vistani ismark doru | [areas/qst/barovia.qst:527](../../../areas/qst/barovia.qst#L527) |
| 91041 | hi hello | [areas/qst/barovia.qst:533](../../../areas/qst/barovia.qst#L533) |
| 91041 | strahd ireena | [areas/qst/barovia.qst:540](../../../areas/qst/barovia.qst#L540) |
| 91050 | proof zombie zombies doru | [areas/qst/barovia.qst:576](../../../areas/qst/barovia.qst#L576) |
| 91050 | hi hello | [areas/qst/barovia.qst:592](../../../areas/qst/barovia.qst#L592) |
| 91052 | hi hello gertrude | [areas/qst/barovia.qst:610](../../../areas/qst/barovia.qst#L610) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

405 parsed reset commands: D: 48, E: 139, F: 12, G: 22, M: 145, O: 32, P: 7.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
