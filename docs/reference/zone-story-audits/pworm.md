# Lair of the Purple Worm: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence pworm \
  --evidence-format markdown --output docs/reference/zone-story-audits/pworm.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 42506 | `give=I:42500,I:42500,I:42500,I:42500,I:42500,I:42500,I:42500,I:42502;receive=I:42504;disappear=0` | request: Bring Drakhov's amulet and seven hides to Draknahov | [areas/qst/pworm.qst:31](../../../areas/qst/pworm.qst#L31) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 42506 | worm purple quest bloodthirst blood hi hello | [areas/qst/pworm.qst:2](../../../areas/qst/pworm.qst#L2) |
| 42506 | actions family did | [areas/qst/pworm.qst:8](../../../areas/qst/pworm.qst#L8) |
| 42506 | brother | [areas/qst/pworm.qst:14](../../../areas/qst/pworm.qst#L14) |
| 42506 | drakhov | [areas/qst/pworm.qst:20](../../../areas/qst/pworm.qst#L20) |
| 42506 | amulet | [areas/qst/pworm.qst:25](../../../areas/qst/pworm.qst#L25) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

88 parsed reset commands: D: 6, E: 9, G: 9, M: 62, P: 2.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
