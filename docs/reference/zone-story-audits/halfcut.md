# The Halfcut Hills: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence halfcut \
  --evidence-format markdown --output docs/reference/zone-story-audits/halfcut.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 27002 | `give=I:27037;receive=C:25000,E:25000;disappear=0` | request: Medicine for the wounded dwarf | [areas/qst/halfcut.qst:13](../../../areas/qst/halfcut.qst#L13) |
| 27005 | `give=I:27044;receive=I:27045;disappear=1` | story: Return Bartis's note to the sentry | [areas/qst/halfcut.qst:31](../../../areas/qst/halfcut.qst#L31) |
| 27033 | `give=I:27051;receive=I:27060;disappear=0` | request: The raid leader's scalp for Remi | [areas/qst/halfcut.qst:53](../../../areas/qst/halfcut.qst#L53) |
| 27059 | `give=I:27039;receive=E:2000,I:27040;disappear=1` | story: A jar for the first old miner | [areas/qst/halfcut.qst:61](../../../areas/qst/halfcut.qst#L61) |
| 27060 | `give=I:27039;receive=E:2500,I:27042;disappear=1` | story: A jar for the miner in the well | [areas/qst/halfcut.qst:75](../../../areas/qst/halfcut.qst#L75) |
| 27065 | `give=I:27039;receive=C:150000,I:27044;disappear=1` | story: Bartis's final jar and note | [areas/qst/halfcut.qst:121](../../../areas/qst/halfcut.qst#L121) |
| 27065 | `give=I:27040,I:27041,I:27042;receive=I:27043;disappear=0` | story: Three distinct badges for Bartis | [areas/qst/halfcut.qst:112](../../../areas/qst/halfcut.qst#L112) |
| 27071 | `give=I:27039;receive=E:20000,I:27041;disappear=1` | story: A jar for the second old miner | [areas/qst/halfcut.qst:136](../../../areas/qst/halfcut.qst#L136) |
| 27078 | `give=I:27052,I:27053,I:27054,I:27055,I:27057,I:27058;receive=I:27059;disappear=0` | story: Six proofs for the raid leader | [areas/qst/halfcut.qst:177](../../../areas/qst/halfcut.qst#L177) |
| 27080 | `give=I:27054;receive=C:15000,E:25000;disappear=0` | request: The goblin's scalp for the orc | [areas/qst/halfcut.qst:197](../../../areas/qst/halfcut.qst#L197) |
| 27081 | `give=I:27052;receive=C:20000,E:15000;disappear=0` | request: The orc's scalp for the goblin | [areas/qst/halfcut.qst:214](../../../areas/qst/halfcut.qst#L214) |
| 27082 | `give=I:27055;receive=I:25000,I:27056;disappear=0` | request: The duergar's scalp for the drow | [areas/qst/halfcut.qst:231](../../../areas/qst/halfcut.qst#L231) |
| 27083 | `give=I:27053;receive=C:20000,E:25000;disappear=0` | request: The drow's scalp for the duergar | [areas/qst/halfcut.qst:246](../../../areas/qst/halfcut.qst#L246) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 27002 | hi hello wounded | [areas/qst/halfcut.qst:2](../../../areas/qst/halfcut.qst#L2) |
| 27002 | poison | [areas/qst/halfcut.qst:8](../../../areas/qst/halfcut.qst#L8) |
| 27005 | mine where | [areas/qst/halfcut.qst:23](../../../areas/qst/halfcut.qst#L23) |
| 27033 | evils evil raid | [areas/qst/halfcut.qst:47](../../../areas/qst/halfcut.qst#L47) |
| 27065 | worried | [areas/qst/halfcut.qst:93](../../../areas/qst/halfcut.qst#L93) |
| 27065 | miner miners | [areas/qst/halfcut.qst:100](../../../areas/qst/halfcut.qst#L100) |
| 27065 | free | [areas/qst/halfcut.qst:106](../../../areas/qst/halfcut.qst#L106) |
| 27078 | worried | [areas/qst/halfcut.qst:150](../../../areas/qst/halfcut.qst#L150) |
| 27078 | rook plan | [areas/qst/halfcut.qst:158](../../../areas/qst/halfcut.qst#L158) |
| 27078 | help | [areas/qst/halfcut.qst:167](../../../areas/qst/halfcut.qst#L167) |
| 27080 | goblin gumbling mad | [areas/qst/halfcut.qst:191](../../../areas/qst/halfcut.qst#L191) |
| 27081 | orc ponder pondering something | [areas/qst/halfcut.qst:206](../../../areas/qst/halfcut.qst#L206) |
| 27081 | kill | [areas/qst/halfcut.qst:210](../../../areas/qst/halfcut.qst#L210) |
| 27082 | duergar | [areas/qst/halfcut.qst:224](../../../areas/qst/halfcut.qst#L224) |
| 27083 | drow mad bite | [areas/qst/halfcut.qst:240](../../../areas/qst/halfcut.qst#L240) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 27009 | `crossbow_ambusher` | [src/specs/specs.assign.c:300](../../../src/specs/specs.assign.c#L300) |

## Reset coverage

413 parsed reset commands: D: 42, E: 23, G: 19, M: 309, O: 13, P: 7.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
