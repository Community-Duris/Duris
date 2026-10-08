# Vecna's Tomb: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence vecna \
  --evidence-format markdown --output docs/reference/zone-story-audits/vecna.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 130023 | `give=I:130034;receive=I:130035;disappear=1` | request: Bring a rotting brain to Kairvo | [areas/qst/vecna.qst:11](../../../areas/qst/vecna.qst#L11) |
| 130024 | `give=I:130033;receive=I:130036;disappear=1` | request: Return the stolen trinket to Kaervek | [areas/qst/vecna.qst:30](../../../areas/qst/vecna.qst#L30) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 130023 | quest vecna hello hi | [areas/qst/vecna.qst:4](../../../areas/qst/vecna.qst#L4) |
| 130024 | quest vecna hello hi | [areas/qst/vecna.qst:22](../../../areas/qst/vecna.qst#L22) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 130002 | `vecna_mob_rebirth` | [src/specs/specs.assign.c:744](../../../src/specs/specs.assign.c#L744) |
| mob | 130003 | `vecna_mob_rebirth` | [src/specs/specs.assign.c:745](../../../src/specs/specs.assign.c#L745) |
| mob | 130004 | `vecna_mob_rebirth` | [src/specs/specs.assign.c:746](../../../src/specs/specs.assign.c#L746) |
| mob | 130005 | `vecna_mob_rebirth` | [src/specs/specs.assign.c:747](../../../src/specs/specs.assign.c#L747) |
| mob | 130030 | `block_dir` | [src/specs/specs.assign.c:748](../../../src/specs/specs.assign.c#L748) |
| mob | 130031 | `block_dir` | [src/specs/specs.assign.c:749](../../../src/specs/specs.assign.c#L749) |
| mob | 130032 | `block_dir` | [src/specs/specs.assign.c:750](../../../src/specs/specs.assign.c#L750) |
| mob | 130033 | `block_dir` | [src/specs/specs.assign.c:751](../../../src/specs/specs.assign.c#L751) |
| mob | 130034 | `block_dir` | [src/specs/specs.assign.c:752](../../../src/specs/specs.assign.c#L752) |
| mob | 130035 | `vecnas_fight_proc` | [src/specs/specs.assign.c:753](../../../src/specs/specs.assign.c#L753) |
| mob | 130016 | `chressan_shout` | [src/specs/specs.assign.c:754](../../../src/specs/specs.assign.c#L754) |
| mob | 130028 | `vecna_black_mass` | [src/specs/specs.assign.c:755](../../../src/specs/specs.assign.c#L755) |
| obj | 130006 | `vecna_deathportal` | [src/specs/specs.assign.c:1680](../../../src/specs/specs.assign.c#L1680) |
| obj | 130003 | `vecna_deathaltar` | [src/specs/specs.assign.c:1681](../../../src/specs/specs.assign.c#L1681) |
| obj | 130040 | `vecna_stonemist` | [src/specs/specs.assign.c:1682](../../../src/specs/specs.assign.c#L1682) |
| obj | 130041 | `vecna_ghosthands` | [src/specs/specs.assign.c:1683](../../../src/specs/specs.assign.c#L1683) |
| obj | 130041 | `vecna_torturerroom` | [src/specs/specs.assign.c:1684](../../../src/specs/specs.assign.c#L1684) |
| obj | 130000 | `vecna_gorge` | [src/specs/specs.assign.c:1685](../../../src/specs/specs.assign.c#L1685) |
| obj | 130013 | `vecna_pestilence` | [src/specs/specs.assign.c:1686](../../../src/specs/specs.assign.c#L1686) |
| obj | 130009 | `vecna_minifist` | [src/specs/specs.assign.c:1687](../../../src/specs/specs.assign.c#L1687) |
| obj | 130007 | `vecna_dispel` | [src/specs/specs.assign.c:1688](../../../src/specs/specs.assign.c#L1688) |
| obj | 130008 | `vecna_boneaxe` | [src/specs/specs.assign.c:1689](../../../src/specs/specs.assign.c#L1689) |
| obj | 130027 | `vecna_staffoaken` | [src/specs/specs.assign.c:1690](../../../src/specs/specs.assign.c#L1690) |
| obj | 130028 | `vecna_krindor_main` | [src/specs/specs.assign.c:1691](../../../src/specs/specs.assign.c#L1691) |
| obj | 130018 | `vecna_death_mask` | [src/specs/specs.assign.c:1692](../../../src/specs/specs.assign.c#L1692) |
| obj | 130038 | `mob_vecna_procs` | [src/specs/specs.assign.c:1693](../../../src/specs/specs.assign.c#L1693) |
| room | 130079 | `vecna_bubble_room` | [src/specs/specs.assign.c:2347](../../../src/specs/specs.assign.c#L2347) |

## Reset coverage

227 parsed reset commands: D: 16, E: 51, F: 3, G: 6, M: 94, O: 52, P: 4, R: 1.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
