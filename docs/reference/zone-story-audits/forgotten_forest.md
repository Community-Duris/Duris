# The Forgotten Forest: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence forgotten_forest \
  --evidence-format markdown --output docs/reference/zone-story-audits/forgotten_forest.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 82702 | `give=I:82701;receive=E:18100;disappear=0` | request: Offer a chunk of meat | [areas/qst/forgotten_forest.qst:16](../../../areas/qst/forgotten_forest.qst#L16) |
| 82702 | `give=I:82702;receive=E:14000;disappear=0` | request: Offer a yellow mushroom | [areas/qst/forgotten_forest.qst:9](../../../areas/qst/forgotten_forest.qst#L9) |
| 82702 | `give=I:82703;receive=E:17000;disappear=0` | request: Offer blackberries | [areas/qst/forgotten_forest.qst:2](../../../areas/qst/forgotten_forest.qst#L2) |
| 82702 | `give=I:82706;receive=E:49000;disappear=0` | request: Offer a large chunk of meat | [areas/qst/forgotten_forest.qst:23](../../../areas/qst/forgotten_forest.qst#L23) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

96 parsed reset commands: E: 3, F: 3, G: 3, M: 83, O: 4.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
