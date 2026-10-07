# The Reliquary Nexus of the Roper Den: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence nexus_roper \
  --evidence-format markdown --output docs/reference/zone-story-audits/nexus_roper.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 130407 | `give=I:130400,I:130400,I:130400,I:130400;receive=I:130401;disappear=0` | request: Four tentacles for Sebastian | [areas/qst/nexus_roper.qst:6](../../../areas/qst/nexus_roper.qst#L6) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 130407 | roper tentacle ropers hello hi | [areas/qst/nexus_roper.qst:2](../../../areas/qst/nexus_roper.qst#L2) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

63 parsed reset commands: D: 2, G: 4, M: 57.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
