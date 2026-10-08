# The Great Realm of Duris: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence connectorzones \
  --evidence-format markdown --output docs/reference/zone-story-audits/connectorzones.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 53637 | `give=I:53625;receive=C:20000;disappear=1` | story: Storm druid: burnt-tree evidence | [areas/qst/connectorzones.qst:9](../../../areas/qst/connectorzones.qst#L9) |
| 53640 | `give=C:10000,I:53622,I:53622,I:53622,I:53622;receive=I:53623;disappear=0` | service: Barbarian: four-feather mask | [areas/qst/connectorzones.qst:31](../../../areas/qst/connectorzones.qst#L31) |
| 53658 | `give=I:402,I:26614,I:32490;receive=I:410;disappear=0` | story: Chauseis: remote Wisdom scroll | [areas/qst/connectorzones.qst:52](../../../areas/qst/connectorzones.qst#L52) |
| 53700 | `give=I:53643,I:53644;receive=I:53646;disappear=1` | story: Ahanz: both royal heads | [areas/qst/connectorzones.qst:72](../../../areas/qst/connectorzones.qst#L72) |
| 53704 | `give=I:16210,I:16224,I:16232,I:16249,I:16269;receive=I:53650,I:53660;disappear=1` | story: Adventurer: choose one reward path | [areas/qst/connectorzones.qst:171](../../../areas/qst/connectorzones.qst#L171) |
| 53704 | `give=I:16210,I:16224,I:16235,I:16244,I:16269;receive=I:53651,I:53660;disappear=1` | story: Adventurer: choose one reward path | [areas/qst/connectorzones.qst:205](../../../areas/qst/connectorzones.qst#L205) |
| 53704 | `give=I:16210,I:16234,I:16248,I:16249,I:16269;receive=I:53652,I:53660;disappear=1` | story: Adventurer: choose one reward path | [areas/qst/connectorzones.qst:188](../../../areas/qst/connectorzones.qst#L188) |
| 53720 | `give=I:330,I:332;receive=I:53667,I:53667;disappear=0` | service: Asus: crab-and-shrimp bisque | [areas/qst/connectorzones.qst:241](../../../areas/qst/connectorzones.qst#L241) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 53637 | hello hi dark moor moors | [areas/qst/connectorzones.qst:2](../../../areas/qst/connectorzones.qst#L2) |
| 53640 | hi hello feathers craft feather feathered | [areas/qst/connectorzones.qst:22](../../../areas/qst/connectorzones.qst#L22) |
| 53700 | kraken | [areas/qst/connectorzones.qst:62](../../../areas/qst/connectorzones.qst#L62) |
| 53702 | tomb | [areas/qst/connectorzones.qst:90](../../../areas/qst/connectorzones.qst#L90) |
| 53702 | powerful mystic | [areas/qst/connectorzones.qst:95](../../../areas/qst/connectorzones.qst#L95) |
| 53704 | hi greetings | [areas/qst/connectorzones.qst:102](../../../areas/qst/connectorzones.qst#L102) |
| 53704 | priest | [areas/qst/connectorzones.qst:124](../../../areas/qst/connectorzones.qst#L124) |
| 53704 | mage | [areas/qst/connectorzones.qst:139](../../../areas/qst/connectorzones.qst#L139) |
| 53704 | warrior | [areas/qst/connectorzones.qst:155](../../../areas/qst/connectorzones.qst#L155) |
| 53720 | hi hello | [areas/qst/connectorzones.qst:228](../../../areas/qst/connectorzones.qst#L228) |
| 53720 | soup | [areas/qst/connectorzones.qst:232](../../../areas/qst/connectorzones.qst#L232) |
| 53720 | fishing | [areas/qst/connectorzones.qst:237](../../../areas/qst/connectorzones.qst#L237) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 53670 | `world_quest` | [src/specs/specs.assign.c:734](../../../src/specs/specs.assign.c#L734) |
| obj | 53661 | `damnation_staff` | [src/specs/specs.assign.c:2152](../../../src/specs/specs.assign.c#L2152) |
| obj | 53662 | `nuke_damnation` | [src/specs/specs.assign.c:2153](../../../src/specs/specs.assign.c#L2153) |
| obj | 53663 | `nuke_damnation` | [src/specs/specs.assign.c:2154](../../../src/specs/specs.assign.c#L2154) |
| room | 54240 | `crew_shop_proc` | [src/specs/specs.assign.c:2402](../../../src/specs/specs.assign.c#L2402) |

## Reset coverage

492 parsed reset commands: D: 88, E: 23, F: 4, G: 19, M: 331, O: 23, P: 4.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
