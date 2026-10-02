# The Town of Breale: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence breale \
  --evidence-format markdown --output docs/reference/zone-story-audits/breale.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 2600 | `give=I:2673;receive=I:2681;disappear=0` | request: Dirt for the Triad | [areas/qst/breale.qst:11](../../../areas/qst/breale.qst#L11) |
| 2600 | `give=I:2674;receive=I:2682;disappear=0` | request: Begin the Triad Reagents | [areas/qst/breale.qst:18](../../../areas/qst/breale.qst#L18) |
| 2601 | `give=I:2671,I:2674,I:2674;receive=I:2683;disappear=0` | request: The First Triad Mixture | [areas/qst/breale.qst:40](../../../areas/qst/breale.qst#L40) |
| 2601 | `give=I:2672,I:2672,I:2673;receive=I:2684;disappear=0` | request: The Second Triad Mixture | [areas/qst/breale.qst:49](../../../areas/qst/breale.qst#L49) |
| 2602 | `give=I:2670,I:2672,I:2672,I:2675,I:2675;receive=I:2602;disappear=0` | story: Finish the Triad Mixture | [areas/qst/breale.qst:72](../../../areas/qst/breale.qst#L72) |
| 2617 | `give=I:2679;receive=I:2645;disappear=1` | story: Aid the Dying Witch | [areas/qst/breale.qst:94](../../../areas/qst/breale.qst#L94) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 2600 | hi hello | [areas/qst/breale.qst:2](../../../areas/qst/breale.qst#L2) |
| 2601 | hi hello | [areas/qst/breale.qst:28](../../../areas/qst/breale.qst#L28) |
| 2601 | brontella | [areas/qst/breale.qst:33](../../../areas/qst/breale.qst#L33) |
| 2602 | hi hello | [areas/qst/breale.qst:61](../../../areas/qst/breale.qst#L61) |
| 2602 | montra | [areas/qst/breale.qst:66](../../../areas/qst/breale.qst#L66) |
| 2617 | hi hello wrist | [areas/qst/breale.qst:86](../../../areas/qst/breale.qst#L86) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 2613 | `breale_townsfolk` | [src/specs/specs.assign.c:333](../../../src/specs/specs.assign.c#L333) |
| mob | 2614 | `breale_townsfolk` | [src/specs/specs.assign.c:334](../../../src/specs/specs.assign.c#L334) |
| mob | 2619 | `breale_townsfolk` | [src/specs/specs.assign.c:335](../../../src/specs/specs.assign.c#L335) |
| mob | 2620 | `breale_townsfolk` | [src/specs/specs.assign.c:336](../../../src/specs/specs.assign.c#L336) |
| mob | 2621 | `breale_townsfolk` | [src/specs/specs.assign.c:337](../../../src/specs/specs.assign.c#L337) |
| mob | 2622 | `breale_townsfolk` | [src/specs/specs.assign.c:338](../../../src/specs/specs.assign.c#L338) |
| mob | 2623 | `breale_townsfolk` | [src/specs/specs.assign.c:339](../../../src/specs/specs.assign.c#L339) |
| mob | 2624 | `breale_townsfolk` | [src/specs/specs.assign.c:340](../../../src/specs/specs.assign.c#L340) |
| mob | 2625 | `breale_townsfolk` | [src/specs/specs.assign.c:341](../../../src/specs/specs.assign.c#L341) |

## Reset coverage

399 parsed reset commands: D: 24, E: 206, G: 52, M: 109, O: 8.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
