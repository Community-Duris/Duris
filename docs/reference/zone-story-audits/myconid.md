# Myconid Mushroom Forest: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence myconid \
  --evidence-format markdown --output docs/reference/zone-story-audits/myconid.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 2320 | `give=I:88502;receive=C:50000;disappear=0` | request: Bring giant mushroom spores to the alchemist | [areas/qst/myconid.qst:18](../../../areas/qst/myconid.qst#L18) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 2320 | specific | [areas/qst/myconid.qst:2](../../../areas/qst/myconid.qst#L2) |
| 2320 | giant mushrooms mushroom | [areas/qst/myconid.qst:7](../../../areas/qst/myconid.qst#L7) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

103 parsed reset commands: D: 2, E: 3, G: 2, M: 74, O: 20, P: 2.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
