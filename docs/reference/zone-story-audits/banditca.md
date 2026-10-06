# Bandit Camp: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence banditca \
  --evidence-format markdown --output docs/reference/zone-story-audits/banditca.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 14807 | `give=I:14831;receive=E:10000;disappear=1` | request: Return a shackle key to the slaves | [areas/qst/banditca.qst:38](../../../areas/qst/banditca.qst#L38) |
| 14820 | `give=I:14821;receive=C:5000;disappear=0` | request: Return an inner-circle insignia | [areas/qst/banditca.qst:158](../../../areas/qst/banditca.qst#L158) |
| 14820 | `give=I:14826;receive=I:14827;disappear=0` | request: Bring Perrin's son's ring to the paladin | [areas/qst/banditca.qst:150](../../../areas/qst/banditca.qst#L150) |
| 14824 | `give=I:14828;receive=E:125000;disappear=1` | request: Return the daughter's key for shackles | [areas/qst/banditca.qst:174](../../../areas/qst/banditca.qst#L174) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 14820 | hi hello quest help hail | [areas/qst/banditca.qst:91](../../../areas/qst/banditca.qst#L91) |
| 14820 | boy | [areas/qst/banditca.qst:112](../../../areas/qst/banditca.qst#L112) |
| 14820 | girl | [areas/qst/banditca.qst:123](../../../areas/qst/banditca.qst#L123) |
| 14820 | kill | [areas/qst/banditca.qst:134](../../../areas/qst/banditca.qst#L134) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

234 parsed reset commands: D: 30, E: 27, F: 6, G: 19, M: 92, O: 39, P: 21.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
