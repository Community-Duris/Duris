# The Para-Elemental Plane of Magma: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence magma \
  --evidence-format markdown --output docs/reference/zone-story-audits/magma.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 142014 | `give=I:142000;receive=I:142002,I:142003,I:142004;disappear=1` | request: Bring the smoldering heart to Palenian | [areas/qst/magma.qst:34](../../../areas/qst/magma.qst#L34) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 142014 | hi | [areas/qst/magma.qst:2](../../../areas/qst/magma.qst#L2) |
| 142014 | task | [areas/qst/magma.qst:15](../../../areas/qst/magma.qst#L15) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| room | 140854 | `ship_shop_proc` | [src/specs/specs.assign.c:2381](../../../src/specs/specs.assign.c#L2381) |

## Reset coverage

19 parsed reset commands: F: 9, G: 1, M: 9.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
