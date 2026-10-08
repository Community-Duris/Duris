# Ny'Neth's Stronghold Continued: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence nyneth3 \
  --evidence-format markdown --output docs/reference/zone-story-audits/nyneth3.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 38736 | `give=I:38741,I:38742,I:38743,I:38744,I:38745,I:38746,I:38747,I:38748,I:38749,I:38750,I:38751,I:38752,I:38753,I:38754;receive=I:38755;disappear=0` | request: Fourteen souls for the Hunger | [areas/qst/nyneth3.qst:6](../../../areas/qst/nyneth3.qst#L6) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 38736 | soul souls | [areas/qst/nyneth3.qst:2](../../../areas/qst/nyneth3.qst#L2) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 38737 | `nyneth` | [src/specs/specs.assign.c:1250](../../../src/specs/specs.assign.c#L1250) |
| obj | 38772 | `platemail_of_defense` | [src/specs/specs.assign.c:1336](../../../src/specs/specs.assign.c#L1336) |
| obj | 38763 | `ring_of_regeneration` | [src/specs/specs.assign.c:1439](../../../src/specs/specs.assign.c#L1439) |
| obj | 38725 | `stormbringer` | [src/specs/specs.assign.c:1589](../../../src/specs/specs.assign.c#L1589) |
| obj | 38761 | `generic_shield_block_proc` | [src/specs/specs.assign.c:2170](../../../src/specs/specs.assign.c#L2170) |

## Reset coverage

246 parsed reset commands: D: 6, E: 60, F: 18, G: 67, M: 84, O: 11.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
