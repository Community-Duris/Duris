# Ny'Neth's Stronghold: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence nyneth2 \
  --evidence-format markdown --output docs/reference/zone-story-audits/nyneth2.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 23002 | `give=I:23016,I:23017,I:23018,I:23019,I:23020;receive=I:23021;disappear=0` | request: Five ores for the foreman | [areas/qst/nyneth2.qst:8](../../../areas/qst/nyneth2.qst#L8) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 23002 | tiloxi | [areas/qst/nyneth2.qst:2](../../../areas/qst/nyneth2.qst#L2) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| obj | 23056 | `lifereaver` | [src/specs/specs.assign.c:1702](../../../src/specs/specs.assign.c#L1702) |

## Reset coverage

329 parsed reset commands: D: 24, E: 82, F: 7, G: 63, M: 111, O: 42.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
