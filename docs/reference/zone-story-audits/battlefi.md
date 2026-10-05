# The Battlefield: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence battlefi \
  --evidence-format markdown --output docs/reference/zone-story-audits/battlefi.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 66400 | `give=I:66400;receive=I:66462;disappear=0` | story: Malok: return Parka | [areas/qst/battlefi.qst:7](../../../areas/qst/battlefi.qst#L7) |
| 66418 | `give=I:66427,I:66427;receive=I:66426;disappear=0` | service: Blacksmith: two-skin vest | [areas/qst/battlefi.qst:22](../../../areas/qst/battlefi.qst#L22) |
| 66428 | `give=I:66415,I:66416,I:66417,I:66425;receive=I:66419;disappear=1` | story: Spirit: four offerings for Righteous | [areas/qst/battlefi.qst:67](../../../areas/qst/battlefi.qst#L67) |
| 66451 | `give=I:66430;receive=I:66458;disappear=0` | story: Corporal: deserter scalp | [areas/qst/battlefi.qst:84](../../../areas/qst/battlefi.qst#L84) |
| 66464 | `give=I:66451;receive=I:66452;disappear=0` | story: Justunian: black magic book | [areas/qst/battlefi.qst:114](../../../areas/qst/battlefi.qst#L114) |
| 66464 | `give=I:66453;receive=I:66454;disappear=0` | story: Justunian: unholy medallion | [areas/qst/battlefi.qst:119](../../../areas/qst/battlefi.qst#L119) |
| 66464 | `give=I:66455;receive=I:66456;disappear=0` | story: Justunian: large war hammer | [areas/qst/battlefi.qst:124](../../../areas/qst/battlefi.qst#L124) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 66400 | hi quest rat parka malok halfling | [areas/qst/battlefi.qst:2](../../../areas/qst/battlefi.qst#L2) |
| 66418 | snakeskin snake skin | [areas/qst/battlefi.qst:14](../../../areas/qst/battlefi.qst#L14) |
| 66418 | quest hi hello eq | [areas/qst/battlefi.qst:18](../../../areas/qst/battlefi.qst#L18) |
| 66428 | hi spirit shimmering quest ghost | [areas/qst/battlefi.qst:30](../../../areas/qst/battlefi.qst#L30) |
| 66428 | mist | [areas/qst/battlefi.qst:51](../../../areas/qst/battlefi.qst#L51) |
| 66428 | sacred cross | [areas/qst/battlefi.qst:55](../../../areas/qst/battlefi.qst#L55) |
| 66428 | skull | [areas/qst/battlefi.qst:59](../../../areas/qst/battlefi.qst#L59) |
| 66428 | cloth material abominable | [areas/qst/battlefi.qst:63](../../../areas/qst/battlefi.qst#L63) |
| 66451 | deserter quest hi | [areas/qst/battlefi.qst:80](../../../areas/qst/battlefi.qst#L80) |
| 66464 | book black minanupoli | [areas/qst/battlefi.qst:91](../../../areas/qst/battlefi.qst#L91) |
| 66464 | unholy crystal medallion | [areas/qst/battlefi.qst:99](../../../areas/qst/battlefi.qst#L99) |
| 66464 | hammer large | [areas/qst/battlefi.qst:107](../../../areas/qst/battlefi.qst#L107) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| obj | 66419 | `righteous_blade` | [src/specs/specs.assign.c:1504](../../../src/specs/specs.assign.c#L1504) |
| room | 66355 | `undead_inn` | [src/specs/specs.assign.c:2330](../../../src/specs/specs.assign.c#L2330) |

## Reset coverage

260 parsed reset commands: D: 42, E: 24, F: 4, G: 20, M: 152, O: 17, P: 1.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
