# The Transparent Tower: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence trnsptow \
  --evidence-format markdown --output docs/reference/zone-story-audits/trnsptow.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 16203 | `give=I:16241;receive=I:16241,I:16264;disappear=1` | story: Gullivier's pale purple token | [areas/qst/trnsptow.qst:68](../../../areas/qst/trnsptow.qst#L68) |
| 16206 | `give=I:16241;receive=I:16241,I:16264;disappear=1` | story: Devilish's pale purple token | [areas/qst/trnsptow.qst:104](../../../areas/qst/trnsptow.qst#L104) |
| 16207 | `give=I:16241,I:16264,I:16264,I:16264;receive=I:16258;disappear=0` | story: The librarian's mist key | [areas/qst/trnsptow.qst:195](../../../areas/qst/trnsptow.qst#L195) |
| 16234 | `give=I:16241;receive=I:16241,I:16264;disappear=1` | story: Lisa's pale purple token | [areas/qst/trnsptow.qst:281](../../../areas/qst/trnsptow.qst#L281) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 16203 | tower | [areas/qst/trnsptow.qst:2](../../../areas/qst/trnsptow.qst#L2) |
| 16203 | devilish | [areas/qst/trnsptow.qst:9](../../../areas/qst/trnsptow.qst#L9) |
| 16203 | gullivier | [areas/qst/trnsptow.qst:21](../../../areas/qst/trnsptow.qst#L21) |
| 16203 | lisa | [areas/qst/trnsptow.qst:30](../../../areas/qst/trnsptow.qst#L30) |
| 16203 | four three | [areas/qst/trnsptow.qst:40](../../../areas/qst/trnsptow.qst#L40) |
| 16203 | hell | [areas/qst/trnsptow.qst:48](../../../areas/qst/trnsptow.qst#L48) |
| 16203 | escape | [areas/qst/trnsptow.qst:55](../../../areas/qst/trnsptow.qst#L55) |
| 16206 | tower escape hell | [areas/qst/trnsptow.qst:82](../../../areas/qst/trnsptow.qst#L82) |
| 16206 | aceralde | [areas/qst/trnsptow.qst:90](../../../areas/qst/trnsptow.qst#L90) |
| 16206 | scepter | [areas/qst/trnsptow.qst:97](../../../areas/qst/trnsptow.qst#L97) |
| 16207 | tower library | [areas/qst/trnsptow.qst:125](../../../areas/qst/trnsptow.qst#L125) |
| 16207 | escape | [areas/qst/trnsptow.qst:134](../../../areas/qst/trnsptow.qst#L134) |
| 16207 | book books | [areas/qst/trnsptow.qst:144](../../../areas/qst/trnsptow.qst#L144) |
| 16207 | four | [areas/qst/trnsptow.qst:152](../../../areas/qst/trnsptow.qst#L152) |
| 16207 | aceralde | [areas/qst/trnsptow.qst:159](../../../areas/qst/trnsptow.qst#L159) |
| 16207 | lisa | [areas/qst/trnsptow.qst:166](../../../areas/qst/trnsptow.qst#L166) |
| 16207 | gullivier | [areas/qst/trnsptow.qst:173](../../../areas/qst/trnsptow.qst#L173) |
| 16207 | devilish | [areas/qst/trnsptow.qst:180](../../../areas/qst/trnsptow.qst#L180) |
| 16207 | lord lords | [areas/qst/trnsptow.qst:188](../../../areas/qst/trnsptow.qst#L188) |
| 16234 | tower | [areas/qst/trnsptow.qst:214](../../../areas/qst/trnsptow.qst#L214) |
| 16234 | aceralde | [areas/qst/trnsptow.qst:221](../../../areas/qst/trnsptow.qst#L221) |
| 16234 | scepter | [areas/qst/trnsptow.qst:231](../../../areas/qst/trnsptow.qst#L231) |
| 16234 | three four | [areas/qst/trnsptow.qst:240](../../../areas/qst/trnsptow.qst#L240) |
| 16234 | devilish | [areas/qst/trnsptow.qst:251](../../../areas/qst/trnsptow.qst#L251) |
| 16234 | gullivier | [areas/qst/trnsptow.qst:261](../../../areas/qst/trnsptow.qst#L261) |
| 16234 | librarian | [areas/qst/trnsptow.qst:271](../../../areas/qst/trnsptow.qst#L271) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 16205 | `transp_tow_acerlade` | [src/specs/specs.assign.c:380](../../../src/specs/specs.assign.c#L380) |
| obj | 16263 | `artifact_stone` | [src/specs/specs.assign.c:1373](../../../src/specs/specs.assign.c#L1373) |
| obj | 16268 | `artifact_stone` | [src/specs/specs.assign.c:1374](../../../src/specs/specs.assign.c#L1374) |
| obj | 16262 | `trans_tower_shadow_globe` | [src/specs/specs.assign.c:1722](../../../src/specs/specs.assign.c#L1722) |
| obj | 16242 | `zion_light_dark` | [src/specs/specs.assign.c:2172](../../../src/specs/specs.assign.c#L2172) |

## Reset coverage

208 parsed reset commands: D: 32, E: 52, F: 3, G: 5, M: 73, O: 38, P: 5.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
