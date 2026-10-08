# The Chasm of the Misty Vale: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence mist_chasm \
  --evidence-format markdown --output docs/reference/zone-story-audits/mist_chasm.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 15006 | `give=I:15017,I:15018;receive=I:15008;disappear=1` | request: Earn the shaman potion | [areas/qst/mist_chasm.qst:11](../../../areas/qst/mist_chasm.qst#L11) |
| 15007 | `give=I:15017;receive=C:2000;disappear=1` | request: Sell a small scale | [areas/qst/mist_chasm.qst:35](../../../areas/qst/mist_chasm.qst#L35) |
| 15007 | `give=I:15018;receive=C:4000;disappear=1` | request: Sell a large scale | [areas/qst/mist_chasm.qst:47](../../../areas/qst/mist_chasm.qst#L47) |
| 15018 | `give=I:15017,I:15017,I:15018,I:15018;receive=E:81000,I:15019;disappear=1` | request: Commission the weaver shield | [areas/qst/mist_chasm.qst:69](../../../areas/qst/mist_chasm.qst#L69) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 15006 | scales salamander jade | [areas/qst/mist_chasm.qst:2](../../../areas/qst/mist_chasm.qst#L2) |
| 15007 | scale scales jade salamander | [areas/qst/mist_chasm.qst:29](../../../areas/qst/mist_chasm.qst#L29) |
| 15018 | scales scale salamander jade | [areas/qst/mist_chasm.qst:61](../../../areas/qst/mist_chasm.qst#L61) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

142 parsed reset commands: D: 6, E: 19, G: 24, M: 81, O: 9, P: 3.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
