# Githzerai Stronghold: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence githzer \
  --evidence-format markdown --output docs/reference/zone-story-audits/githzer.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 44402 | `give=I:44401;receive=I:44515;disappear=0` | story: The sergeant: outpost intelligence | [areas/qst/githzer.qst:9](../../../areas/qst/githzer.qst#L9) |
| 44422 | `give=C:500000;receive=I:44516;disappear=0` | service: Zangzk: the paid master key | [areas/qst/githzer.qst:55](../../../areas/qst/githzer.qst#L55) |
| 44422 | `give=I:44401,I:44470,I:44512;receive=I:44516;disappear=0` | story: Zangzk: potion, map and poison | [areas/qst/githzer.qst:60](../../../areas/qst/githzer.qst#L60) |
| 44422 | `give=I:44422,I:44428,I:44429;receive=I:44431,I:44516;disappear=0` | story: Zangzk: three githyanki trophies | [areas/qst/githzer.qst:41](../../../areas/qst/githzer.qst#L41) |
| 44422 | `give=I:44509;receive=;disappear=0` | service: Zangzk: adamantite information | [areas/qst/githzer.qst:69](../../../areas/qst/githzer.qst#L69) |
| 44431 | `give=I:44436,I:44440,I:44440;receive=I:44439;disappear=0` | story: Prazyz: the red slaadi bundle | [areas/qst/githzer.qst:87](../../../areas/qst/githzer.qst#L87) |
| 44431 | `give=I:44437,I:44441,I:44441;receive=I:44438;disappear=0` | story: Prazyz: the green slaadi bundle | [areas/qst/githzer.qst:94](../../../areas/qst/githzer.qst#L94) |
| 44437 | `give=I:44434,I:44444,I:44448;receive=I:44456;disappear=0` | story: Zerthimon: the three relics of chaos | [areas/qst/githzer.qst:107](../../../areas/qst/githzer.qst#L107) |
| 44461 | `give=I:44446;receive=I:44501;disappear=1` | story: Tayr-Dryn: Zerthimon's heartstone | [areas/qst/githzer.qst:139](../../../areas/qst/githzer.qst#L139) |
| 44465 | `give=I:44427;receive=I:44519;disappear=0` | story: Vozalyzk: the Zerthimonian necklace | [areas/qst/githzer.qst:189](../../../areas/qst/githzer.qst#L189) |
| 44465 | `give=I:44550;receive=I:44577;disappear=0` | story: Vozalyzk: the Zerthimonian ring | [areas/qst/githzer.qst:198](../../../areas/qst/githzer.qst#L198) |
| 44494 | `give=I:44580;receive=I:44581;disappear=1` | story: The stranded githyanki: a torturer's implement | [areas/qst/githzer.qst:212](../../../areas/qst/githzer.qst#L212) |
| 44509 | `give=I:44563,I:44563,I:44563,I:44563,I:44563;receive=I:44572;disappear=1` | story: The prophet: five royal signet rings | [areas/qst/githzer.qst:283](../../../areas/qst/githzer.qst#L283) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 44402 | zangzk map | [areas/qst/githzer.qst:2](../../../areas/qst/githzer.qst#L2) |
| 44422 | githyanki | [areas/qst/githzer.qst:16](../../../areas/qst/githzer.qst#L16) |
| 44422 | news | [areas/qst/githzer.qst:23](../../../areas/qst/githzer.qst#L23) |
| 44422 | money | [areas/qst/githzer.qst:30](../../../areas/qst/githzer.qst#L30) |
| 44431 | slaad | [areas/qst/githzer.qst:83](../../../areas/qst/githzer.qst#L83) |
| 44461 | savior | [areas/qst/githzer.qst:117](../../../areas/qst/githzer.qst#L117) |
| 44461 | strength | [areas/qst/githzer.qst:123](../../../areas/qst/githzer.qst#L123) |
| 44461 | zerthimon | [areas/qst/githzer.qst:128](../../../areas/qst/githzer.qst#L128) |
| 44465 | hello hi | [areas/qst/githzer.qst:157](../../../areas/qst/githzer.qst#L157) |
| 44465 | inspection | [areas/qst/githzer.qst:161](../../../areas/qst/githzer.qst#L161) |
| 44465 | progress | [areas/qst/githzer.qst:167](../../../areas/qst/githzer.qst#L167) |
| 44465 | zerth zerths | [areas/qst/githzer.qst:176](../../../areas/qst/githzer.qst#L176) |
| 44494 | help hello hi | [areas/qst/githzer.qst:209](../../../areas/qst/githzer.qst#L209) |
| 44509 | hi hello quest | [areas/qst/githzer.qst:222](../../../areas/qst/githzer.qst#L222) |
| 44509 | chaos | [areas/qst/githzer.qst:227](../../../areas/qst/githzer.qst#L227) |
| 44509 | tayr-dryn zerthimon | [areas/qst/githzer.qst:234](../../../areas/qst/githzer.qst#L234) |
| 44509 | father kezyr-dryn | [areas/qst/githzer.qst:245](../../../areas/qst/githzer.qst#L245) |
| 44509 | secret tomb | [areas/qst/githzer.qst:254](../../../areas/qst/githzer.qst#L254) |
| 44509 | signet rings ring | [areas/qst/githzer.qst:262](../../../areas/qst/githzer.qst#L262) |
| 44509 | psycophantic psycophant magi magis | [areas/qst/githzer.qst:275](../../../areas/qst/githzer.qst#L275) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| obj | 44499 | `lucky_weapon` | [src/specs/specs.assign.c:1320](../../../src/specs/specs.assign.c#L1320) |

## Reset coverage

590 parsed reset commands: D: 108, E: 110, F: 30, G: 61, M: 207, O: 56, P: 17, R: 1.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
