# Tower of High Sorcery: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence tower \
  --evidence-format markdown --output docs/reference/zone-story-audits/tower.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 9321 | `give=I:5011,I:5016,I:5035;receive=C:53000,E:18100,I:9375;disappear=1` | request: Bring Labyrinth materials to Smedgewack | [areas/qst/tower.qst:15](../../../areas/qst/tower.qst#L15) |
| 9367 | `give=I:9365,I:9366;receive=E:33333,I:9369,I:9373;disappear=1` | request: Return the kraken parts to Zbarnos | [areas/qst/tower.qst:47](../../../areas/qst/tower.qst#L47) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 9321 | labyrinth no return maze | [areas/qst/tower.qst:2](../../../areas/qst/tower.qst#L2) |
| 9367 | kraken eye scale | [areas/qst/tower.qst:36](../../../areas/qst/tower.qst#L36) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 9342 | `bulette` | [src/specs/specs.assign.c:1524](../../../src/specs/specs.assign.c#L1524) |
| mob | 9344 | `bulette` | [src/specs/specs.assign.c:1525](../../../src/specs/specs.assign.c#L1525) |
| mob | 9345 | `bulette` | [src/specs/specs.assign.c:1526](../../../src/specs/specs.assign.c#L1526) |
| mob | 9346 | `bulette` | [src/specs/specs.assign.c:1527](../../../src/specs/specs.assign.c#L1527) |
| mob | 9347 | `bulette` | [src/specs/specs.assign.c:1528](../../../src/specs/specs.assign.c#L1528) |
| mob | 9348 | `bulette` | [src/specs/specs.assign.c:1529](../../../src/specs/specs.assign.c#L1529) |
| mob | 9349 | `bulette` | [src/specs/specs.assign.c:1530](../../../src/specs/specs.assign.c#L1530) |
| mob | 9350 | `bulette` | [src/specs/specs.assign.c:1531](../../../src/specs/specs.assign.c#L1531) |

## Reset coverage

233 parsed reset commands: D: 30, E: 89, F: 2, G: 16, M: 93, O: 3.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
