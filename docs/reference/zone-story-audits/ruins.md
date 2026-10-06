# Village of Refugees: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence ruins \
  --evidence-format markdown --output docs/reference/zone-story-audits/ruins.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 98615 | `give=C:1000,I:98601,I:98602,I:98603,I:98604;receive=I:98605;disappear=0` | service: Anguinel: four-feather ring | [areas/qst/ruins.qst:16](../../../areas/qst/ruins.qst#L16) |
| 98624 | `give=I:98642;receive=C:20000,I:98643,I:98644,I:98645;disappear=1` | story: Farmer: proof from below | [areas/qst/ruins.qst:113](../../../areas/qst/ruins.qst#L113) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 98615 | feathers | [areas/qst/ruins.qst:2](../../../areas/qst/ruins.qst#L2) |
| 98616 | moan | [areas/qst/ruins.qst:43](../../../areas/qst/ruins.qst#L43) |
| 98616 | tragedy | [areas/qst/ruins.qst:47](../../../areas/qst/ruins.qst#L47) |
| 98624 | tragedy know | [areas/qst/ruins.qst:53](../../../areas/qst/ruins.qst#L53) |
| 98624 | tell tragic | [areas/qst/ruins.qst:59](../../../areas/qst/ruins.qst#L59) |
| 98624 | trauma | [areas/qst/ruins.qst:65](../../../areas/qst/ruins.qst#L65) |
| 98624 | surprise monsters | [areas/qst/ruins.qst:75](../../../areas/qst/ruins.qst#L75) |
| 98624 | give venture | [areas/qst/ruins.qst:84](../../../areas/qst/ruins.qst#L84) |
| 98624 | takes attract | [areas/qst/ruins.qst:91](../../../areas/qst/ruins.qst#L91) |
| 98624 | hi hello | [areas/qst/ruins.qst:101](../../../areas/qst/ruins.qst#L101) |
| 98624 | want | [areas/qst/ruins.qst:105](../../../areas/qst/ruins.qst#L105) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

202 parsed reset commands: D: 12, E: 16, F: 15, G: 15, M: 108, O: 20, P: 16.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
