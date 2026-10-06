# The Maze of Undead Army: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence maze_are \
  --evidence-format markdown --output docs/reference/zone-story-audits/maze_are.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 94018 | `give=I:94002,I:94002,I:94003;receive=I:94013;disappear=0` | request: Lich bargain: polished sphere | [areas/qst/maze_are.qst:55](../../../areas/qst/maze_are.qst#L55) |
| 94018 | `give=I:94002,I:94003,I:94004;receive=I:94005;disappear=1` | request: Lich bargain: fiery pentagram | [areas/qst/maze_are.qst:33](../../../areas/qst/maze_are.qst#L33) |
| 94018 | `give=I:94002,I:94004,I:94004;receive=I:94011;disappear=0` | request: Lich bargain: nightmare hammer | [areas/qst/maze_are.qst:46](../../../areas/qst/maze_are.qst#L46) |
| 94018 | `give=I:94003,I:94004,I:94004;receive=I:94017;disappear=0` | request: Lich bargain: decaying quiver | [areas/qst/maze_are.qst:73](../../../areas/qst/maze_are.qst#L73) |
| 94018 | `give=I:94004,I:94004,I:94004;receive=I:94015;disappear=0` | request: Lich bargain: demon-faced mask | [areas/qst/maze_are.qst:64](../../../areas/qst/maze_are.qst#L64) |
| 94035 | `give=I:94024,I:94024,I:94024,I:94024,I:94024;receive=I:94020;disappear=0` | request: Return the coordinator's five fingers | [areas/qst/maze_are.qst:96](../../../areas/qst/maze_are.qst#L96) |
| 94043 | `give=I:94001,I:94001,I:94001,I:94001,I:94001;receive=I:94014;disappear=1` | request: Gather dust for Krugor's mixture | [areas/qst/maze_are.qst:113](../../../areas/qst/maze_are.qst#L113) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 94017 | hi hello potion | [areas/qst/maze_are.qst:2](../../../areas/qst/maze_are.qst#L2) |
| 94018 | quest power | [areas/qst/maze_are.qst:10](../../../areas/qst/maze_are.qst#L10) |
| 94018 | list items | [areas/qst/maze_are.qst:16](../../../areas/qst/maze_are.qst#L16) |
| 94035 | fingers finger | [areas/qst/maze_are.qst:84](../../../areas/qst/maze_are.qst#L84) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

150 parsed reset commands: D: 20, E: 10, F: 1, G: 7, M: 99, O: 9, P: 4.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
