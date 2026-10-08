# Shady Grove: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence shady \
  --evidence-format markdown --output docs/reference/zone-story-audits/shady.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 97510 | `give=I:97514;receive=C:10000,I:97570;disappear=0` | request: Return T'Zoul's diamond collar | [areas/qst/shady.qst:11](../../../areas/qst/shady.qst#L11) |
| 97545 | `give=I:97582;receive=E:2100;disappear=1` | request: Free Morgar with the golden skeleton key | [areas/qst/shady.qst:36](../../../areas/qst/shady.qst#L36) |
| 97548 | `give=I:97572;receive=E:5200;disappear=0` | request: Return Lyren's old bloodied amulet | [areas/qst/shady.qst:61](../../../areas/qst/shady.qst#L61) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 97510 | god | [areas/qst/shady.qst:2](../../../areas/qst/shady.qst#L2) |
| 97537 | cry spouse | [areas/qst/shady.qst:23](../../../areas/qst/shady.qst#L23) |
| 97545 | free help bound | [areas/qst/shady.qst:29](../../../areas/qst/shady.qst#L29) |
| 97548 | think thought thoughts | [areas/qst/shady.qst:48](../../../areas/qst/shady.qst#L48) |
| 97548 | amulet sacred | [areas/qst/shady.qst:56](../../../areas/qst/shady.qst#L56) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 97545 | `troll_slave` | [src/specs/specs.assign.c:346](../../../src/specs/specs.assign.c#L346) |
| mob | 97509 | `stray_dog` | [src/specs/specs.assign.c:347](../../../src/specs/specs.assign.c#L347) |
| mob | 97534 | `hardworking_fisherman` | [src/specs/specs.assign.c:348](../../../src/specs/specs.assign.c#L348) |
| mob | 97554 | `orcish_jailkeeper` | [src/specs/specs.assign.c:349](../../../src/specs/specs.assign.c#L349) |
| mob | 97539 | `orcish_woman` | [src/specs/specs.assign.c:350](../../../src/specs/specs.assign.c#L350) |
| mob | 97540 | `world_quest` | [src/specs/specs.assign.c:730](../../../src/specs/specs.assign.c#L730) |
| mob | 97540 | `world_quest` | [src/specs/specs.assign.c:758](../../../src/specs/specs.assign.c#L758) |
| room | 97757 | `pet_shops` | [src/specs/specs.assign.c:2353](../../../src/specs/specs.assign.c#L2353) |
| room | 97663 | `inn` | [src/specs/specs.assign.c:2356](../../../src/specs/specs.assign.c#L2356) |

## Reset coverage

861 parsed reset commands: D: 84, E: 234, F: 28, G: 119, M: 357, O: 31, R: 8.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
