# The Charcoal Palace: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence firep \
  --evidence-format markdown --output docs/reference/zone-story-audits/firep.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 88318 | `give=I:88322,I:88323;receive=I:55270,I:88320;disappear=0` | request: Forge the vampiric dragonscale gauntlets | [areas/qst/firep.qst:33](../../../areas/qst/firep.qst#L33) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 88318 | hi hello craft create forge | [areas/qst/firep.qst:2](../../../areas/qst/firep.qst#L2) |
| 88318 | dragonscales dragon dragonscale | [areas/qst/firep.qst:11](../../../areas/qst/firep.qst#L11) |
| 88318 | create forge items | [areas/qst/firep.qst:24](../../../areas/qst/firep.qst#L24) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| obj | 88327 | `zion_fnf` | [src/specs/specs.assign.c:2171](../../../src/specs/specs.assign.c#L2171) |
| mob | 88316 | `kossuth` | [src/specs/specs.assign.c:2229](../../../src/specs/specs.assign.c#L2229) |
| mob | 88319 | `fruaack_shout` | [src/specs/specs.assign.c:2230](../../../src/specs/specs.assign.c#L2230) |
| mob | 88301 | `charcoal_guard` | [src/specs/specs.assign.c:2231](../../../src/specs/specs.assign.c#L2231) |
| mob | 88302 | `charcoal_guard` | [src/specs/specs.assign.c:2232](../../../src/specs/specs.assign.c#L2232) |
| mob | 88303 | `charcoal_guard` | [src/specs/specs.assign.c:2233](../../../src/specs/specs.assign.c#L2233) |
| mob | 88304 | `charcoal_guard` | [src/specs/specs.assign.c:2234](../../../src/specs/specs.assign.c#L2234) |
| mob | 88305 | `charcoal_guard` | [src/specs/specs.assign.c:2235](../../../src/specs/specs.assign.c#L2235) |
| mob | 88306 | `charcoal_guard` | [src/specs/specs.assign.c:2236](../../../src/specs/specs.assign.c#L2236) |
| mob | 88308 | `charcoal_guard` | [src/specs/specs.assign.c:2237](../../../src/specs/specs.assign.c#L2237) |
| mob | 88310 | `charcoal_guard` | [src/specs/specs.assign.c:2238](../../../src/specs/specs.assign.c#L2238) |
| mob | 88323 | `charcoal_guard` | [src/specs/specs.assign.c:2239](../../../src/specs/specs.assign.c#L2239) |
| mob | 88324 | `charcoal_guard` | [src/specs/specs.assign.c:2240](../../../src/specs/specs.assign.c#L2240) |
| mob | 88325 | `charcoal_guard` | [src/specs/specs.assign.c:2241](../../../src/specs/specs.assign.c#L2241) |
| mob | 88329 | `block_dir` | [src/specs/specs.assign.c:2242](../../../src/specs/specs.assign.c#L2242) |

## Reset coverage

281 parsed reset commands: D: 22, E: 32, F: 35, G: 10, M: 172, O: 9, P: 1.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
