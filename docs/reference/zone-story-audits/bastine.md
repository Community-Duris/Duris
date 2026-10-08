# Bastine Castle: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence bastine \
  --evidence-format markdown --output docs/reference/zone-story-audits/bastine.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 7600 | `give=I:12802;receive=C:5000;disappear=0` | request: The Bastine Road: Adept Warrior | [areas/qst/bastine.qst:42](../../../areas/qst/bastine.qst#L42) |
| 7600 | `give=I:2607;receive=C:75000;disappear=0` | request: The Bastine Road: Lieutenant | [areas/qst/bastine.qst:69](../../../areas/qst/bastine.qst#L69) |
| 7600 | `give=I:41327;receive=C:15000;disappear=0` | request: The Bastine Road: Veteran | [areas/qst/bastine.qst:51](../../../areas/qst/bastine.qst#L51) |
| 7600 | `give=I:41375;receive=C:100000;disappear=0` | request: The Bastine Road: Captain | [areas/qst/bastine.qst:109](../../../areas/qst/bastine.qst#L109) |
| 7600 | `give=I:41388;receive=C:2000;disappear=0` | request: The Bastine Road: Young Warrior | [areas/qst/bastine.qst:22](../../../areas/qst/bastine.qst#L22) |
| 7600 | `give=I:41407;receive=C:10000;disappear=0` | request: The Bastine Road: Warrior | [areas/qst/bastine.qst:34](../../../areas/qst/bastine.qst#L34) |
| 7600 | `give=I:41408;receive=C:100000;disappear=0` | request: The Bastine Road: Third Ranked Captain | [areas/qst/bastine.qst:81](../../../areas/qst/bastine.qst#L81) |
| 7600 | `give=I:41411;receive=C:200000,I:41304;disappear=0` | request: The Bastine Road: General | [areas/qst/bastine.qst:124](../../../areas/qst/bastine.qst#L124) |
| 7600 | `give=I:41920;receive=C:50000;disappear=0` | request: The Bastine Road: First Ranked Captain | [areas/qst/bastine.qst:101](../../../areas/qst/bastine.qst#L101) |
| 7600 | `give=I:41922;receive=C:100000;disappear=0` | request: The Bastine Road: Second Ranked Captain | [areas/qst/bastine.qst:92](../../../areas/qst/bastine.qst#L92) |
| 7600 | `give=I:41924;receive=C:50000;disappear=0` | request: The Bastine Road: Soldier | [areas/qst/bastine.qst:59](../../../areas/qst/bastine.qst#L59) |
| 7600 | `give=I:70970;receive=C:1000000,I:7616,I:7617;disappear=0` | request: The Bastine Road: Knight of the Bastine Order | [areas/qst/bastine.qst:141](../../../areas/qst/bastine.qst#L141) |
| 7600 | `give=I:7673;receive=I:7674;disappear=0` | story: The Werewolf Prince | [areas/qst/bastine.qst:157](../../../areas/qst/bastine.qst#L157) |
| 7603 | `give=I:41397;receive=I:41394,I:41398;disappear=0` | story: The Apprentice's Trust | [areas/qst/bastine.qst:187](../../../areas/qst/bastine.qst#L187) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 7600 | hi hello knight | [areas/qst/bastine.qst:2](../../../areas/qst/bastine.qst#L2) |
| 7600 | no | [areas/qst/bastine.qst:9](../../../areas/qst/bastine.qst#L9) |
| 7600 | yes | [areas/qst/bastine.qst:14](../../../areas/qst/bastine.qst#L14) |
| 7603 | hello hi morlanthra | [areas/qst/bastine.qst:174](../../../areas/qst/bastine.qst#L174) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

428 parsed reset commands: D: 18, E: 200, G: 37, M: 158, O: 13, P: 2.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
