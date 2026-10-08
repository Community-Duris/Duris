# Pharr Valley: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence pharrvly \
  --evidence-format markdown --output docs/reference/zone-story-audits/pharrvly.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 40201 | `give=I:40208,I:40209,I:40210;receive=E:2500;disappear=1` | story: The farmer: valley provisions | [areas/qst/pharrvly.qst:87](../../../areas/qst/pharrvly.qst#L87) |
| 40201 | `give=I:40213;receive=E:4000;disappear=0` | story: The farmer: gartham shelter | [areas/qst/pharrvly.qst:100](../../../areas/qst/pharrvly.qst#L100) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 40200 | hi hello | [areas/qst/pharrvly.qst:2](../../../areas/qst/pharrvly.qst#L2) |
| 40200 | key | [areas/qst/pharrvly.qst:7](../../../areas/qst/pharrvly.qst#L7) |
| 40200 | chest | [areas/qst/pharrvly.qst:12](../../../areas/qst/pharrvly.qst#L12) |
| 40200 | wives farmers | [areas/qst/pharrvly.qst:19](../../../areas/qst/pharrvly.qst#L19) |
| 40200 | gartham | [areas/qst/pharrvly.qst:26](../../../areas/qst/pharrvly.qst#L26) |
| 40200 | old farmer grandpod | [areas/qst/pharrvly.qst:33](../../../areas/qst/pharrvly.qst#L33) |
| 40201 | hi hello | [areas/qst/pharrvly.qst:43](../../../areas/qst/pharrvly.qst#L43) |
| 40201 | chest | [areas/qst/pharrvly.qst:47](../../../areas/qst/pharrvly.qst#L47) |
| 40201 | contents | [areas/qst/pharrvly.qst:52](../../../areas/qst/pharrvly.qst#L52) |
| 40201 | scroll | [areas/qst/pharrvly.qst:58](../../../areas/qst/pharrvly.qst#L58) |
| 40201 | where found | [areas/qst/pharrvly.qst:63](../../../areas/qst/pharrvly.qst#L63) |
| 40201 | apple orange | [areas/qst/pharrvly.qst:70](../../../areas/qst/pharrvly.qst#L70) |
| 40201 | carapace gartham | [areas/qst/pharrvly.qst:76](../../../areas/qst/pharrvly.qst#L76) |
| 40201 | types | [areas/qst/pharrvly.qst:82](../../../areas/qst/pharrvly.qst#L82) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

375 parsed reset commands: D: 28, G: 2, M: 155, O: 187, P: 3.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
