# The Forest City of Aravne: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence clfhaven \
  --evidence-format markdown --output docs/reference/zone-story-audits/clfhaven.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 21514 | `give=I:21649,I:21650,I:21651,I:21652,I:21653,I:21654,I:21655,I:21656,I:21657;receive=E:250000,I:21663;disappear=0` | story: Nine hearts for the throne-room key | [areas/qst/clfhaven.qst:32](../../../areas/qst/clfhaven.qst#L32) |
| 21514 | `give=I:21658,I:21659,I:21660;receive=E:300000,I:21661,I:21662;disappear=0` | story: The royal hearts for blade and gauntlets | [areas/qst/clfhaven.qst:51](../../../areas/qst/clfhaven.qst#L51) |
| 21535 | `give=I:402,I:26614,I:32490;receive=I:409;disappear=0` | story: Babedo’s legacy luck-scroll exchange | [areas/qst/clfhaven.qst:83](../../../areas/qst/clfhaven.qst#L83) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 21514 | weep weeping | [areas/qst/clfhaven.qst:2](../../../areas/qst/clfhaven.qst#L2) |
| 21514 | proof | [areas/qst/clfhaven.qst:16](../../../areas/qst/clfhaven.qst#L16) |
| 21514 | hi hello quest crying | [areas/qst/clfhaven.qst:28](../../../areas/qst/clfhaven.qst#L28) |
| 21549 | unique uniques | [areas/qst/clfhaven.qst:101](../../../areas/qst/clfhaven.qst#L101) |
| 21549 | major main | [areas/qst/clfhaven.qst:105](../../../areas/qst/clfhaven.qst#L105) |
| 21549 | ioun iouns | [areas/qst/clfhaven.qst:109](../../../areas/qst/clfhaven.qst#L109) |
| 21549 | artifact artifacts | [areas/qst/clfhaven.qst:113](../../../areas/qst/clfhaven.qst#L113) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 21673 | `wh_corpse_to_object` | [src/specs/specs.assign.c:287](../../../src/specs/specs.assign.c#L287) |
| mob | 21549 | `llyren` | [src/specs/specs.assign.c:574](../../../src/specs/specs.assign.c#L574) |
| room | 21611 | `inn` | [src/specs/specs.assign.c:2297](../../../src/specs/specs.assign.c#L2297) |

## Reset coverage

962 parsed reset commands: D: 108, E: 187, F: 31, G: 70, M: 537, O: 27, P: 2.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
