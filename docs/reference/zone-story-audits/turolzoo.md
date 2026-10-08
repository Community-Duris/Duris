# Turolopolis Zoo: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence turolzoo \
  --evidence-format markdown --output docs/reference/zone-story-audits/turolzoo.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 53003 | `give=I:53001,I:53003;receive=I:53004;disappear=1` | request: Complete the hunter’s tooth collection | [areas/qst/turolzoo.qst:11](../../../areas/qst/turolzoo.qst#L11) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 53003 | hunt prey hunting help | [areas/qst/turolzoo.qst:2](../../../areas/qst/turolzoo.qst#L2) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

76 parsed reset commands: D: 12, E: 5, G: 2, M: 56, O: 1.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
