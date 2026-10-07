# Killing Fields: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence killing_fields \
  --evidence-format markdown --output docs/reference/zone-story-audits/killing_fields.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 33300 | `give=I:33302;receive=C:45000;disappear=1` | request: Bring the missing message to Maelron | [areas/qst/killing_fields.qst:14](../../../areas/qst/killing_fields.qst#L14) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 33300 | friend | [areas/qst/killing_fields.qst:2](../../../areas/qst/killing_fields.qst#L2) |
| 33300 | hi hello | [areas/qst/killing_fields.qst:9](../../../areas/qst/killing_fields.qst#L9) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

26 parsed reset commands: E: 4, G: 6, M: 10, O: 2, P: 4.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
