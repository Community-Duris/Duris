# The City of Newhaven: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence newhaven \
  --evidence-format markdown --output docs/reference/zone-story-audits/newhaven.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 35204 | `give=I:35226;receive=E:50000;disappear=0` | story: The blacksmith's missing wife | [areas/qst/newhaven.qst:32](../../../areas/qst/newhaven.qst#L32) |
| 35211 | `give=C:45000,I:93901;receive=I:35228;disappear=0` | service: Scale and balance badge | [areas/qst/newhaven.qst:84](../../../areas/qst/newhaven.qst#L84) |
| 35211 | `give=C:50000,I:35237;receive=I:35240;disappear=0` | service: Displacer-hide cloak | [areas/qst/newhaven.qst:105](../../../areas/qst/newhaven.qst#L105) |
| 35211 | `give=C:55000,I:93901;receive=I:35239;disappear=0` | service: Hammer and anvil badge | [areas/qst/newhaven.qst:98](../../../areas/qst/newhaven.qst#L98) |
| 35211 | `give=C:75000,I:35227;receive=I:35229;disappear=0` | service: Lizard-tail bracer | [areas/qst/newhaven.qst:91](../../../areas/qst/newhaven.qst#L91) |
| 35213 | `give=I:35233;receive=E:18000;disappear=0` | request: Blackberries for the living baker | [areas/qst/newhaven.qst:135](../../../areas/qst/newhaven.qst#L135) |
| 35216 | `give=I:35238;receive=C:3500;disappear=0` | request: Dibbly's missing tobacco | [areas/qst/newhaven.qst:183](../../../areas/qst/newhaven.qst#L183) |
| 35216 | `give=I:88905;receive=C:5000;disappear=0` | service: Dibbly's snorkel-pipe buyback | [areas/qst/newhaven.qst:177](../../../areas/qst/newhaven.qst#L177) |
| 35286 | `give=C:100000,I:13221,I:98606;receive=I:35224;disappear=0` | service: Vulgaris's Veldian collar | [areas/qst/newhaven.qst:331](../../../areas/qst/newhaven.qst#L331) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 35204 | hi hello quest wife angry | [areas/qst/newhaven.qst:2](../../../areas/qst/newhaven.qst#L2) |
| 35211 | hi hello quest quests armor | [areas/qst/newhaven.qst:42](../../../areas/qst/newhaven.qst#L42) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| obj | 35102 | `magic_pool` | [src/specs/specs.assign.c:1280](../../../src/specs/specs.assign.c#L1280) |
| obj | 35103 | `magic_pool` | [src/specs/specs.assign.c:1281](../../../src/specs/specs.assign.c#L1281) |
| room | 35264 | `inn` | [src/specs/specs.assign.c:2292](../../../src/specs/specs.assign.c#L2292) |

## Reset coverage

252 parsed reset commands: D: 48, E: 16, F: 13, G: 12, M: 138, O: 24, P: 1.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
