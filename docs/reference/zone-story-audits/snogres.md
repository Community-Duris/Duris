# Valley of the Snow Ogres: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence snogres \
  --evidence-format markdown --output docs/reference/zone-story-audits/snogres.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 87733 | `give=I:87706;receive=I:87713;disappear=0` | story: Return the lich's reverse-running hourglass | [areas/qst/snogres.qst:19](../../../areas/qst/snogres.qst#L19) |
| 87733 | `give=I:87713,I:87730,I:87731;receive=I:87732;disappear=0` | story: Three distinct shards for the prismatic pyramid | [areas/qst/snogres.qst:34](../../../areas/qst/snogres.qst#L34) |
| 87733 | `give=I:87715;receive=I:87730;disappear=0` | story: An astereater eye for the yellow shard | [areas/qst/snogres.qst:54](../../../areas/qst/snogres.qst#L54) |
| 87733 | `give=I:87716;receive=I:87729;disappear=0` | request: An ice hydra head for the lich | [areas/qst/snogres.qst:71](../../../areas/qst/snogres.qst#L71) |
| 87733 | `give=I:87717;receive=I:87731;disappear=0` | story: An illithid tentacle for the blue shard | [areas/qst/snogres.qst:64](../../../areas/qst/snogres.qst#L64) |
| 87733 | `give=I:87718;receive=I:87714;disappear=0` | story: A swamp-dragon scale for the old robe | [areas/qst/snogres.qst:91](../../../areas/qst/snogres.qst#L91) |
| 87733 | `give=I:87719;receive=I:87727;disappear=0` | request: The lich's sundew reagent request | [areas/qst/snogres.qst:83](../../../areas/qst/snogres.qst#L83) |
| 87733 | `give=I:87725;receive=I:87725;disappear=0` | Excluded: The lich rejects one remorhaz hide and refers the player to Leppts. The same-kind replacement is a refusal, not a quest achievement, new material supply or a required personal step in the armor commission. | [areas/qst/snogres.qst:108](../../../areas/qst/snogres.qst#L108) |
| 87742 | `give=C:2500000,I:87710,I:87711,I:87725,I:87725,I:87725,I:87725,I:87725,I:87725;receive=I:87728;disappear=0` | service: Leppts' remorhaz armor commission | [areas/qst/snogres.qst:159](../../../areas/qst/snogres.qst#L159) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 87733 | hi hello | [areas/qst/snogres.qst:2](../../../areas/qst/snogres.qst#L2) |
| 87733 | time negative hourglass | [areas/qst/snogres.qst:10](../../../areas/qst/snogres.qst#L10) |
| 87742 | hi hello | [areas/qst/snogres.qst:117](../../../areas/qst/snogres.qst#L117) |
| 87742 | lrethlamn soel lich | [areas/qst/snogres.qst:123](../../../areas/qst/snogres.qst#L123) |
| 87742 | yes sent find | [areas/qst/snogres.qst:130](../../../areas/qst/snogres.qst#L130) |
| 87742 | remorhaz ice worm | [areas/qst/snogres.qst:138](../../../areas/qst/snogres.qst#L138) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 87704 | `block_dir` | [src/specs/specs.assign.c:2192](../../../src/specs/specs.assign.c#L2192) |
| mob | 87741 | `block_dir` | [src/specs/specs.assign.c:2193](../../../src/specs/specs.assign.c#L2193) |
| mob | 87743 | `block_dir` | [src/specs/specs.assign.c:2194](../../../src/specs/specs.assign.c#L2194) |
| mob | 87734 | `block_dir` | [src/specs/specs.assign.c:2195](../../../src/specs/specs.assign.c#L2195) |
| mob | 87733 | `snogres_lich_shout` | [src/specs/specs.assign.c:2196](../../../src/specs/specs.assign.c#L2196) |
| mob | 87734 | `snogres_flesh_golem` | [src/specs/specs.assign.c:2197](../../../src/specs/specs.assign.c#L2197) |
| mob | 87700 | `berserker_toss` | [src/specs/specs.assign.c:2198](../../../src/specs/specs.assign.c#L2198) |
| mob | 87724 | `remo_burn` | [src/specs/specs.assign.c:2199](../../../src/specs/specs.assign.c#L2199) |
| obj | 87712 | `hellfire_axe` | [src/specs/specs.assign.c:2200](../../../src/specs/specs.assign.c#L2200) |
| obj | 87724 | `illithid_whip` | [src/specs/specs.assign.c:2201](../../../src/specs/specs.assign.c#L2201) |
| obj | 87701 | `skull_leggings` | [src/specs/specs.assign.c:2202](../../../src/specs/specs.assign.c#L2202) |
| obj | 87737 | `flesh_golem_repop` | [src/specs/specs.assign.c:2203](../../../src/specs/specs.assign.c#L2203) |

## Reset coverage

200 parsed reset commands: D: 4, E: 22, F: 18, G: 15, M: 139, O: 2.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
