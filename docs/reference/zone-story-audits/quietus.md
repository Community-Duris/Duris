# The Docks of Quietus Quay: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence quietus \
  --evidence-format markdown --output docs/reference/zone-story-audits/quietus.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 1728 | `give=I:1701;receive=I:1701;disappear=0` | service: Show Membership Credentials | [areas/qst/quietus.qst:38](../../../areas/qst/quietus.qst#L38) |
| 1728 | `give=I:80808;receive=I:80808;disappear=0` | service: Show Membership Credentials | [areas/qst/quietus.qst:45](../../../areas/qst/quietus.qst#L45) |
| 1734 | `give=I:1701;receive=I:1701,I:1732;disappear=0` | service: Obtain the Drow Mission Note | [areas/qst/quietus.qst:85](../../../areas/qst/quietus.qst#L85) |
| 1734 | `give=I:1732,I:1746;receive=I:1747;disappear=0` | story: The Drow Lieutenant's Contract | [areas/qst/quietus.qst:107](../../../areas/qst/quietus.qst#L107) |
| 1734 | `give=I:80808;receive=I:1732,I:80808;disappear=0` | service: Obtain the Drow Mission Note | [areas/qst/quietus.qst:96](../../../areas/qst/quietus.qst#L96) |
| 1735 | `give=I:1701;receive=I:1701;disappear=0` | service: Hear the Orcish Mission | [areas/qst/quietus.qst:119](../../../areas/qst/quietus.qst#L119) |
| 1735 | `give=I:80808;receive=I:80808;disappear=0` | service: Hear the Orcish Mission | [areas/qst/quietus.qst:129](../../../areas/qst/quietus.qst#L129) |
| 1735 | `give=I:9436;receive=I:1748,I:1749,I:1750;disappear=1` | story: The Orcish Lieutenant's Contract | [areas/qst/quietus.qst:139](../../../areas/qst/quietus.qst#L139) |
| 1736 | `give=I:16450;receive=I:1751;disappear=0` | story: The Angry Lieutenant's Contract | [areas/qst/quietus.qst:182](../../../areas/qst/quietus.qst#L182) |
| 1736 | `give=I:1701;receive=I:1701;disappear=0` | service: Hear the Angry Lieutenant's Mission | [areas/qst/quietus.qst:158](../../../areas/qst/quietus.qst#L158) |
| 1736 | `give=I:80808;receive=I:80808;disappear=0` | service: Hear the Angry Lieutenant's Mission | [areas/qst/quietus.qst:170](../../../areas/qst/quietus.qst#L170) |
| 1749 | `give=I:16429;receive=;disappear=0` | service: Show the Captain a Seal | [areas/qst/quietus.qst:243](../../../areas/qst/quietus.qst#L243) |
| 1749 | `give=I:1701;receive=I:1701;disappear=0` | service: Hear the Captain's Mission | [areas/qst/quietus.qst:230](../../../areas/qst/quietus.qst#L230) |
| 1749 | `give=I:1730;receive=I:1752,I:1753,I:1754;disappear=1` | story: Proof Against Quietus | [areas/qst/quietus.qst:270](../../../areas/qst/quietus.qst#L270) |
| 1749 | `give=I:1747;receive=I:1747;disappear=0` | service: Show the Captain a Mission Reward | [areas/qst/quietus.qst:256](../../../areas/qst/quietus.qst#L256) |
| 1749 | `give=I:80808;receive=I:80808;disappear=0` | service: Hear the Captain's Mission | [areas/qst/quietus.qst:217](../../../areas/qst/quietus.qst#L217) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 1704 | aresliean | [areas/qst/quietus.qst:2](../../../areas/qst/quietus.qst#L2) |
| 1709 | aresliean | [areas/qst/quietus.qst:14](../../../areas/qst/quietus.qst#L14) |
| 1728 | darvanu mercenary mercenaries | [areas/qst/quietus.qst:25](../../../areas/qst/quietus.qst#L25) |
| 1732 | aresliean | [areas/qst/quietus.qst:54](../../../areas/qst/quietus.qst#L54) |
| 1732 | darvanu | [areas/qst/quietus.qst:64](../../../areas/qst/quietus.qst#L64) |
| 1734 | duty duties job | [areas/qst/quietus.qst:74](../../../areas/qst/quietus.qst#L74) |
| 1734 | aresliean | [areas/qst/quietus.qst:79](../../../areas/qst/quietus.qst#L79) |
| 1737 | aresliean | [areas/qst/quietus.qst:192](../../../areas/qst/quietus.qst#L192) |
| 1737 | drunk despair sad | [areas/qst/quietus.qst:197](../../../areas/qst/quietus.qst#L197) |
| 1737 | god | [areas/qst/quietus.qst:202](../../../areas/qst/quietus.qst#L202) |
| 1749 | krimeneha | [areas/qst/quietus.qst:212](../../../areas/qst/quietus.qst#L212) |
| 1751 | name | [areas/qst/quietus.qst:285](../../../areas/qst/quietus.qst#L285) |
| 1751 | aresliean | [areas/qst/quietus.qst:292](../../../areas/qst/quietus.qst#L292) |
| 1751 | darvanu | [areas/qst/quietus.qst:298](../../../areas/qst/quietus.qst#L298) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 1709 | `world_quest` | [src/specs/specs.assign.c:2225](../../../src/specs/specs.assign.c#L2225) |
| room | 1719 | `ship_shop_proc` | [src/specs/specs.assign.c:2226](../../../src/specs/specs.assign.c#L2226) |
| room | 1736 | `inn` | [src/specs/specs.assign.c:2312](../../../src/specs/specs.assign.c#L2312) |
| room | 1734 | `crew_shop_proc` | [src/specs/specs.assign.c:2393](../../../src/specs/specs.assign.c#L2393) |

## Reset coverage

216 parsed reset commands: D: 38, E: 26, F: 1, G: 20, M: 87, O: 35, P: 9.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
