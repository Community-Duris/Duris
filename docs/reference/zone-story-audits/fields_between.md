# The Fields Between: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence fields_between \
  --evidence-format markdown --output docs/reference/zone-story-audits/fields_between.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 71036 | `give=I:71007,I:71008;receive=I:71020;disappear=0` | story: Grox: deliver the orders and gift | [areas/qst/fields_between.qst:10](../../../areas/qst/fields_between.qst#L10) |
| 71037 | `give=I:71005,I:71016;receive=I:71024;disappear=0` | story: The brewer: bring reading and bananas | [areas/qst/fields_between.qst:42](../../../areas/qst/fields_between.qst#L42) |
| 71038 | `give=I:71010,I:71011,I:71012,I:71013,I:71014;receive=I:71017;disappear=0` | story: Grog: recover five different wildmage heads | [areas/qst/fields_between.qst:95](../../../areas/qst/fields_between.qst#L95) |
| 71040 | `give=I:71030;receive=I:71033;disappear=0` | story: The shaman’s circular-rift request: source unavailable | [areas/qst/fields_between.qst:125](../../../areas/qst/fields_between.qst#L125) |
| 71056 | `give=I:71021,I:71022;receive=I:71023;disappear=0` | story: The professor: inspire a devious invention | [areas/qst/fields_between.qst:147](../../../areas/qst/fields_between.qst#L147) |
| 71065 | `give=I:71021;receive=I:71027;disappear=1` | story: Timmy: help the transformed boy write home | [areas/qst/fields_between.qst:190](../../../areas/qst/fields_between.qst#L190) |
| 71066 | `give=I:71027;receive=I:71028;disappear=0` | story: His mother: deliver Timmy’s letter | [areas/qst/fields_between.qst:222](../../../areas/qst/fields_between.qst#L222) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 71036 | grog orders order ape human body gift | [areas/qst/fields_between.qst:4](../../../areas/qst/fields_between.qst#L4) |
| 71037 | bored drunk entertainment | [areas/qst/fields_between.qst:21](../../../areas/qst/fields_between.qst#L21) |
| 71037 | bannana food bananna | [areas/qst/fields_between.qst:31](../../../areas/qst/fields_between.qst#L31) |
| 71037 | intellectual stimulation book read reading | [areas/qst/fields_between.qst:37](../../../areas/qst/fields_between.qst#L37) |
| 71038 | tallin torg son goddess godess spawn story stories grog raider | [areas/qst/fields_between.qst:59](../../../areas/qst/fields_between.qst#L59) |
| 71038 | head heads wildmage wild mage zooox problem task grox | [areas/qst/fields_between.qst:90](../../../areas/qst/fields_between.qst#L90) |
| 71040 | free way bully picked on orc shaman problem bodyguard out freedom camp | [areas/qst/fields_between.qst:118](../../../areas/qst/fields_between.qst#L118) |
| 71056 | learn interest dark different bored boring devious torture weapons weapon torturer weaponsmith | [areas/qst/fields_between.qst:133](../../../areas/qst/fields_between.qst#L133) |
| 71065 | upset scared sad mouse shy sadness mithril dark professor | [areas/qst/fields_between.qst:171](../../../areas/qst/fields_between.qst#L171) |
| 71066 | cries crying cry sad upset son timmy husband death ambition ambitionsj | [areas/qst/fields_between.qst:210](../../../areas/qst/fields_between.qst#L210) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

286 parsed reset commands: D: 22, E: 11, F: 3, G: 14, M: 230, O: 6.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
