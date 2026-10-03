# Verspin: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence verspin \
  --evidence-format markdown --output docs/reference/zone-story-audits/verspin.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 28116 | `give=I:28144,I:28144,I:28144,I:28144,I:28144;receive=E:65000,I:28145;disappear=0` | story: Tottan's five totems | [areas/qst/verspin.qst:20](../../../areas/qst/verspin.qst#L20) |
| 28126 | `give=I:28112;receive=I:28113;disappear=0` | service: Ramous's apple-for-bone service | [areas/qst/verspin.qst:40](../../../areas/qst/verspin.qst#L40) |
| 28128 | `give=I:28113;receive=I:28114;disappear=1` | story: The golden lion's collar | [areas/qst/verspin.qst:49](../../../areas/qst/verspin.qst#L49) |
| 28144 | `give=C:10000,I:28112,I:28116;receive=I:28126;disappear=0` | service: Lozin's Verspin hammer service | [areas/qst/verspin.qst:83](../../../areas/qst/verspin.qst#L83) |
| 28144 | `give=C:10000,I:28112;receive=I:28128;disappear=0` | service: Lozin's black gladius service | [areas/qst/verspin.qst:78](../../../areas/qst/verspin.qst#L78) |
| 28144 | `give=C:20000,I:28107;receive=I:28127;disappear=0` | service: Lozin's red steel armor service | [areas/qst/verspin.qst:89](../../../areas/qst/verspin.qst#L89) |
| 28144 | `give=C:5000,I:28124,I:28124,I:28125,I:28125;receive=I:28129;disappear=0` | service: Lozin's hunting knife service | [areas/qst/verspin.qst:94](../../../areas/qst/verspin.qst#L94) |
| 28144 | `give=C:6000,I:28124,I:28124,I:28124,I:28124;receive=I:28130;disappear=0` | service: Lozin's hunting shield service | [areas/qst/verspin.qst:102](../../../areas/qst/verspin.qst#L102) |
| 28145 | `give=I:28146;receive=E:50000,I:223,I:224,I:225;disappear=0` | story: Vulm's stolen amethyst | [areas/qst/verspin.qst:120](../../../areas/qst/verspin.qst#L120) |
| 28147 | `give=I:28138,I:28138,I:28138,I:28138,I:28138;receive=E:50000,I:28139;disappear=1` | story: The shrine's five symbols | [areas/qst/verspin.qst:165](../../../areas/qst/verspin.qst#L165) |
| 28155 | `give=I:28123,I:28124,I:28125;receive=E:70000,I:28141;disappear=0` | story: Transo's three amulets | [areas/qst/verspin.qst:243](../../../areas/qst/verspin.qst#L243) |
| 28173 | `give=I:74298;receive=E:85000,I:28153;disappear=0` | story: The monk's corruption sigil | [areas/qst/verspin.qst:339](../../../areas/qst/verspin.qst#L339) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 28116 | hi hello | [areas/qst/verspin.qst:6](../../../areas/qst/verspin.qst#L6) |
| 28116 | gnomes gnome | [areas/qst/verspin.qst:11](../../../areas/qst/verspin.qst#L11) |
| 28145 | hi hello | [areas/qst/verspin.qst:112](../../../areas/qst/verspin.qst#L112) |
| 28147 | hi hello | [areas/qst/verspin.qst:157](../../../areas/qst/verspin.qst#L157) |
| 28155 | hi hello | [areas/qst/verspin.qst:217](../../../areas/qst/verspin.qst#L217) |
| 28155 | fight | [areas/qst/verspin.qst:222](../../../areas/qst/verspin.qst#L222) |
| 28155 | ramous amulets | [areas/qst/verspin.qst:230](../../../areas/qst/verspin.qst#L230) |
| 28173 | hi hello | [areas/qst/verspin.qst:324](../../../areas/qst/verspin.qst#L324) |
| 28173 | yes pleasant | [areas/qst/verspin.qst:329](../../../areas/qst/verspin.qst#L329) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| room | 28281 | `stat_shops` | [src/specs/specs.assign.c:2332](../../../src/specs/specs.assign.c#L2332) |
| room | 28197 | `crew_shop_proc` | [src/specs/specs.assign.c:2410](../../../src/specs/specs.assign.c#L2410) |

## Reset coverage

389 parsed reset commands: D: 52, E: 20, F: 9, G: 56, M: 233, O: 14, P: 5.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
