# The Village of New Hope: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence newhope \
  --evidence-format markdown --output docs/reference/zone-story-audits/newhope.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 89102 | `give=C:10000,I:89117,I:89140;receive=I:89143;disappear=0` | service: Vitrius: shining dagger | [areas/qst/newhope.qst:60](../../../areas/qst/newhope.qst#L60) |
| 89102 | `give=C:100000,I:89117,I:89142;receive=I:89119;disappear=0` | service: Vitrius: shining long sword | [areas/qst/newhope.qst:43](../../../areas/qst/newhope.qst#L43) |
| 89102 | `give=C:250000,I:89117,I:89142;receive=I:89120;disappear=0` | service: Vitrius: shining two-handed sword | [areas/qst/newhope.qst:51](../../../areas/qst/newhope.qst#L51) |
| 89102 | `give=C:45000,I:89117,I:89141;receive=I:89118;disappear=0` | service: Vitrius: shining short sword | [areas/qst/newhope.qst:35](../../../areas/qst/newhope.qst#L35) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 89102 | weapon sword dagger | [areas/qst/newhope.qst:2](../../../areas/qst/newhope.qst#L2) |
| 89102 | shortsword | [areas/qst/newhope.qst:10](../../../areas/qst/newhope.qst#L10) |
| 89102 | longsword | [areas/qst/newhope.qst:16](../../../areas/qst/newhope.qst#L16) |
| 89102 | two-handed | [areas/qst/newhope.qst:22](../../../areas/qst/newhope.qst#L22) |
| 89102 | dagger | [areas/qst/newhope.qst:29](../../../areas/qst/newhope.qst#L29) |
| 89165 | lord master | [areas/qst/newhope.qst:71](../../../areas/qst/newhope.qst#L71) |
| 89165 | lich | [areas/qst/newhope.qst:79](../../../areas/qst/newhope.qst#L79) |
| 89165 | vault | [areas/qst/newhope.qst:88](../../../areas/qst/newhope.qst#L88) |
| 89165 | key | [areas/qst/newhope.qst:95](../../../areas/qst/newhope.qst#L95) |
| 89165 | dead death | [areas/qst/newhope.qst:101](../../../areas/qst/newhope.qst#L101) |
| 89165 | keep | [areas/qst/newhope.qst:110](../../../areas/qst/newhope.qst#L110) |
| 89165 | hammerhead mercenary | [areas/qst/newhope.qst:118](../../../areas/qst/newhope.qst#L118) |
| 89166 | lord master | [areas/qst/newhope.qst:130](../../../areas/qst/newhope.qst#L130) |
| 89166 | hammerhead | [areas/qst/newhope.qst:138](../../../areas/qst/newhope.qst#L138) |
| 89166 | scout information | [areas/qst/newhope.qst:145](../../../areas/qst/newhope.qst#L145) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 89181 | `tentacler_death` | [src/specs/specs.assign.c:816](../../../src/specs/specs.assign.c#L816) |

## Reset coverage

1033 parsed reset commands: D: 184, E: 69, F: 20, G: 120, M: 571, O: 37, P: 32.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
