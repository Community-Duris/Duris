# The Lizardman Swamps of Clavikord: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence lizard \
  --evidence-format markdown --output docs/reference/zone-story-audits/lizard.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 6013 | `give=I:6008;receive=I:6009;disappear=1` | request: Help Bemon with Sslith’s head | [areas/qst/lizard.qst:14](../../../areas/qst/lizard.qst#L14) |
| 6014 | `give=I:6005;receive=I:6007;disappear=1` | request: Bring Bemon’s head to Vornin | [areas/qst/lizard.qst:40](../../../areas/qst/lizard.qst#L40) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 6013 | quest charge | [areas/qst/lizard.qst:2](../../../areas/qst/lizard.qst#L2) |
| 6014 | bemon | [areas/qst/lizard.qst:27](../../../areas/qst/lizard.qst#L27) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

214 parsed reset commands: D: 2, E: 7, F: 5, G: 2, M: 195, O: 3.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
