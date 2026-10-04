# Sarmiz'Duul: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence sarmiz \
  --evidence-format markdown --output docs/reference/zone-story-audits/sarmiz.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 9411 | `give=I:9448;receive=I:9449;disappear=0` | story: An obsidian commission | [areas/qst/sarmiz.qst:23](../../../areas/qst/sarmiz.qst#L23) |
| 9415 | `give=I:9455;receive=I:9451,I:9454;disappear=0` | story: The apothecary's reply | [areas/qst/sarmiz.qst:52](../../../areas/qst/sarmiz.qst#L52) |
| 9420 | `give=I:9449;receive=I:9455;disappear=0` | story: A dagger and a sealed letter | [areas/qst/sarmiz.qst:111](../../../areas/qst/sarmiz.qst#L111) |
| 9429 | `give=I:9452;receive=I:9419,I:9453;disappear=0` | story: A symbol for the regular guard | [areas/qst/sarmiz.qst:164](../../../areas/qst/sarmiz.qst#L164) |
| 9432 | `give=I:9431;receive=I:9452;disappear=0` | story: Lockpicks and an ancient relic | [areas/qst/sarmiz.qst:217](../../../areas/qst/sarmiz.qst#L217) |
| 9448 | `give=I:9445,I:97099,I:97135;receive=I:9459;disappear=1` | story: Three ingredients for the royal alchemists | [areas/qst/sarmiz.qst:284](../../../areas/qst/sarmiz.qst#L284) |
| 9449 | `give=I:9442,I:9450,I:9451,I:9453;receive=I:9447;disappear=1` | story: The advisor's four-part conspiracy | [areas/qst/sarmiz.qst:328](../../../areas/qst/sarmiz.qst#L328) |
| 9452 | `give=I:9424;receive=I:9450,I:9456;disappear=1` | story: The diplomat's requested token | [areas/qst/sarmiz.qst:378](../../../areas/qst/sarmiz.qst#L378) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 9411 | weapons | [areas/qst/sarmiz.qst:2](../../../areas/qst/sarmiz.qst#L2) |
| 9415 | derval | [areas/qst/sarmiz.qst:43](../../../areas/qst/sarmiz.qst#L43) |
| 9420 | severne | [areas/qst/sarmiz.qst:76](../../../areas/qst/sarmiz.qst#L76) |
| 9420 | dagger | [areas/qst/sarmiz.qst:103](../../../areas/qst/sarmiz.qst#L103) |
| 9429 | regulars | [areas/qst/sarmiz.qst:127](../../../areas/qst/sarmiz.qst#L127) |
| 9429 | sword | [areas/qst/sarmiz.qst:157](../../../areas/qst/sarmiz.qst#L157) |
| 9432 | derval | [areas/qst/sarmiz.qst:191](../../../areas/qst/sarmiz.qst#L191) |
| 9432 | fighting | [areas/qst/sarmiz.qst:200](../../../areas/qst/sarmiz.qst#L200) |
| 9438 | tavril | [areas/qst/sarmiz.qst:230](../../../areas/qst/sarmiz.qst#L230) |
| 9448 | strife | [areas/qst/sarmiz.qst:251](../../../areas/qst/sarmiz.qst#L251) |
| 9448 | brave | [areas/qst/sarmiz.qst:258](../../../areas/qst/sarmiz.qst#L258) |
| 9449 | advisor | [areas/qst/sarmiz.qst:304](../../../areas/qst/sarmiz.qst#L304) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 9445 | `money_changer` | [src/specs/specs.assign.c:805](../../../src/specs/specs.assign.c#L805) |
| mob | 9453 | `erzul_proc` | [src/specs/specs.assign.c:806](../../../src/specs/specs.assign.c#L806) |
| room | 9704 | `crew_shop_proc` | [src/specs/specs.assign.c:2394](../../../src/specs/specs.assign.c#L2394) |
| room | 9738 | `inn` | [src/specs/specs.assign.c:2435](../../../src/specs/specs.assign.c#L2435) |
| room | 9967 | `ship_shop_proc` | [src/specs/specs.assign.c:2436](../../../src/specs/specs.assign.c#L2436) |

## Reset coverage

535 parsed reset commands: D: 50, E: 158, F: 21, G: 47, M: 255, O: 3, P: 1.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
