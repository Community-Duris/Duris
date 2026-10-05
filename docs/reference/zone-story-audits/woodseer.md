# Woodseer: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence woodseer \
  --evidence-format markdown --output docs/reference/zone-story-audits/woodseer.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 16530 | `give=I:16526;receive=C:1000;disappear=1` | story: Bard: the broken mandolin | [areas/qst/woodseer.qst:20](../../../areas/qst/woodseer.qst#L20) |
| 16599 | `give=I:16545;receive=C:500;disappear=0` | story: Farmer: birthday dinner honey | [areas/qst/woodseer.qst:45](../../../areas/qst/woodseer.qst#L45) |
| 16614 | `give=I:16561;receive=C:500;disappear=0` | story: Artist: green pigment | [areas/qst/woodseer.qst:68](../../../areas/qst/woodseer.qst#L68) |
| 16625 | `give=I:16546;receive=I:16576;disappear=1` | story: Woodworker: beeswax polish | [areas/qst/woodseer.qst:81](../../../areas/qst/woodseer.qst#L81) |
| 16632 | `give=I:16547;receive=I:16579;disappear=0` | story: Mage: the bee egg | [areas/qst/woodseer.qst:100](../../../areas/qst/woodseer.qst#L100) |
| 16637 | `give=I:16544;receive=C:500;disappear=0` | story: Wizard: the hummingbird feather | [areas/qst/woodseer.qst:122](../../../areas/qst/woodseer.qst#L122) |
| 16688 | `give=I:16543;receive=C:500,I:16610;disappear=0` | story: Maitre d’: turtle soup | [areas/qst/woodseer.qst:140](../../../areas/qst/woodseer.qst#L140) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 16530 | instrument play | [areas/qst/woodseer.qst:8](../../../areas/qst/woodseer.qst#L8) |
| 16530 | park mandolin | [areas/qst/woodseer.qst:13](../../../areas/qst/woodseer.qst#L13) |
| 16599 | searching cupboard recipe | [areas/qst/woodseer.qst:33](../../../areas/qst/woodseer.qst#L33) |
| 16599 | honey | [areas/qst/woodseer.qst:38](../../../areas/qst/woodseer.qst#L38) |
| 16614 | pigment pigments | [areas/qst/woodseer.qst:56](../../../areas/qst/woodseer.qst#L56) |
| 16614 | green | [areas/qst/woodseer.qst:62](../../../areas/qst/woodseer.qst#L62) |
| 16625 | beeswax wax polish | [areas/qst/woodseer.qst:76](../../../areas/qst/woodseer.qst#L76) |
| 16632 | potion potions | [areas/qst/woodseer.qst:93](../../../areas/qst/woodseer.qst#L93) |
| 16637 | search searching components | [areas/qst/woodseer.qst:109](../../../areas/qst/woodseer.qst#L109) |
| 16637 | feather | [areas/qst/woodseer.qst:116](../../../areas/qst/woodseer.qst#L116) |
| 16688 | turtle soup | [areas/qst/woodseer.qst:132](../../../areas/qst/woodseer.qst#L132) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 16553 | `world_quest` | [src/specs/specs.assign.c:732](../../../src/specs/specs.assign.c#L732) |
| mob | 16553 | `world_quest` | [src/specs/specs.assign.c:782](../../../src/specs/specs.assign.c#L782) |
| mob | 16553 | `world_quest` | [src/specs/specs.assign.c:783](../../../src/specs/specs.assign.c#L783) |
| mob | 16553 | `world_quest` | [src/specs/specs.assign.c:784](../../../src/specs/specs.assign.c#L784) |
| mob | 16553 | `world_quest` | [src/specs/specs.assign.c:785](../../../src/specs/specs.assign.c#L785) |
| mob | 16501 | `guild_guard` | [src/specs/specs.assign.c:923](../../../src/specs/specs.assign.c#L923) |
| obj | 16904 | `artifact_invisible` | [src/specs/specs.assign.c:1403](../../../src/specs/specs.assign.c#L1403) |
| room | 16558 | `inn` | [src/specs/specs.assign.c:2537](../../../src/specs/specs.assign.c#L2537) |
| room | 16886 | `pet_shops` | [src/specs/specs.assign.c:2538](../../../src/specs/specs.assign.c#L2538) |

## Reset coverage

1186 parsed reset commands: D: 46, E: 272, G: 164, M: 608, O: 90, P: 6.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
