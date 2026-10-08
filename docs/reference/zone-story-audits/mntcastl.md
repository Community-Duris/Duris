# Du'Maathe Castle: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence mntcastl \
  --evidence-format markdown --output docs/reference/zone-story-audits/mntcastl.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 37100 | `give=I:37103;receive=C:200000;disappear=0` | story: The lord: present the blue-tinged horn | [areas/qst/mntcastl.qst:70](../../../areas/qst/mntcastl.qst#L70) |
| 37100 | `give=I:37104;receive=I:37100;disappear=0` | story: The lord: exchange the lich’s black gem | [areas/qst/mntcastl.qst:137](../../../areas/qst/mntcastl.qst#L137) |
| 37100 | `give=I:37105;receive=C:200000;disappear=0` | story: The lord: bring a granular potion | [areas/qst/mntcastl.qst:83](../../../areas/qst/mntcastl.qst#L83) |
| 37100 | `give=I:37114;receive=I:37115;disappear=0` | story: The lord: present the spectral warrior’s tooth | [areas/qst/mntcastl.qst:105](../../../areas/qst/mntcastl.qst#L105) |
| 37102 | `give=I:37106,I:97903;receive=I:37105,I:37105,I:37105;disappear=0` | service: The frail man: prepare three granular potions | [areas/qst/mntcastl.qst:237](../../../areas/qst/mntcastl.qst#L237) |
| 37102 | `give=I:37108;receive=I:37109;disappear=0` | service: The frail man: prepare a frost potion | [areas/qst/mntcastl.qst:255](../../../areas/qst/mntcastl.qst#L255) |
| 37102 | `give=I:37110;receive=I:37111;disappear=0` | service: The frail man: prepare a protective potion | [areas/qst/mntcastl.qst:272](../../../areas/qst/mntcastl.qst#L272) |
| 37102 | `give=I:37121;receive=I:37122;disappear=0` | service: The frail man: prepare a home potion | [areas/qst/mntcastl.qst:286](../../../areas/qst/mntcastl.qst#L286) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 37100 | fool | [areas/qst/mntcastl.qst:2](../../../areas/qst/mntcastl.qst#L2) |
| 37100 | help | [areas/qst/mntcastl.qst:10](../../../areas/qst/mntcastl.qst#L10) |
| 37100 | talent | [areas/qst/mntcastl.qst:23](../../../areas/qst/mntcastl.qst#L23) |
| 37100 | hero | [areas/qst/mntcastl.qst:41](../../../areas/qst/mntcastl.qst#L41) |
| 37100 | proof | [areas/qst/mntcastl.qst:58](../../../areas/qst/mntcastl.qst#L58) |
| 37102 | granular | [areas/qst/mntcastl.qst:160](../../../areas/qst/mntcastl.qst#L160) |
| 37102 | frost | [areas/qst/mntcastl.qst:176](../../../areas/qst/mntcastl.qst#L176) |
| 37102 | protective | [areas/qst/mntcastl.qst:185](../../../areas/qst/mntcastl.qst#L185) |
| 37102 | hello hi howdy | [areas/qst/mntcastl.qst:198](../../../areas/qst/mntcastl.qst#L198) |
| 37102 | potion potions | [areas/qst/mntcastl.qst:207](../../../areas/qst/mntcastl.qst#L207) |
| 37102 | homely | [areas/qst/mntcastl.qst:214](../../../areas/qst/mntcastl.qst#L214) |
| 37102 | recipe | [areas/qst/mntcastl.qst:226](../../../areas/qst/mntcastl.qst#L226) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| room | 37313 | `inn` | [src/specs/specs.assign.c:2294](../../../src/specs/specs.assign.c#L2294) |
| room | 37434 | `inn` | [src/specs/specs.assign.c:2296](../../../src/specs/specs.assign.c#L2296) |

## Reset coverage

419 parsed reset commands: D: 12, E: 35, F: 5, G: 12, M: 352, O: 3.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
