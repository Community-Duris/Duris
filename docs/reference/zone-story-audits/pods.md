# Pharr Valley Swamp: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence pods \
  --evidence-format markdown --output docs/reference/zone-story-audits/pods.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 28533 | `give=C:50000;receive=I:28598;disappear=0` | service: The chamberlain: the dwarf’s tongue key | [areas/qst/pods.qst:39](../../../areas/qst/pods.qst#L39) |
| 28533 | `give=I:28579;receive=C:75000;disappear=0` | story: The chamberlain: a shard for an emperor | [areas/qst/pods.qst:33](../../../areas/qst/pods.qst#L33) |
| 28576 | `give=C:1000,I:28554,I:28554,I:28554,I:28554,I:28554,I:28554,I:28555;receive=I:28552;disappear=0` | service: The podaling: a birdfeather headdress | [areas/qst/pods.qst:187](../../../areas/qst/pods.qst#L187) |
| 28576 | `give=C:500,I:28555,I:28555,I:28555;receive=I:28553;disappear=0` | service: The podaling: a woven neberihide necklace | [areas/qst/pods.qst:199](../../../areas/qst/pods.qst#L199) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 28520 | pest | [areas/qst/pods.qst:2](../../../areas/qst/pods.qst#L2) |
| 28533 | elf elf-kin | [areas/qst/pods.qst:8](../../../areas/qst/pods.qst#L8) |
| 28533 | crystal shard | [areas/qst/pods.qst:14](../../../areas/qst/pods.qst#L14) |
| 28533 | dwarf mummified mummy | [areas/qst/pods.qst:22](../../../areas/qst/pods.qst#L22) |
| 28533 | key | [areas/qst/pods.qst:27](../../../areas/qst/pods.qst#L27) |
| 28565 | swamp home alone | [areas/qst/pods.qst:46](../../../areas/qst/pods.qst#L46) |
| 28565 | evil neighbor | [areas/qst/pods.qst:54](../../../areas/qst/pods.qst#L54) |
| 28565 | skexis | [areas/qst/pods.qst:62](../../../areas/qst/pods.qst#L62) |
| 28565 | ahgra | [areas/qst/pods.qst:73](../../../areas/qst/pods.qst#L73) |
| 28565 | mystic mystics | [areas/qst/pods.qst:80](../../../areas/qst/pods.qst#L80) |
| 28565 | messenger | [areas/qst/pods.qst:87](../../../areas/qst/pods.qst#L87) |
| 28565 | ruins wall walls | [areas/qst/pods.qst:93](../../../areas/qst/pods.qst#L93) |
| 28565 | gartham garthams | [areas/qst/pods.qst:101](../../../areas/qst/pods.qst#L101) |
| 28571 | ruins | [areas/qst/pods.qst:110](../../../areas/qst/pods.qst#L110) |
| 28571 | elf elf-kin elfling kin | [areas/qst/pods.qst:122](../../../areas/qst/pods.qst#L122) |
| 28571 | gartham | [areas/qst/pods.qst:128](../../../areas/qst/pods.qst#L128) |
| 28571 | skexis | [areas/qst/pods.qst:135](../../../areas/qst/pods.qst#L135) |
| 28576 | make weave woven hands | [areas/qst/pods.qst:143](../../../areas/qst/pods.qst#L143) |
| 28576 | feather feathers headband headdress | [areas/qst/pods.qst:149](../../../areas/qst/pods.qst#L149) |
| 28576 | neberihide neberi strips strip necklace | [areas/qst/pods.qst:156](../../../areas/qst/pods.qst#L156) |
| 28576 | ruins ahgra walls | [areas/qst/pods.qst:162](../../../areas/qst/pods.qst#L162) |
| 28576 | keera | [areas/qst/pods.qst:169](../../../areas/qst/pods.qst#L169) |
| 28576 | elf elf-kin | [areas/qst/pods.qst:174](../../../areas/qst/pods.qst#L174) |
| 28576 | skexis gartham | [areas/qst/pods.qst:182](../../../areas/qst/pods.qst#L182) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

1349 parsed reset commands: D: 18, E: 25, F: 10, G: 118, M: 1015, O: 145, P: 18.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
