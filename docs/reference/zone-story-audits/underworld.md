# The Underworld: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence underworld \
  --evidence-format markdown --output docs/reference/zone-story-audits/underworld.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 4401 | `give=I:88807;receive=S:113;disappear=1` | request: Return Zorta's stolen relic | [areas/qst/underworld.qst:18](../../../areas/qst/underworld.qst#L18) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 4401 | resurection | [areas/qst/underworld.qst:2](../../../areas/qst/underworld.qst#L2) |
| 4401 | payment | [areas/qst/underworld.qst:7](../../../areas/qst/underworld.qst#L7) |
| 4401 | relic kitan | [areas/qst/underworld.qst:12](../../../areas/qst/underworld.qst#L12) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 4480 | `purple_worm` | [src/specs/specs.assign.c:579](../../../src/specs/specs.assign.c#L579) |
| mob | 4530 | `piercer` | [src/specs/specs.assign.c:582](../../../src/specs/specs.assign.c#L582) |
| mob | 4520 | `underdark_track` | [src/specs/specs.assign.c:583](../../../src/specs/specs.assign.c#L583) |
| mob | 4510 | `underdark_track` | [src/specs/specs.assign.c:584](../../../src/specs/specs.assign.c#L584) |
| obj | 4505 | `hammer` | [src/specs/specs.assign.c:1814](../../../src/specs/specs.assign.c#L1814) |
| obj | 4403 | `magic_pool` | [src/specs/specs.assign.c:1815](../../../src/specs/specs.assign.c#L1815) |
| obj | 4404 | `magic_pool` | [src/specs/specs.assign.c:1816](../../../src/specs/specs.assign.c#L1816) |

## Reset coverage

217 parsed reset commands: D: 30, E: 33, F: 2, G: 11, M: 102, O: 25, P: 14.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
