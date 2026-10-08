# The Gagga'Jobo Cave System: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence goblincave \
  --evidence-format markdown --output docs/reference/zone-story-audits/goblincave.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 19005 | `give=C:1000,I:19006;receive=I:19007;disappear=0` | service: Commission leather shoes | [areas/qst/goblincave.qst:26](../../../areas/qst/goblincave.qst#L26) |
| 19005 | `give=C:10000,I:19006,I:19006,I:19006,I:19006;receive=I:19010;disappear=0` | service: Commission goblin-made gloves | [areas/qst/goblincave.qst:46](../../../areas/qst/goblincave.qst#L46) |
| 19005 | `give=C:2000,I:19006,I:19006;receive=I:19008;disappear=0` | service: Commission a goblin-made shirt | [areas/qst/goblincave.qst:32](../../../areas/qst/goblincave.qst#L32) |
| 19005 | `give=I:19006,I:19006,I:19006;receive=I:19009;disappear=0` | request: Three hides for a backpack | [areas/qst/goblincave.qst:39](../../../areas/qst/goblincave.qst#L39) |
| 19006 | `give=C:100000,I:19006,I:19011,I:19013;receive=I:19012;disappear=0` | service: Commission a blade of bone | [areas/qst/goblincave.qst:67](../../../areas/qst/goblincave.qst#L67) |
| 19006 | `give=I:19011,I:19011;receive=I:19014;disappear=0` | request: Two bones for an earring | [areas/qst/goblincave.qst:75](../../../areas/qst/goblincave.qst#L75) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 19005 | chothe hide strip | [areas/qst/goblincave.qst:2](../../../areas/qst/goblincave.qst#L2) |
| 19005 | backpack bp | [areas/qst/goblincave.qst:7](../../../areas/qst/goblincave.qst#L7) |
| 19005 | shoes | [areas/qst/goblincave.qst:11](../../../areas/qst/goblincave.qst#L11) |
| 19005 | shirt | [areas/qst/goblincave.qst:16](../../../areas/qst/goblincave.qst#L16) |
| 19005 | gloves | [areas/qst/goblincave.qst:21](../../../areas/qst/goblincave.qst#L21) |
| 19006 | bone sword | [areas/qst/goblincave.qst:57](../../../areas/qst/goblincave.qst#L57) |
| 19006 | earring | [areas/qst/goblincave.qst:63](../../../areas/qst/goblincave.qst#L63) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

65 parsed reset commands: D: 4, E: 4, G: 22, M: 23, O: 2, P: 10.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
