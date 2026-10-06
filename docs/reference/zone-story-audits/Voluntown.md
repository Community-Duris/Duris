# The Royal Mausoleum of Castle IceCrag: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence Voluntown \
  --evidence-format markdown --output docs/reference/zone-story-audits/Voluntown.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 142400 | `give=I:142404,I:142405,I:142406,I:142407,I:142408,I:142409;receive=I:142443;disappear=0` | story: Rebuild the Drakenstone key | [areas/qst/Voluntown.qst:64](../../../areas/qst/Voluntown.qst#L64) |
| 142400 | `give=I:142450;receive=I:142451;disappear=0` | story: Bring back a strand of time | [areas/qst/Voluntown.qst:77](../../../areas/qst/Voluntown.qst#L77) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 142400 | necropolis | [areas/qst/Voluntown.qst:2](../../../areas/qst/Voluntown.qst#L2) |
| 142400 | family families | [areas/qst/Voluntown.qst:12](../../../areas/qst/Voluntown.qst#L12) |
| 142400 | auril | [areas/qst/Voluntown.qst:23](../../../areas/qst/Voluntown.qst#L23) |
| 142400 | drakenstone | [areas/qst/Voluntown.qst:33](../../../areas/qst/Voluntown.qst#L33) |
| 142400 | mary tailor time | [areas/qst/Voluntown.qst:53](../../../areas/qst/Voluntown.qst#L53) |
| 142401 | drakenstone | [areas/qst/Voluntown.qst:86](../../../areas/qst/Voluntown.qst#L86) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

185 parsed reset commands: D: 22, E: 33, F: 10, G: 12, M: 79, O: 15, P: 14.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
