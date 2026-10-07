# Arachdrathos - Drow City: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence arac-web \
  --evidence-format markdown --output docs/reference/zone-story-audits/arac-web.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 36423 | `give=C:10000;receive=I:36400;disappear=1` | service: The gatekeeper’s private-room key | [areas/qst/arac-web.qst:6](../../../areas/qst/arac-web.qst#L6) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 36423 | hi hello door | [areas/qst/arac-web.qst:2](../../../areas/qst/arac-web.qst#L2) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 36420 | `world_quest` | [src/specs/specs.assign.c:729](../../../src/specs/specs.assign.c#L729) |
| mob | 36420 | `world_quest` | [src/specs/specs.assign.c:777](../../../src/specs/specs.assign.c#L777) |
| mob | 36413 | `money_changer` | [src/specs/specs.assign.c:813](../../../src/specs/specs.assign.c#L813) |
| room | 36564 | `inn` | [src/specs/specs.assign.c:2316](../../../src/specs/specs.assign.c#L2316) |
| room | 36567 | `pet_shops` | [src/specs/specs.assign.c:2317](../../../src/specs/specs.assign.c#L2317) |

## Reset coverage

191 parsed reset commands: D: 6, E: 3, F: 3, G: 46, M: 126, O: 7.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
