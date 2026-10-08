# Mitashi - Capital City of the Jade Empire: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence mitashi \
  --evidence-format markdown --output docs/reference/zone-story-audits/mitashi.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 138261 | `give=I:138267,I:138268,I:138269,I:138270,I:138271,I:138272;receive=I:138279;disappear=1` | request: Return the six clan swords to Kunji | [areas/qst/mitashi.qst:48](../../../areas/qst/mitashi.qst#L48) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 138261 | sword swords | [areas/qst/mitashi.qst:2](../../../areas/qst/mitashi.qst#L2) |
| 138261 | torment | [areas/qst/mitashi.qst:17](../../../areas/qst/mitashi.qst#L17) |
| 138261 | empress | [areas/qst/mitashi.qst:30](../../../areas/qst/mitashi.qst#L30) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

337 parsed reset commands: D: 6, E: 145, F: 3, G: 21, M: 162.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
