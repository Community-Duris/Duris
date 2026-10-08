# The Hall of the Ancients: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence hall \
  --evidence-format markdown --output docs/reference/zone-story-audits/hall.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 77718 | `give=I:77744;receive=I:77745;disappear=0` | service: Prepare a magical steel bar | [areas/qst/hall.qst:22](../../../areas/qst/hall.qst#L22) |
| 77724 | `give=C:10000,I:77724;receive=I:77728;disappear=0` | service: The key for the southern tower | [areas/qst/hall.qst:40](../../../areas/qst/hall.qst#L40) |
| 77724 | `give=C:100000,I:77732;receive=I:77733;disappear=1` | service: The silver shard and snapped key | [areas/qst/hall.qst:48](../../../areas/qst/hall.qst#L48) |
| 77735 | `give=I:77712,I:77712,I:77719,I:77719,I:77729,I:77729,I:77742,I:77742,I:77742,I:77742,I:77748,I:77748,I:77748,I:77748,I:77748,I:77748;receive=I:77749;disappear=1` | story: Jadem's device of protection | [areas/qst/hall.qst:77](../../../areas/qst/hall.qst#L77) |
| 77739 | `give=I:77747;receive=;disappear=0` | Excluded: The elder refusal consumes the same lock of hair without any reward and stays present. It is loaded before the positive ore recipe. It is not a quest/daily achievement or a persisted save-the-son prerequisite. | [areas/qst/hall.qst:114](../../../areas/qst/hall.qst#L114) |
| 77739 | `give=I:77747;receive=I:77719;disappear=1` | Excluded: The hair-for-ore retirement is shadowed by the identical consuming refusal, which the loader prepends and dispatches first. Preserve its native identity and historical receipt, but exclude the unreachable current outcome until a deliberate, qualified repair selects intended eligibility and binding. | [areas/qst/hall.qst:104](../../../areas/qst/hall.qst#L104) |
| 77740 | `give=C:10000,I:77733;receive=I:77739;disappear=0` | service: Repair the snapped silver key | [areas/qst/hall.qst:177](../../../areas/qst/hall.qst#L177) |
| 77740 | `give=I:77713,I:77713,I:77719,I:77719,I:77720,I:77724,I:77724,I:77745,I:77745,I:77750;receive=I:77746;disappear=0` | story: The platemail of awe | [areas/qst/hall.qst:184](../../../areas/qst/hall.qst#L184) |
| 77741 | `give=I:77741;receive=I:77744;disappear=0` | request: The forgotten ring and hidden steel | [areas/qst/hall.qst:207](../../../areas/qst/hall.qst#L207) |
| 77742 | `give=I:18309;receive=I:77743;disappear=1` | story: The tormented child's letter | [areas/qst/hall.qst:223](../../../areas/qst/hall.qst#L223) |
| 77744 | `give=I:77743;receive=I:77747;disappear=1` | story: A letter in the living shades | [areas/qst/hall.qst:235](../../../areas/qst/hall.qst#L235) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 77718 | book | [areas/qst/hall.qst:4](../../../areas/qst/hall.qst#L4) |
| 77718 | neogi | [areas/qst/hall.qst:8](../../../areas/qst/hall.qst#L8) |
| 77718 | bar steel | [areas/qst/hall.qst:17](../../../areas/qst/hall.qst#L17) |
| 77724 | key tower | [areas/qst/hall.qst:30](../../../areas/qst/hall.qst#L30) |
| 77735 | belt | [areas/qst/hall.qst:67](../../../areas/qst/hall.qst#L67) |
| 77735 | parts | [areas/qst/hall.qst:71](../../../areas/qst/hall.qst#L71) |
| 77740 | mad | [areas/qst/hall.qst:122](../../../areas/qst/hall.qst#L122) |
| 77740 | fix | [areas/qst/hall.qst:131](../../../areas/qst/hall.qst#L131) |
| 77740 | brother | [areas/qst/hall.qst:140](../../../areas/qst/hall.qst#L140) |
| 77740 | make | [areas/qst/hall.qst:150](../../../areas/qst/hall.qst#L150) |
| 77740 | items | [areas/qst/hall.qst:159](../../../areas/qst/hall.qst#L159) |
| 77740 | platemail | [areas/qst/hall.qst:165](../../../areas/qst/hall.qst#L165) |
| 77741 | sad crying | [areas/qst/hall.qst:201](../../../areas/qst/hall.qst#L201) |
| 77742 | torment | [areas/qst/hall.qst:219](../../../areas/qst/hall.qst#L219) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 77714 | `morkoth_mother` | [src/specs/specs.assign.c:737](../../../src/specs/specs.assign.c#L737) |
| mob | 77747 | `akckx` | [src/specs/specs.assign.c:738](../../../src/specs/specs.assign.c#L738) |
| mob | 77750 | `human_girl` | [src/specs/specs.assign.c:739](../../../src/specs/specs.assign.c#L739) |
| mob | 77751 | `hoa_death` | [src/specs/specs.assign.c:740](../../../src/specs/specs.assign.c#L740) |
| mob | 77752 | `hoa_sin` | [src/specs/specs.assign.c:741](../../../src/specs/specs.assign.c#L741) |
| obj | 77706 | `trap_razor_hooks` | [src/specs/specs.assign.c:1671](../../../src/specs/specs.assign.c#L1671) |
| obj | 77721 | `trap_tower1_para` | [src/specs/specs.assign.c:1672](../../../src/specs/specs.assign.c#L1672) |
| obj | 77731 | `trap_tower2_sleep` | [src/specs/specs.assign.c:1673](../../../src/specs/specs.assign.c#L1673) |
| obj | 77738 | `illesarus` | [src/specs/specs.assign.c:1674](../../../src/specs/specs.assign.c#L1674) |
| obj | 77734 | `artifact_stone` | [src/specs/specs.assign.c:1675](../../../src/specs/specs.assign.c#L1675) |
| obj | 77752 | `hoa_plat` | [src/specs/specs.assign.c:1676](../../../src/specs/specs.assign.c#L1676) |
| obj | 77749 | `artifact_stone` | [src/specs/specs.assign.c:1677](../../../src/specs/specs.assign.c#L1677) |

## Reset coverage

395 parsed reset commands: D: 48, E: 35, F: 21, G: 19, M: 248, O: 17, P: 7.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
