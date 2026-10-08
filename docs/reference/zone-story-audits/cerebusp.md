# Pits of Cerberus: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence cerebusp \
  --evidence-format markdown --output docs/reference/zone-story-audits/cerebusp.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 22006 | `give=I:22013,I:22014,I:22014,I:22015,I:22016;receive=I:22062;disappear=0` | story: Craft Mojo’s coconut belt | [areas/qst/cerebusp.qst:20](../../../areas/qst/cerebusp.qst#L20) |
| 22006 | `give=I:22013,I:22015,I:22015,I:22017,I:22017;receive=I:22028;disappear=0` | story: Craft Mojo’s coconut shoulder guard | [areas/qst/cerebusp.qst:2](../../../areas/qst/cerebusp.qst#L2) |
| 22006 | `give=I:22014,I:22014,I:22014,I:22014,I:22017;receive=I:22061;disappear=0` | story: Craft Mojo’s coconut kilt | [areas/qst/cerebusp.qst:11](../../../areas/qst/cerebusp.qst#L11) |
| 22007 | `give=I:22004;receive=I:22009;disappear=0` | story: Trade the flaming brazier for a prybar | [areas/qst/cerebusp.qst:55](../../../areas/qst/cerebusp.qst#L55) |
| 22007 | `give=I:22021;receive=I:22009;disappear=0` | story: Trade the overfiend’s gauntlets for a prybar | [areas/qst/cerebusp.qst:50](../../../areas/qst/cerebusp.qst#L50) |
| 22007 | `give=I:22045;receive=I:22009;disappear=0` | story: Trade the snake scepter for a prybar | [areas/qst/cerebusp.qst:60](../../../areas/qst/cerebusp.qst#L60) |
| 22029 | `give=I:22022,I:22023,I:22025;receive=I:22024;disappear=0` | story: Ask the imp to craft a bone key | [areas/qst/cerebusp.qst:84](../../../areas/qst/cerebusp.qst#L84) |
| 22029 | `give=I:22046,I:22047;receive=I:22048;disappear=0` | story: Ask the imp to craft a roc bone longbow | [areas/qst/cerebusp.qst:92](../../../areas/qst/cerebusp.qst#L92) |
| 22048 | `give=I:22038,I:22039,I:22040,I:22041;receive=I:22042;disappear=0` | story: Exchange the four badges for a vault key | [areas/qst/cerebusp.qst:100](../../../areas/qst/cerebusp.qst#L100) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 22007 | hello | [areas/qst/cerebusp.qst:31](../../../areas/qst/cerebusp.qst#L31) |
| 22007 | treasure | [areas/qst/cerebusp.qst:43](../../../areas/qst/cerebusp.qst#L43) |
| 22029 | hello | [areas/qst/cerebusp.qst:67](../../../areas/qst/cerebusp.qst#L67) |
| 22029 | key | [areas/qst/cerebusp.qst:72](../../../areas/qst/cerebusp.qst#L72) |
| 22029 | goods | [areas/qst/cerebusp.qst:78](../../../areas/qst/cerebusp.qst#L78) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 22024 | `cerberus_load` | [src/specs/specs.assign.c:283](../../../src/specs/specs.assign.c#L283) |
| obj | 22063 | `master_set` | [src/specs/specs.assign.c:1339](../../../src/specs/specs.assign.c#L1339) |
| obj | 22070 | `revenant_helm` | [src/specs/specs.assign.c:1538](../../../src/specs/specs.assign.c#L1538) |

## Reset coverage

391 parsed reset commands: D: 36, E: 13, F: 12, G: 22, M: 244, O: 60, P: 4.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
