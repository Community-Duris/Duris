# Faerie Realm: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence realm \
  --evidence-format markdown --output docs/reference/zone-story-audits/realm.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 14015 | `give=I:14018;receive=C:500,I:14041;disappear=0` | service: Finn's glowing-ring trade | [areas/qst/realm.qst:29](../../../areas/qst/realm.qst#L29) |
| 14015 | `give=I:14028;receive=I:14028;disappear=0` | Excluded: Finn rejects the walnut and returns the same kind without a reward. This replacement response is not a story, personal recovery, or daily objective. | [areas/qst/realm.qst:53](../../../areas/qst/realm.qst#L53) |
| 14015 | `give=I:14036;receive=I:14018,I:14041;disappear=0` | story: Finn's lost signet | [areas/qst/realm.qst:41](../../../areas/qst/realm.qst#L41) |
| 14015 | `give=I:14037;receive=C:500000,I:14023;disappear=1` | story: Finn's castle key and departure | [areas/qst/realm.qst:60](../../../areas/qst/realm.qst#L60) |
| 14028 | `give=I:14001;receive=I:14011,I:14076;disappear=1` | story: Celriya's family blade | [areas/qst/realm.qst:94](../../../areas/qst/realm.qst#L94) |
| 14073 | `give=C:5000000,I:14121,I:14122,I:14123,I:14124,I:14125;receive=I:14126;disappear=0` | service: The five-plane forge service | [areas/qst/realm.qst:125](../../../areas/qst/realm.qst#L125) |
| 14074 | `give=C:5000000,I:14121,I:14122,I:14123,I:14124,I:14125;receive=I:14126;disappear=0` | service: The five-plane forge service | [areas/qst/realm.qst:153](../../../areas/qst/realm.qst#L153) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 14015 | anna cottage | [areas/qst/realm.qst:2](../../../areas/qst/realm.qst#L2) |
| 14015 | ring home leave realm | [areas/qst/realm.qst:9](../../../areas/qst/realm.qst#L9) |
| 14015 | key castle | [areas/qst/realm.qst:20](../../../areas/qst/realm.qst#L20) |
| 14028 | gaelis elf elves ruins temple | [areas/qst/realm.qst:76](../../../areas/qst/realm.qst#L76) |
| 14028 | brother tvelor | [areas/qst/realm.qst:83](../../../areas/qst/realm.qst#L83) |
| 14073 | hi hello | [areas/qst/realm.qst:111](../../../areas/qst/realm.qst#L111) |
| 14073 | forge | [areas/qst/realm.qst:115](../../../areas/qst/realm.qst#L115) |
| 14073 | lost | [areas/qst/realm.qst:119](../../../areas/qst/realm.qst#L119) |
| 14074 | hi hello | [areas/qst/realm.qst:136](../../../areas/qst/realm.qst#L136) |
| 14074 | forge assist | [areas/qst/realm.qst:143](../../../areas/qst/realm.qst#L143) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 14015 | `finn` | [src/specs/specs.assign.c:980](../../../src/specs/specs.assign.c#L980) |
| mob | 14026 | `tree_spirit` | [src/specs/specs.assign.c:981](../../../src/specs/specs.assign.c#L981) |
| mob | 14029 | `faerie` | [src/specs/specs.assign.c:982](../../../src/specs/specs.assign.c#L982) |
| mob | 14048 | `cricket` | [src/specs/specs.assign.c:983](../../../src/specs/specs.assign.c#L983) |
| mob | 14202 | `bridge_troll` | [src/specs/specs.assign.c:988](../../../src/specs/specs.assign.c#L988) |

## Reset coverage

454 parsed reset commands: D: 26, E: 45, G: 66, M: 246, O: 41, P: 30.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
