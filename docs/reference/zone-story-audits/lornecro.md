# The Ancient Halls of Ironstar: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence lornecro \
  --evidence-format markdown --output docs/reference/zone-story-audits/lornecro.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 138908 | `give=I:138913;receive=I:138914;disappear=0` | story: Larra’s lost wedding ring | [areas/qst/lornecro.qst:16](../../../areas/qst/lornecro.qst#L16) |
| 138921 | `give=C:100000,I:138909;receive=I:138945;disappear=0` | service: Dralor’s dragonbone mail | [areas/qst/lornecro.qst:107](../../../areas/qst/lornecro.qst#L107) |
| 138921 | `give=C:1000000,I:138944,I:138952,I:138965;receive=I:138963;disappear=0` | service: Dralor’s demonic dagger | [areas/qst/lornecro.qst:124](../../../areas/qst/lornecro.qst#L124) |
| 138921 | `give=C:1000000,I:138952,I:138962,I:138965;receive=I:138964;disappear=0` | service: Dralor’s demonic hammer | [areas/qst/lornecro.qst:134](../../../areas/qst/lornecro.qst#L134) |
| 138921 | `give=C:500000,I:138943;receive=I:138958;disappear=0` | story: Dralor’s Ironstar vault key | [areas/qst/lornecro.qst:114](../../../areas/qst/lornecro.qst#L114) |
| 138921 | `give=I:138942;receive=I:138943;disappear=0` | story: Haldron’s lost crown | [areas/qst/lornecro.qst:92](../../../areas/qst/lornecro.qst#L92) |
| 138927 | `give=I:138914;receive=I:138950;disappear=1` | story: Robert’s ring and Soulcatcher | [areas/qst/lornecro.qst:228](../../../areas/qst/lornecro.qst#L228) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 138908 | robert necromancer zrack school love lover | [areas/qst/lornecro.qst:2](../../../areas/qst/lornecro.qst#L2) |
| 138921 | hi haldron | [areas/qst/lornecro.qst:37](../../../areas/qst/lornecro.qst#L37) |
| 138921 | ironstar | [areas/qst/lornecro.qst:55](../../../areas/qst/lornecro.qst#L55) |
| 138921 | darksteel | [areas/qst/lornecro.qst:63](../../../areas/qst/lornecro.qst#L63) |
| 138921 | scales dragon | [areas/qst/lornecro.qst:71](../../../areas/qst/lornecro.qst#L71) |
| 138921 | key vault mithril | [areas/qst/lornecro.qst:77](../../../areas/qst/lornecro.qst#L77) |
| 138921 | maltheas demonic weapon dagger warhammer demon mold | [areas/qst/lornecro.qst:87](../../../areas/qst/lornecro.qst#L87) |
| 138927 | larra | [areas/qst/lornecro.qst:148](../../../areas/qst/lornecro.qst#L148) |
| 138927 | zrack | [areas/qst/lornecro.qst:164](../../../areas/qst/lornecro.qst#L164) |
| 138927 | marius | [areas/qst/lornecro.qst:174](../../../areas/qst/lornecro.qst#L174) |
| 138927 | janos | [areas/qst/lornecro.qst:182](../../../areas/qst/lornecro.qst#L182) |
| 138927 | ezzra | [areas/qst/lornecro.qst:188](../../../areas/qst/lornecro.qst#L188) |
| 138927 | ali faethor | [areas/qst/lornecro.qst:195](../../../areas/qst/lornecro.qst#L195) |
| 138927 | darksteel dwarf dwarves ironstar | [areas/qst/lornecro.qst:203](../../../areas/qst/lornecro.qst#L203) |
| 138927 | maltheas angel dark | [areas/qst/lornecro.qst:219](../../../areas/qst/lornecro.qst#L219) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

198 parsed reset commands: D: 50, E: 27, F: 21, G: 7, M: 51, O: 26, P: 16.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
