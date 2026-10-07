# The Bugger Caves: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence bugger \
  --evidence-format markdown --output docs/reference/zone-story-audits/bugger.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 6402 | `give=I:6402;receive=I:6402;disappear=0` | Excluded: The Queen rejects food egg6402 and returns the same prototype kind. Preserve native settlement evidence without counting this wrong-egg response as a completed story, achievement or daily. Same-kind replacement is not proof of returning the same physical UID. | [areas/qst/bugger.qst:14](../../../areas/qst/bugger.qst#L14) |
| 6402 | `give=I:6403,I:6404,I:6405;receive=I:6406;disappear=0` | request: Return the Queen’s lost eggs and abandoned carapace | [areas/qst/bugger.qst:7](../../../areas/qst/bugger.qst#L7) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 6402 | hi hello agitated | [areas/qst/bugger.qst:2](../../../areas/qst/bugger.qst#L2) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

91 parsed reset commands: D: 2, E: 1, M: 81, O: 4, P: 3.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
