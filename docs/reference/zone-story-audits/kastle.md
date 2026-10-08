# Varathorn Keep: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence kastle \
  --evidence-format markdown --output docs/reference/zone-story-audits/kastle.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 99434 | `give=I:98914,I:98914;receive=C:384000,I:99447;disappear=1` | request: Return the Night Crawler talons | [areas/qst/kastle.qst:24](../../../areas/qst/kastle.qst#L24) |
| 99436 | `give=I:99414;receive=C:50000,E:124000;disappear=1` | request: Return the stolen ring | [areas/qst/kastle.qst:84](../../../areas/qst/kastle.qst#L84) |
| 99453 | `give=I:99430;receive=I:99419;disappear=1` | request: Return Varathorn's ossuary | [areas/qst/kastle.qst:153](../../../areas/qst/kastle.qst#L153) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 99434 | tezcatlipoca night crawler talons | [areas/qst/kastle.qst:2](../../../areas/qst/kastle.qst#L2) |
| 99436 | nrohtereb ring skull snake | [areas/qst/kastle.qst:57](../../../areas/qst/kastle.qst#L57) |
| 99453 | keep history | [areas/qst/kastle.qst:103](../../../areas/qst/kastle.qst#L103) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| obj | 99447 | `nightcrawler_dagger` | [src/specs/specs.assign.c:1503](../../../src/specs/specs.assign.c#L1503) |
| obj | 99432 | `zarthos_vampire_slayer` | [src/specs/specs.assign.c:2585](../../../src/specs/specs.assign.c#L2585) |

## Reset coverage

331 parsed reset commands: D: 32, E: 108, G: 2, M: 181, O: 6, P: 2.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
