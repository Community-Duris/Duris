# The Tharnadian Ruin: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence tharnadian_ruin \
  --evidence-format markdown --output docs/reference/zone-story-audits/tharnadian_ruin.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 5506 | `give=I:1375,I:66225,I:66226,I:66227,I:66230;receive=C:300000,E:275000,I:66200;disappear=0` | story: Five proofs for armoury clearance | [areas/qst/tharnadian_ruin.qst:71](../../../areas/qst/tharnadian_ruin.qst#L71) |
| 5506 | `give=I:66201;receive=I:5504;disappear=0` | story: The necromancer’s proof for the gate key | [areas/qst/tharnadian_ruin.qst:57](../../../areas/qst/tharnadian_ruin.qst#L57) |
| 5514 | `give=I:5513;receive=E:75000;disappear=1` | story: Return the spirit’s black key | [areas/qst/tharnadian_ruin.qst:87](../../../areas/qst/tharnadian_ruin.qst#L87) |
| 5528 | `give=I:1375,I:66225,I:66226,I:66227,I:66230;receive=C:300000,E:275000,I:66200;disappear=0` | story: Five proofs for armoury clearance | [areas/qst/tharnadian_ruin.qst:131](../../../areas/qst/tharnadian_ruin.qst#L131) |
| 5528 | `give=I:66201;receive=I:5504;disappear=1` | story: The necromancer’s proof for the gate key | [areas/qst/tharnadian_ruin.qst:143](../../../areas/qst/tharnadian_ruin.qst#L143) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 5506 | hi help hello tharnadia | [areas/qst/tharnadian_ruin.qst:2](../../../areas/qst/tharnadian_ruin.qst#L2) |
| 5506 | old quarter | [areas/qst/tharnadian_ruin.qst:16](../../../areas/qst/tharnadian_ruin.qst#L16) |
| 5506 | braddistock | [areas/qst/tharnadian_ruin.qst:38](../../../areas/qst/tharnadian_ruin.qst#L38) |
| 5506 | cathedral | [areas/qst/tharnadian_ruin.qst:48](../../../areas/qst/tharnadian_ruin.qst#L48) |
| 5528 | hi hello help quest | [areas/qst/tharnadian_ruin.qst:99](../../../areas/qst/tharnadian_ruin.qst#L99) |
| 5528 | braddistock mansion | [areas/qst/tharnadian_ruin.qst:107](../../../areas/qst/tharnadian_ruin.qst#L107) |
| 5528 | old quarter | [areas/qst/tharnadian_ruin.qst:116](../../../areas/qst/tharnadian_ruin.qst#L116) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 5500 | `guild_guard_one` | [src/specs/specs.assign.c:645](../../../src/specs/specs.assign.c#L645) |
| mob | 5504 | `young_paladin_one` | [src/specs/specs.assign.c:646](../../../src/specs/specs.assign.c#L646) |
| mob | 5505 | `guild_guard_two` | [src/specs/specs.assign.c:647](../../../src/specs/specs.assign.c#L647) |
| mob | 5507 | `wrestler_one` | [src/specs/specs.assign.c:648](../../../src/specs/specs.assign.c#L648) |
| mob | 5508 | `young_mercenary_one` | [src/specs/specs.assign.c:649](../../../src/specs/specs.assign.c#L649) |
| mob | 5511 | `guild_guard_three` | [src/specs/specs.assign.c:650](../../../src/specs/specs.assign.c#L650) |
| mob | 5514 | `young_monk_one` | [src/specs/specs.assign.c:651](../../../src/specs/specs.assign.c#L651) |
| mob | 5516 | `selune_one` | [src/specs/specs.assign.c:652](../../../src/specs/specs.assign.c#L652) |
| mob | 5517 | `selune_two` | [src/specs/specs.assign.c:653](../../../src/specs/specs.assign.c#L653) |
| mob | 5518 | `selune_three` | [src/specs/specs.assign.c:654](../../../src/specs/specs.assign.c#L654) |
| mob | 5519 | `selune_four` | [src/specs/specs.assign.c:655](../../../src/specs/specs.assign.c#L655) |
| mob | 5520 | `selune_five` | [src/specs/specs.assign.c:656](../../../src/specs/specs.assign.c#L656) |
| mob | 5521 | `selune_six` | [src/specs/specs.assign.c:657](../../../src/specs/specs.assign.c#L657) |
| mob | 5523 | `bouncer_four` | [src/specs/specs.assign.c:658](../../../src/specs/specs.assign.c#L658) |
| mob | 5524 | `guild_guard_four` | [src/specs/specs.assign.c:659](../../../src/specs/specs.assign.c#L659) |
| mob | 5527 | `prostitute_one` | [src/specs/specs.assign.c:660](../../../src/specs/specs.assign.c#L660) |
| mob | 5528 | `guild_guard_five` | [src/specs/specs.assign.c:661](../../../src/specs/specs.assign.c#L661) |
| mob | 5531 | `guild_guard_six` | [src/specs/specs.assign.c:662](../../../src/specs/specs.assign.c#L662) |
| mob | 5533 | `young_druid_one` | [src/specs/specs.assign.c:663](../../../src/specs/specs.assign.c#L663) |
| mob | 5535 | `guild_guard_seven` | [src/specs/specs.assign.c:664](../../../src/specs/specs.assign.c#L664) |
| mob | 5537 | `guild_guard_eight` | [src/specs/specs.assign.c:665](../../../src/specs/specs.assign.c#L665) |
| mob | 5538 | `young_necro_one` | [src/specs/specs.assign.c:666](../../../src/specs/specs.assign.c#L666) |
| mob | 5541 | `bouncer_two` | [src/specs/specs.assign.c:667](../../../src/specs/specs.assign.c#L667) |
| mob | 5542 | `bouncer_three` | [src/specs/specs.assign.c:668](../../../src/specs/specs.assign.c#L668) |
| mob | 5543 | `bouncer_one` | [src/specs/specs.assign.c:669](../../../src/specs/specs.assign.c#L669) |
| obj | 5484 | `frost_elb_dagger` | [src/specs/specs.assign.c:1716](../../../src/specs/specs.assign.c#L1716) |
| obj | 5436 | `dagger_submission` | [src/specs/specs.assign.c:1717](../../../src/specs/specs.assign.c#L1717) |
| obj | 5515 | `verzanan_portal` | [src/specs/specs.assign.c:1828](../../../src/specs/specs.assign.c#L1828) |
| obj | 5516 | `verzanan_portal` | [src/specs/specs.assign.c:1829](../../../src/specs/specs.assign.c#L1829) |
| mob | 5503 | `undead_howl` | [src/specs/specs.assign.c:2216](../../../src/specs/specs.assign.c#L2216) |

## Reset coverage

238 parsed reset commands: D: 26, E: 14, F: 34, G: 6, M: 101, O: 52, P: 5.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
