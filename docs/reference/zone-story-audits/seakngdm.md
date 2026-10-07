# Sea Kingdom: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence seakngdm \
  --evidence-format markdown --output docs/reference/zone-story-audits/seakngdm.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 31531 | `give=I:31529;receive=I:31531;disappear=1` | request: Return the fisherman’s silver amulet half | [areas/qst/seakngdm.qst:9](../../../areas/qst/seakngdm.qst#L9) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 31531 | hello hi poseidon help fish fisherman | [areas/qst/seakngdm.qst:2](../../../areas/qst/seakngdm.qst#L2) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| obj | 31549 | `glowing_necklace` | [src/specs/specs.assign.c:1440](../../../src/specs/specs.assign.c#L1440) |
| obj | 31514 | `SeaKingdom_Tsunami` | [src/specs/specs.assign.c:1706](../../../src/specs/specs.assign.c#L1706) |

## Reset coverage

253 parsed reset commands: D: 20, E: 38, F: 21, G: 10, M: 137, O: 27.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
