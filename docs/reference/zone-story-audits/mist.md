# The Shadow Forest: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence mist \
  --evidence-format markdown --output docs/reference/zone-story-audits/mist.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 6306 | `give=I:6305;receive=I:6306;disappear=1` | request: Bring Palon the bloody head of Darnac | [areas/qst/mist.qst:6](../../../areas/qst/mist.qst#L6) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 6306 | darnac | [areas/qst/mist.qst:2](../../../areas/qst/mist.qst#L2) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 6307 | `mist_protect` | [src/specs/specs.assign.c:2222](../../../src/specs/specs.assign.c#L2222) |

## Reset coverage

61 parsed reset commands: E: 5, G: 1, M: 55.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
