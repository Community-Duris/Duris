# Prison of Fort Boyard: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence prisonb \
  --evidence-format markdown --output docs/reference/zone-story-audits/prisonb.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 43010 | `give=I:43003,I:43006,I:43008,I:43011,I:43012;receive=I:43015;disappear=0` | request: Bring five dragon scales to Gixmo | [areas/qst/prisonb.qst:43](../../../areas/qst/prisonb.qst#L43) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 43010 | hi hello | [areas/qst/prisonb.qst:2](../../../areas/qst/prisonb.qst#L2) |
| 43010 | journey | [areas/qst/prisonb.qst:8](../../../areas/qst/prisonb.qst#L8) |
| 43010 | dragon dragons | [areas/qst/prisonb.qst:14](../../../areas/qst/prisonb.qst#L14) |
| 43010 | item special | [areas/qst/prisonb.qst:24](../../../areas/qst/prisonb.qst#L24) |
| 43010 | fort boyard | [areas/qst/prisonb.qst:36](../../../areas/qst/prisonb.qst#L36) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

212 parsed reset commands: D: 44, E: 38, G: 10, M: 44, O: 15, P: 61.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
