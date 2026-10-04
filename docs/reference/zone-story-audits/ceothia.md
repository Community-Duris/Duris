# Ceothia: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence ceothia \
  --evidence-format markdown --output docs/reference/zone-story-audits/ceothia.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 80801 | `give=I:80806,I:80810,I:80811;receive=I:80803,I:80813;disappear=1` | story: Choose the surviving thief guild | [areas/qst/ceothia.qst:20](../../../areas/qst/ceothia.qst#L20) |
| 80802 | `give=I:80805,I:80810,I:80811;receive=I:80813,I:80814;disappear=1` | story: Choose the surviving thief guild | [areas/qst/ceothia.qst:60](../../../areas/qst/ceothia.qst#L60) |
| 80803 | `give=I:80813;receive=C:500000,I:80815;disappear=0` | story: Lenbrea: earn trust and open the past | [areas/qst/ceothia.qst:152](../../../areas/qst/ceothia.qst#L152) |
| 80803 | `give=I:81410;receive=C:500000,I:80830;disappear=0` | story: Lenbrea: return the flickering dragon horn | [areas/qst/ceothia.qst:185](../../../areas/qst/ceothia.qst#L185) |
| 80803 | `give=I:81423;receive=I:80832;disappear=0` | story: Lenbrea: return a thread of time | [areas/qst/ceothia.qst:202](../../../areas/qst/ceothia.qst#L202) |
| 80807 | `give=I:80805,I:80806,I:80811;receive=I:80813,I:80817;disappear=1` | story: Choose the surviving thief guild | [areas/qst/ceothia.qst:239](../../../areas/qst/ceothia.qst#L239) |
| 80808 | `give=I:80805,I:80806,I:80810;receive=I:80813,I:80818;disappear=1` | story: Choose the surviving thief guild | [areas/qst/ceothia.qst:274](../../../areas/qst/ceothia.qst#L274) |
| 80875 | `give=I:80826,I:80826;receive=C:200000;disappear=0` | story: Merchant guard: deliver two oaken crates | [areas/qst/ceothia.qst:305](../../../areas/qst/ceothia.qst#L305) |
| 80907 | `give=I:402,I:26614,I:32490;receive=I:404;disappear=0` | story: Captain: exchange the legacy dexterity materials | [areas/qst/ceothia.qst:341](../../../areas/qst/ceothia.qst#L341) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 80801 | hello hi howdy | [areas/qst/ceothia.qst:2](../../../areas/qst/ceothia.qst#L2) |
| 80802 | hello hi howdy | [areas/qst/ceothia.qst:47](../../../areas/qst/ceothia.qst#L47) |
| 80803 | hello hi howdy | [areas/qst/ceothia.qst:85](../../../areas/qst/ceothia.qst#L85) |
| 80803 | legacy | [areas/qst/ceothia.qst:97](../../../areas/qst/ceothia.qst#L97) |
| 80803 | service town | [areas/qst/ceothia.qst:109](../../../areas/qst/ceothia.qst#L109) |
| 80803 | task | [areas/qst/ceothia.qst:129](../../../areas/qst/ceothia.qst#L129) |
| 80807 | hello hi howdy | [areas/qst/ceothia.qst:221](../../../areas/qst/ceothia.qst#L221) |
| 80808 | hi hello howdy | [areas/qst/ceothia.qst:259](../../../areas/qst/ceothia.qst#L259) |
| 80875 | hello hi howdy | [areas/qst/ceothia.qst:292](../../../areas/qst/ceothia.qst#L292) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| obj | 80830 | `ogre_warlords_sword` | [src/specs/specs.assign.c:1323](../../../src/specs/specs.assign.c#L1323) |
| room | 81070 | `inn` | [src/specs/specs.assign.c:2301](../../../src/specs/specs.assign.c#L2301) |
| room | 81028 | `inn` | [src/specs/specs.assign.c:2302](../../../src/specs/specs.assign.c#L2302) |
| room | 81019 | `inn` | [src/specs/specs.assign.c:2303](../../../src/specs/specs.assign.c#L2303) |
| room | 81078 | `inn` | [src/specs/specs.assign.c:2304](../../../src/specs/specs.assign.c#L2304) |
| room | 81003 | `inn` | [src/specs/specs.assign.c:2305](../../../src/specs/specs.assign.c#L2305) |
| room | 81021 | `crew_shop_proc` | [src/specs/specs.assign.c:2405](../../../src/specs/specs.assign.c#L2405) |

## Reset coverage

510 parsed reset commands: D: 10, E: 155, G: 5, M: 327, O: 11, P: 2.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
