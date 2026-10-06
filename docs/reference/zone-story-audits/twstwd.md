# A Dark and Twisted Wood: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence twstwd \
  --evidence-format markdown --output docs/reference/zone-story-audits/twstwd.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 16311 | `give=I:16014,I:16016;receive=I:16313;disappear=0` | request: Reassure the guardian faerie | [areas/qst/twstwd.qst:29](../../../areas/qst/twstwd.qst#L29) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 16311 | guard guarding | [areas/qst/twstwd.qst:2](../../../areas/qst/twstwd.qst#L2) |
| 16311 | man dark | [areas/qst/twstwd.qst:11](../../../areas/qst/twstwd.qst#L11) |
| 16311 | auriam | [areas/qst/twstwd.qst:20](../../../areas/qst/twstwd.qst#L20) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

145 parsed reset commands: D: 2, E: 21, F: 2, G: 13, M: 89, O: 9, P: 9.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
