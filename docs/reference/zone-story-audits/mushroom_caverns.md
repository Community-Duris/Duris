# the Mushroom Caverns: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence mushroom_caverns \
  --evidence-format markdown --output docs/reference/zone-story-audits/mushroom_caverns.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 24021 | `give=I:1515;receive=I:24013;disappear=1` | story: Haz’on’wyz’s stolen half | [areas/qst/mobs_underdark.qst:25](../../../areas/qst/mobs_underdark.qst#L25) |
| 24022 | `give=I:4660;receive=C:150000,I:24014;disappear=1` | story: Ozman’s entrusted half | [areas/qst/mobs_underdark.qst:89](../../../areas/qst/mobs_underdark.qst#L89) |
| 24023 | `give=I:24013,I:24014;receive=E:35000,I:24016,I:24017,I:24018;disappear=1` | story: Kryz’s two amulet halves | [areas/qst/mobs_underdark.qst:127](../../../areas/qst/mobs_underdark.qst#L127) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 24021 | hi hello | [areas/qst/mobs_underdark.qst:2](../../../areas/qst/mobs_underdark.qst#L2) |
| 24021 | draknah | [areas/qst/mobs_underdark.qst:6](../../../areas/qst/mobs_underdark.qst#L6) |
| 24021 | master | [areas/qst/mobs_underdark.qst:11](../../../areas/qst/mobs_underdark.qst#L11) |
| 24021 | weak | [areas/qst/mobs_underdark.qst:17](../../../areas/qst/mobs_underdark.qst#L17) |
| 24022 | hi hello | [areas/qst/mobs_underdark.qst:48](../../../areas/qst/mobs_underdark.qst#L48) |
| 24022 | house | [areas/qst/mobs_underdark.qst:52](../../../areas/qst/mobs_underdark.qst#L52) |
| 24022 | zarbonesti | [areas/qst/mobs_underdark.qst:62](../../../areas/qst/mobs_underdark.qst#L62) |
| 24022 | bregnar | [areas/qst/mobs_underdark.qst:77](../../../areas/qst/mobs_underdark.qst#L77) |
| 24023 | hi hello | [areas/qst/mobs_underdark.qst:106](../../../areas/qst/mobs_underdark.qst#L106) |
| 24023 | hunt hunting stalking | [areas/qst/mobs_underdark.qst:110](../../../areas/qst/mobs_underdark.qst#L110) |
| 24023 | zarbonesti amulet | [areas/qst/mobs_underdark.qst:117](../../../areas/qst/mobs_underdark.qst#L117) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

54 parsed reset commands: D: 4, E: 2, F: 1, M: 39, O: 8.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
