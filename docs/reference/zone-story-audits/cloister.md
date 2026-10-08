# Father Tel's Holy Cloister: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence cloister \
  --evidence-format markdown --output docs/reference/zone-story-audits/cloister.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 67100 | `give=I:76728;receive=I:67129;disappear=1` | story: Brother Doss: return Mande’s blood-stained robes | [areas/qst/cloister.qst:2](../../../areas/qst/cloister.qst#L2) |
| 67102 | `give=I:67100;receive=I:67101;disappear=1` | story: Brother Mahr: expose the intruder’s instructions | [areas/qst/cloister.qst:34](../../../areas/qst/cloister.qst#L34) |
| 67103 | `give=I:67101;receive=I:67101;disappear=0` | service: Father Tel: a rejected recommendation | [areas/qst/cloister.qst:75](../../../areas/qst/cloister.qst#L75) |
| 67103 | `give=I:67102;receive=I:67104;disappear=0` | story: Father Tel: deliver Bakarakh’s named head | [areas/qst/cloister.qst:67](../../../areas/qst/cloister.qst#L67) |
| 67104 | `give=I:83374;receive=I:67130;disappear=0` | story: Bakarakh: return the stolen clan memoirs | [areas/qst/cloister.qst:119](../../../areas/qst/cloister.qst#L119) |
| 67107 | `give=I:67101;receive=E:15000;disappear=1` | story: The disappointed disciple: offer a recommendation | [areas/qst/cloister.qst:164](../../../areas/qst/cloister.qst#L164) |
| 67114 | `give=I:67116;receive=I:67117;disappear=1` | story: The clan priest: bring Troggahn’s egg | [areas/qst/cloister.qst:207](../../../areas/qst/cloister.qst#L207) |
| 67120 | `give=I:67103,I:67113;receive=I:67111;disappear=1` | story: The adviser: recover the ring and poison | [areas/qst/cloister.qst:256](../../../areas/qst/cloister.qst#L256) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 67102 | what looking | [areas/qst/cloister.qst:16](../../../areas/qst/cloister.qst#L16) |
| 67103 | bakarakh | [areas/qst/cloister.qst:55](../../../areas/qst/cloister.qst#L55) |
| 67104 | hi hello | [areas/qst/cloister.qst:86](../../../areas/qst/cloister.qst#L86) |
| 67104 | elder elders | [areas/qst/cloister.qst:90](../../../areas/qst/cloister.qst#L90) |
| 67104 | ring | [areas/qst/cloister.qst:99](../../../areas/qst/cloister.qst#L99) |
| 67104 | kirrb faktl | [areas/qst/cloister.qst:110](../../../areas/qst/cloister.qst#L110) |
| 67104 | tome | [areas/qst/cloister.qst:115](../../../areas/qst/cloister.qst#L115) |
| 67107 | hello hi | [areas/qst/cloister.qst:127](../../../areas/qst/cloister.qst#L127) |
| 67107 | rejection | [areas/qst/cloister.qst:135](../../../areas/qst/cloister.qst#L135) |
| 67107 | mahr | [areas/qst/cloister.qst:142](../../../areas/qst/cloister.qst#L142) |
| 67107 | letter | [areas/qst/cloister.qst:147](../../../areas/qst/cloister.qst#L147) |
| 67109 | disciple | [areas/qst/cloister.qst:181](../../../areas/qst/cloister.qst#L181) |
| 67114 | dragon egg | [areas/qst/cloister.qst:193](../../../areas/qst/cloister.qst#L193) |
| 67114 | podium | [areas/qst/cloister.qst:199](../../../areas/qst/cloister.qst#L199) |
| 67120 | hi hello | [areas/qst/cloister.qst:219](../../../areas/qst/cloister.qst#L219) |
| 67120 | elder bakarakh | [areas/qst/cloister.qst:224](../../../areas/qst/cloister.qst#L224) |
| 67120 | deceived deceive | [areas/qst/cloister.qst:229](../../../areas/qst/cloister.qst#L229) |
| 67120 | ring | [areas/qst/cloister.qst:237](../../../areas/qst/cloister.qst#L237) |
| 67120 | assistance | [areas/qst/cloister.qst:245](../../../areas/qst/cloister.qst#L245) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

113 parsed reset commands: D: 26, E: 14, F: 8, G: 5, M: 45, O: 8, P: 7.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
