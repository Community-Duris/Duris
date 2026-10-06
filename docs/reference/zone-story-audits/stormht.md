# Strathor Valley of the Storm Giants: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence stormht \
  --evidence-format markdown --output docs/reference/zone-story-audits/stormht.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 30871 | `give=I:40073;receive=E:60000,I:30846;disappear=0` | story: The Sultan’s sword for the king’s crown | [areas/qst/stormht.qst:63](../../../areas/qst/stormht.qst#L63) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 30814 | hi hello | [areas/qst/stormht.qst:2](../../../areas/qst/stormht.qst#L2) |
| 30820 | hi hello | [areas/qst/stormht.qst:10](../../../areas/qst/stormht.qst#L10) |
| 30821 | hi hello | [areas/qst/stormht.qst:16](../../../areas/qst/stormht.qst#L16) |
| 30831 | hi hello | [areas/qst/stormht.qst:22](../../../areas/qst/stormht.qst#L22) |
| 30833 | hi hello | [areas/qst/stormht.qst:30](../../../areas/qst/stormht.qst#L30) |
| 30852 | hi hello | [areas/qst/stormht.qst:37](../../../areas/qst/stormht.qst#L37) |
| 30871 | hi hello | [areas/qst/stormht.qst:50](../../../areas/qst/stormht.qst#L50) |
| 30871 | king stragathor | [areas/qst/stormht.qst:55](../../../areas/qst/stormht.qst#L55) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 30831 | `world_quest` | [src/specs/specs.assign.c:769](../../../src/specs/specs.assign.c#L769) |

## Reset coverage

336 parsed reset commands: D: 36, E: 25, F: 2, G: 16, M: 224, O: 26, P: 7.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
