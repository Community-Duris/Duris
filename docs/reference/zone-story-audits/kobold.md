# Kobold Settlement: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence kobold \
  --evidence-format markdown --output docs/reference/zone-story-audits/kobold.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 1420 | `give=C:10000,I:1447,I:1447,I:1447,I:1447,I:1447,I:1447,I:1447,I:1447;receive=I:1448;disappear=0` | service: Szxvu: smelt a silver block (guarded) | [areas/qst/kobold.qst:65](../../../areas/qst/kobold.qst#L65) |
| 1420 | `give=C:170000,I:1448,I:1448;receive=I:1451;disappear=0` | service: Szxvu: silver shield commission (guarded) | [areas/qst/kobold.qst:52](../../../areas/qst/kobold.qst#L52) |
| 1420 | `give=C:20000,I:1431,I:1433;receive=I:1432;disappear=0` | story: Szxvu: fashion gem spectacles (guarded) | [areas/qst/kobold.qst:93](../../../areas/qst/kobold.qst#L93) |
| 1420 | `give=I:1431;receive=I:1431;disappear=0` | service: Szxvu: inspect the demon gems | [areas/qst/kobold.qst:85](../../../areas/qst/kobold.qst#L85) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 1420 | rod rulership tyxru | [areas/qst/kobold.qst:12](../../../areas/qst/kobold.qst#L12) |
| 1420 | mine mines silver | [areas/qst/kobold.qst:20](../../../areas/qst/kobold.qst#L20) |
| 1420 | fee | [areas/qst/kobold.qst:26](../../../areas/qst/kobold.qst#L26) |
| 1420 | block blocks shield | [areas/qst/kobold.qst:32](../../../areas/qst/kobold.qst#L32) |
| 1420 | nugget nuggets | [areas/qst/kobold.qst:41](../../../areas/qst/kobold.qst#L41) |
| 1420 | jewels jewel eyes eye | [areas/qst/kobold.qst:47](../../../areas/qst/kobold.qst#L47) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 1407 | `chicken` | [src/specs/specs.assign.c:410](../../../src/specs/specs.assign.c#L410) |
| mob | 1433 | `stone_crumble` | [src/specs/specs.assign.c:411](../../../src/specs/specs.assign.c#L411) |
| mob | 1436 | `tako_demon` | [src/specs/specs.assign.c:412](../../../src/specs/specs.assign.c#L412) |
| mob | 1437 | `kobold_priest` | [src/specs/specs.assign.c:413](../../../src/specs/specs.assign.c#L413) |
| mob | 1438 | `stone_golem` | [src/specs/specs.assign.c:414](../../../src/specs/specs.assign.c#L414) |
| obj | 1421 | `item_switch` | [src/specs/specs.assign.c:1809](../../../src/specs/specs.assign.c#L1809) |
| obj | 1425 | `item_switch` | [src/specs/specs.assign.c:1810](../../../src/specs/specs.assign.c#L1810) |
| obj | 1427 | `item_switch` | [src/specs/specs.assign.c:1811](../../../src/specs/specs.assign.c#L1811) |
| room | 1444 | `inn` | [src/specs/specs.assign.c:2512](../../../src/specs/specs.assign.c#L2512) |

## Reset coverage

303 parsed reset commands: D: 16, E: 42, G: 28, M: 182, O: 23, P: 12.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
