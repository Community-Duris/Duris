# Temple of the Earth: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence earth \
  --evidence-format markdown --output docs/reference/zone-story-audits/earth.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 43502 | `give=I:43525;receive=I:43561;disappear=1` | story: Gromdishar: a brother's badge | [areas/qst/earth.qst:27](../../../areas/qst/earth.qst#L27) |
| 43509 | `give=I:43539;receive=I:43562;disappear=1` | story: The captain: the fallen patrol leader | [areas/qst/earth.qst:104](../../../areas/qst/earth.qst#L104) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 43502 | hi hello | [areas/qst/earth.qst:2](../../../areas/qst/earth.qst#L2) |
| 43502 | yes scouts | [areas/qst/earth.qst:7](../../../areas/qst/earth.qst#L7) |
| 43502 | mission gloomhaven | [areas/qst/earth.qst:17](../../../areas/qst/earth.qst#L17) |
| 43504 | hi hello trouble troubling problem | [areas/qst/earth.qst:40](../../../areas/qst/earth.qst#L40) |
| 43504 | cursed temple | [areas/qst/earth.qst:46](../../../areas/qst/earth.qst#L46) |
| 43504 | hide hidden | [areas/qst/earth.qst:56](../../../areas/qst/earth.qst#L56) |
| 43504 | yes hint | [areas/qst/earth.qst:63](../../../areas/qst/earth.qst#L63) |
| 43509 | hi hello | [areas/qst/earth.qst:77](../../../areas/qst/earth.qst#L77) |
| 43509 | yes woodseer scout scouts | [areas/qst/earth.qst:82](../../../areas/qst/earth.qst#L82) |
| 43509 | mission | [areas/qst/earth.qst:93](../../../areas/qst/earth.qst#L93) |
| 43533 | hi hello | [areas/qst/earth.qst:117](../../../areas/qst/earth.qst#L117) |
| 43547 | hi hello | [areas/qst/earth.qst:127](../../../areas/qst/earth.qst#L127) |
| 43571 | hi hello | [areas/qst/earth.qst:135](../../../areas/qst/earth.qst#L135) |
| 43571 | keeper | [areas/qst/earth.qst:144](../../../areas/qst/earth.qst#L144) |
| 43571 | bloodrune | [areas/qst/earth.qst:149](../../../areas/qst/earth.qst#L149) |
| 43573 | quest hi hello | [areas/qst/earth.qst:158](../../../areas/qst/earth.qst#L158) |
| 43573 | bloodrune | [areas/qst/earth.qst:165](../../../areas/qst/earth.qst#L165) |
| 43588 | hi hello | [areas/qst/earth.qst:175](../../../areas/qst/earth.qst#L175) |
| 43591 | hi hello | [areas/qst/earth.qst:183](../../../areas/qst/earth.qst#L183) |
| 43593 | hi hello | [areas/qst/earth.qst:191](../../../areas/qst/earth.qst#L191) |
| 43595 | hi hello | [areas/qst/earth.qst:199](../../../areas/qst/earth.qst#L199) |
| 43596 | hi hello | [areas/qst/earth.qst:205](../../../areas/qst/earth.qst#L205) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 43576 | `eligoth_rift_spawn` | [src/specs/specs.assign.c:2188](../../../src/specs/specs.assign.c#L2188) |
| obj | 43584 | `toe_chamber_switch` | [src/specs/specs.assign.c:2189](../../../src/specs/specs.assign.c#L2189) |
| room | 43341 | `patrol_shops` | [src/specs/specs.assign.c:2333](../../../src/specs/specs.assign.c#L2333) |

## Reset coverage

356 parsed reset commands: D: 56, E: 43, F: 22, G: 12, M: 165, O: 51, P: 7.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
