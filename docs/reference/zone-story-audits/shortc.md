# The Orcish Slave Camp: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence shortc \
  --evidence-format markdown --output docs/reference/zone-story-audits/shortc.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 53200 | `give=I:53200;receive=C:1500,I:53201;disappear=0` | story: Slave master: return the missing key | [areas/qst/shortc.qst:16](../../../areas/qst/shortc.qst#L16) |
| 53201 | `give=I:53201;receive=E:15000,I:53203;disappear=1` | story: Barbarian hero: the final steak | [areas/qst/shortc.qst:35](../../../areas/qst/shortc.qst#L35) |
| 53201 | `give=T:19;receive=;disappear=0` | Excluded: The ordinary-food request uses type19 instead of an exact durable item. Active accounting currently refuses type-based offerings; keep this branch excluded until typed admission, selected-root consumption and accepted outcome are qualified. No reward or recipient departure is authored, and it is not the exact steak exchange or a rescue. | [areas/qst/shortc.qst:53](../../../areas/qst/shortc.qst#L53) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 53200 | slaves | [areas/qst/shortc.qst:2](../../../areas/qst/shortc.qst#L2) |
| 53200 | camp encampement | [areas/qst/shortc.qst:7](../../../areas/qst/shortc.qst#L7) |
| 53201 | food | [areas/qst/shortc.qst:27](../../../areas/qst/shortc.qst#L27) |
| 53201 | hi hello howdy greetings help quest camp orc encampenet | [areas/qst/shortc.qst:31](../../../areas/qst/shortc.qst#L31) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

64 parsed reset commands: D: 16, E: 11, G: 3, M: 31, O: 1, P: 2.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
