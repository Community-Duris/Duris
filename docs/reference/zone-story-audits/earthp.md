# Grumbar's Domain: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence earthp \
  --evidence-format markdown --output docs/reference/zone-story-audits/earthp.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 131214 | `give=I:400104,I:400104,I:400104,I:400104,I:400104,I:400104,I:400104,I:400104,I:400104,I:400104;receive=I:131211;disappear=1` | story: Sunnis: ten planar granite shards | [areas/qst/earthp.qst:65](../../../areas/qst/earthp.qst#L65) |
| 131236 | `give=I:131227;receive=E:200000,I:131232;disappear=1` | story: Thulum: the Golden Lash | [areas/qst/earthp.qst:178](../../../areas/qst/earthp.qst#L178) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 131214 | hi hello | [areas/qst/earthp.qst:13](../../../areas/qst/earthp.qst#L13) |
| 131214 | earth plane | [areas/qst/earthp.qst:25](../../../areas/qst/earthp.qst#L25) |
| 131214 | grumbar | [areas/qst/earthp.qst:31](../../../areas/qst/earthp.qst#L31) |
| 131214 | prison balance | [areas/qst/earthp.qst:42](../../../areas/qst/earthp.qst#L42) |
| 131214 | feed hunger hungry | [areas/qst/earthp.qst:51](../../../areas/qst/earthp.qst#L51) |
| 131236 | hi hello | [areas/qst/earthp.qst:122](../../../areas/qst/earthp.qst#L122) |
| 131236 | pick song sing singing | [areas/qst/earthp.qst:133](../../../areas/qst/earthp.qst#L133) |
| 131236 | earth plane dao daos master masters slave slaves enslaved | [areas/qst/earthp.qst:147](../../../areas/qst/earthp.qst#L147) |
| 131236 | entemoch ogremoch grumbar | [areas/qst/earthp.qst:156](../../../areas/qst/earthp.qst#L156) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 131232 | `purple_worm` | [src/specs/specs.assign.c:581](../../../src/specs/specs.assign.c#L581) |

## Reset coverage

543 parsed reset commands: D: 14, E: 162, F: 104, G: 96, M: 160, O: 7.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
