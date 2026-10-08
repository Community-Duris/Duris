# The Prisons of Carthapia: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence prison \
  --evidence-format markdown --output docs/reference/zone-story-audits/prison.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 7304 | `give=I:7372;receive=C:15000;disappear=0` | story: Recover the visitor’s tarnished coin | [areas/qst/prison.qst:20](../../../areas/qst/prison.qst#L20) |
| 7306 | `give=I:7342,I:7343,I:7344;receive=I:7345;disappear=0` | story: Forge a shield from Smaug’s scales | [areas/qst/prison.qst:47](../../../areas/qst/prison.qst#L47) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 7304 | hi hello | [areas/qst/prison.qst:2](../../../areas/qst/prison.qst#L2) |
| 7304 | partner | [areas/qst/prison.qst:7](../../../areas/qst/prison.qst#L7) |
| 7304 | item | [areas/qst/prison.qst:13](../../../areas/qst/prison.qst#L13) |
| 7306 | hello | [areas/qst/prison.qst:29](../../../areas/qst/prison.qst#L29) |
| 7306 | craft talents | [areas/qst/prison.qst:35](../../../areas/qst/prison.qst#L35) |
| 7306 | material | [areas/qst/prison.qst:41](../../../areas/qst/prison.qst#L41) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 7333 | `warden_shout` | [src/specs/specs.assign.c:1061](../../../src/specs/specs.assign.c#L1061) |
| obj | 7365 | `flaming_axe_of_azer` | [src/specs/specs.assign.c:1324](../../../src/specs/specs.assign.c#L1324) |
| obj | 7371 | `nexus` | [src/specs/specs.assign.c:1410](../../../src/specs/specs.assign.c#L1410) |

## Reset coverage

422 parsed reset commands: D: 76, E: 171, F: 20, G: 13, M: 107, O: 32, P: 3.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
