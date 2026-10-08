# Southern Coastal Highway: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence highway \
  --evidence-format markdown --output docs/reference/zone-story-audits/highway.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 41315 | `give=I:41398;receive=E:2500,I:41405;disappear=1` | story: Morlanthra’s proof of trust | [areas/qst/highway.qst:19](../../../areas/qst/highway.qst#L19) |
| 41360 | `give=I:41348;receive=C:10000,E:10000,I:41417;disappear=0` | story: Magnamus’s emerald chalice | [areas/qst/highway.qst:75](../../../areas/qst/highway.qst#L75) |
| 41360 | `give=I:41415,I:41416;receive=I:41419;disappear=0` | story: Hair and a tooth for Magnamus | [areas/qst/highway.qst:90](../../../areas/qst/highway.qst#L90) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 41315 | hi hello greetings welcome prison free | [areas/qst/highway.qst:2](../../../areas/qst/highway.qst#L2) |
| 41315 | proof | [areas/qst/highway.qst:9](../../../areas/qst/highway.qst#L9) |
| 41315 | volheru king mrotha victor knights | [areas/qst/highway.qst:14](../../../areas/qst/highway.qst#L14) |
| 41360 | hi artifacts quest | [areas/qst/highway.qst:34](../../../areas/qst/highway.qst#L34) |
| 41360 | items help yes interested | [areas/qst/highway.qst:44](../../../areas/qst/highway.qst#L44) |
| 41360 | harpy | [areas/qst/highway.qst:53](../../../areas/qst/highway.qst#L53) |
| 41360 | out | [areas/qst/highway.qst:63](../../../areas/qst/highway.qst#L63) |
| 41360 | in hunt hunting more | [areas/qst/highway.qst:67](../../../areas/qst/highway.qst#L67) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| obj | 41304 | `kearonor_hide` | [src/specs/specs.assign.c:1736](../../../src/specs/specs.assign.c#L1736) |
| obj | 41349 | `hewards_mystical_organ` | [src/specs/specs.assign.c:1737](../../../src/specs/specs.assign.c#L1737) |
| obj | 41350 | `wand_of_wonder` | [src/specs/specs.assign.c:1739](../../../src/specs/specs.assign.c#L1739) |

## Reset coverage

296 parsed reset commands: D: 36, E: 82, G: 9, M: 130, O: 25, P: 12, R: 2.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
