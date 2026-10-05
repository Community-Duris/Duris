# The Valoisian Castle: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence val \
  --evidence-format markdown --output docs/reference/zone-story-audits/val.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 38422 | `give=I:38415,I:38416,I:38417;receive=I:38418;disappear=0` | story: King Ulgris: the three family seals | [areas/qst/val.qst:18](../../../areas/qst/val.qst#L18) |
| 38426 | `give=I:38409,I:38410,I:38413,I:38414;receive=I:38420;disappear=1` | story: Hearty dwarf: four crafting models | [areas/qst/val.qst:38](../../../areas/qst/val.qst#L38) |
| 38427 | `give=I:38430,I:38431;receive=I:38432;disappear=0` | story: Stealthy figure: the two royal seals | [areas/qst/val.qst:67](../../../areas/qst/val.qst#L67) |
| 38434 | `give=I:38424;receive=I:38425;disappear=0` | story: Cook: prepare the queen’s dinner | [areas/qst/val.qst:86](../../../areas/qst/val.qst#L86) |
| 38438 | `give=I:38425;receive=I:38428;disappear=0` | story: Queen Napolia: serve the dinner | [areas/qst/val.qst:96](../../../areas/qst/val.qst#L96) |
| 38449 | `give=I:38442;receive=I:38456;disappear=0` | story: Madam Oakencrest: the beautiful roses | [areas/qst/val.qst:104](../../../areas/qst/val.qst#L104) |
| 38449 | `give=I:38451;receive=I:38455;disappear=0` | story: Madam Oakencrest: the white rose | [areas/qst/val.qst:108](../../../areas/qst/val.qst#L108) |
| 38463 | `give=I:38438;receive=I:38439;disappear=0` | story: Elven ambassador: the overdue note | [areas/qst/val.qst:124](../../../areas/qst/val.qst#L124) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 38422 | hi king quest | [areas/qst/val.qst:2](../../../areas/qst/val.qst#L2) |
| 38426 | hi quest dwarf old hearty mountain | [areas/qst/val.qst:27](../../../areas/qst/val.qst#L27) |
| 38427 | hi shadowy figure dark assassin king | [areas/qst/val.qst:55](../../../areas/qst/val.qst#L55) |
| 38434 | hi quest cook bakery wine | [areas/qst/val.qst:75](../../../areas/qst/val.qst#L75) |
| 38463 | hi quest note elven ambassador porter | [areas/qst/val.qst:114](../../../areas/qst/val.qst#L114) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

320 parsed reset commands: D: 106, E: 47, F: 2, G: 18, M: 142, O: 5.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
