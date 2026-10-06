# Cloud Giant Kingdom: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence cldgt \
  --evidence-format markdown --output docs/reference/zone-story-audits/cldgt.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 99501 | `give=I:99509,I:99509,I:99509,I:99509,I:99509,I:99510;receive=I:99503;disappear=0` | request: Bring five pelts and a strap to the trader | [areas/qst/cldgt.qst:7](../../../areas/qst/cldgt.qst#L7) |
| 99520 | `give=I:99518,I:99519,I:99520,I:99521,I:99522,I:99523;receive=I:99516;disappear=1` | request: Bring six different scalps to Anne | [areas/qst/cldgt.qst:36](../../../areas/qst/cldgt.qst#L36) |
| 99548 | `give=I:402,I:26614,I:32490;receive=I:405;disappear=0` | request: Bring three exact ingredients to Nonstros | [areas/qst/cldgt.qst:68](../../../areas/qst/cldgt.qst#L68) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 99501 | yeti | [areas/qst/cldgt.qst:2](../../../areas/qst/cldgt.qst#L2) |
| 99508 | giants city giant cloud | [areas/qst/cldgt.qst:19](../../../areas/qst/cldgt.qst#L19) |
| 99520 | hi hello greetings salutations | [areas/qst/cldgt.qst:27](../../../areas/qst/cldgt.qst#L27) |
| 99520 | scalp evils evil revenge | [areas/qst/cldgt.qst:32](../../../areas/qst/cldgt.qst#L32) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

323 parsed reset commands: D: 86, E: 26, F: 9, G: 24, M: 177, O: 1.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
