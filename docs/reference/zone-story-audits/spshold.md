# Storm Port Stronghold: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence spshold \
  --evidence-format markdown --output docs/reference/zone-story-audits/spshold.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 22617 | `give=I:22613;receive=I:22621;disappear=0` | story: Decker: exchange the ticket for the Master’s helm | [areas/qst/spshold.qst:2](../../../areas/qst/spshold.qst#L2) |
| 22626 | `give=I:22622;receive=I:22625,I:22627,I:40771;disappear=0` | story: Hordine: return the sea maps | [areas/qst/spshold.qst:10](../../../areas/qst/spshold.qst#L10) |
| 22626 | `give=I:77209;receive=I:22633;disappear=0` | story: Hordine: exchange the torn map for the treasure key | [areas/qst/spshold.qst:18](../../../areas/qst/spshold.qst#L18) |
| 22636 | `give=I:22610,I:22612;receive=I:22613;disappear=0` | story: Drifter captain: exchange coal and valve for a ticket | [areas/qst/spshold.qst:31](../../../areas/qst/spshold.qst#L31) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| obj | 22621 | `master_set` | [src/specs/specs.assign.c:1341](../../../src/specs/specs.assign.c#L1341) |
| room | 22648 | `crew_shop_proc` | [src/specs/specs.assign.c:2410](../../../src/specs/specs.assign.c#L2410) |

## Reset coverage

129 parsed reset commands: D: 14, E: 20, F: 4, G: 7, M: 76, O: 7, P: 1.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
