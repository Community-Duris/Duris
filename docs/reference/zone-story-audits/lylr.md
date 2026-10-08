# Lylr-Meop: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence lylr \
  --evidence-format markdown --output docs/reference/zone-story-audits/lylr.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 82201 | `give=I:82217;receive=I:82200;disappear=1` | request: Bring Venmar the ogre scalp | [areas/qst/lylr.qst:11](../../../areas/qst/lylr.qst#L11) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 82201 | evil ogre | [areas/qst/lylr.qst:2](../../../areas/qst/lylr.qst#L2) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 82224 | `money_changer` | [src/specs/specs.assign.c:718](../../../src/specs/specs.assign.c#L718) |
| mob | 82229 | `world_quest` | [src/specs/specs.assign.c:778](../../../src/specs/specs.assign.c#L778) |
| mob | 82208 | `world_quest` | [src/specs/specs.assign.c:779](../../../src/specs/specs.assign.c#L779) |
| room | 82217 | `inn` | [src/specs/specs.assign.c:2307](../../../src/specs/specs.assign.c#L2307) |

## Reset coverage

144 parsed reset commands: D: 18, E: 25, F: 2, G: 21, M: 72, O: 4, P: 2.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
