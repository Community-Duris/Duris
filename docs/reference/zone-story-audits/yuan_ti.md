# The Temple to Skrentherlog: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence yuan_ti \
  --evidence-format markdown --output docs/reference/zone-story-audits/yuan_ti.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 80517 | `give=I:80570;receive=I:80572;disappear=1` | story: Blood for the captive ranger | [areas/qst/yuan_ti.qst:51](../../../areas/qst/yuan_ti.qst#L51) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 80517 | yuan-ti yuan | [areas/qst/yuan_ti.qst:2](../../../areas/qst/yuan_ti.qst#L2) |
| 80517 | shaman | [areas/qst/yuan_ti.qst:10](../../../areas/qst/yuan_ti.qst#L10) |
| 80517 | cure | [areas/qst/yuan_ti.qst:20](../../../areas/qst/yuan_ti.qst#L20) |
| 80517 | god | [areas/qst/yuan_ti.qst:29](../../../areas/qst/yuan_ti.qst#L29) |
| 80517 | skrentherlog | [areas/qst/yuan_ti.qst:40](../../../areas/qst/yuan_ti.qst#L40) |
| 80534 | die death | [areas/qst/yuan_ti.qst:67](../../../areas/qst/yuan_ti.qst#L67) |
| 80534 | stone | [areas/qst/yuan_ti.qst:74](../../../areas/qst/yuan_ti.qst#L74) |
| 80534 | priest priests destroy | [areas/qst/yuan_ti.qst:81](../../../areas/qst/yuan_ti.qst#L81) |
| 80534 | god skrentherlog | [areas/qst/yuan_ti.qst:91](../../../areas/qst/yuan_ti.qst#L91) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| obj | 80556 | `drowcrusher` | [src/specs/specs.assign.c:1730](../../../src/specs/specs.assign.c#L1730) |
| obj | 80579 | `squelcher` | [src/specs/specs.assign.c:1731](../../../src/specs/specs.assign.c#L1731) |
| obj | 80569 | `dragonarmor` | [src/specs/specs.assign.c:1732](../../../src/specs/specs.assign.c#L1732) |

## Reset coverage

172 parsed reset commands: D: 16, E: 15, F: 1, G: 9, M: 62, O: 49, P: 20.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
