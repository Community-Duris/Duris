# Treasure Caves: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence dungeon \
  --evidence-format markdown --output docs/reference/zone-story-audits/dungeon.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 93001 | `give=I:93006;receive=C:10000;disappear=0` | request: Return the thief’s brass tiger figurine | [areas/qst/dungeon.qst:13](../../../areas/qst/dungeon.qst#L13) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 93001 | hi hello hail help quest | [areas/qst/dungeon.qst:2](../../../areas/qst/dungeon.qst#L2) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| obj | 93007 | `blue_sword_armor` | [src/specs/specs.assign.c:2219](../../../src/specs/specs.assign.c#L2219) |

## Reset coverage

194 parsed reset commands: D: 8, E: 34, G: 3, M: 139, O: 4, P: 4, R: 2.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
