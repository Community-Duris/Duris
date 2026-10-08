# Temple of Flames: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence temple \
  --evidence-format markdown --output docs/reference/zone-story-audits/temple.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 18302 | `give=I:18300;receive=I:18301,I:18302;disappear=1` | story: Illyn: reclaim the lost soul | [areas/qst/temple.qst:20](../../../areas/qst/temple.qst#L20) |
| 18310 | `give=I:18309;receive=;disappear=1` | story: Lleddraick: the source of the plague | [areas/qst/temple.qst:87](../../../areas/qst/temple.qst#L87) |
| 18325 | `give=I:18322,I:18322,I:18324,I:18325;receive=C:500000;disappear=0` | story: The sage: four proofs from the faerie forest | [areas/qst/temple.qst:233](../../../areas/qst/temple.qst#L233) |
| 18331 | `give=I:18336;receive=I:18337,I:18337;disappear=1` | story: The angel: a token of another soul | [areas/qst/temple.qst:273](../../../areas/qst/temple.qst#L273) |
| 18336 | `give=I:18307;receive=I:18316;disappear=1` | story: The weapons master: a memory of his love | [areas/qst/temple.qst:294](../../../areas/qst/temple.qst#L294) |
| 18337 | `give=I:18339;receive=;disappear=0` | story: Ommsh: the stonecrusher and the smash-hole | [areas/qst/temple.qst:325](../../../areas/qst/temple.qst#L325) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 18302 | fire emblem | [areas/qst/temple.qst:2](../../../areas/qst/temple.qst#L2) |
| 18302 | temple | [areas/qst/temple.qst:7](../../../areas/qst/temple.qst#L7) |
| 18302 | messiah | [areas/qst/temple.qst:14](../../../areas/qst/temple.qst#L14) |
| 18310 | temple | [areas/qst/temple.qst:32](../../../areas/qst/temple.qst#L32) |
| 18310 | first plague | [areas/qst/temple.qst:39](../../../areas/qst/temple.qst#L39) |
| 18310 | possibility cause | [areas/qst/temple.qst:51](../../../areas/qst/temple.qst#L51) |
| 18310 | water well | [areas/qst/temple.qst:60](../../../areas/qst/temple.qst#L60) |
| 18310 | second | [areas/qst/temple.qst:70](../../../areas/qst/temple.qst#L70) |
| 18310 | king | [areas/qst/temple.qst:78](../../../areas/qst/temple.qst#L78) |
| 18311 | blood rape | [areas/qst/temple.qst:110](../../../areas/qst/temple.qst#L110) |
| 18311 | graveyard attendant | [areas/qst/temple.qst:117](../../../areas/qst/temple.qst#L117) |
| 18317 | blood | [areas/qst/temple.qst:128](../../../areas/qst/temple.qst#L128) |
| 18317 | hunt sustenance | [areas/qst/temple.qst:132](../../../areas/qst/temple.qst#L132) |
| 18317 | battle | [areas/qst/temple.qst:138](../../../areas/qst/temple.qst#L138) |
| 18325 | globe | [areas/qst/temple.qst:150](../../../areas/qst/temple.qst#L150) |
| 18325 | here | [areas/qst/temple.qst:156](../../../areas/qst/temple.qst#L156) |
| 18325 | temple | [areas/qst/temple.qst:161](../../../areas/qst/temple.qst#L161) |
| 18325 | faeries | [areas/qst/temple.qst:169](../../../areas/qst/temple.qst#L169) |
| 18325 | things | [areas/qst/temple.qst:177](../../../areas/qst/temple.qst#L177) |
| 18325 | demon demons | [areas/qst/temple.qst:183](../../../areas/qst/temple.qst#L183) |
| 18325 | illyn | [areas/qst/temple.qst:201](../../../areas/qst/temple.qst#L201) |
| 18325 | samael | [areas/qst/temple.qst:217](../../../areas/qst/temple.qst#L217) |
| 18325 | soul | [areas/qst/temple.qst:228](../../../areas/qst/temple.qst#L228) |
| 18329 | name | [areas/qst/temple.qst:243](../../../areas/qst/temple.qst#L243) |
| 18329 | hole | [areas/qst/temple.qst:247](../../../areas/qst/temple.qst#L247) |
| 18331 | help | [areas/qst/temple.qst:258](../../../areas/qst/temple.qst#L258) |
| 18331 | possessed | [areas/qst/temple.qst:264](../../../areas/qst/temple.qst#L264) |
| 18331 | others | [areas/qst/temple.qst:269](../../../areas/qst/temple.qst#L269) |
| 18336 | sad | [areas/qst/temple.qst:284](../../../areas/qst/temple.qst#L284) |
| 18336 | her | [areas/qst/temple.qst:289](../../../areas/qst/temple.qst#L289) |
| 18337 | rock rocks | [areas/qst/temple.qst:311](../../../areas/qst/temple.qst#L311) |
| 18337 | smart | [areas/qst/temple.qst:315](../../../areas/qst/temple.qst#L315) |
| 18337 | puzzle | [areas/qst/temple.qst:319](../../../areas/qst/temple.qst#L319) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 18302 | `temple_illyn` | [src/specs/specs.assign.c:992](../../../src/specs/specs.assign.c#L992) |

## Reset coverage

346 parsed reset commands: D: 52, E: 33, G: 12, M: 206, O: 41, P: 2.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
