# The Realm of Barovia Continued: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence barovia2 \
  --evidence-format markdown --output docs/reference/zone-story-audits/barovia2.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 58804 | `give=I:58416;receive=I:58849;disappear=1` | story: Urik: news of Perganan | [areas/qst/barovia2.qst:41](../../../areas/qst/barovia2.qst#L41) |
| 58812 | `give=I:58817,I:58824;receive=I:58825,I:58826;disappear=0` | story: Madam Eva: relics of the two witches | [areas/qst/barovia2.qst:132](../../../areas/qst/barovia2.qst#L132) |
| 58812 | `give=I:58844,I:58845;receive=I:58844,I:58846;disappear=0` | story: Madam Eva: the Vistani marks | [areas/qst/barovia2.qst:143](../../../areas/qst/barovia2.qst#L143) |
| 58822 | `give=I:58809;receive=I:58810;disappear=1` | story: Mirkodesiuska: the red dragon's egg | [areas/qst/barovia2.qst:176](../../../areas/qst/barovia2.qst#L176) |
| 58846 | `give=I:58834;receive=I:58826;disappear=1` | story: Megosh: the heart of Chernovog | [areas/qst/barovia2.qst:307](../../../areas/qst/barovia2.qst#L307) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 58804 | hi hello | [areas/qst/barovia2.qst:14](../../../areas/qst/barovia2.qst#L14) |
| 58804 | vistani perganan | [areas/qst/barovia2.qst:26](../../../areas/qst/barovia2.qst#L26) |
| 58812 | hi hello | [areas/qst/barovia2.qst:56](../../../areas/qst/barovia2.qst#L56) |
| 58812 | fortune | [areas/qst/barovia2.qst:66](../../../areas/qst/barovia2.qst#L66) |
| 58812 | strahd castle ravenloft | [areas/qst/barovia2.qst:71](../../../areas/qst/barovia2.qst#L71) |
| 58812 | barovia village | [areas/qst/barovia2.qst:82](../../../areas/qst/barovia2.qst#L82) |
| 58812 | sunsword symbol ravenkind | [areas/qst/barovia2.qst:90](../../../areas/qst/barovia2.qst#L90) |
| 58812 | witches zelena baba drowned lady | [areas/qst/barovia2.qst:104](../../../areas/qst/barovia2.qst#L104) |
| 58812 | vistani | [areas/qst/barovia2.qst:114](../../../areas/qst/barovia2.qst#L114) |
| 58812 | stealth | [areas/qst/barovia2.qst:126](../../../areas/qst/barovia2.qst#L126) |
| 58822 | hi hello | [areas/qst/barovia2.qst:157](../../../areas/qst/barovia2.qst#L157) |
| 58822 | mount ravenloft | [areas/qst/barovia2.qst:168](../../../areas/qst/barovia2.qst#L168) |
| 58846 | hi hello | [areas/qst/barovia2.qst:267](../../../areas/qst/barovia2.qst#L267) |
| 58846 | treasure vistani | [areas/qst/barovia2.qst:273](../../../areas/qst/barovia2.qst#L273) |
| 58846 | witches | [areas/qst/barovia2.qst:287](../../../areas/qst/barovia2.qst#L287) |
| 58846 | devil | [areas/qst/barovia2.qst:300](../../../areas/qst/barovia2.qst#L300) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| obj | 58825 | `barovia_undead_necklace` | [src/specs/specs.assign.c:2175](../../../src/specs/specs.assign.c#L2175) |

## Reset coverage

435 parsed reset commands: D: 22, E: 131, F: 34, G: 47, M: 172, O: 16, P: 12, R: 1.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
