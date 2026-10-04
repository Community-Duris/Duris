# Crakkaros' Liar: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence crakkaro \
  --evidence-format markdown --output docs/reference/zone-story-audits/crakkaro.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 87000 | `give=I:87007;receive=I:87000;disappear=0` | story: The centaur's brother | [areas/qst/crakkaro.qst:18](../../../areas/qst/crakkaro.qst#L18) |
| 87014 | `give=I:87016,I:87017,I:87018,I:87019,I:87020;receive=I:87021;disappear=0` | story: Burnhard's five-part shield | [areas/qst/crakkaro.qst:59](../../../areas/qst/crakkaro.qst#L59) |
| 87014 | `give=I:87021;receive=I:87022;disappear=0` | story: Burnhard's second attempt | [areas/qst/crakkaro.qst:73](../../../areas/qst/crakkaro.qst#L73) |
| 87014 | `give=I:87022;receive=I:87023;disappear=0` | story: Burnhard's better creation | [areas/qst/crakkaro.qst:84](../../../areas/qst/crakkaro.qst#L84) |
| 87014 | `give=I:87023;receive=I:87024;disappear=0` | story: Burnhard's final ogre creation | [areas/qst/crakkaro.qst:95](../../../areas/qst/crakkaro.qst#L95) |
| 87014 | `give=I:87028,I:87029;receive=I:87030;disappear=0` | service: Dragon-part eyepatch service | [areas/qst/crakkaro.qst:107](../../../areas/qst/crakkaro.qst#L107) |
| 87014 | `give=I:87031,I:87032,I:87033;receive=I:87034;disappear=0` | service: Dragonkin whip service | [areas/qst/crakkaro.qst:119](../../../areas/qst/crakkaro.qst#L119) |
| 87014 | `give=I:87035,I:87036;receive=I:87037;disappear=0` | service: Illithid ring service | [areas/qst/crakkaro.qst:140](../../../areas/qst/crakkaro.qst#L140) |
| 87014 | `give=I:87039,I:87040,I:87041;receive=I:87042;disappear=0` | service: Bantu-devil horns service | [areas/qst/crakkaro.qst:152](../../../areas/qst/crakkaro.qst#L152) |
| 87014 | `give=I:87080,I:87080,I:87080,I:87080,I:87080,I:87080,I:87080,I:87080,I:87080,I:87080,I:87080,I:87080,I:87080,I:87080,I:87080,I:87080,I:87080;receive=C:100000;disappear=0` | service: Seventeen-fur payment (guarded) | [areas/qst/crakkaro.qst:166](../../../areas/qst/crakkaro.qst#L166) |
| 87030 | `give=I:87044,I:87045,I:87046,I:87047;receive=I:87064;disappear=1` | story: The woman's four temple badges | [areas/qst/crakkaro.qst:211](../../../areas/qst/crakkaro.qst#L211) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 87000 | hello | [areas/qst/crakkaro.qst:2](../../../areas/qst/crakkaro.qst#L2) |
| 87000 | drow dark | [areas/qst/crakkaro.qst:12](../../../areas/qst/crakkaro.qst#L12) |
| 87014 | hello | [areas/qst/crakkaro.qst:30](../../../areas/qst/crakkaro.qst#L30) |
| 87014 | yes | [areas/qst/crakkaro.qst:36](../../../areas/qst/crakkaro.qst#L36) |
| 87014 | no | [areas/qst/crakkaro.qst:42](../../../areas/qst/crakkaro.qst#L42) |
| 87014 | parts | [areas/qst/crakkaro.qst:51](../../../areas/qst/crakkaro.qst#L51) |
| 87030 | lover | [areas/qst/crakkaro.qst:189](../../../areas/qst/crakkaro.qst#L189) |
| 87030 | paladin paladins | [areas/qst/crakkaro.qst:193](../../../areas/qst/crakkaro.qst#L193) |
| 87030 | statue | [areas/qst/crakkaro.qst:198](../../../areas/qst/crakkaro.qst#L198) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

501 parsed reset commands: D: 94, E: 82, F: 12, G: 64, M: 191, O: 51, P: 6, R: 1.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
