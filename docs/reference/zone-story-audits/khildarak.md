# Khildarak Stronghold: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence khildarak \
  --evidence-format markdown --output docs/reference/zone-story-audits/khildarak.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 17118 | `give=I:17074;receive=C:5000;disappear=0` | story: An egg sack for a mine clue | [areas/qst/khildarak.qst:36](../../../areas/qst/khildarak.qst#L36) |
| 17248 | `give=I:17022;receive=I:17020;disappear=0` | story: Kraken proof for a rib bone | [areas/qst/khildarak.qst:106](../../../areas/qst/khildarak.qst#L106) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 17059 | khildarak hammer enchantress bone mine | [areas/qst/khildarak.qst:2](../../../areas/qst/khildarak.qst#L2) |
| 17118 | mine mines entrance | [areas/qst/khildarak.qst:18](../../../areas/qst/khildarak.qst#L18) |
| 17118 | item retrieve retreive | [areas/qst/khildarak.qst:27](../../../areas/qst/khildarak.qst#L27) |
| 17248 | bones bone mine | [areas/qst/khildarak.qst:52](../../../areas/qst/khildarak.qst#L52) |
| 17248 | hello hi greetings | [areas/qst/khildarak.qst:64](../../../areas/qst/khildarak.qst#L64) |
| 17248 | journey trapped | [areas/qst/khildarak.qst:71](../../../areas/qst/khildarak.qst#L71) |
| 17248 | kraken krakens | [areas/qst/khildarak.qst:81](../../../areas/qst/khildarak.qst#L81) |
| 17248 | task | [areas/qst/khildarak.qst:90](../../../areas/qst/khildarak.qst#L90) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 17022 | `world_quest` | [src/specs/specs.assign.c:771](../../../src/specs/specs.assign.c#L771) |
| mob | 17012 | `archer` | [src/specs/specs.assign.c:1003](../../../src/specs/specs.assign.c#L1003) |
| mob | 17132 | `devour` | [src/specs/specs.assign.c:1004](../../../src/specs/specs.assign.c#L1004) |
| mob | 17134 | `devour` | [src/specs/specs.assign.c:1004](../../../src/specs/specs.assign.c#L1004) |
| mob | 17092 | `devour` | [src/specs/specs.assign.c:1005](../../../src/specs/specs.assign.c#L1005) |
| mob | 17049 | `devour` | [src/specs/specs.assign.c:1005](../../../src/specs/specs.assign.c#L1005) |
| mob | 17199 | `devour` | [src/specs/specs.assign.c:1006](../../../src/specs/specs.assign.c#L1006) |
| mob | 17202 | `devour` | [src/specs/specs.assign.c:1008](../../../src/specs/specs.assign.c#L1008) |
| mob | 17247 | `guild_guard` | [src/specs/specs.assign.c:1009](../../../src/specs/specs.assign.c#L1009) |
| mob | 17193 | `devour` | [src/specs/specs.assign.c:1010](../../../src/specs/specs.assign.c#L1010) |
| mob | 17194 | `devour` | [src/specs/specs.assign.c:1011](../../../src/specs/specs.assign.c#L1011) |
| mob | 17195 | `devour` | [src/specs/specs.assign.c:1012](../../../src/specs/specs.assign.c#L1012) |
| mob | 17261 | `poison` | [src/specs/specs.assign.c:1013](../../../src/specs/specs.assign.c#L1013) |
| obj | 17021 | `khildarak_warhammer` | [src/specs/specs.assign.c:1322](../../../src/specs/specs.assign.c#L1322) |
| room | 17302 | `pet_shops` | [src/specs/specs.assign.c:2367](../../../src/specs/specs.assign.c#L2367) |
| room | 17591 | `pet_shops` | [src/specs/specs.assign.c:2368](../../../src/specs/specs.assign.c#L2368) |
| room | 17087 | `inn` | [src/specs/specs.assign.c:2369](../../../src/specs/specs.assign.c#L2369) |
| mob | 17264 | `learn_tradeskill` | [src/specs/specs.assign.c:2575](../../../src/specs/specs.assign.c#L2575) |
| mob | 17265 | `assoc_founder` | [src/specs/specs.assign.c:2579](../../../src/specs/specs.assign.c#L2579) |

## Reset coverage

1621 parsed reset commands: D: 528, E: 38, F: 91, G: 232, M: 639, O: 61, P: 5, R: 27.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
