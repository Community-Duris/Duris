# Desolate: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence desolate \
  --evidence-format markdown --output docs/reference/zone-story-audits/desolate.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 22225 | `give=I:22230;receive=C:50000;disappear=0` | story: A drink for the tired halfling | [areas/qst/desolate.qst:2](../../../areas/qst/desolate.qst#L2) |
| 22227 | `give=I:22215;receive=C:10000,I:22220;disappear=0` | story: The minotaur's lost chain | [areas/qst/desolate.qst:10](../../../areas/qst/desolate.qst#L10) |
| 22234 | `give=I:22229;receive=I:22230;disappear=0` | service: Fill a tankard with ale | [areas/qst/desolate.qst:29](../../../areas/qst/desolate.qst#L29) |
| 22238 | `give=I:22214;receive=C:15000,I:22269;disappear=0` | story: Recover the lost monkey | [areas/qst/desolate.qst:39](../../../areas/qst/desolate.qst#L39) |
| 22248 | `give=I:22251;receive=I:22252;disappear=1` | story: Help the stranded wagon driver | [areas/qst/desolate.qst:49](../../../areas/qst/desolate.qst#L49) |
| 22250 | `give=I:22283,I:22284;receive=I:22288;disappear=0` | story: Beregan's two proofs | [areas/qst/desolate.qst:61](../../../areas/qst/desolate.qst#L61) |
| 22266 | `give=I:22216;receive=I:22286;disappear=0` | story: The delegate's warning | [areas/qst/desolate.qst:70](../../../areas/qst/desolate.qst#L70) |
| 22269 | `give=C:5000,I:22220,I:22231;receive=I:22251;disappear=0` | story: Scotson's wheel repair | [areas/qst/desolate.qst:81](../../../areas/qst/desolate.qst#L81) |
| 22282 | `give=C:10000;receive=I:22255;disappear=1` | service: Buy the travelling merchant's potion | [areas/qst/desolate.qst:92](../../../areas/qst/desolate.qst#L92) |
| 22285 | `give=I:22211;receive=I:22250;disappear=0` | story: Feed the shaggy dog | [areas/qst/desolate.qst:102](../../../areas/qst/desolate.qst#L102) |
| 22294 | `give=I:82543;receive=C:10000,I:22289;disappear=0` | story: The Seraphim's information | [areas/qst/desolate.qst:111](../../../areas/qst/desolate.qst#L111) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 22234 | ale | [areas/qst/desolate.qst:23](../../../areas/qst/desolate.qst#L23) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| obj | 22237 | `master_set` | [src/specs/specs.assign.c:1340](../../../src/specs/specs.assign.c#L1340) |

## Reset coverage

400 parsed reset commands: D: 60, E: 61, F: 9, G: 24, M: 195, O: 45, P: 4, R: 2.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
