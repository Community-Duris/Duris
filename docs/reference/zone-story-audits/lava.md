# Lava Springs: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence lava \
  --evidence-format markdown --output docs/reference/zone-story-audits/lava.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 99705 | `give=I:99701,I:99701,I:99701,I:99701,I:99701,I:99706;receive=I:99700;disappear=0` | request: Commission Cinder from Drembel | [areas/qst/lava.qst:14](../../../areas/qst/lava.qst#L14) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 99705 | hi lava rock weapon forge need want armor | [areas/qst/lava.qst:2](../../../areas/qst/lava.qst#L2) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| room | 99715 | `inn` | [src/specs/specs.assign.c:2309](../../../src/specs/specs.assign.c#L2309) |

## Reset coverage

109 parsed reset commands: D: 24, E: 3, F: 3, G: 13, M: 65, O: 1.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
