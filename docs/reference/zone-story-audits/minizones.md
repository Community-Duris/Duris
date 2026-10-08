# Mini Zones: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence minizones \
  --evidence-format markdown --output docs/reference/zone-story-audits/minizones.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 5750 | `give=I:5750;receive=;disappear=0` | request: The dishwasher and the missing tips | [areas/qst/minizones.qst:7](../../../areas/qst/minizones.qst#L7) |
| 5800 | `give=I:5804;receive=I:5794;disappear=1` | story: Release the city's forgotten knight | [areas/qst/minizones.qst:46](../../../areas/qst/minizones.qst#L46) |
| 5801 | `give=I:5793,I:5794,I:5795,I:5806;receive=I:5805;disappear=1` | story: Restore the silver sword Magik | [areas/qst/minizones.qst:66](../../../areas/qst/minizones.qst#L66) |
| 5813 | `give=C:400000,I:5811,I:500025,I:500025,I:500025,I:500025,I:500025;receive=I:5815;disappear=0` | service: Ghostly sleeves into blood crystal arm plates | [areas/qst/minizones.qst:143](../../../areas/qst/minizones.qst#L143) |
| 5813 | `give=C:400000,I:5812,I:500025,I:500025,I:500025,I:500025,I:500025;receive=I:5816;disappear=0` | service: Ghostly pants into blood crystal leg plates | [areas/qst/minizones.qst:155](../../../areas/qst/minizones.qst#L155) |
| 5813 | `give=C:400000,I:5813,I:500025,I:500025,I:500025,I:500025,I:500025;receive=I:5817;disappear=0` | service: Ghostly gloves into blood crystal gloves | [areas/qst/minizones.qst:166](../../../areas/qst/minizones.qst#L166) |
| 5813 | `give=C:400000,I:5814,I:500025,I:500025,I:500025,I:500025,I:500025;receive=I:5818;disappear=0` | service: Ghostly boots into blood crystal boots | [areas/qst/minizones.qst:177](../../../areas/qst/minizones.qst#L177) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 5750 | waittress | [areas/qst/minizones.qst:2](../../../areas/qst/minizones.qst#L2) |
| 5797 | bastard driedel | [areas/qst/minizones.qst:19](../../../areas/qst/minizones.qst#L19) |
| 5800 | driedel | [areas/qst/minizones.qst:26](../../../areas/qst/minizones.qst#L26) |
| 5800 | worach | [areas/qst/minizones.qst:33](../../../areas/qst/minizones.qst#L33) |
| 5801 | driedel | [areas/qst/minizones.qst:58](../../../areas/qst/minizones.qst#L58) |
| 5813 | hi hello | [areas/qst/minizones.qst:94](../../../areas/qst/minizones.qst#L94) |
| 5813 | blood gem shards | [areas/qst/minizones.qst:103](../../../areas/qst/minizones.qst#L103) |
| 5813 | legplates | [areas/qst/minizones.qst:114](../../../areas/qst/minizones.qst#L114) |
| 5813 | armplates | [areas/qst/minizones.qst:122](../../../areas/qst/minizones.qst#L122) |
| 5813 | gloves | [areas/qst/minizones.qst:129](../../../areas/qst/minizones.qst#L129) |
| 5813 | boots | [areas/qst/minizones.qst:136](../../../areas/qst/minizones.qst#L136) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 5701 | `dryad` | [src/specs/specs.assign.c:681](../../../src/specs/specs.assign.c#L681) |
| mob | 5702 | `dryad` | [src/specs/specs.assign.c:682](../../../src/specs/specs.assign.c#L682) |
| mob | 5739 | `navagator` | [src/specs/specs.assign.c:683](../../../src/specs/specs.assign.c#L683) |
| mob | 5755 | `world_quest` | [src/specs/specs.assign.c:774](../../../src/specs/specs.assign.c#L774) |
| room | 5783 | `pet_shops` | [src/specs/specs.assign.c:2323](../../../src/specs/specs.assign.c#L2323) |
| obj | 5805 | `sword_named_magik` | [src/specs/specs.assign.c:2324](../../../src/specs/specs.assign.c#L2324) |

## Reset coverage

548 parsed reset commands: D: 90, E: 10, F: 19, G: 143, M: 245, O: 20, P: 21.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
