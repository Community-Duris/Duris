# Caves of Mt. Skelenak: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence caves_skelenak \
  --evidence-format markdown --output docs/reference/zone-story-audits/caves_skelenak.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 4027 | `give=I:4021;receive=;disappear=0` | story: Goortok’s silver-disc test | [areas/qst/caves_skelenak.qst:36](../../../areas/qst/caves_skelenak.qst#L36) |
| 4027 | `give=I:4022;receive=;disappear=0` | story: Goortok’s platinum-disc test | [areas/qst/caves_skelenak.qst:49](../../../areas/qst/caves_skelenak.qst#L49) |
| 4027 | `give=I:4023;receive=;disappear=0` | story: Goortok’s strange-vial test | [areas/qst/caves_skelenak.qst:63](../../../areas/qst/caves_skelenak.qst#L63) |
| 4038 | `give=I:16071;receive=C:100000;disappear=0` | story: A marble eye for the monk | [areas/qst/caves_skelenak.qst:166](../../../areas/qst/caves_skelenak.qst#L166) |
| 4038 | `give=I:20604;receive=C:150000;disappear=0` | story: A glass eye for the monk | [areas/qst/caves_skelenak.qst:172](../../../areas/qst/caves_skelenak.qst#L172) |
| 4038 | `give=I:26013;receive=I:4031;disappear=1` | story: The troll king’s crystal sphere | [areas/qst/caves_skelenak.qst:192](../../../areas/qst/caves_skelenak.qst#L192) |
| 4038 | `give=I:26438;receive=I:4029;disappear=0` | story: The warlord’s flaming shield | [areas/qst/caves_skelenak.qst:155](../../../areas/qst/caves_skelenak.qst#L155) |
| 4038 | `give=I:4005,I:4025;receive=I:4030;disappear=0` | story: Proof from the tribal leaders | [areas/qst/caves_skelenak.qst:143](../../../areas/qst/caves_skelenak.qst#L143) |
| 4038 | `give=I:4016,I:4017;receive=I:4028;disappear=0` | story: The pyrohydra’s two bracelets | [areas/qst/caves_skelenak.qst:178](../../../areas/qst/caves_skelenak.qst#L178) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 4027 | moralon | [areas/qst/caves_skelenak.qst:2](../../../areas/qst/caves_skelenak.qst#L2) |
| 4038 | greetings hello hi | [areas/qst/caves_skelenak.qst:80](../../../areas/qst/caves_skelenak.qst#L80) |
| 4038 | solace rock | [areas/qst/caves_skelenak.qst:85](../../../areas/qst/caves_skelenak.qst#L85) |
| 4038 | tyrrany warlord quest | [areas/qst/caves_skelenak.qst:91](../../../areas/qst/caves_skelenak.qst#L91) |
| 4038 | eyes blind eye | [areas/qst/caves_skelenak.qst:103](../../../areas/qst/caves_skelenak.qst#L103) |
| 4038 | help | [areas/qst/caves_skelenak.qst:109](../../../areas/qst/caves_skelenak.qst#L109) |
| 4038 | task | [areas/qst/caves_skelenak.qst:121](../../../areas/qst/caves_skelenak.qst#L121) |
| 4038 | glass eyeball eyeballs | [areas/qst/caves_skelenak.qst:135](../../../areas/qst/caves_skelenak.qst#L135) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 4070 | `piercer` | [src/specs/specs.assign.c:570](../../../src/specs/specs.assign.c#L570) |
| mob | 4120 | `guild_guard` | [src/specs/specs.assign.c:571](../../../src/specs/specs.assign.c#L571) |

## Reset coverage

160 parsed reset commands: D: 16, E: 10, G: 6, M: 116, O: 10, P: 2.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
