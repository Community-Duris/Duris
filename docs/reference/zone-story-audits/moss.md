# Mosswood: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence moss \
  --evidence-format markdown --output docs/reference/zone-story-audits/moss.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 23518 | `give=I:23436;receive=C:1000;disappear=0` | request: Supply a large beet to Ijale | [areas/qst/moss.qst:9](../../../areas/qst/moss.qst#L9) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 23518 | hi hello | [areas/qst/moss.qst:2](../../../areas/qst/moss.qst#L2) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

250 parsed reset commands: D: 36, F: 2, G: 26, M: 183, O: 3.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
