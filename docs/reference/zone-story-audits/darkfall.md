# Darkfall Forest: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence darkfall \
  --evidence-format markdown --output docs/reference/zone-story-audits/darkfall.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 95304 | `give=I:402,I:26614,I:32490;receive=I:406;disappear=0` | story: Grellinar’s Agility scroll | [areas/qst/darkfall.qst:37](../../../areas/qst/darkfall.qst#L37) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

34 parsed reset commands: E: 4, G: 1, M: 29.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
