# Desolate Under Fire: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence desolateinv \
  --evidence-format markdown --output docs/reference/zone-story-audits/desolateinv.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 77311 | `give=I:77326;receive=E:30000,I:77345;disappear=1` | story: Free the loud Storm Port orc | [areas/qst/desolateinv.qst:2](../../../areas/qst/desolateinv.qst#L2) |
| 77312 | `give=I:77326;receive=I:77345;disappear=1` | story: Free the captive goblin | [areas/qst/desolateinv.qst:13](../../../areas/qst/desolateinv.qst#L13) |
| 77313 | `give=I:77326;receive=E:10000,I:77345;disappear=1` | story: Free the Shady Grove orc | [areas/qst/desolateinv.qst:23](../../../areas/qst/desolateinv.qst#L23) |
| 77319 | `give=I:77373,I:77385,I:77392;receive=I:77329,I:77340,I:77384;disappear=1` | story: Jandar: deliver the three invaders’ hands | [areas/qst/desolateinv.qst:34](../../../areas/qst/desolateinv.qst#L34) |
| 77324 | `give=I:77326;receive=I:77345;disappear=1` | story: Free the old logger | [areas/qst/desolateinv.qst:53](../../../areas/qst/desolateinv.qst#L53) |
| 77325 | `give=I:77331;receive=C:50000;disappear=0` | story: Halfling: unresolved blade versus drink request | [areas/qst/desolateinv.qst:67](../../../areas/qst/desolateinv.qst#L67) |
| 77327 | `give=I:77316;receive=I:77311;disappear=1` | story: Minotaur: exchange the monkey’s neckchain | [areas/qst/desolateinv.qst:75](../../../areas/qst/desolateinv.qst#L75) |
| 77338 | `give=I:77315;receive=C:15000,I:77370;disappear=0` | story: Hunter: return the lost monkey | [areas/qst/desolateinv.qst:85](../../../areas/qst/desolateinv.qst#L85) |
| 77338 | `give=I:77326;receive=I:77345;disappear=1` | story: Free the monkey hunter | [areas/qst/desolateinv.qst:93](../../../areas/qst/desolateinv.qst#L93) |
| 77348 | `give=I:77352;receive=I:77353;disappear=1` | story: Driver: unresolved repaired wheel delivery | [areas/qst/desolateinv.qst:104](../../../areas/qst/desolateinv.qst#L104) |
| 77350 | `give=I:77345,I:77345,I:77345,I:77345,I:77345,I:77345,I:77345,I:77345;receive=I:77389;disappear=1` | story: Beregan: collect eight cut bindings | [areas/qst/desolateinv.qst:116](../../../areas/qst/desolateinv.qst#L116) |
| 77365 | `give=I:77317;receive=I:77387;disappear=0` | story: Delegate: deliver the Armageddon badge | [areas/qst/desolateinv.qst:139](../../../areas/qst/desolateinv.qst#L139) |
| 77370 | `give=I:77326;receive=I:77345;disappear=1` | story: Free the Pinehollow logger | [areas/qst/desolateinv.qst:148](../../../areas/qst/desolateinv.qst#L148) |
| 77380 | `give=C:10000;receive=I:77356;disappear=0` | service: Merchant: guarded potion purchase | [areas/qst/desolateinv.qst:158](../../../areas/qst/desolateinv.qst#L158) |
| 77383 | `give=I:77312;receive=I:77351;disappear=0` | story: Dog: trade stinky bones for the collar | [areas/qst/desolateinv.qst:165](../../../areas/qst/desolateinv.qst#L165) |
| 77385 | `give=I:77326;receive=I:77345;disappear=1` | story: Free the monk | [areas/qst/desolateinv.qst:174](../../../areas/qst/desolateinv.qst#L174) |
| 77386 | `give=I:77326;receive=I:77345;disappear=1` | story: Free the cleric | [areas/qst/desolateinv.qst:185](../../../areas/qst/desolateinv.qst#L185) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| room | 77442 | `inn` | [src/specs/specs.assign.c:2306](../../../src/specs/specs.assign.c#L2306) |

## Reset coverage

332 parsed reset commands: D: 64, E: 30, F: 9, G: 9, M: 159, O: 44, P: 16, R: 1.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
