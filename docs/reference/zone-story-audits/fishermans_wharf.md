# Fishermans Wharf: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence fishermans_wharf \
  --evidence-format markdown --output docs/reference/zone-story-audits/fishermans_wharf.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 88902 | `give=I:88910,I:88910,I:88910,I:88910,I:88911,I:88911,I:88911,I:88913;receive=I:88914;disappear=0` | story: Baltik’s supplies and cavern totem | [areas/qst/fishermans_wharf.qst:14](../../../areas/qst/fishermans_wharf.qst#L14) |
| 88904 | `give=I:88900;receive=E:7500,I:88904;disappear=0` | request: Dimbled’s missing fishing guide | [areas/qst/fishermans_wharf.qst:49](../../../areas/qst/fishermans_wharf.qst#L49) |
| 88905 | `give=I:88902,I:88904;receive=E:7500,I:88906;disappear=0` | story: Supplies for the young anglers | [areas/qst/fishermans_wharf.qst:68](../../../areas/qst/fishermans_wharf.qst#L68) |
| 88906 | `give=I:88912,I:88912,I:88912,I:88912;receive=E:7500,I:88902;disappear=0` | request: Clean bottles from the lake | [areas/qst/fishermans_wharf.qst:94](../../../areas/qst/fishermans_wharf.qst#L94) |
| 88907 | `give=I:88909,I:88909,I:88909,I:88909;receive=E:7500,I:88905;disappear=0` | request: Frog jelly for a snorkel | [areas/qst/fishermans_wharf.qst:114](../../../areas/qst/fishermans_wharf.qst#L114) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 88902 | supplies sticks egg pelt hi | [areas/qst/fishermans_wharf.qst:2](../../../areas/qst/fishermans_wharf.qst#L2) |
| 88902 | chores tasks chore task help favor | [areas/qst/fishermans_wharf.qst:8](../../../areas/qst/fishermans_wharf.qst#L8) |
| 88904 | lost confusion help find hi | [areas/qst/fishermans_wharf.qst:38](../../../areas/qst/fishermans_wharf.qst#L38) |
| 88904 | fish fishing dock pier lake relax | [areas/qst/fishermans_wharf.qst:43](../../../areas/qst/fishermans_wharf.qst#L43) |
| 88905 | limits supplies fish fishing bait line reel worms hi | [areas/qst/fishermans_wharf.qst:58](../../../areas/qst/fishermans_wharf.qst#L58) |
| 88906 | attention glass bottle hi | [areas/qst/fishermans_wharf.qst:78](../../../areas/qst/fishermans_wharf.qst#L78) |
| 88906 | help clean trash | [areas/qst/fishermans_wharf.qst:87](../../../areas/qst/fishermans_wharf.qst#L87) |
| 88907 | dip frog pocket hi jelly spawn | [areas/qst/fishermans_wharf.qst:108](../../../areas/qst/fishermans_wharf.qst#L108) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

119 parsed reset commands: D: 6, E: 6, G: 10, M: 78, O: 19.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
