# The Plains of Life: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence newbie2 \
  --evidence-format markdown --output docs/reference/zone-story-audits/newbie2.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |

No native Q contracts. Scripted outcomes require separate semantic review.

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 22801 | `newbie_paladin` | [src/specs/specs.assign.c:1227](../../../src/specs/specs.assign.c#L1227) |
| obj | 22803 | `stream_of_life` | [src/specs/specs.assign.c:2083](../../../src/specs/specs.assign.c#L2083) |
| obj | 22800 | `newbie_sign1` | [src/specs/specs.assign.c:2084](../../../src/specs/specs.assign.c#L2084) |
| obj | 22801 | `newbie_sign2` | [src/specs/specs.assign.c:2085](../../../src/specs/specs.assign.c#L2085) |

## Reset coverage

21 parsed reset commands: G: 1, M: 7, O: 6, P: 7.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
