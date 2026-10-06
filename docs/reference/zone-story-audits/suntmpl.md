# The Temple of the Sun: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence suntmpl \
  --evidence-format markdown --output docs/reference/zone-story-audits/suntmpl.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 82400 | `give=I:82402,I:82403;receive=I:82420;disappear=0` | request: Bring Trentloss evidence from the forest | [areas/qst/suntmpl.qst:11](../../../areas/qst/suntmpl.qst#L11) |
| 82401 | `give=I:82408;receive=I:82409;disappear=0` | request: Return a sunbeam to the oak spirit | [areas/qst/suntmpl.qst:29](../../../areas/qst/suntmpl.qst#L29) |
| 82407 | `give=I:82406;receive=I:82405;disappear=0` | request: Bring Grentkas a mystical rock | [areas/qst/suntmpl.qst:44](../../../areas/qst/suntmpl.qst#L44) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 82400 | key temple lock | [areas/qst/suntmpl.qst:2](../../../areas/qst/suntmpl.qst#L2) |
| 82401 | temple | [areas/qst/suntmpl.qst:21](../../../areas/qst/suntmpl.qst#L21) |
| 82407 | hi hello hey howdy item items craft | [areas/qst/suntmpl.qst:37](../../../areas/qst/suntmpl.qst#L37) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

297 parsed reset commands: D: 38, E: 17, G: 9, M: 85, O: 148.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
