# The Obsidian Citadel: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence obcita \
  --evidence-format markdown --output docs/reference/zone-story-audits/obcita.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 75604 | `give=I:75602;receive=I:75604;disappear=0` | request: Return the cleric staff | [areas/qst/obcita.qst:13](../../../areas/qst/obcita.qst#L13) |
| 75614 | `give=C:10000,I:75618;receive=I:75619;disappear=1` | request: Commission the shadowsteel ring | [areas/qst/obcita.qst:37](../../../areas/qst/obcita.qst#L37) |
| 75614 | `give=C:100000,I:75620;receive=I:75621;disappear=1` | request: Commission the shadowsteel boots | [areas/qst/obcita.qst:47](../../../areas/qst/obcita.qst#L47) |
| 75614 | `give=C:20000,I:75607;receive=I:75608;disappear=1` | request: Commission the shadowsteel bracelet | [areas/qst/obcita.qst:27](../../../areas/qst/obcita.qst#L27) |
| 75630 | `give=I:75641;receive=I:75642;disappear=0` | request: Return Rolart's shield to his shade | [areas/qst/obcita.qst:68](../../../areas/qst/obcita.qst#L68) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 75604 | want quest help what | [areas/qst/obcita.qst:2](../../../areas/qst/obcita.qst#L2) |
| 75614 | tools forge shadow want shadowsteel quest smithy | [areas/qst/obcita.qst:20](../../../areas/qst/obcita.qst#L20) |
| 75630 | death knight shade undead help quest | [areas/qst/obcita.qst:60](../../../areas/qst/obcita.qst#L60) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 75631 | `obsid_cit_death_knight` | [src/specs/specs.assign.c:893](../../../src/specs/specs.assign.c#L893) |
| mob | 75632 | `obsid_cit_death_knight` | [src/specs/specs.assign.c:894](../../../src/specs/specs.assign.c#L894) |
| mob | 75633 | `obsid_cit_death_knight` | [src/specs/specs.assign.c:895](../../../src/specs/specs.assign.c#L895) |
| mob | 75634 | `obsid_cit_death_knight` | [src/specs/specs.assign.c:896](../../../src/specs/specs.assign.c#L896) |
| mob | 75635 | `obsid_cit_death_knight` | [src/specs/specs.assign.c:897](../../../src/specs/specs.assign.c#L897) |
| mob | 75636 | `obsid_cit_death_knight` | [src/specs/specs.assign.c:898](../../../src/specs/specs.assign.c#L898) |
| mob | 75637 | `obsid_cit_death_knight` | [src/specs/specs.assign.c:899](../../../src/specs/specs.assign.c#L899) |
| mob | 75638 | `obsid_cit_death_knight` | [src/specs/specs.assign.c:900](../../../src/specs/specs.assign.c#L900) |
| mob | 75639 | `obsid_cit_death_knight` | [src/specs/specs.assign.c:901](../../../src/specs/specs.assign.c#L901) |
| mob | 75640 | `obsid_cit_satar_ghulan` | [src/specs/specs.assign.c:902](../../../src/specs/specs.assign.c#L902) |

## Reset coverage

345 parsed reset commands: D: 64, E: 32, F: 7, G: 16, M: 170, O: 34, P: 22.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
