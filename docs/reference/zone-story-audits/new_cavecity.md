# New Cave city: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence new_cavecity \
  --evidence-format markdown --output docs/reference/zone-story-audits/new_cavecity.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 15120 | `give=I:402,I:26614,I:32490;receive=I:411;disappear=0` | story: The wizard’s three-material commission | [areas/qst/new_cavecity.qst:16](../../../areas/qst/new_cavecity.qst#L16) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 15113 | `dranum_jurtrem` | [src/specs/specs.assign.c:1023](../../../src/specs/specs.assign.c#L1023) |
| mob | 15125 | `dranum_jurtrem` | [src/specs/specs.assign.c:1024](../../../src/specs/specs.assign.c#L1024) |
| obj | 15116 | `torment` | [src/specs/specs.assign.c:1889](../../../src/specs/specs.assign.c#L1889) |

## Reset coverage

129 parsed reset commands: D: 24, E: 23, F: 7, G: 9, M: 62, O: 3, P: 1.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
