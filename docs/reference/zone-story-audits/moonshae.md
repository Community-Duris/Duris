# Moonshae Island: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence moonshae \
  --evidence-format markdown --output docs/reference/zone-story-audits/moonshae.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 26208 | `give=I:26233;receive=E:99000,I:26235;disappear=0` | request: Return the lost sword to Tristan | [areas/qst/moonshae.qst:18](../../../areas/qst/moonshae.qst#L18) |
| 26221 | `give=I:26212;receive=I:26209;disappear=1` | request: Bring moonwell proof to Brigit | [areas/qst/moonshae.qst:43](../../../areas/qst/moonshae.qst#L43) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 26208 | hi hello | [areas/qst/moonshae.qst:2](../../../areas/qst/moonshae.qst#L2) |
| 26208 | sword cymrych hugh lost | [areas/qst/moonshae.qst:6](../../../areas/qst/moonshae.qst#L6) |
| 26208 | king byron | [areas/qst/moonshae.qst:13](../../../areas/qst/moonshae.qst#L13) |
| 26221 | sword thieves firbolg firbolgs | [areas/qst/moonshae.qst:29](../../../areas/qst/moonshae.qst#L29) |
| 26221 | orb onyx | [areas/qst/moonshae.qst:38](../../../areas/qst/moonshae.qst#L38) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 26218 | `sister_knight` | [src/specs/specs.assign.c:937](../../../src/specs/specs.assign.c#L937) |
| mob | 26219 | `sister_knight` | [src/specs/specs.assign.c:938](../../../src/specs/specs.assign.c#L938) |
| mob | 26220 | `sister_knight` | [src/specs/specs.assign.c:939](../../../src/specs/specs.assign.c#L939) |
| mob | 26221 | `sister_knight` | [src/specs/specs.assign.c:940](../../../src/specs/specs.assign.c#L940) |
| mob | 26222 | `sister_knight` | [src/specs/specs.assign.c:941](../../../src/specs/specs.assign.c#L941) |

## Reset coverage

165 parsed reset commands: D: 26, E: 40, G: 18, M: 77, O: 4.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
