# Negative Material Plane: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence negplane \
  --evidence-format markdown --output docs/reference/zone-story-audits/negplane.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 26608 | `give=I:26609,I:26611,I:26616,I:26619,I:26638,I:26643;receive=I:26603,I:26644;disappear=1` | story: Gather the stars and the word of unmaking | [areas/qst/negplane.qst:24](../../../areas/qst/negplane.qst#L24) |
| 26644 | `give=I:26614;receive=C:500000,E:750000,I:26662,I:26667;disappear=1` | story: Return the orb to Sodolum’s spirit | [areas/qst/negplane.qst:98](../../../areas/qst/negplane.qst#L98) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 26608 | searching something | [areas/qst/negplane.qst:2](../../../areas/qst/negplane.qst#L2) |
| 26608 | hi hello | [areas/qst/negplane.qst:19](../../../areas/qst/negplane.qst#L19) |
| 26644 | hi hello | [areas/qst/negplane.qst:46](../../../areas/qst/negplane.qst#L46) |
| 26644 | force | [areas/qst/negplane.qst:55](../../../areas/qst/negplane.qst#L55) |
| 26644 | dark | [areas/qst/negplane.qst:70](../../../areas/qst/negplane.qst#L70) |
| 26644 | hope | [areas/qst/negplane.qst:87](../../../areas/qst/negplane.qst#L87) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 26603 | `neg_pocket` | [src/specs/specs.assign.c:1154](../../../src/specs/specs.assign.c#L1154) |
| obj | 26665 | `elvenkind_cloak` | [src/specs/specs.assign.c:1360](../../../src/specs/specs.assign.c#L1360) |
| obj | 26653 | `artifact_stone` | [src/specs/specs.assign.c:1374](../../../src/specs/specs.assign.c#L1374) |
| obj | 26662 | `orb_of_destruction` | [src/specs/specs.assign.c:1413](../../../src/specs/specs.assign.c#L1413) |
| obj | 26621 | `sanguine` | [src/specs/specs.assign.c:1414](../../../src/specs/specs.assign.c#L1414) |
| obj | 26662 | `neg_orb` | [src/specs/specs.assign.c:1415](../../../src/specs/specs.assign.c#L1415) |
| obj | 26666 | `transp_tow_misty_gloves` | [src/specs/specs.assign.c:1622](../../../src/specs/specs.assign.c#L1622) |

## Reset coverage

463 parsed reset commands: D: 38, E: 65, F: 7, G: 88, M: 222, O: 36, P: 7.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
