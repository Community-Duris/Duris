# The Stone Tomb of Kelek: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence kelek \
  --evidence-format markdown --output docs/reference/zone-story-audits/kelek.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 87860 | `give=I:87869;receive=C:250000,E:100000;disappear=0` | request: Bring proof to the bishop | [areas/qst/church.qst:47](../../../areas/qst/church.qst#L47) |
| 87869 | `give=I:87870,I:87871,I:87872;receive=I:87873;disappear=0` | request: Bring three proofs to the captive paladin | [areas/qst/church.qst:62](../../../areas/qst/church.qst#L62) |
| 87963 | `give=I:87961,I:87962;receive=I:87963;disappear=0` | request: Bring materials to the duergar smith | [areas/qst/kelek.qst:11](../../../areas/qst/kelek.qst#L11) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 87860 | initiation | [areas/qst/church.qst:36](../../../areas/qst/church.qst#L36) |
| 87963 | mithril | [areas/qst/kelek.qst:2](../../../areas/qst/kelek.qst#L2) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 87891 | `world_quest` | [src/specs/specs.assign.c:2207](../../../src/specs/specs.assign.c#L2207) |
| obj | 87950 | `deliverer_hammer` | [src/specs/specs.assign.c:2210](../../../src/specs/specs.assign.c#L2210) |

## Reset coverage

110 parsed reset commands: D: 10, E: 11, F: 1, G: 2, M: 85, O: 1.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
