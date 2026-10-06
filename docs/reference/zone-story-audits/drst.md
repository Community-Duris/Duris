# Clan Stoutdorf Settlement: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence drst \
  --evidence-format markdown --output docs/reference/zone-story-audits/drst.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 34221 | `give=I:34203,I:34204,I:34205,I:34206,I:34207,I:34209,I:34212,I:34216,I:34216,I:34216,I:34216,I:34216,I:34216,I:34216,I:34216,I:34216;receive=E:100000,I:34217;disappear=0` | request: Gather the tinkerer's collection | [areas/qst/drst.qst:34](../../../areas/qst/drst.qst#L34) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 34221 | hi | [areas/qst/drst.qst:2](../../../areas/qst/drst.qst#L2) |
| 34221 | task | [areas/qst/drst.qst:7](../../../areas/qst/drst.qst#L7) |
| 34221 | list | [areas/qst/drst.qst:15](../../../areas/qst/drst.qst#L15) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| obj | 34226 | `dwarfslayer` | [src/specs/specs.assign.c:1293](../../../src/specs/specs.assign.c#L1293) |

## Reset coverage

414 parsed reset commands: D: 100, E: 23, F: 11, G: 6, M: 259, O: 15.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
