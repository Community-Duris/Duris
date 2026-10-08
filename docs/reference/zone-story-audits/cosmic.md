# The Sky City of Ultarium: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence cosmic \
  --evidence-format markdown --output docs/reference/zone-story-audits/cosmic.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 76019 | `give=I:76027;receive=I:76058;disappear=1` | story: The missing second draft | [areas/qst/cosmic.qst:14](../../../areas/qst/cosmic.qst#L14) |
| 76027 | `give=I:76038;receive=I:76066;disappear=0` | story: Zeenium's soul offering | [areas/qst/cosmic.qst:25](../../../areas/qst/cosmic.qst#L25) |
| 76027 | `give=I:76039;receive=;disappear=0` | service: Other covenant soul offerings | [areas/qst/cosmic.qst:29](../../../areas/qst/cosmic.qst#L29) |
| 76027 | `give=I:76040;receive=;disappear=0` | service: Other covenant soul offerings | [areas/qst/cosmic.qst:32](../../../areas/qst/cosmic.qst#L32) |
| 76027 | `give=I:76041;receive=;disappear=0` | service: Other covenant soul offerings | [areas/qst/cosmic.qst:35](../../../areas/qst/cosmic.qst#L35) |
| 76029 | `give=I:76068;receive=I:76069;disappear=0` | story: The recovered planetary study | [areas/qst/cosmic.qst:40](../../../areas/qst/cosmic.qst#L40) |
| 76047 | `give=I:76038,I:76039,I:76040,I:76041;receive=I:55174,I:76032,I:76050,I:76051;disappear=1` | story: The four-soul seal | [areas/qst/cosmic.qst:59](../../../areas/qst/cosmic.qst#L59) |
| 76048 | `give=I:76028;receive=C:100000;disappear=1` | story: The smuggler's escape key | [areas/qst/cosmic.qst:74](../../../areas/qst/cosmic.qst#L74) |
| 76049 | `give=I:31115;receive=I:76059;disappear=1` | story: The engineer's old golem plans | [areas/qst/cosmic.qst:94](../../../areas/qst/cosmic.qst#L94) |
| 76054 | `give=I:76064;receive=I:76065;disappear=1` | story: The windwalker's lost companion | [areas/qst/cosmic.qst:108](../../../areas/qst/cosmic.qst#L108) |
| 76243 | `give=I:13366;receive=I:76243;disappear=0` | service: Trapper barter: the heart of Abbadon | [areas/qst/cosmic.qst:151](../../../areas/qst/cosmic.qst#L151) |
| 76243 | `give=I:31117;receive=I:202,I:202;disappear=0` | service: Trapper barter: a gigasaur tail | [areas/qst/cosmic.qst:142](../../../areas/qst/cosmic.qst#L142) |
| 76243 | `give=I:40731;receive=C:150000;disappear=0` | service: Trapper barter: an ethereal falcon skull | [areas/qst/cosmic.qst:132](../../../areas/qst/cosmic.qst#L132) |
| 76243 | `give=I:40733;receive=E:150000;disappear=0` | service: Trapper barter: a shade tiger claw | [areas/qst/cosmic.qst:137](../../../areas/qst/cosmic.qst#L137) |
| 76243 | `give=I:76031;receive=I:196,I:196,I:196;disappear=0` | service: Trapper barter: slink fur | [areas/qst/cosmic.qst:126](../../../areas/qst/cosmic.qst#L126) |
| 76243 | `give=I:82561;receive=E:100000;disappear=0` | service: Trapper barter: a su-monster pelt | [areas/qst/cosmic.qst:147](../../../areas/qst/cosmic.qst#L147) |
| 76244 | `give=I:76631;receive=I:76244;disappear=0` | service: Hydra-scale gorget service | [areas/qst/cosmic.qst:158](../../../areas/qst/cosmic.qst#L158) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 76008 | hi hello | [areas/qst/cosmic.qst:2](../../../areas/qst/cosmic.qst#L2) |
| 76013 | service | [areas/qst/cosmic.qst:8](../../../areas/qst/cosmic.qst#L8) |
| 76036 | blueprints | [areas/qst/cosmic.qst:50](../../../areas/qst/cosmic.qst#L50) |
| 76049 | friend golem | [areas/qst/cosmic.qst:83](../../../areas/qst/cosmic.qst#L83) |
| 76243 | hi furst stuff hello howdy hey | [areas/qst/cosmic.qst:119](../../../areas/qst/cosmic.qst#L119) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 76013 | `mob_do_rename_hook` | [src/specs/specs.assign.c:191](../../../src/specs/specs.assign.c#L191) |
| obj | 76032 | `proc_whirlwinds` | [src/specs/specs.assign.c:1443](../../../src/specs/specs.assign.c#L1443) |
| room | 76241 | `pet_shops` | [src/specs/specs.assign.c:2354](../../../src/specs/specs.assign.c#L2354) |

## Reset coverage

338 parsed reset commands: D: 64, E: 31, F: 4, G: 28, M: 154, O: 56, P: 1.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
