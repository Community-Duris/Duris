# Menden-on-the-Deep: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence menden \
  --evidence-format markdown --output docs/reference/zone-story-audits/menden.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 88816 | `give=I:4402;receive=C:100000;disappear=1` | request: Return the holy elven relic to Kitan | [areas/qst/menden.qst:19](../../../areas/qst/menden.qst#L19) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 88816 | resurection | [areas/qst/menden.qst:2](../../../areas/qst/menden.qst#L2) |
| 88816 | price | [areas/qst/menden.qst:7](../../../areas/qst/menden.qst#L7) |
| 88816 | relic elves | [areas/qst/menden.qst:11](../../../areas/qst/menden.qst#L11) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 88805 | `menden_fisherman` | [src/specs/specs.assign.c:1088](../../../src/specs/specs.assign.c#L1088) |
| mob | 88806 | `menden_magus` | [src/specs/specs.assign.c:1089](../../../src/specs/specs.assign.c#L1089) |
| mob | 88812 | `menden_inv_serv_die` | [src/specs/specs.assign.c:1090](../../../src/specs/specs.assign.c#L1090) |
| mob | 88813 | `menden_figurine_die` | [src/specs/specs.assign.c:1091](../../../src/specs/specs.assign.c#L1091) |
| mob | 88814 | `crystal_golem_die` | [src/specs/specs.assign.c:1092](../../../src/specs/specs.assign.c#L1092) |
| mob | 88815 | `hippogriff_die` | [src/specs/specs.assign.c:1093](../../../src/specs/specs.assign.c#L1093) |
| obj | 88825 | `menden_figurine` | [src/specs/specs.assign.c:2058](../../../src/specs/specs.assign.c#L2058) |
| obj | 88830 | `llyms_altar` | [src/specs/specs.assign.c:2059](../../../src/specs/specs.assign.c#L2059) |
| obj | 88821 | `magic_pool` | [src/specs/specs.assign.c:2060](../../../src/specs/specs.assign.c#L2060) |
| obj | 88827 | `magic_pool` | [src/specs/specs.assign.c:2061](../../../src/specs/specs.assign.c#L2061) |
| room | 88846 | `ship_shop_proc` | [src/specs/specs.assign.c:2378](../../../src/specs/specs.assign.c#L2378) |

## Reset coverage

130 parsed reset commands: D: 24, E: 12, G: 16, M: 67, O: 9, P: 2.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
