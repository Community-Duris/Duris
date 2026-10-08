# Mistywood: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence mistywood \
  --evidence-format markdown --output docs/reference/zone-story-audits/mistywood.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 95019 | `give=I:95000;receive=C:15000,E:25000;disappear=0` | story: Riliatar: bear claw trophy | [areas/qst/mistywood.qst:32](../../../areas/qst/mistywood.qst#L32) |
| 95020 | `give=I:95001;receive=I:95004;disappear=0` | story: Ascuren: proof for the forest | [areas/qst/mistywood.qst:58](../../../areas/qst/mistywood.qst#L58) |
| 95022 | `give=I:95003;receive=E:10000,I:95002;disappear=1` | story: Jarnes: mushroom delivery | [areas/qst/mistywood.qst:113](../../../areas/qst/mistywood.qst#L113) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 95019 | hi hello hail | [areas/qst/mistywood.qst:2](../../../areas/qst/mistywood.qst#L2) |
| 95019 | yes job | [areas/qst/mistywood.qst:9](../../../areas/qst/mistywood.qst#L9) |
| 95019 | ascuren druid | [areas/qst/mistywood.qst:19](../../../areas/qst/mistywood.qst#L19) |
| 95019 | bear scar | [areas/qst/mistywood.qst:26](../../../areas/qst/mistywood.qst#L26) |
| 95020 | hi hello hail | [areas/qst/mistywood.qst:42](../../../areas/qst/mistywood.qst#L42) |
| 95020 | yes protect forest | [areas/qst/mistywood.qst:50](../../../areas/qst/mistywood.qst#L50) |
| 95021 | hi hail hello | [areas/qst/mistywood.qst:71](../../../areas/qst/mistywood.qst#L71) |
| 95021 | jarnes concern distress | [areas/qst/mistywood.qst:77](../../../areas/qst/mistywood.qst#L77) |
| 95021 | yes sure | [areas/qst/mistywood.qst:84](../../../areas/qst/mistywood.qst#L84) |
| 95022 | hail hi hello | [areas/qst/mistywood.qst:94](../../../areas/qst/mistywood.qst#L94) |
| 95022 | trapped mushroom no | [areas/qst/mistywood.qst:99](../../../areas/qst/mistywood.qst#L99) |
| 95022 | yes | [areas/qst/mistywood.qst:108](../../../areas/qst/mistywood.qst#L108) |
| 95023 | hi hello hail | [areas/qst/mistywood.qst:127](../../../areas/qst/mistywood.qst#L127) |
| 95023 | fix he him alive | [areas/qst/mistywood.qst:135](../../../areas/qst/mistywood.qst#L135) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

171 parsed reset commands: D: 10, E: 18, G: 6, M: 122, O: 8, P: 7.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
