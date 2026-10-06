# The Troll Hills: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence troll_hills \
  --evidence-format markdown --output docs/reference/zone-story-audits/troll_hills.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 1810 | `give=I:1804;receive=E:5000,I:1807;disappear=1` | request: Return the ogre idol to the adventurer | [areas/qst/troll_hills.qst:23](../../../areas/qst/troll_hills.qst#L23) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 1810 | hello hi greetings | [areas/qst/troll_hills.qst:2](../../../areas/qst/troll_hills.qst#L2) |
| 1810 | they | [areas/qst/troll_hills.qst:13](../../../areas/qst/troll_hills.qst#L13) |
| 1810 | statuette idol description details | [areas/qst/troll_hills.qst:17](../../../areas/qst/troll_hills.qst#L17) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 1919 | `bridge_troll` | [src/specs/specs.assign.c:417](../../../src/specs/specs.assign.c#L417) |

## Reset coverage

123 parsed reset commands: D: 2, E: 12, G: 1, M: 106, O: 1, P: 1.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
