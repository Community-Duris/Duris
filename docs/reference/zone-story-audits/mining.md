# The Halfling Silver Mine: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence mining \
  --evidence-format markdown --output docs/reference/zone-story-audits/mining.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 4921 | `give=I:4911;receive=I:4912;disappear=0` | request: Bring the worried priest the balor’s whip | [areas/qst/mining.qst:8](../../../areas/qst/mining.qst#L8) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 4921 | worried worry hi | [areas/qst/mining.qst:2](../../../areas/qst/mining.qst#L2) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 4812 | `poison` | [src/specs/specs.assign.c:623](../../../src/specs/specs.assign.c#L623) |
| mob | 4830 | `wanderer` | [src/specs/specs.assign.c:624](../../../src/specs/specs.assign.c#L624) |

## Reset coverage

106 parsed reset commands: D: 22, E: 13, G: 2, M: 53, O: 13, P: 3.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
