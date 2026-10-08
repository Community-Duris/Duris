# The Dark Stone Tower of the Northern Realms: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence teka2 \
  --evidence-format markdown --output docs/reference/zone-story-audits/teka2.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 75500 | `give=I:75559;receive=I:75560,I:75561;disappear=1` | request: Return the lost rhinestone to Aerlyn | [areas/qst/teka2.qst:32](../../../areas/qst/teka2.qst#L32) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 75500 | spell fail | [areas/qst/teka2.qst:15](../../../areas/qst/teka2.qst#L15) |
| 75500 | relic | [areas/qst/teka2.qst:19](../../../areas/qst/teka2.qst#L19) |
| 75535 | lost relic aerlyn | [areas/qst/teka2.qst:48](../../../areas/qst/teka2.qst#L48) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

231 parsed reset commands: D: 26, E: 63, F: 12, G: 19, M: 96, O: 11, P: 4.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
