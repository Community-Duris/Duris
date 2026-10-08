# Storm Port: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence stormport \
  --evidence-format markdown --output docs/reference/zone-story-audits/stormport.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 22428 | `give=I:22431,I:22432,I:22433,I:22434;receive=I:22435;disappear=0` | request: Bring Tchan the four Storm Port proofs | [areas/qst/stormport.qst:12](../../../areas/qst/stormport.qst#L12) |
| 22436 | `give=I:22405,I:22406;receive=I:22407;disappear=0` | request: Bring the citysmith granite and leather straps | [areas/qst/stormport.qst:21](../../../areas/qst/stormport.qst#L21) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 22428 | hi hello | [areas/qst/stormport.qst:2](../../../areas/qst/stormport.qst#L2) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 22428 | `clear_epic_task_spec` | [src/specs/specs.assign.c:194](../../../src/specs/specs.assign.c#L194) |
| room | 22439 | `inn` | [src/specs/specs.assign.c:296](../../../src/specs/specs.assign.c#L296) |
| room | 22441 | `ship_shop_proc` | [src/specs/specs.assign.c:2383](../../../src/specs/specs.assign.c#L2383) |
| room | 22481 | `crew_shop_proc` | [src/specs/specs.assign.c:2399](../../../src/specs/specs.assign.c#L2399) |

## Reset coverage

275 parsed reset commands: D: 64, E: 23, F: 3, G: 28, M: 148, O: 8, P: 1.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
