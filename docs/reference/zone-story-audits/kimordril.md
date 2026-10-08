# Kimordril: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence kimordril \
  --evidence-format markdown --output docs/reference/zone-story-audits/kimordril.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 95508 | `give=I:95506,I:95507,I:95508;receive=C:60,E:100;disappear=1` | request: Help prepare the family supper | [areas/qst/kimordril.qst:20](../../../areas/qst/kimordril.qst#L20) |
| 95512 | `give=I:95512;receive=C:30;disappear=0` | request: Trade prepared goatskin | [areas/qst/kimordril.qst:44](../../../areas/qst/kimordril.qst#L44) |
| 95512 | `give=I:95513;receive=C:35;disappear=0` | request: Trade prepared black boarskin | [areas/qst/kimordril.qst:49](../../../areas/qst/kimordril.qst#L49) |
| 95512 | `give=I:95514;receive=C:20;disappear=0` | request: Trade prepared brown boarskin | [areas/qst/kimordril.qst:54](../../../areas/qst/kimordril.qst#L54) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 95508 | hi hello | [areas/qst/kimordril.qst:2](../../../areas/qst/kimordril.qst#L2) |
| 95508 | help busy | [areas/qst/kimordril.qst:6](../../../areas/qst/kimordril.qst#L6) |
| 95508 | vegetables carrot potato | [areas/qst/kimordril.qst:16](../../../areas/qst/kimordril.qst#L16) |
| 95512 | hi hello | [areas/qst/kimordril.qst:35](../../../areas/qst/kimordril.qst#L35) |
| 95512 | skin skins boar goat money | [areas/qst/kimordril.qst:39](../../../areas/qst/kimordril.qst#L39) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 95517 | `world_quest` | [src/specs/specs.assign.c:759](../../../src/specs/specs.assign.c#L759) |
| mob | 95506 | `archer` | [src/specs/specs.assign.c:834](../../../src/specs/specs.assign.c#L834) |
| mob | 95503 | `money_changer` | [src/specs/specs.assign.c:835](../../../src/specs/specs.assign.c#L835) |
| mob | 95535 | `kimordril_shout` | [src/specs/specs.assign.c:836](../../../src/specs/specs.assign.c#L836) |
| room | 95569 | `inn` | [src/specs/specs.assign.c:2515](../../../src/specs/specs.assign.c#L2515) |

## Reset coverage

297 parsed reset commands: D: 38, E: 42, F: 4, G: 53, M: 126, O: 15, P: 15, R: 4.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
