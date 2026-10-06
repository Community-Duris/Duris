# Myrloch Vale: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence myrloch_vale \
  --evidence-format markdown --output docs/reference/zone-story-audits/myrloch_vale.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 26411 | `give=I:26417;receive=I:26409;disappear=0` | request: Return the master's old key | [areas/qst/myrloch_vale.qst:33](../../../areas/qst/myrloch_vale.qst#L33) |
| 26411 | `give=I:26420;receive=I:26421;disappear=0` | request: Return the master's old bracelet | [areas/qst/myrloch_vale.qst:40](../../../areas/qst/myrloch_vale.qst#L40) |
| 26411 | `give=I:26433;receive=I:26434;disappear=1` | request: Return the master's old dagger | [areas/qst/myrloch_vale.qst:47](../../../areas/qst/myrloch_vale.qst#L47) |
| 26417 | `give=I:26418;receive=I:26406;disappear=1` | request: Bring the warlord head to the elder | [areas/qst/myrloch_vale.qst:72](../../../areas/qst/myrloch_vale.qst#L72) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 26402 | qc_unblock 26555 north | [areas/qst/myrloch_vale.qst:2](../../../areas/qst/myrloch_vale.qst#L2) |
| 26411 | quest quests | [areas/qst/myrloch_vale.qst:7](../../../areas/qst/myrloch_vale.qst#L7) |
| 26411 | head warlord fire | [areas/qst/myrloch_vale.qst:22](../../../areas/qst/myrloch_vale.qst#L22) |
| 26417 | chest | [areas/qst/myrloch_vale.qst:60](../../../areas/qst/myrloch_vale.qst#L60) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 26402 | `unblock_on_death` | [src/specs/specs.assign.c:1253](../../../src/specs/specs.assign.c#L1253) |
| obj | 26413 | `item_switch` | [src/specs/specs.assign.c:2034](../../../src/specs/specs.assign.c#L2034) |
| obj | 26414 | `item_switch` | [src/specs/specs.assign.c:2035](../../../src/specs/specs.assign.c#L2035) |
| obj | 26415 | `item_switch` | [src/specs/specs.assign.c:2036](../../../src/specs/specs.assign.c#L2036) |
| obj | 26416 | `item_switch` | [src/specs/specs.assign.c:2037](../../../src/specs/specs.assign.c#L2037) |
| room | 26566 | `inn` | [src/specs/specs.assign.c:2513](../../../src/specs/specs.assign.c#L2513) |

## Reset coverage

184 parsed reset commands: D: 18, E: 45, G: 16, M: 84, O: 18, P: 3.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
