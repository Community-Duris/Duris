# Centaur Villages: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence centaur_zone \
  --evidence-format markdown --output docs/reference/zone-story-audits/centaur_zone.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 93301 | `give=I:93310;receive=I:93311;disappear=0` | story: Llewyn: bring Naergot’s heart | [areas/qst/centaur_zone.qst:21](../../../areas/qst/centaur_zone.qst#L21) |
| 93301 | `give=I:93313;receive=I:93313;disappear=0` | service: Llewyn: identify the half amulet | [areas/qst/centaur_zone.qst:34](../../../areas/qst/centaur_zone.qst#L34) |
| 93302 | `give=I:93311,I:93312;receive=I:93313;disappear=0` | story: The treant elder: return Gilandrimeel’s horn | [areas/qst/centaur_zone.qst:62](../../../areas/qst/centaur_zone.qst#L62) |
| 93302 | `give=I:93311;receive=I:93311;disappear=0` | service: The treant elder: learn about the stolen horn | [areas/qst/centaur_zone.qst:46](../../../areas/qst/centaur_zone.qst#L46) |
| 93309 | `give=I:93313;receive=I:93313;disappear=0` | service: Banitoor: ask about the broken amulet | [areas/qst/centaur_zone.qst:126](../../../areas/qst/centaur_zone.qst#L126) |
| 93309 | `give=I:93317;receive=I:93313;disappear=0` | story: Banitoor: recover her lost staff | [areas/qst/centaur_zone.qst:139](../../../areas/qst/centaur_zone.qst#L139) |
| 93310 | `give=I:93313,I:93313;receive=I:93314,I:93330;disappear=1` | story: Hateeu: return both amulet halves | [areas/qst/centaur_zone.qst:152](../../../areas/qst/centaur_zone.qst#L152) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 93301 | dragon heart | [areas/qst/centaur_zone.qst:2](../../../areas/qst/centaur_zone.qst#L2) |
| 93301 | amulet | [areas/qst/centaur_zone.qst:15](../../../areas/qst/centaur_zone.qst#L15) |
| 93308 | learned one banitoor vorsileez | [areas/qst/centaur_zone.qst:78](../../../areas/qst/centaur_zone.qst#L78) |
| 93309 | amulet | [areas/qst/centaur_zone.qst:121](../../../areas/qst/centaur_zone.qst#L121) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

199 parsed reset commands: D: 6, E: 1, F: 7, G: 23, M: 70, O: 21, P: 71.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
