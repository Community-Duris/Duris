# The Spires of the Elder Eternal Evil: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence eternal \
  --evidence-format markdown --output docs/reference/zone-story-audits/eternal.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 135402 | `give=I:135451,I:135452,I:135453,I:135454;receive=I:135459;disappear=0` | request: Gather the four serpent scales | [areas/qst/eternal.qst:2](../../../areas/qst/eternal.qst#L2) |
| 135440 | `give=I:135442;receive=I:135427;disappear=0` | request: Bring Dendar's visage to Tix | [areas/qst/eternal.qst:11](../../../areas/qst/eternal.qst#L11) |
| 135441 | `give=I:135440;receive=I:135458;disappear=0` | request: Answer Issis's petition | [areas/qst/eternal.qst:33](../../../areas/qst/eternal.qst#L33) |
| 135442 | `give=I:135455;receive=I:135428;disappear=0` | request: Recover Vaprak's stolen standard | [areas/qst/eternal.qst:57](../../../areas/qst/eternal.qst#L57) |
| 135443 | `give=I:135437,I:135438,I:135439;receive=I:135435;disappear=0` | request: Recover Flant's three books | [areas/qst/eternal.qst:73](../../../areas/qst/eternal.qst#L73) |
| 135444 | `give=I:135442;receive=I:135427;disappear=0` | request: Bring Dendar's visage to the other maker | [areas/qst/eternal.qst:81](../../../areas/qst/eternal.qst#L81) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 135441 | loss | [areas/qst/eternal.qst:17](../../../areas/qst/eternal.qst#L17) |
| 135442 | outrage | [areas/qst/eternal.qst:43](../../../areas/qst/eternal.qst#L43) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

331 parsed reset commands: D: 10, E: 181, F: 9, G: 9, M: 112, O: 10.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
