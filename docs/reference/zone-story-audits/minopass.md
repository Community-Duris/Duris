# The Deep Ravine of Passage: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence minopass \
  --evidence-format markdown --output docs/reference/zone-story-audits/minopass.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 94704 | `give=I:94723;receive=I:94724;disappear=0` | story: Oesh'lyn: the test of truth | [areas/qst/minopass.qst:38](../../../areas/qst/minopass.qst#L38) |
| 94705 | `give=I:94715;receive=I:94716,I:94727,I:94727,I:94727;disappear=1` | story: Contractor: the lost signet ring | [areas/qst/minopass.qst:67](../../../areas/qst/minopass.qst#L67) |
| 94738 | `give=I:4402,I:88807;receive=E:250000,I:94726;disappear=1` | story: Dwarf spirit: between life and death | [areas/qst/minopass.qst:130](../../../areas/qst/minopass.qst#L130) |
| 94757 | `give=C:10000;receive=I:94729;disappear=0` | service: Nahasp: the paid grate key | [areas/qst/minopass.qst:174](../../../areas/qst/minopass.qst#L174) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 94704 | hi hello gates truth key | [areas/qst/minopass.qst:22](../../../areas/qst/minopass.qst#L22) |
| 94705 | hi hello quest | [areas/qst/minopass.qst:53](../../../areas/qst/minopass.qst#L53) |
| 94706 | hi hello borthur | [areas/qst/minopass.qst:87](../../../areas/qst/minopass.qst#L87) |
| 94738 | hi hello | [areas/qst/minopass.qst:112](../../../areas/qst/minopass.qst#L112) |
| 94738 | berronar truesilver | [areas/qst/minopass.qst:116](../../../areas/qst/minopass.qst#L116) |
| 94738 | relic life death | [areas/qst/minopass.qst:122](../../../areas/qst/minopass.qst#L122) |
| 94738 | zorta kitan | [areas/qst/minopass.qst:126](../../../areas/qst/minopass.qst#L126) |
| 94757 | hi hello | [areas/qst/minopass.qst:145](../../../areas/qst/minopass.qst#L145) |
| 94757 | key | [areas/qst/minopass.qst:155](../../../areas/qst/minopass.qst#L155) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

347 parsed reset commands: D: 28, E: 9, G: 20, M: 161, O: 129.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
