# The Bronze Citadel: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence bctdl \
  --evidence-format markdown --output docs/reference/zone-story-audits/bctdl.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 32448 | `give=I:32483;receive=I:32422;disappear=0` | request: Bring the Dungeon Master’s seal to Zariel | [areas/qst/bctdl.qst:11](../../../areas/qst/bctdl.qst#L11) |
| 32448 | `give=I:32490;receive=I:32029;disappear=1` | request: Bring Bel’s heart to Zariel | [areas/qst/bctdl.qst:23](../../../areas/qst/bctdl.qst#L23) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 32448 | hello | [areas/qst/bctdl.qst:2](../../../areas/qst/bctdl.qst#L2) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| obj | 32428 | `artifact_invisible` | [src/specs/specs.assign.c:1405](../../../src/specs/specs.assign.c#L1405) |
| obj | 32486 | `bel_sword` | [src/specs/specs.assign.c:2582](../../../src/specs/specs.assign.c#L2582) |

## Reset coverage

302 parsed reset commands: D: 32, E: 123, F: 13, G: 46, M: 86, O: 2.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
