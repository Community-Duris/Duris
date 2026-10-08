# The Lair of Tiamat: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence azhural \
  --evidence-format markdown --output docs/reference/zone-story-audits/azhural.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 135219 | `give=I:135211,I:135211,I:135211,I:135211,I:135211,I:135211,I:135211,I:135211;receive=I:135210;disappear=1` | story: Bring eight bone shards to the gatekeeper | [areas/qst/azhural.qst:18](../../../areas/qst/azhural.qst#L18) |
| 135220 | `give=I:135201,I:135202,I:135203,I:135204,I:135205;receive=I:135214;disappear=1` | story: Bring all five flight essences to Ynndakaneil | [areas/qst/azhural.qst:70](../../../areas/qst/azhural.qst#L70) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 135219 | tiamat gate entrance pass hi | [areas/qst/azhural.qst:2](../../../areas/qst/azhural.qst#L2) |
| 135220 | tiamat archway gateway passage | [areas/qst/azhural.qst:54](../../../areas/qst/azhural.qst#L54) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| obj | 135218 | `sphinx_prefect_crown` | [src/specs/specs.assign.c:1330](../../../src/specs/specs.assign.c#L1330) |

## Reset coverage

129 parsed reset commands: D: 18, E: 14, G: 18, M: 75, O: 4.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
