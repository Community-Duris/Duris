# Ashrumite Village: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence ashrumite \
  --evidence-format markdown --output docs/reference/zone-story-audits/ashrumite.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 66017 | `give=C:25000,I:4372,I:66050;receive=I:66051;disappear=0` | service: The current magical necklace commission | [areas/qst/ashrumite.qst:18](../../../areas/qst/ashrumite.qst#L18) |
| 66021 | `give=C:100,I:66048,I:66064;receive=I:66066;disappear=0` | service: The current ring setting recipe | [areas/qst/ashrumite.qst:94](../../../areas/qst/ashrumite.qst#L94) |
| 66021 | `give=C:100,I:66048,I:66065;receive=I:66067;disappear=0` | service: The current earring setting recipe | [areas/qst/ashrumite.qst:87](../../../areas/qst/ashrumite.qst#L87) |
| 66021 | `give=C:1000,I:66044,I:66045,I:66046,I:66047,I:66048,I:66049;receive=I:66050;disappear=0` | service: The current six-material necklace exchange | [areas/qst/ashrumite.qst:101](../../../areas/qst/ashrumite.qst#L101) |
| 66021 | `give=C:20,I:66039;receive=I:66044;disappear=0` | service: The exchange for an amethyst | [areas/qst/ashrumite.qst:57](../../../areas/qst/ashrumite.qst#L57) |
| 66021 | `give=C:20,I:66040;receive=I:66045;disappear=0` | service: The exchange for an exotic tigers-eye gem | [areas/qst/ashrumite.qst:63](../../../areas/qst/ashrumite.qst#L63) |
| 66021 | `give=C:20,I:66041;receive=I:66046;disappear=0` | service: The exchange for a lustrous diamond | [areas/qst/ashrumite.qst:69](../../../areas/qst/ashrumite.qst#L69) |
| 66021 | `give=C:20,I:66042;receive=I:66047;disappear=0` | service: The exchange for a plain silver necklace | [areas/qst/ashrumite.qst:75](../../../areas/qst/ashrumite.qst#L75) |
| 66021 | `give=C:20,I:66043;receive=I:66048;disappear=0` | service: The exchange for an ordinary gem-set silver necklace | [areas/qst/ashrumite.qst:81](../../../areas/qst/ashrumite.qst#L81) |
| 66040 | `give=C:10,I:66032;receive=I:66033;disappear=0` | service: The current refining exchange | [areas/qst/ashrumite.qst:132](../../../areas/qst/ashrumite.qst#L132) |
| 66040 | `give=C:2500,I:66033,I:66033,I:66033,I:66033,I:66033;receive=I:66049;disappear=0` | service: Five raw gems for a magical necklace | [areas/qst/ashrumite.qst:138](../../../areas/qst/ashrumite.qst#L138) |
| 66041 | `give=C:1000;receive=;disappear=0` | service: The paid platinum-disc rumor | [areas/qst/ashrumite.qst:157](../../../areas/qst/ashrumite.qst#L157) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 66017 | necklace | [areas/qst/ashrumite.qst:2](../../../areas/qst/ashrumite.qst#L2) |
| 66017 | platinum disc | [areas/qst/ashrumite.qst:7](../../../areas/qst/ashrumite.qst#L7) |
| 66017 | scrolls potions | [areas/qst/ashrumite.qst:13](../../../areas/qst/ashrumite.qst#L13) |
| 66021 | gem gems cutting | [areas/qst/ashrumite.qst:32](../../../areas/qst/ashrumite.qst#L32) |
| 66021 | set setting jewelry | [areas/qst/ashrumite.qst:36](../../../areas/qst/ashrumite.qst#L36) |
| 66021 | ring | [areas/qst/ashrumite.qst:40](../../../areas/qst/ashrumite.qst#L40) |
| 66021 | necklace | [areas/qst/ashrumite.qst:44](../../../areas/qst/ashrumite.qst#L44) |
| 66021 | earring | [areas/qst/ashrumite.qst:49](../../../areas/qst/ashrumite.qst#L49) |
| 66021 | cost money price | [areas/qst/ashrumite.qst:53](../../../areas/qst/ashrumite.qst#L53) |
| 66040 | nugget ore silver | [areas/qst/ashrumite.qst:115](../../../areas/qst/ashrumite.qst#L115) |
| 66040 | cost price money | [areas/qst/ashrumite.qst:120](../../../areas/qst/ashrumite.qst#L120) |
| 66040 | necklace jewelry | [areas/qst/ashrumite.qst:127](../../../areas/qst/ashrumite.qst#L127) |
| 66041 | disc disk platinum | [areas/qst/ashrumite.qst:151](../../../areas/qst/ashrumite.qst#L151) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 66041 | `world_quest` | [src/specs/specs.assign.c:765](../../../src/specs/specs.assign.c#L765) |
| mob | 66037 | `drunk_one` | [src/specs/specs.assign.c:1073](../../../src/specs/specs.assign.c#L1073) |
| mob | 66001 | `cityguard` | [src/specs/specs.assign.c:1074](../../../src/specs/specs.assign.c#L1074) |
| mob | 66002 | `cityguard` | [src/specs/specs.assign.c:1075](../../../src/specs/specs.assign.c#L1075) |
| mob | 66003 | `cityguard` | [src/specs/specs.assign.c:1076](../../../src/specs/specs.assign.c#L1076) |
| mob | 66031 | `guild_guard` | [src/specs/specs.assign.c:1077](../../../src/specs/specs.assign.c#L1077) |
| mob | 66024 | `guild_guard` | [src/specs/specs.assign.c:1078](../../../src/specs/specs.assign.c#L1078) |
| mob | 66023 | `guild_guard` | [src/specs/specs.assign.c:1079](../../../src/specs/specs.assign.c#L1079) |
| mob | 66025 | `guild_guard` | [src/specs/specs.assign.c:1080](../../../src/specs/specs.assign.c#L1080) |
| mob | 66022 | `guild_guard` | [src/specs/specs.assign.c:1081](../../../src/specs/specs.assign.c#L1081) |
| mob | 66036 | `janitor` | [src/specs/specs.assign.c:1082](../../../src/specs/specs.assign.c#L1082) |
| mob | 66038 | `money_changer` | [src/specs/specs.assign.c:1083](../../../src/specs/specs.assign.c#L1083) |
| room | 66054 | `inn` | [src/specs/specs.assign.c:2548](../../../src/specs/specs.assign.c#L2548) |
| room | 66080 | `dump` | [src/specs/specs.assign.c:2549](../../../src/specs/specs.assign.c#L2549) |
| room | 66116 | `pet_shops` | [src/specs/specs.assign.c:2550](../../../src/specs/specs.assign.c#L2550) |

## Reset coverage

275 parsed reset commands: D: 4, E: 28, G: 101, M: 118, O: 24.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
