# The Twisting Tunnels of the Durian Underdark: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence underdark \
  --evidence-format markdown --output docs/reference/zone-story-audits/underdark.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 700036 | `give=I:700000,I:700001;receive=E:250000,I:700005;disappear=0` | request: Return both ancient amulet halves | [areas/qst/underdark.qst:35](../../../areas/qst/underdark.qst#L35) |
| 700036 | `give=I:700008;receive=C:100000;disappear=0` | request: Return the exact bloody roper skin | [areas/qst/underdark.qst:29](../../../areas/qst/underdark.qst#L29) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 700036 | roper skin bloody | [areas/qst/underdark.qst:2](../../../areas/qst/underdark.qst#L2) |
| 700036 | hi hello hey howdy | [areas/qst/underdark.qst:12](../../../areas/qst/underdark.qst#L12) |
| 700036 | first second half halves ancient amulet amulets | [areas/qst/underdark.qst:18](../../../areas/qst/underdark.qst#L18) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 700004 | `purple_worm` | [src/specs/specs.assign.c:580](../../../src/specs/specs.assign.c#L580) |

## Reset coverage

1090 parsed reset commands: E: 3, F: 28, M: 1059.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
