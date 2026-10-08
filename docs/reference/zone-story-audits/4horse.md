# Apocalypse Castle: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence 4horse \
  --evidence-format markdown --output docs/reference/zone-story-audits/4horse.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 34502 | `give=I:34548,I:34549,I:34550,I:34551;receive=I:34552;disappear=0` | request: Bring the four Horsemen skulls to the keeper | [areas/qst/4horse.qst:27](../../../areas/qst/4horse.qst#L27) |
| 34503 | `give=I:34505;receive=I:34527;disappear=1` | request: Return the zombie's lost diamond bracelet | [areas/qst/4horse.qst:52](../../../areas/qst/4horse.qst#L52) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 34502 | horsemen horseman challenge | [areas/qst/4horse.qst:2](../../../areas/qst/4horse.qst#L2) |
| 34502 | portal | [areas/qst/4horse.qst:11](../../../areas/qst/4horse.qst#L11) |
| 34503 | hi | [areas/qst/4horse.qst:38](../../../areas/qst/4horse.qst#L38) |
| 34503 | bracelet | [areas/qst/4horse.qst:42](../../../areas/qst/4horse.qst#L42) |
| 34503 | help | [areas/qst/4horse.qst:47](../../../areas/qst/4horse.qst#L47) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| obj | 34545 | `mankiller` | [src/specs/specs.assign.c:1546](../../../src/specs/specs.assign.c#L1546) |
| obj | 34559 | `brainripper` | [src/specs/specs.assign.c:1585](../../../src/specs/specs.assign.c#L1585) |

## Reset coverage

418 parsed reset commands: D: 50, E: 74, F: 10, G: 14, M: 247, O: 18, P: 5.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
