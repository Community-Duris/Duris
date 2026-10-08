# Ixxillikor: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence ixxillikor \
  --evidence-format markdown --output docs/reference/zone-story-audits/ixxillikor.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 4203 | `give=I:402,I:26614,I:32490;receive=I:408;disappear=0` | request: Bring three exact ingredients to Ezallixxel | [areas/qst/ixxillikor.qst:10](../../../areas/qst/ixxillikor.qst#L10) |
| 4224 | `give=C:10000;receive=I:35708;disappear=0` | request: Legacy auction purchase — currently unavailable | [areas/qst/ixxillikor.qst:47](../../../areas/qst/ixxillikor.qst#L47) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 4224 | hi hello | [areas/qst/ixxillikor.qst:20](../../../areas/qst/ixxillikor.qst#L20) |
| 4224 | goblin | [areas/qst/ixxillikor.qst:25](../../../areas/qst/ixxillikor.qst#L25) |
| 4224 | dwarf | [areas/qst/ixxillikor.qst:30](../../../areas/qst/ixxillikor.qst#L30) |
| 4224 | spider | [areas/qst/ixxillikor.qst:36](../../../areas/qst/ixxillikor.qst#L36) |
| 4245 | hi hello | [areas/qst/ixxillikor.qst:54](../../../areas/qst/ixxillikor.qst#L54) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

252 parsed reset commands: D: 30, E: 3, F: 2, G: 16, M: 194, O: 7.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
