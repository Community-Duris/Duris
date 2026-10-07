# Orrak: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence orrak \
  --evidence-format markdown --output docs/reference/zone-story-audits/orrak.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 9016 | `give=I:9011;receive=I:9012;disappear=1` | request: Return the prisoner’s stolen staff | [areas/qst/orrak.qst:8](../../../areas/qst/orrak.qst#L8) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 9001 | orrak | [areas/qst/orrak.qst:2](../../../areas/qst/orrak.qst#L2) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

55 parsed reset commands: D: 12, E: 11, F: 7, G: 4, M: 18, O: 3.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
