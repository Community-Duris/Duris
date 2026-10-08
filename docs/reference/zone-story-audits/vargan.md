# Vargan: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence vargan \
  --evidence-format markdown --output docs/reference/zone-story-audits/vargan.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 2104 | `give=C:40000,I:2126,I:2126,I:2127;receive=I:2128;disappear=0` | request: Recover Norkon's stolen armor parts | [areas/qst/vargan.qst:26](../../../areas/qst/vargan.qst#L26) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 2104 | armor | [areas/qst/vargan.qst:2](../../../areas/qst/vargan.qst#L2) |
| 2104 | betrayed | [areas/qst/vargan.qst:7](../../../areas/qst/vargan.qst#L7) |
| 2104 | pieces | [areas/qst/vargan.qst:12](../../../areas/qst/vargan.qst#L12) |
| 2104 | weld | [areas/qst/vargan.qst:17](../../../areas/qst/vargan.qst#L17) |
| 2104 | hello hi | [areas/qst/vargan.qst:21](../../../areas/qst/vargan.qst#L21) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

70 parsed reset commands: D: 4, E: 17, G: 8, M: 35, O: 5, P: 1.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
