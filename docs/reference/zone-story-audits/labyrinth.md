# Labyrinth of No Return: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence labyrinth \
  --evidence-format markdown --output docs/reference/zone-story-audits/labyrinth.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 5024 | `give=I:5014;receive=C:24000,E:24000,I:5073;disappear=1` | story: A map for the lost adventurer | [areas/qst/labyrinth.qst:24](../../../areas/qst/labyrinth.qst#L24) |
| 5028 | `give=I:5001,I:5002,I:5003,I:5005,I:5006,I:5007,I:5008,I:5009,I:5020;receive=E:50000,I:5031;disappear=0` | story: Nine proofs for the Golden Flame | [areas/qst/labyrinth.qst:85](../../../areas/qst/labyrinth.qst#L85) |
| 5055 | `give=I:5053,I:5058,I:5061;receive=I:5069,I:5070;disappear=1` | story: Three parts for Vadatorn | [areas/qst/labyrinth.qst:117](../../../areas/qst/labyrinth.qst#L117) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 5024 | lost map exit maze | [areas/qst/labyrinth.qst:2](../../../areas/qst/labyrinth.qst#L2) |
| 5024 | minotaurs minotaur horror horrors | [areas/qst/labyrinth.qst:12](../../../areas/qst/labyrinth.qst#L12) |
| 5028 | evil creatures monsters | [areas/qst/labyrinth.qst:41](../../../areas/qst/labyrinth.qst#L41) |
| 5028 | who are you knight golden flame | [areas/qst/labyrinth.qst:52](../../../areas/qst/labyrinth.qst#L52) |
| 5028 | where from come | [areas/qst/labyrinth.qst:56](../../../areas/qst/labyrinth.qst#L56) |
| 5028 | hi hello greetings | [areas/qst/labyrinth.qst:61](../../../areas/qst/labyrinth.qst#L61) |
| 5028 | proof evidence list destruction | [areas/qst/labyrinth.qst:65](../../../areas/qst/labyrinth.qst#L65) |
| 5055 | minotaur minotaurs quest curse | [areas/qst/labyrinth.qst:106](../../../areas/qst/labyrinth.qst#L106) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

282 parsed reset commands: D: 14, E: 38, F: 6, G: 62, M: 127, O: 28, P: 7.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
