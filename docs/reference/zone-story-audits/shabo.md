# The Great Shaboath: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence shabo \
  --evidence-format markdown --output docs/reference/zone-story-audits/shabo.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 32827 | `give=I:32812;receive=;disappear=0` | story: The lost notes and tower entry phrase | [areas/qst/shabo.qst:41](../../../areas/qst/shabo.qst#L41) |
| 32878 | `give=I:32842,I:32843,I:32844,I:32845;receive=I:32847;disappear=1` | story: Four tower essences for the spirit’s key | [areas/qst/shabo.qst:87](../../../areas/qst/shabo.qst#L87) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 32827 | default hi hello quest | [areas/qst/shabo.qst:2](../../../areas/qst/shabo.qst#L2) |
| 32827 | aboleth slave illithid | [areas/qst/shabo.qst:9](../../../areas/qst/shabo.qst#L9) |
| 32827 | tower domination heart | [areas/qst/shabo.qst:20](../../../areas/qst/shabo.qst#L20) |
| 32827 | notes phrase secret | [areas/qst/shabo.qst:33](../../../areas/qst/shabo.qst#L33) |
| 32842 | darkaland | [areas/qst/shabo.qst:52](../../../areas/qst/shabo.qst#L52) |
| 32842 | default | [areas/qst/shabo.qst:65](../../../areas/qst/shabo.qst#L65) |
| 32878 | hi hello quest | [areas/qst/shabo.qst:75](../../../areas/qst/shabo.qst#L75) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 32828 | `strychnesch_shout` | [src/specs/specs.assign.c:1041](../../../src/specs/specs.assign.c#L1041) |
| mob | 32830 | `morgoor_shout` | [src/specs/specs.assign.c:1042](../../../src/specs/specs.assign.c#L1042) |
| mob | 32829 | `jabulanth_shout` | [src/specs/specs.assign.c:1043](../../../src/specs/specs.assign.c#L1043) |
| mob | 32831 | `redpal_shout` | [src/specs/specs.assign.c:1044](../../../src/specs/specs.assign.c#L1044) |
| mob | 32832 | `cyvrand_shout` | [src/specs/specs.assign.c:1045](../../../src/specs/specs.assign.c#L1045) |
| mob | 32802 | `overseer_shout` | [src/specs/specs.assign.c:1046](../../../src/specs/specs.assign.c#L1046) |
| mob | 32838 | `shabo_caran` | [src/specs/specs.assign.c:1047](../../../src/specs/specs.assign.c#L1047) |
| obj | 32861 | `artifact_invisible` | [src/specs/specs.assign.c:1404](../../../src/specs/specs.assign.c#L1404) |
| obj | 32831 | `pesky_imp_chest` | [src/specs/specs.assign.c:1451](../../../src/specs/specs.assign.c#L1451) |
| obj | 32832 | `pesky_imp_chest` | [src/specs/specs.assign.c:1452](../../../src/specs/specs.assign.c#L1452) |
| obj | 32833 | `pesky_imp_chest` | [src/specs/specs.assign.c:1453](../../../src/specs/specs.assign.c#L1453) |
| obj | 32822 | `holy_weapon` | [src/specs/specs.assign.c:1454](../../../src/specs/specs.assign.c#L1454) |
| obj | 32836 | `mox_totem` | [src/specs/specs.assign.c:1455](../../../src/specs/specs.assign.c#L1455) |
| obj | 32816 | `flayed_mind_mask` | [src/specs/specs.assign.c:1457](../../../src/specs/specs.assign.c#L1457) |
| obj | 32837 | `finslayer_air` | [src/specs/specs.assign.c:1459](../../../src/specs/specs.assign.c#L1459) |
| obj | 32862 | `aboleth_pendant` | [src/specs/specs.assign.c:1460](../../../src/specs/specs.assign.c#L1460) |
| obj | 32864 | `artifact_stone` | [src/specs/specs.assign.c:1461](../../../src/specs/specs.assign.c#L1461) |
| mob | 32840 | `shabo_butler` | [src/specs/specs.assign.c:1463](../../../src/specs/specs.assign.c#L1463) |
| mob | 32843 | `shabo_petre` | [src/specs/specs.assign.c:1464](../../../src/specs/specs.assign.c#L1464) |
| mob | 32842 | `shabo_palle` | [src/specs/specs.assign.c:1465](../../../src/specs/specs.assign.c#L1465) |
| mob | 32803 | `shabo_derro_savant` | [src/specs/specs.assign.c:1466](../../../src/specs/specs.assign.c#L1466) |
| obj | 32850 | `tower_summoning` | [src/specs/specs.assign.c:1468](../../../src/specs/specs.assign.c#L1468) |
| obj | 32852 | `shabo_trap_north_two` | [src/specs/specs.assign.c:1469](../../../src/specs/specs.assign.c#L1469) |
| obj | 32851 | `shabo_trap_south` | [src/specs/specs.assign.c:1470](../../../src/specs/specs.assign.c#L1470) |
| obj | 32848 | `shabo_trap_south_two` | [src/specs/specs.assign.c:1471](../../../src/specs/specs.assign.c#L1471) |
| obj | 32834 | `shabo_trap_down` | [src/specs/specs.assign.c:1472](../../../src/specs/specs.assign.c#L1472) |
| obj | 32849 | `shabo_trap_up` | [src/specs/specs.assign.c:1473](../../../src/specs/specs.assign.c#L1473) |
| obj | 32826 | `shabo_trap_up_two` | [src/specs/specs.assign.c:1474](../../../src/specs/specs.assign.c#L1474) |

## Reset coverage

299 parsed reset commands: D: 64, E: 26, F: 12, G: 34, M: 137, O: 18, P: 8.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
