# The Shaughin Settlement: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence shaughin \
  --evidence-format markdown --output docs/reference/zone-story-audits/shaughin.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 6513 | `give=I:6509,I:6509;receive=I:6510;disappear=0` | request: Bring two upper constrictor hearts | [areas/qst/shaughin.qst:40](../../../areas/qst/shaughin.qst#L40) |
| 6513 | `give=I:6613,I:6613;receive=I:6512;disappear=0` | request: Bring two great bear teeth | [areas/qst/shaughin.qst:58](../../../areas/qst/shaughin.qst#L58) |
| 6513 | `give=I:6614,I:6614;receive=I:6513;disappear=0` | request: Bring two mighty panther paws | [areas/qst/shaughin.qst:69](../../../areas/qst/shaughin.qst#L69) |
| 6513 | `give=I:6615,I:6615;receive=I:6511;disappear=0` | request: Bring two cold-blooded hearts | [areas/qst/shaughin.qst:47](../../../areas/qst/shaughin.qst#L47) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 6513 | hi quest | [areas/qst/shaughin.qst:2](../../../areas/qst/shaughin.qst#L2) |
| 6513 | reward trophy | [areas/qst/shaughin.qst:8](../../../areas/qst/shaughin.qst#L8) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| room | 6535 | `inn` | [src/specs/specs.assign.c:2511](../../../src/specs/specs.assign.c#L2511) |

## Reset coverage

136 parsed reset commands: D: 14, E: 5, G: 30, M: 83, O: 4.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
