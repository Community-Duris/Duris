# The Desert City of Venan'Trut: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence desert \
  --evidence-format markdown --output docs/reference/zone-story-audits/desert.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 49020 | `give=I:49072;receive=I:49099;disappear=0` | story: Miner: bring the large glowing potion | [areas/qst/desert.qst:10](../../../areas/qst/desert.qst#L10) |
| 49061 | `give=I:49171;receive=I:49042;disappear=0` | story: Cloaked figure: deliver the dusty signet | [areas/qst/desert.qst:27](../../../areas/qst/desert.qst#L27) |
| 49087 | `give=I:49057;receive=I:49058;disappear=0` | story: Traveler: supply the exact compass | [areas/qst/desert.qst:43](../../../areas/qst/desert.qst#L43) |
| 49099 | `give=I:49148;receive=I:49167;disappear=0` | story: Wizard: return Vernadad’s signet | [areas/qst/desert.qst:58](../../../areas/qst/desert.qst#L58) |
| 49155 | `give=I:49079;receive=I:49098;disappear=0` | story: Shady merchant: return the smuggled contraband | [areas/qst/desert.qst:74](../../../areas/qst/desert.qst#L74) |
| 49161 | `give=I:49173;receive=I:55371;disappear=0` | story: Eriic: return the hunter medallion | [areas/qst/desert.qst:91](../../../areas/qst/desert.qst#L91) |
| 49220 | `give=I:49027;receive=I:49170;disappear=0` | story: White-robed figure: return the wyrm eyeball | [areas/qst/desert.qst:115](../../../areas/qst/desert.qst#L115) |
| 49223 | `give=I:49063;receive=I:49173;disappear=0` | story: Goranon: retrieve the Queen’s royal garb | [areas/qst/desert.qst:131](../../../areas/qst/desert.qst#L131) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 49020 | miner mortally wounded cavern mines shaft what why who hi quest | [areas/qst/desert.qst:2](../../../areas/qst/desert.qst#L2) |
| 49061 | jolly old fat hi quest kill | [areas/qst/desert.qst:17](../../../areas/qst/desert.qst#L17) |
| 49087 | compass desert hi | [areas/qst/desert.qst:33](../../../areas/qst/desert.qst#L33) |
| 49099 | lo ushuur hi quest | [areas/qst/desert.qst:49](../../../areas/qst/desert.qst#L49) |
| 49155 | hi merchandise arguing buy sell drugs drug contra contraband | [areas/qst/desert.qst:64](../../../areas/qst/desert.qst#L64) |
| 49161 | hi hello howdy | [areas/qst/desert.qst:80](../../../areas/qst/desert.qst#L80) |
| 49220 | wyrm dragon rivers mines hi | [areas/qst/desert.qst:102](../../../areas/qst/desert.qst#L102) |
| 49223 | thri-kreen queen quest hi robe | [areas/qst/desert.qst:121](../../../areas/qst/desert.qst#L121) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 49064 | `world_quest` | [src/specs/specs.assign.c:793](../../../src/specs/specs.assign.c#L793) |
| room | 49051 | `crew_shop_proc` | [src/specs/specs.assign.c:2404](../../../src/specs/specs.assign.c#L2404) |
| room | 49090 | `ship_shop_proc` | [src/specs/specs.assign.c:2419](../../../src/specs/specs.assign.c#L2419) |

## Reset coverage

1256 parsed reset commands: D: 124, E: 168, F: 25, G: 81, M: 814, O: 42, P: 2.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
