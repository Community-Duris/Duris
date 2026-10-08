# The Basin Wastes: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence basin_wa \
  --evidence-format markdown --output docs/reference/zone-story-audits/basin_wa.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 34013 | `give=I:34000,I:34030;receive=I:34026;disappear=0` | story: Witch: craft the milky white potion | [areas/qst/basin_wa.qst:63](../../../areas/qst/basin_wa.qst#L63) |
| 34013 | `give=I:34000;receive=C:25000;disappear=0` | story: Witch: sell one useful beetle part | [areas/qst/basin_wa.qst:77](../../../areas/qst/basin_wa.qst#L77) |
| 34013 | `give=I:34001,I:34030;receive=I:34025;disappear=0` | story: Witch: craft the milky brown potion | [areas/qst/basin_wa.qst:56](../../../areas/qst/basin_wa.qst#L56) |
| 34013 | `give=I:34001;receive=C:25000;disappear=0` | story: Witch: sell one useful beetle part | [areas/qst/basin_wa.qst:82](../../../areas/qst/basin_wa.qst#L82) |
| 34013 | `give=I:34002,I:34030;receive=I:34019;disappear=0` | story: Witch: craft the glowing red potion | [areas/qst/basin_wa.qst:49](../../../areas/qst/basin_wa.qst#L49) |
| 34013 | `give=I:34002;receive=C:25000;disappear=0` | story: Witch: sell one useful beetle part | [areas/qst/basin_wa.qst:87](../../../areas/qst/basin_wa.qst#L87) |
| 34013 | `give=I:34003,I:34030;receive=I:34027;disappear=0` | story: Witch: craft the spotted white potion | [areas/qst/basin_wa.qst:70](../../../areas/qst/basin_wa.qst#L70) |
| 34013 | `give=I:34003;receive=C:25000;disappear=0` | story: Witch: sell one useful beetle part | [areas/qst/basin_wa.qst:92](../../../areas/qst/basin_wa.qst#L92) |
| 34013 | `give=I:34018,I:34019,I:34031;receive=E:500000,I:34028;disappear=0` | story: Witch: enchant the ancient signet | [areas/qst/basin_wa.qst:35](../../../areas/qst/basin_wa.qst#L35) |
| 34013 | `give=I:34024;receive=I:34024;disappear=0` | service: Witch: understand the heartstone refusal | [areas/qst/basin_wa.qst:97](../../../areas/qst/basin_wa.qst#L97) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 34013 | hello hi howdy | [areas/qst/basin_wa.qst:2](../../../areas/qst/basin_wa.qst#L2) |
| 34013 | greetings welcome | [areas/qst/basin_wa.qst:6](../../../areas/qst/basin_wa.qst#L6) |
| 34013 | help search yes | [areas/qst/basin_wa.qst:10](../../../areas/qst/basin_wa.qst#L10) |
| 34013 | no nope sorry | [areas/qst/basin_wa.qst:15](../../../areas/qst/basin_wa.qst#L15) |
| 34013 | components | [areas/qst/basin_wa.qst:19](../../../areas/qst/basin_wa.qst#L19) |
| 34013 | ring signet | [areas/qst/basin_wa.qst:24](../../../areas/qst/basin_wa.qst#L24) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 34014 | `block_dir` | [src/specs/specs.assign.c:2185](../../../src/specs/specs.assign.c#L2185) |

## Reset coverage

204 parsed reset commands: E: 47, G: 3, M: 135, O: 7, P: 12.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
