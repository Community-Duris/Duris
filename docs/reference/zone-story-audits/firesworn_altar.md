# The Altar of the Firesworn: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence firesworn_altar \
  --evidence-format markdown --output docs/reference/zone-story-audits/firesworn_altar.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 135114 | `give=I:135106,I:135107,I:135108,I:135109;receive=I:135110,I:135119,I:135120;disappear=0` | story: Four essences against Maelshem’s ascent | [areas/qst/firesworn_altar.qst:93](../../../areas/qst/firesworn_altar.qst#L93) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 135114 | maelshem | [areas/qst/firesworn_altar.qst:4](../../../areas/qst/firesworn_altar.qst#L4) |
| 135114 | tower | [areas/qst/firesworn_altar.qst:26](../../../areas/qst/firesworn_altar.qst#L26) |
| 135114 | treasures | [areas/qst/firesworn_altar.qst:33](../../../areas/qst/firesworn_altar.qst#L33) |
| 135114 | voluntown | [areas/qst/firesworn_altar.qst:43](../../../areas/qst/firesworn_altar.qst#L43) |
| 135114 | frost queen auril | [areas/qst/firesworn_altar.qst:56](../../../areas/qst/firesworn_altar.qst#L56) |
| 135114 | hi hello | [areas/qst/firesworn_altar.qst:67](../../../areas/qst/firesworn_altar.qst#L67) |
| 135114 | firesworn legion devils | [areas/qst/firesworn_altar.qst:74](../../../areas/qst/firesworn_altar.qst#L74) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

112 parsed reset commands: D: 18, E: 24, F: 2, G: 12, M: 45, O: 10, P: 1.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
