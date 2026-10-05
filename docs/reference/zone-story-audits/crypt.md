# Nakral's Crypt: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence crypt \
  --evidence-format markdown --output docs/reference/zone-story-audits/crypt.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 14302 | `give=I:14305,I:14305,I:14305,I:14305,I:14315,I:14315;receive=;disappear=1` | story: Hermit: bring the exact firewood bundle | [areas/qst/crypt.qst:35](../../../areas/qst/crypt.qst#L35) |
| 14432 | `give=I:14494,I:14501,I:14521,I:14536,I:14540;receive=I:14531;disappear=1` | story: Surok: collect the five distinct adamantite chunks | [areas/qst/crypt.qst:100](../../../areas/qst/crypt.qst#L100) |
| 14438 | `give=I:14498,I:14500,I:14526,I:14534;receive=I:14539;disappear=0` | story: Jade statue: exchange the four trophies | [areas/qst/crypt.qst:128](../../../areas/qst/crypt.qst#L128) |
| 14438 | `give=I:14539;receive=I:14542;disappear=1` | story: Jade statue: trade the stone token | [areas/qst/crypt.qst:140](../../../areas/qst/crypt.qst#L140) |
| 14438 | `give=I:14556;receive=I:14557;disappear=1` | story: Jade statue: enhance the mithril collar | [areas/qst/crypt.qst:154](../../../areas/qst/crypt.qst#L154) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 14302 | hello hi howdy name | [areas/qst/crypt.qst:2](../../../areas/qst/crypt.qst#L2) |
| 14302 | hunting hunt | [areas/qst/crypt.qst:18](../../../areas/qst/crypt.qst#L18) |
| 14302 | graveyard strange scared tomb | [areas/qst/crypt.qst:26](../../../areas/qst/crypt.qst#L26) |
| 14432 | hello hi name | [areas/qst/crypt.qst:65](../../../areas/qst/crypt.qst#L65) |
| 14432 | crypt dungeon catacomb catacombs | [areas/qst/crypt.qst:89](../../../areas/qst/crypt.qst#L89) |
| 14438 | hello hi howdy name | [areas/qst/crypt.qst:124](../../../areas/qst/crypt.qst#L124) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| room | 14362 | `inn` | [src/specs/specs.assign.c:2331](../../../src/specs/specs.assign.c#L2331) |

## Reset coverage

521 parsed reset commands: D: 94, E: 74, F: 11, G: 16, M: 116, O: 129, P: 81.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
