# Harrow -The Gnome Village: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence harrow \
  --evidence-format markdown --output docs/reference/zone-story-audits/harrow.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 29418 | `give=I:29405;receive=I:29406;disappear=1` | story: Young girl: recover the lost ring | [areas/qst/harrow.qst:20](../../../areas/qst/harrow.qst#L20) |
| 29421 | `give=I:29400,I:29402,I:29406;receive=I:29404;disappear=0` | story: Alorka: the lucky alchemist sack | [areas/qst/harrow.qst:73](../../../areas/qst/harrow.qst#L73) |
| 29421 | `give=I:29406,I:29407,I:29414;receive=I:29416;disappear=0` | story: Alorka: the rose glass horn | [areas/qst/harrow.qst:46](../../../areas/qst/harrow.qst#L46) |
| 29421 | `give=I:29406,I:29410,I:29415;receive=I:29417;disappear=0` | story: Alorka: the robe of many colors | [areas/qst/harrow.qst:55](../../../areas/qst/harrow.qst#L55) |
| 29421 | `give=I:29406,I:29413,I:29419;receive=I:29418;disappear=0` | story: Alorka: the swirling wand of light | [areas/qst/harrow.qst:64](../../../areas/qst/harrow.qst#L64) |
| 29444 | `give=I:29440;receive=I:318,I:319,I:330;disappear=0` | story: Bom Fitherwood: a painting for fish | [areas/qst/harrow.qst:114](../../../areas/qst/harrow.qst#L114) |
| 29449 | `give=I:29460;receive=E:4000;disappear=0` | story: Goldfish: offer fish food | [areas/qst/harrow.qst:141](../../../areas/qst/harrow.qst#L141) |
| 29454 | `give=I:29444;receive=C:77777,I:29453;disappear=1` | story: Leprechaun: the pot of gold | [areas/qst/harrow.qst:162](../../../areas/qst/harrow.qst#L162) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 29418 | lost ring sad | [areas/qst/harrow.qst:12](../../../areas/qst/harrow.qst#L12) |
| 29421 | treasure craft list | [areas/qst/harrow.qst:30](../../../areas/qst/harrow.qst#L30) |
| 29444 | fish artsy items trade | [areas/qst/harrow.qst:104](../../../areas/qst/harrow.qst#L104) |
| 29444 | hi hello | [areas/qst/harrow.qst:110](../../../areas/qst/harrow.qst#L110) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| room | 29403 | `inn` | [src/specs/specs.assign.c:2454](../../../src/specs/specs.assign.c#L2454) |

## Reset coverage

280 parsed reset commands: D: 24, E: 20, G: 52, M: 161, O: 22, P: 1.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
