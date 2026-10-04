# Northern Lakes and Settlements: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence nlakes \
  --evidence-format markdown --output docs/reference/zone-story-audits/nlakes.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 75239 | `give=I:75252;receive=I:75263;disappear=1` | story: Help the lost human return home | [areas/qst/nlakes.qst:17](../../../areas/qst/nlakes.qst#L17) |
| 75254 | `give=I:75215,I:75271,I:75271;receive=C:250000,I:55287;disappear=0` | story: Artek’s dragons and demon | [areas/qst/nlakes.qst:44](../../../areas/qst/nlakes.qst#L44) |
| 75255 | `give=I:75274;receive=I:75273;disappear=1` | story: Free the green dragon from its curse | [areas/qst/nlakes.qst:64](../../../areas/qst/nlakes.qst#L64) |
| 75260 | `give=I:75268;receive=I:75281;disappear=0` | story: Tamara: prepare the heart order | [areas/qst/nlakes.qst:101](../../../areas/qst/nlakes.qst#L101) |
| 75260 | `give=I:75280;receive=C:10000,I:75279;disappear=0` | story: Tamara: return the delivery note | [areas/qst/nlakes.qst:110](../../../areas/qst/nlakes.qst#L110) |
| 75261 | `give=I:75281;receive=I:75280;disappear=0` | story: Aerin: deliver the bottled heart | [areas/qst/nlakes.qst:126](../../../areas/qst/nlakes.qst#L126) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 75239 | home homesick | [areas/qst/nlakes.qst:2](../../../areas/qst/nlakes.qst#L2) |
| 75239 | scroll recall word | [areas/qst/nlakes.qst:11](../../../areas/qst/nlakes.qst#L11) |
| 75254 | hi hello hey howdy | [areas/qst/nlakes.qst:31](../../../areas/qst/nlakes.qst#L31) |
| 75254 | dragon dragons demon demons | [areas/qst/nlakes.qst:37](../../../areas/qst/nlakes.qst#L37) |
| 75255 | flight fly help | [areas/qst/nlakes.qst:53](../../../areas/qst/nlakes.qst#L53) |
| 75260 | help order late | [areas/qst/nlakes.qst:88](../../../areas/qst/nlakes.qst#L88) |
| 75261 | order | [areas/qst/nlakes.qst:120](../../../areas/qst/nlakes.qst#L120) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

414 parsed reset commands: D: 50, E: 115, F: 5, G: 23, M: 166, O: 20, P: 31, R: 4.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
