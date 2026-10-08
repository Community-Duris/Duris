# Myrabolus: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence mira \
  --evidence-format markdown --output docs/reference/zone-story-audits/mira.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 82500 | `give=I:13357;receive=;disappear=0` | service: Roland: an unexplained head offering | [areas/qst/mira.qst:2](../../../areas/qst/mira.qst#L2) |
| 82507 | `give=I:13358;receive=I:13337;disappear=0` | story: Xavier: Krazzi’s proof | [areas/qst/mira.qst:16](../../../areas/qst/mira.qst#L16) |
| 82507 | `give=I:76069;receive=I:82522;disappear=0` | story: Xavier: deliver the planetary study | [areas/qst/mira.qst:20](../../../areas/qst/mira.qst#L20) |
| 82507 | `give=I:82517;receive=I:82538;disappear=0` | story: Xavier: answer the summons | [areas/qst/mira.qst:7](../../../areas/qst/mira.qst#L7) |
| 82515 | `give=I:13364;receive=I:13324;disappear=0` | story: Rico: treasure for a combat vest | [areas/qst/mira.qst:26](../../../areas/qst/mira.qst#L26) |
| 82516 | `give=I:13364;receive=I:13322;disappear=0` | story: Random: treasure and Hunter pride | [areas/qst/mira.qst:35](../../../areas/qst/mira.qst#L35) |
| 82518 | `give=C:50000,I:82542;receive=I:40738;disappear=0` | service: Andryn: paid dragon-scale armor | [areas/qst/mira.qst:53](../../../areas/qst/mira.qst#L53) |
| 82518 | `give=I:82518,I:82519;receive=I:82520;disappear=0` | story: Andryn: assemble the keystone | [areas/qst/mira.qst:45](../../../areas/qst/mira.qst#L45) |
| 82522 | `give=I:13364;receive=I:13328;disappear=1` | story: Decker: treasure for a war helm | [areas/qst/mira.qst:60](../../../areas/qst/mira.qst#L60) |
| 82537 | `give=I:13364;receive=I:13318;disappear=0` | story: Alexis: news through recovered treasure | [areas/qst/mira.qst:74](../../../areas/qst/mira.qst#L74) |
| 82538 | `give=I:82517;receive=I:82517,I:82518;disappear=1` | story: Markam: read the warning | [areas/qst/mira.qst:84](../../../areas/qst/mira.qst#L84) |
| 82543 | `give=I:13365;receive=C:20000;disappear=0` | story: The hunter: recover the lost monkey | [areas/qst/mira.qst:102](../../../areas/qst/mira.qst#L102) |
| 82565 | `give=I:431,I:82550;receive=C:10000,I:22279,I:22280;disappear=0` | story: The naval officer: raft and tree sap | [areas/qst/mira.qst:110](../../../areas/qst/mira.qst#L110) |
| 82569 | `give=I:75825,I:75826,I:75834;receive=I:22631,I:82553,I:82554,I:82555;disappear=0` | story: Balance: the three tokens | [areas/qst/mira.qst:140](../../../areas/qst/mira.qst#L140) |
| 82569 | `give=I:82543;receive=I:82543;disappear=0` | service: Balance: an optional letter referral | [areas/qst/mira.qst:126](../../../areas/qst/mira.qst#L126) |
| 82569 | `give=I:82547,I:82549;receive=I:82551,I:82552;disappear=0` | story: Balance: the two local trophies | [areas/qst/mira.qst:134](../../../areas/qst/mira.qst#L134) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 82569 | hi hello howdy hey | [areas/qst/mira.qst:119](../../../areas/qst/mira.qst#L119) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 82521 | `money_changer` | [src/specs/specs.assign.c:290](../../../src/specs/specs.assign.c#L290) |
| mob | 82511 | `world_quest` | [src/specs/specs.assign.c:761](../../../src/specs/specs.assign.c#L761) |
| obj | 82545 | `master_set` | [src/specs/specs.assign.c:1344](../../../src/specs/specs.assign.c#L1344) |
| obj | 82500 | `generic_parry_proc` | [src/specs/specs.assign.c:2050](../../../src/specs/specs.assign.c#L2050) |
| room | 82574 | `inn` | [src/specs/specs.assign.c:2308](../../../src/specs/specs.assign.c#L2308) |
| room | 82641 | `crew_shop_proc` | [src/specs/specs.assign.c:2401](../../../src/specs/specs.assign.c#L2401) |
| room | 82669 | `ship_shop_proc` | [src/specs/specs.assign.c:2416](../../../src/specs/specs.assign.c#L2416) |

## Reset coverage

316 parsed reset commands: D: 44, E: 38, F: 13, G: 22, M: 181, O: 13, P: 3, R: 2.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
