# Bahamut's Palace: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence bahamut \
  --evidence-format markdown --output docs/reference/zone-story-audits/bahamut.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 25723 | `give=I:25760;receive=I:25724;disappear=1` | request: Show the custodian the personal seal | [areas/qst/bahamut.qst:24](../../../areas/qst/bahamut.qst#L24) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 25723 | hello hi help | [areas/qst/bahamut.qst:2](../../../areas/qst/bahamut.qst#L2) |
| 25723 | bahamut permission | [areas/qst/bahamut.qst:8](../../../areas/qst/bahamut.qst#L8) |
| 25723 | vault | [areas/qst/bahamut.qst:17](../../../areas/qst/bahamut.qst#L17) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 25700 | `bahamut` | [src/specs/specs.assign.c:477](../../../src/specs/specs.assign.c#L477) |
| obj | 25711 | `mrinlor_whip` | [src/specs/specs.assign.c:1327](../../../src/specs/specs.assign.c#L1327) |
| obj | 25719 | `artifact_stone` | [src/specs/specs.assign.c:1375](../../../src/specs/specs.assign.c#L1375) |
| obj | 25723 | `dragonlord_plate` | [src/specs/specs.assign.c:1540](../../../src/specs/specs.assign.c#L1540) |
| obj | 25745 | `sunblade` | [src/specs/specs.assign.c:1541](../../../src/specs/specs.assign.c#L1541) |
| obj | 25710 | `bloodfeast` | [src/specs/specs.assign.c:1542](../../../src/specs/specs.assign.c#L1542) |

## Reset coverage

208 parsed reset commands: D: 34, E: 27, F: 16, G: 23, M: 82, O: 25, P: 1.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
