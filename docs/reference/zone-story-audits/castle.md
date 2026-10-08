# Quintaragon Castle: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence castle \
  --evidence-format markdown --output docs/reference/zone-story-audits/castle.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 2417 | `give=I:2435;receive=I:2448;disappear=0` | request: Bring Povtail’s diamond-studded bone to Remy | [areas/qst/castle.qst:31](../../../areas/qst/castle.qst#L31) |
| 2417 | `give=I:2441;receive=I:2447;disappear=0` | request: Return the Quintaragon family necklace | [areas/qst/castle.qst:18](../../../areas/qst/castle.qst#L18) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 2417 | hi hello quest revenge | [areas/qst/castle.qst:2](../../../areas/qst/castle.qst#L2) |
| 2417 | gilman | [areas/qst/castle.qst:14](../../../areas/qst/castle.qst#L14) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

283 parsed reset commands: D: 40, E: 60, F: 28, G: 12, M: 100, O: 19, P: 24.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
