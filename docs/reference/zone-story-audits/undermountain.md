# The Ruins of Undermountain: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence undermountain \
  --evidence-format markdown --output docs/reference/zone-story-audits/undermountain.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 92002 | `give=I:92134;receive=I:92120;disappear=0` | story: Durnan: deliver Tamsil’s crude note | [areas/qst/undermountain.qst:13](../../../areas/qst/undermountain.qst#L13) |
| 92082 | `give=I:92133;receive=I:92134;disappear=1` | story: Tamsil: exchange the grate key for her note | [areas/qst/undermountain.qst:28](../../../areas/qst/undermountain.qst#L28) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 92002 | hello hi | [areas/qst/undermountain.qst:2](../../../areas/qst/undermountain.qst#L2) |
| 92002 | daughter tamsil | [areas/qst/undermountain.qst:6](../../../areas/qst/undermountain.qst#L6) |
| 92082 | hello hi help prison | [areas/qst/undermountain.qst:24](../../../areas/qst/undermountain.qst#L24) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 92020 | `um_kevlar` | [src/specs/specs.assign.c:1198](../../../src/specs/specs.assign.c#L1198) |
| mob | 92021 | `um_thorn` | [src/specs/specs.assign.c:1199](../../../src/specs/specs.assign.c#L1199) |
| mob | 92022 | `um_korelar` | [src/specs/specs.assign.c:1200](../../../src/specs/specs.assign.c#L1200) |
| mob | 92047 | `flying_dagger` | [src/specs/specs.assign.c:1209](../../../src/specs/specs.assign.c#L1209) |
| mob | 92058 | `ochre_jelly` | [src/specs/specs.assign.c:1210](../../../src/specs/specs.assign.c#L1210) |
| mob | 92062 | `helmed_horror` | [src/specs/specs.assign.c:1211](../../../src/specs/specs.assign.c#L1211) |
| obj | 92090 | `undead_trident` | [src/specs/specs.assign.c:2043](../../../src/specs/specs.assign.c#L2043) |
| obj | 92080 | `generic_drow_eq` | [src/specs/specs.assign.c:2044](../../../src/specs/specs.assign.c#L2044) |
| obj | 92081 | `generic_drow_eq` | [src/specs/specs.assign.c:2045](../../../src/specs/specs.assign.c#L2045) |
| obj | 92082 | `generic_drow_eq` | [src/specs/specs.assign.c:2046](../../../src/specs/specs.assign.c#L2046) |
| obj | 92086 | `generic_drow_eq` | [src/specs/specs.assign.c:2047](../../../src/specs/specs.assign.c#L2047) |
| obj | 92065 | `iron_flindbar` | [src/specs/specs.assign.c:2048](../../../src/specs/specs.assign.c#L2048) |
| obj | 92020 | `generic_parry_proc` | [src/specs/specs.assign.c:2049](../../../src/specs/specs.assign.c#L2049) |
| obj | 92121 | `flame_of_north` | [src/specs/specs.assign.c:2053](../../../src/specs/specs.assign.c#L2053) |

## Reset coverage

731 parsed reset commands: D: 226, E: 131, F: 8, G: 11, M: 256, O: 55, P: 44.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
