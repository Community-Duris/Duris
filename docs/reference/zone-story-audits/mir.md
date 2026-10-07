# Forest of Mir: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence mir \
  --evidence-format markdown --output docs/reference/zone-story-audits/mir.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 41916 | `give=I:41929;receive=E:100000,I:41930;disappear=0` | request: Bring the fallen priest the light and dark scroll | [areas/qst/mir.qst:10](../../../areas/qst/mir.qst#L10) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 41916 | hello greetings | [areas/qst/mir.qst:2](../../../areas/qst/mir.qst#L2) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 41900 | `mir_spider` | [src/specs/specs.assign.c:303](../../../src/specs/specs.assign.c#L303) |
| mob | 42172 | `red_wyrm_shout` | [src/specs/specs.assign.c:305](../../../src/specs/specs.assign.c#L305) |
| mob | 42173 | `white_wyrm_shout` | [src/specs/specs.assign.c:306](../../../src/specs/specs.assign.c#L306) |
| mob | 42174 | `blue_wyrm_shout` | [src/specs/specs.assign.c:307](../../../src/specs/specs.assign.c#L307) |
| mob | 42166 | `amphisbean` | [src/specs/specs.assign.c:309](../../../src/specs/specs.assign.c#L309) |
| mob | 42167 | `amphisbean` | [src/specs/specs.assign.c:310](../../../src/specs/specs.assign.c#L310) |
| obj | 41918 | `blade_of_paladins` | [src/specs/specs.assign.c:1740](../../../src/specs/specs.assign.c#L1740) |
| obj | 41913 | `fade_drusus` | [src/specs/specs.assign.c:1741](../../../src/specs/specs.assign.c#L1741) |
| obj | 41915 | `lightning_sword` | [src/specs/specs.assign.c:1742](../../../src/specs/specs.assign.c#L1742) |
| obj | 41912 | `elfdawn_sword` | [src/specs/specs.assign.c:1743](../../../src/specs/specs.assign.c#L1743) |
| obj | 41917 | `flame_of_north_sword` | [src/specs/specs.assign.c:1744](../../../src/specs/specs.assign.c#L1744) |
| obj | 41914 | `magebane_falchion` | [src/specs/specs.assign.c:1745](../../../src/specs/specs.assign.c#L1745) |
| obj | 41916 | `woundhealer_scimitar` | [src/specs/specs.assign.c:1746](../../../src/specs/specs.assign.c#L1746) |
| obj | 41911 | `martelo_mstar` | [src/specs/specs.assign.c:1747](../../../src/specs/specs.assign.c#L1747) |
| obj | 41907 | `mir_fire` | [src/specs/specs.assign.c:1748](../../../src/specs/specs.assign.c#L1748) |

## Reset coverage

128 parsed reset commands: D: 6, E: 8, F: 4, G: 28, M: 54, O: 24, P: 4.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
