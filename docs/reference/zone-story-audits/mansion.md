# The Forgotten Mansion: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence mansion \
  --evidence-format markdown --output docs/reference/zone-story-audits/mansion.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 3505 | `give=I:3505;receive=I:3511,I:3512;disappear=0` | story: Pianist: Rise of the Phoenix | [areas/qst/mansion.qst:48](../../../areas/qst/mansion.qst#L48) |
| 3506 | `give=I:3509;receive=I:3513;disappear=0` | story: Handmaid: the missing rattle | [areas/qst/mansion.qst:70](../../../areas/qst/mansion.qst#L70) |
| 3518 | `give=I:3529,I:3539;receive=E:250000,I:3540;disappear=0` | story: Englehardt: the two demon proofs | [areas/qst/mansion.qst:126](../../../areas/qst/mansion.qst#L126) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 3504 | adventurers luck | [areas/qst/mansion.qst:2](../../../areas/qst/mansion.qst#L2) |
| 3504 | secrets secret manor | [areas/qst/mansion.qst:9](../../../areas/qst/mansion.qst#L9) |
| 3504 | key door | [areas/qst/mansion.qst:19](../../../areas/qst/mansion.qst#L19) |
| 3504 | family name | [areas/qst/mansion.qst:25](../../../areas/qst/mansion.qst#L25) |
| 3505 | piano music | [areas/qst/mansion.qst:34](../../../areas/qst/mansion.qst#L34) |
| 3505 | phoenix rise | [areas/qst/mansion.qst:42](../../../areas/qst/mansion.qst#L42) |
| 3506 | baby crying child cry | [areas/qst/mansion.qst:63](../../../areas/qst/mansion.qst#L63) |
| 3518 | life experiment quest | [areas/qst/mansion.qst:81](../../../areas/qst/mansion.qst#L81) |
| 3518 | offer strong | [areas/qst/mansion.qst:93](../../../areas/qst/mansion.qst#L93) |
| 3518 | vault | [areas/qst/mansion.qst:104](../../../areas/qst/mansion.qst#L104) |
| 3518 | gaultair | [areas/qst/mansion.qst:111](../../../areas/qst/mansion.qst#L111) |
| 3518 | tyrlos | [areas/qst/mansion.qst:119](../../../areas/qst/mansion.qst#L119) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

247 parsed reset commands: D: 38, E: 36, G: 12, M: 147, O: 10, P: 4.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
