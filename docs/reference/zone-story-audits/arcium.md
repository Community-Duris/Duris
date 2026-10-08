# Arcium, the Plagued Kingdom: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence arcium \
  --evidence-format markdown --output docs/reference/zone-story-audits/arcium.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 85700 | `give=I:85715,I:85716,I:85717,I:85718,I:85719,I:85720;receive=I:85721;disappear=1` | request: Bring Joji the six hearts of Arcium | [areas/qst/arcium.qst:11](../../../areas/qst/arcium.qst#L11) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 85700 | hi king kovii granra kelien slican kurlon fragnox lord lords | [areas/qst/arcium.qst:2](../../../areas/qst/arcium.qst#L2) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 85751 | `world_quest` | [src/specs/specs.assign.c:760](../../../src/specs/specs.assign.c#L760) |

## Reset coverage

205 parsed reset commands: D: 22, E: 22, F: 8, G: 19, M: 121, O: 9, P: 4.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
