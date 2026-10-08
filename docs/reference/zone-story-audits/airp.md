# The Tempest Court: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence airp \
  --evidence-format markdown --output docs/reference/zone-story-audits/airp.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 131615 | `give=I:131627;receive=I:131628;disappear=0` | story: Al'Hajib's sapphire eyepiece | [areas/qst/airp.qst:14](../../../areas/qst/airp.qst#L14) |
| 131616 | `give=I:8735,I:55553,I:70976,I:88304,I:97066,I:131617,I:131647,I:138268,I:138515;receive=I:131650;disappear=1` | story: Cloudseeker | [areas/qst/airp.qst:98](../../../areas/qst/airp.qst#L98) |
| 131618 | `give=I:131609,I:131610,I:131611;receive=I:131612;disappear=0` | story: Aurilium's palace key | [areas/qst/airp.qst:152](../../../areas/qst/airp.qst#L152) |
| 131621 | `give=I:131615;receive=I:131636;disappear=0` | story: Darthikya's frost gauntlets | [areas/qst/airp.qst:165](../../../areas/qst/airp.qst#L165) |
| 131630 | `give=I:131642,I:131643,I:131644,I:131645,I:131646;receive=I:131647;disappear=0` | story: Chan's Maelstrom fragment | [areas/qst/airp.qst:186](../../../areas/qst/airp.qst#L186) |
| 131635 | `give=I:131605;receive=I:131627;disappear=1` | story: Zieflia's rescue medallion | [areas/qst/airp.qst:205](../../../areas/qst/airp.qst#L205) |
| 131637 | `give=I:96000,I:96012,I:96055,I:131647;receive=I:131648;disappear=0` | story: Fearfrost's hammer | [areas/qst/airp.qst:240](../../../areas/qst/airp.qst#L240) |
| 131651 | `give=I:131627;receive=I:131628;disappear=0` | story: Al'Hajib's sapphire eyepiece | [areas/qst/airp.qst:261](../../../areas/qst/airp.qst#L261) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 131615 | zieflia | [areas/qst/airp.qst:2](../../../areas/qst/airp.qst#L2) |
| 131615 | medallion | [areas/qst/airp.qst:8](../../../areas/qst/airp.qst#L8) |
| 131616 | akadi | [areas/qst/airp.qst:31](../../../areas/qst/airp.qst#L31) |
| 131616 | aurilium | [areas/qst/airp.qst:39](../../../areas/qst/airp.qst#L39) |
| 131616 | yan yan-c-bin | [areas/qst/airp.qst:51](../../../areas/qst/airp.qst#L51) |
| 131616 | ecthius galzron | [areas/qst/airp.qst:60](../../../areas/qst/airp.qst#L60) |
| 131616 | cloud | [areas/qst/airp.qst:69](../../../areas/qst/airp.qst#L69) |
| 131616 | seeker | [areas/qst/airp.qst:78](../../../areas/qst/airp.qst#L78) |
| 131618 | key akadi | [areas/qst/airp.qst:141](../../../areas/qst/airp.qst#L141) |
| 131630 | solution hi | [areas/qst/airp.qst:171](../../../areas/qst/airp.qst#L171) |
| 131630 | maelstrom | [areas/qst/airp.qst:178](../../../areas/qst/airp.qst#L178) |
| 131637 | something hi | [areas/qst/airp.qst:219](../../../areas/qst/airp.qst#L219) |
| 131637 | blessing | [areas/qst/airp.qst:225](../../../areas/qst/airp.qst#L225) |
| 131637 | maelstrom | [areas/qst/airp.qst:232](../../../areas/qst/airp.qst#L232) |
| 131651 | zieflia hi | [areas/qst/airp.qst:249](../../../areas/qst/airp.qst#L249) |
| 131651 | medallion | [areas/qst/airp.qst:255](../../../areas/qst/airp.qst#L255) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| obj | 131616 | `dagger_of_wind` | [src/specs/specs.assign.c:1981](../../../src/specs/specs.assign.c#L1981) |

## Reset coverage

307 parsed reset commands: D: 30, E: 39, F: 8, G: 15, M: 192, O: 17, R: 6.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
