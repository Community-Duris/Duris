# The Mountain Settlement of the Harpies: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence harpyht \
  --evidence-format markdown --output docs/reference/zone-story-audits/harpyht.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 31109 | `give=I:31111;receive=;disappear=0` | story: Queen: present the prisoner shackles | [areas/qst/harpyht.qst:2](../../../areas/qst/harpyht.qst#L2) |
| 31118 | `give=I:31104;receive=I:31111;disappear=1` | story: Free the chained dwarf: exchange the rusted key | [areas/qst/harpyht.qst:9](../../../areas/qst/harpyht.qst#L9) |
| 31124 | `give=I:31112;receive=;disappear=0` | Excluded: Normal mobile-special dispatch handles the khan’s feather first: neutral actors directly lose the item and change Harpy/racewar/alignment without a quest receipt, while non-neutral actors are refused. This shadowed native Q is not a safe recorded achievement or daily. Typed actor-state/item/recipient integration and explicit one-time policy are required before any custom path credit. | [areas/qst/harpyht.qst:18](../../../areas/qst/harpyht.qst#L18) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 31103 | `money_changer` | [src/specs/specs.assign.c:801](../../../src/specs/specs.assign.c#L801) |
| mob | 31108 | `gargoyle_master` | [src/specs/specs.assign.c:826](../../../src/specs/specs.assign.c#L826) |
| mob | 31109 | `harpy_good` | [src/specs/specs.assign.c:827](../../../src/specs/specs.assign.c#L827) |
| mob | 31124 | `harpy_evil` | [src/specs/specs.assign.c:828](../../../src/specs/specs.assign.c#L828) |

## Reset coverage

176 parsed reset commands: D: 16, E: 7, F: 2, G: 11, M: 126, O: 13, P: 1.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
