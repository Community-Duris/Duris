# Neverwind Valley: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence pyramid \
  --evidence-format markdown --output docs/reference/zone-story-audits/pyramid.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 20400 | `give=I:20400,I:20400,I:20400;receive=I:20418;disappear=1` | story: Drinstan: the three kings | [areas/qst/pyramid.qst:30](../../../areas/qst/pyramid.qst#L30) |
| 20409 | `give=I:20419;receive=I:20412;disappear=1` | story: Goar: a father's apology | [areas/qst/pyramid.qst:47](../../../areas/qst/pyramid.qst#L47) |
| 20413 | `give=I:20403,I:20404,I:20405,I:20406;receive=I:20417;disappear=1` | story: Maern: four forest eggs | [areas/qst/pyramid.qst:84](../../../areas/qst/pyramid.qst#L84) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 20400 | hello | [areas/qst/pyramid.qst:2](../../../areas/qst/pyramid.qst#L2) |
| 20400 | yes pyramid | [areas/qst/pyramid.qst:9](../../../areas/qst/pyramid.qst#L9) |
| 20400 | language | [areas/qst/pyramid.qst:15](../../../areas/qst/pyramid.qst#L15) |
| 20400 | favor task | [areas/qst/pyramid.qst:22](../../../areas/qst/pyramid.qst#L22) |
| 20409 | father | [areas/qst/pyramid.qst:43](../../../areas/qst/pyramid.qst#L43) |
| 20410 | crying | [areas/qst/pyramid.qst:59](../../../areas/qst/pyramid.qst#L59) |
| 20410 | yes | [areas/qst/pyramid.qst:66](../../../areas/qst/pyramid.qst#L66) |
| 20413 | birds | [areas/qst/pyramid.qst:73](../../../areas/qst/pyramid.qst#L73) |
| 20413 | yes | [areas/qst/pyramid.qst:78](../../../areas/qst/pyramid.qst#L78) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

155 parsed reset commands: D: 14, E: 27, G: 8, M: 90, O: 8, P: 8.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
