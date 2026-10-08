# Ceothia: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence ceopast \
  --evidence-format markdown --output docs/reference/zone-story-audits/ceopast.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 81105 | `give=I:81102;receive=I:81101,I:81104;disappear=1` | story: Dryad: offer the white bell blossom | [areas/qst/ceopast.qst:25](../../../areas/qst/ceopast.qst#L25) |
| 81106 | `give=I:81101,I:81103,I:81106;receive=I:81105,I:81107;disappear=0` | story: Majelle: gather the three potion components | [areas/qst/ceopast.qst:104](../../../areas/qst/ceopast.qst#L104) |
| 81109 | `give=I:81108;receive=C:500000;disappear=0` | story: Jamael: bring a black, white or brown wolf pelt | [areas/qst/ceopast.qst:149](../../../areas/qst/ceopast.qst#L149) |
| 81109 | `give=I:81114;receive=C:500000;disappear=0` | story: Jamael: bring a black, white or brown wolf pelt | [areas/qst/ceopast.qst:161](../../../areas/qst/ceopast.qst#L161) |
| 81109 | `give=I:81115;receive=C:500000;disappear=0` | story: Jamael: bring a black, white or brown wolf pelt | [areas/qst/ceopast.qst:173](../../../areas/qst/ceopast.qst#L173) |
| 81142 | `give=I:81108;receive=I:81120;disappear=0` | story: Wolfspeed: check the current black-pelt exchange | [areas/qst/ceopast.qst:215](../../../areas/qst/ceopast.qst#L215) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 81105 | hello hi howdy | [areas/qst/ceopast.qst:2](../../../areas/qst/ceopast.qst#L2) |
| 81105 | help | [areas/qst/ceopast.qst:12](../../../areas/qst/ceopast.qst#L12) |
| 81106 | hello hi howdy | [areas/qst/ceopast.qst:43](../../../areas/qst/ceopast.qst#L43) |
| 81106 | dream dreams prophecy | [areas/qst/ceopast.qst:55](../../../areas/qst/ceopast.qst#L55) |
| 81106 | quest components potion | [areas/qst/ceopast.qst:84](../../../areas/qst/ceopast.qst#L84) |
| 81109 | hello howdy hi | [areas/qst/ceopast.qst:132](../../../areas/qst/ceopast.qst#L132) |
| 81142 | hello hi howdy | [areas/qst/ceopast.qst:187](../../../areas/qst/ceopast.qst#L187) |
| 81142 | wolf winter | [areas/qst/ceopast.qst:201](../../../areas/qst/ceopast.qst#L201) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

129 parsed reset commands: D: 10, E: 5, F: 24, G: 9, M: 75, O: 6.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
