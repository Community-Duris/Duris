# Jindon the Deathwood Forest: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence jin \
  --evidence-format markdown --output docs/reference/zone-story-audits/jin.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 82007 | `give=I:82004;receive=I:82003;disappear=0` | request: Help Sirax recover his lost arms | [areas/qst/jin.qst:8](../../../areas/qst/jin.qst#L8) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 82007 | thri-kreen two limb limbs ogre missing battle | [areas/qst/jin.qst:2](../../../areas/qst/jin.qst#L2) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 82000 | `jindo_ticket_master` | [src/specs/specs.assign.c:1225](../../../src/specs/specs.assign.c#L1225) |

## Reset coverage

132 parsed reset commands: D: 8, E: 25, G: 9, M: 86, O: 4.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
