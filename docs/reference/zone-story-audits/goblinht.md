# The Town of Moregeeth: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence goblinht \
  --evidence-format markdown --output docs/reference/zone-story-audits/goblinht.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 70000 | `give=I:70030;receive=C:10000,I:70028;disappear=0` | story: Moreg: end Dura’s blackmail | [areas/qst/goblinht.qst:8](../../../areas/qst/goblinht.qst#L8) |
| 70001 | `give=I:70001;receive=I:70002;disappear=1` | story: Glub: return the killer’s sword | [areas/qst/goblinht.qst:28](../../../areas/qst/goblinht.qst#L28) |
| 70022 | `give=C:10000,I:70093;receive=I:70094;disappear=0` | service: Paid service: enchant an obsidian dart | [areas/qst/goblinht.qst:84](../../../areas/qst/goblinht.qst#L84) |
| 70022 | `give=I:70013,I:70014,I:70015,I:70016;receive=I:70017;disappear=1` | story: Gimbatul: gather the four planar components | [areas/qst/goblinht.qst:70](../../../areas/qst/goblinht.qst#L70) |
| 70022 | `give=I:70021;receive=I:70022;disappear=0` | story: Gimbatul: recover the component pouch | [areas/qst/goblinht.qst:61](../../../areas/qst/goblinht.qst#L61) |
| 70023 | `give=C:5;receive=I:70018;disappear=0` | Excluded: Coin-only, informational, or skill exchanges; supporting services rather than item-delivery story achievements. | [areas/qst/goblinht.qst:98](../../../areas/qst/goblinht.qst#L98) |
| 70031 | `give=I:70026;receive=I:70026;disappear=0` | Excluded: Returned offered items: rejected requests or nonterminal responses, not new story achievements. | [areas/qst/goblinht.qst:105](../../../areas/qst/goblinht.qst#L105) |
| 70060 | `give=I:70030;receive=I:70030;disappear=0` | Excluded: Returned offered items: rejected requests or nonterminal responses, not new story achievements. | [areas/qst/goblinht.qst:119](../../../areas/qst/goblinht.qst#L119) |
| 70060 | `give=I:70065;receive=I:70066;disappear=0` | story: Tala: uncover Dura’s hired assassin | [areas/qst/goblinht.qst:124](../../../areas/qst/goblinht.qst#L124) |
| 70078 | `give=C:500,I:70075,I:70075,I:70075,I:70075,I:70075;receive=I:70074;disappear=0` | service: Paid service: make the bat-skull necklace | [areas/qst/goblinht.qst:144](../../../areas/qst/goblinht.qst#L144) |
| 70104 | `give=C:5;receive=I:70018;disappear=0` | Excluded: Coin-only, informational, or skill exchanges; supporting services rather than item-delivery story achievements. | [areas/qst/goblinht.qst:157](../../../areas/qst/goblinht.qst#L157) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 70000 | dura | [areas/qst/goblinht.qst:2](../../../areas/qst/goblinht.qst#L2) |
| 70001 | hello hi | [areas/qst/goblinht.qst:17](../../../areas/qst/goblinht.qst#L17) |
| 70001 | pain rest end | [areas/qst/goblinht.qst:22](../../../areas/qst/goblinht.qst#L22) |
| 70022 | spell components | [areas/qst/goblinht.qst:42](../../../areas/qst/goblinht.qst#L42) |
| 70022 | enchanting enchantments | [areas/qst/goblinht.qst:47](../../../areas/qst/goblinht.qst#L47) |
| 70022 | agree yes | [areas/qst/goblinht.qst:56](../../../areas/qst/goblinht.qst#L56) |
| 70060 | dura revenge spite | [areas/qst/goblinht.qst:113](../../../areas/qst/goblinht.qst#L113) |
| 70078 | hi gold | [areas/qst/goblinht.qst:133](../../../areas/qst/goblinht.qst#L133) |
| 70078 | material materials skull skulls make | [areas/qst/goblinht.qst:139](../../../areas/qst/goblinht.qst#L139) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 70029 | `world_quest` | [src/specs/specs.assign.c:764](../../../src/specs/specs.assign.c#L764) |
| room | 70175 | `inn` | [src/specs/specs.assign.c:2558](../../../src/specs/specs.assign.c#L2558) |

## Reset coverage

549 parsed reset commands: D: 94, E: 37, F: 16, G: 72, M: 306, O: 17, P: 7.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
