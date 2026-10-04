# Ixarkon: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence ixarkon \
  --evidence-format markdown --output docs/reference/zone-story-audits/ixarkon.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 96419 | `give=I:96431;receive=C:25000,E:50000;disappear=0` | story: Ixaruuk: supply the mushroom spore | [areas/qst/ixarkon.qst:20](../../../areas/qst/ixarkon.qst#L20) |
| 96423 | `give=I:96434;receive=E:5500,I:96435;disappear=0` | story: The pacing elder: return Lloth’s spider amulet | [areas/qst/ixarkon.qst:63](../../../areas/qst/ixarkon.qst#L63) |
| 96436 | `give=C:1000000,I:96414;receive=E:50000,I:96434;disappear=0` | service: The drow banker: paid amulet preparation | [areas/qst/ixarkon.qst:116](../../../areas/qst/ixarkon.qst#L116) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 96419 | hi hello | [areas/qst/ixarkon.qst:2](../../../areas/qst/ixarkon.qst#L2) |
| 96419 | arcane studies | [areas/qst/ixarkon.qst:8](../../../areas/qst/ixarkon.qst#L8) |
| 96419 | components | [areas/qst/ixarkon.qst:14](../../../areas/qst/ixarkon.qst#L14) |
| 96423 | hi hello | [areas/qst/ixarkon.qst:33](../../../areas/qst/ixarkon.qst#L33) |
| 96423 | pleasantries busy | [areas/qst/ixarkon.qst:38](../../../areas/qst/ixarkon.qst#L38) |
| 96423 | crisis | [areas/qst/ixarkon.qst:42](../../../areas/qst/ixarkon.qst#L42) |
| 96423 | appease | [areas/qst/ixarkon.qst:49](../../../areas/qst/ixarkon.qst#L49) |
| 96423 | talisman | [areas/qst/ixarkon.qst:56](../../../areas/qst/ixarkon.qst#L56) |
| 96436 | hi hello | [areas/qst/ixarkon.qst:75](../../../areas/qst/ixarkon.qst#L75) |
| 96436 | business | [areas/qst/ixarkon.qst:80](../../../areas/qst/ixarkon.qst#L80) |
| 96436 | menzoberranzan menzo | [areas/qst/ixarkon.qst:85](../../../areas/qst/ixarkon.qst#L85) |
| 96436 | drow | [areas/qst/ixarkon.qst:90](../../../areas/qst/ixarkon.qst#L90) |
| 96436 | slaves slave | [areas/qst/ixarkon.qst:95](../../../areas/qst/ixarkon.qst#L95) |
| 96436 | amulet | [areas/qst/ixarkon.qst:102](../../../areas/qst/ixarkon.qst#L102) |
| 96436 | cost | [areas/qst/ixarkon.qst:108](../../../areas/qst/ixarkon.qst#L108) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 96449 | `money_changer` | [src/specs/specs.assign.c:839](../../../src/specs/specs.assign.c#L839) |
| room | 96549 | `pet_shops` | [src/specs/specs.assign.c:840](../../../src/specs/specs.assign.c#L840) |
| obj | 96402 | `illithid_teleport_veil` | [src/specs/specs.assign.c:1874](../../../src/specs/specs.assign.c#L1874) |
| room | 96537 | `inn` | [src/specs/specs.assign.c:2516](../../../src/specs/specs.assign.c#L2516) |

## Reset coverage

385 parsed reset commands: D: 36, E: 34, G: 29, M: 280, O: 6.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
