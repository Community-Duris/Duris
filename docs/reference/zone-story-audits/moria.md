# Neverwinter Woods: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence moria \
  --evidence-format markdown --output docs/reference/zone-story-audits/moria.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 99028 | `give=I:99002,I:99003,I:99004,I:99005,I:99006;receive=I:99009;disappear=0` | story: Malchor's Five Runes | [areas/qst/moria.qst:65](../../../areas/qst/moria.qst#L65) |
| 99028 | `give=I:99002,I:99003,I:99004,I:99005,I:99006;receive=I:99071;disappear=0` | story: Malchor's Five Runes | [areas/qst/moria.qst:54](../../../areas/qst/moria.qst#L54) |
| 99028 | `give=I:99002,I:99003,I:99004,I:99005,I:99006;receive=I:99073;disappear=0` | story: Malchor's Five Runes | [areas/qst/moria.qst:21](../../../areas/qst/moria.qst#L21) |
| 99028 | `give=I:99002,I:99003,I:99004,I:99005,I:99006;receive=I:99074;disappear=0` | story: Malchor's Five Runes | [areas/qst/moria.qst:43](../../../areas/qst/moria.qst#L43) |
| 99028 | `give=I:99002,I:99003,I:99004,I:99005,I:99006;receive=I:99075;disappear=0` | story: Malchor's Five Runes | [areas/qst/moria.qst:32](../../../areas/qst/moria.qst#L32) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 99003 | rune malchor artifact runes | [areas/qst/moria.qst:2](../../../areas/qst/moria.qst#L2) |
| 99028 | masters rune emerald diamond ruby sapphire amethyst | [areas/qst/moria.qst:10](../../../areas/qst/moria.qst#L10) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 99001 | `nw_woodelf` | [src/specs/specs.assign.c:1159](../../../src/specs/specs.assign.c#L1159) |
| mob | 99002 | `nw_elfhealer` | [src/specs/specs.assign.c:1160](../../../src/specs/specs.assign.c#L1160) |
| mob | 99003 | `nw_ammaster` | [src/specs/specs.assign.c:1161](../../../src/specs/specs.assign.c#L1161) |
| mob | 99004 | `nw_sapmaster` | [src/specs/specs.assign.c:1162](../../../src/specs/specs.assign.c#L1162) |
| mob | 99005 | `nw_diamaster` | [src/specs/specs.assign.c:1163](../../../src/specs/specs.assign.c#L1163) |
| mob | 99006 | `nw_rubmaster` | [src/specs/specs.assign.c:1164](../../../src/specs/specs.assign.c#L1164) |
| mob | 99007 | `nw_emmaster` | [src/specs/specs.assign.c:1165](../../../src/specs/specs.assign.c#L1165) |
| mob | 99009 | `nw_human` | [src/specs/specs.assign.c:1166](../../../src/specs/specs.assign.c#L1166) |
| mob | 99010 | `nw_hafbreed` | [src/specs/specs.assign.c:1167](../../../src/specs/specs.assign.c#L1167) |
| mob | 99011 | `nw_owl` | [src/specs/specs.assign.c:1168](../../../src/specs/specs.assign.c#L1168) |
| mob | 99015 | `nw_golem` | [src/specs/specs.assign.c:1169](../../../src/specs/specs.assign.c#L1169) |
| mob | 99016 | `nw_mirroid` | [src/specs/specs.assign.c:1170](../../../src/specs/specs.assign.c#L1170) |
| mob | 99017 | `nw_agatha` | [src/specs/specs.assign.c:1171](../../../src/specs/specs.assign.c#L1171) |
| mob | 99018 | `nw_farmer` | [src/specs/specs.assign.c:1172](../../../src/specs/specs.assign.c#L1172) |
| mob | 99019 | `nw_chicken` | [src/specs/specs.assign.c:1173](../../../src/specs/specs.assign.c#L1173) |
| mob | 99021 | `nw_pig` | [src/specs/specs.assign.c:1174](../../../src/specs/specs.assign.c#L1174) |
| mob | 99022 | `nw_cow` | [src/specs/specs.assign.c:1175](../../../src/specs/specs.assign.c#L1175) |
| mob | 99023 | `nw_chief` | [src/specs/specs.assign.c:1176](../../../src/specs/specs.assign.c#L1176) |
| mob | 99028 | `nw_malchor` | [src/specs/specs.assign.c:1177](../../../src/specs/specs.assign.c#L1177) |
| mob | 99029 | `nw_builder` | [src/specs/specs.assign.c:1178](../../../src/specs/specs.assign.c#L1178) |
| mob | 99030 | `nw_carpen` | [src/specs/specs.assign.c:1179](../../../src/specs/specs.assign.c#L1179) |
| mob | 99031 | `nw_logger` | [src/specs/specs.assign.c:1180](../../../src/specs/specs.assign.c#L1180) |
| mob | 99032 | `nw_cutter` | [src/specs/specs.assign.c:1181](../../../src/specs/specs.assign.c#L1181) |
| mob | 99033 | `nw_foreman` | [src/specs/specs.assign.c:1182](../../../src/specs/specs.assign.c#L1182) |
| mob | 99034 | `nw_ansal` | [src/specs/specs.assign.c:1183](../../../src/specs/specs.assign.c#L1183) |
| mob | 99035 | `nw_brock` | [src/specs/specs.assign.c:1184](../../../src/specs/specs.assign.c#L1184) |
| mob | 99036 | `nw_merthol` | [src/specs/specs.assign.c:1185](../../../src/specs/specs.assign.c#L1185) |
| mob | 99037 | `nw_vitnor` | [src/specs/specs.assign.c:1186](../../../src/specs/specs.assign.c#L1186) |

## Reset coverage

424 parsed reset commands: D: 8, E: 157, G: 44, M: 213, O: 2.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
