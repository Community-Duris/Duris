# The Church of the Eternal Dusk: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence church \
  --evidence-format markdown --output docs/reference/zone-story-audits/church.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 87821 | `give=I:87846;receive=E:100000;disappear=0` | request: Bring the badge of holy patronage to the cardinal | [areas/qst/church.qst:14](../../../areas/qst/church.qst#L14) |
| 87847 | `give=I:87873;receive=I:87874;disappear=0` | request: Deliver Relxis's report to the field general | [areas/qst/church.qst:24](../../../areas/qst/church.qst#L24) |
| 87860 | `give=I:87869;receive=C:250000,E:100000;disappear=0` | request: Bring proof to the bishop | [areas/qst/church.qst:47](../../../areas/qst/church.qst#L47) |
| 87869 | `give=I:87870,I:87871,I:87872;receive=I:87873;disappear=0` | request: Bring three proofs to the captive paladin | [areas/qst/church.qst:62](../../../areas/qst/church.qst#L62) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 87821 | paladin paladins | [areas/qst/church.qst:2](../../../areas/qst/church.qst#L2) |
| 87821 | hello hi | [areas/qst/church.qst:9](../../../areas/qst/church.qst#L9) |
| 87860 | initiation | [areas/qst/church.qst:36](../../../areas/qst/church.qst#L36) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

161 parsed reset commands: D: 26, E: 52, F: 5, G: 11, M: 62, O: 4, P: 1.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
