# Phantasmagoric Caverns: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence valdrak \
  --evidence-format markdown --output docs/reference/zone-story-audits/valdrak.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 53110 | `give=I:53108,I:53117,I:53118;receive=C:81000,E:13300;disappear=1` | request: Return the three spider items to Xolot | [areas/qst/valdrak.qst:40](../../../areas/qst/valdrak.qst#L40) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 53110 | gravf | [areas/qst/valdrak.qst:2](../../../areas/qst/valdrak.qst#L2) |
| 53110 | valdrak | [areas/qst/valdrak.qst:14](../../../areas/qst/valdrak.qst#L14) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

311 parsed reset commands: E: 8, F: 1, G: 20, M: 253, O: 29.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
