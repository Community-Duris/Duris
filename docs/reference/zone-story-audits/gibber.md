# Lair of the Gibberling King: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence gibber \
  --evidence-format markdown --output docs/reference/zone-story-audits/gibber.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 8715 | `give=I:8706;receive=I:8709;disappear=1` | request: Bring the master cook a roper tentacle | [areas/qst/gibber.qst:2](../../../areas/qst/gibber.qst#L2) |
| 8731 | `give=I:8717,I:8734;receive=I:8718;disappear=1` | request: Bring the noble the wand and essence | [areas/qst/gibber.qst:32](../../../areas/qst/gibber.qst#L32) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 8731 | hi hello | [areas/qst/gibber.qst:12](../../../areas/qst/gibber.qst#L12) |
| 8731 | assist | [areas/qst/gibber.qst:19](../../../areas/qst/gibber.qst#L19) |
| 8731 | dismiss soul | [areas/qst/gibber.qst:25](../../../areas/qst/gibber.qst#L25) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

387 parsed reset commands: D: 26, E: 68, F: 30, G: 8, M: 216, O: 34, P: 5.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
