# The Tower of Darkness: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence lortower \
  --evidence-format markdown --output docs/reference/zone-story-audits/lortower.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 134010 | `give=I:134039;receive=E:500000;disappear=0` | story: Katalia’s binding key | [areas/qst/lortower.qst:12](../../../areas/qst/lortower.qst#L12) |
| 134026 | `give=I:134035;receive=E:70000,I:134034;disappear=0` | story: The Bloodstone messenger | [areas/qst/lortower.qst:33](../../../areas/qst/lortower.qst#L33) |
| 134029 | `give=C:100000;receive=I:134019;disappear=1` | story: A key to the captain’s quarters | [areas/qst/lortower.qst:61](../../../areas/qst/lortower.qst#L61) |
| 134029 | `give=I:134034;receive=I:134019;disappear=0` | story: A key to the captain’s quarters | [areas/qst/lortower.qst:71](../../../areas/qst/lortower.qst#L71) |
| 134053 | `give=I:134004,I:134004,I:134004,I:134004,I:134004,I:134030;receive=I:134028;disappear=0` | story: Dorthan’s shield of Dubneth | [areas/qst/lortower.qst:119](../../../areas/qst/lortower.qst#L119) |
| 134054 | `give=I:134026;receive=I:134030;disappear=1` | story: Amelia’s release and locket | [areas/qst/lortower.qst:143](../../../areas/qst/lortower.qst#L143) |
| 134074 | `give=I:134111,I:134112,I:134113;receive=I:134117;disappear=0` | story: The three keys of stasis | [areas/qst/lortower.qst:155](../../../areas/qst/lortower.qst#L155) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 134010 | slave | [areas/qst/lortower.qst:2](../../../areas/qst/lortower.qst#L2) |
| 134026 | traveler emissary bloodstone | [areas/qst/lortower.qst:26](../../../areas/qst/lortower.qst#L26) |
| 134029 | money mercenary | [areas/qst/lortower.qst:46](../../../areas/qst/lortower.qst#L46) |
| 134029 | token message letter traveler | [areas/qst/lortower.qst:55](../../../areas/qst/lortower.qst#L55) |
| 134053 | daughter picture | [areas/qst/lortower.qst:79](../../../areas/qst/lortower.qst#L79) |
| 134053 | urian | [areas/qst/lortower.qst:93](../../../areas/qst/lortower.qst#L93) |
| 134053 | black tower | [areas/qst/lortower.qst:98](../../../areas/qst/lortower.qst#L98) |
| 134053 | black iron sword | [areas/qst/lortower.qst:106](../../../areas/qst/lortower.qst#L106) |
| 134053 | shield | [areas/qst/lortower.qst:113](../../../areas/qst/lortower.qst#L113) |
| 134054 | key | [areas/qst/lortower.qst:136](../../../areas/qst/lortower.qst#L136) |
| 134125 | key keys door doors | [areas/qst/lortower.qst:164](../../../areas/qst/lortower.qst#L164) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

580 parsed reset commands: D: 114, E: 116, F: 50, G: 32, M: 176, O: 74, P: 18.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
