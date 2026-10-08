# Braddistock Mansion: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence brad \
  --evidence-format markdown --output docs/reference/zone-story-audits/brad.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 134146 | `give=I:134105;receive=I:134106;disappear=1` | story: Break Azlion's binding circle | [areas/qst/lortower.qst:201](../../../areas/qst/lortower.qst#L201) |
| 134150 | `give=I:134006;receive=E:100000;disappear=0` | story: Tell Jenifer Joseph's fate | [areas/qst/lortower.qst:218](../../../areas/qst/lortower.qst#L218) |
| 134162 | `give=I:134131,I:134132,I:134133,I:134134,I:134135;receive=I:134125;disappear=1` | story: Assemble Darrin's Star Key | [areas/qst/lortower.qst:266](../../../areas/qst/lortower.qst#L266) |
| 134167 | `give=I:134144;receive=I:134145;disappear=0` | story: Carry Isabia's ring to Danthas | [areas/qst/lortower.qst:292](../../../areas/qst/lortower.qst#L292) |
| 134169 | `give=I:134048;receive=I:134144;disappear=0` | story: Bring Isabia the Cardinal's key | [areas/qst/lortower.qst:304](../../../areas/qst/lortower.qst#L304) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 134146 | urian shadowborn shadow ritual | [areas/qst/lortower.qst:174](../../../areas/qst/lortower.qst#L174) |
| 134146 | sargon darkness master lord evil | [areas/qst/lortower.qst:186](../../../areas/qst/lortower.qst#L186) |
| 134146 | power circle freedom favour | [areas/qst/lortower.qst:194](../../../areas/qst/lortower.qst#L194) |
| 134162 | urian shadowborn master | [areas/qst/lortower.qst:230](../../../areas/qst/lortower.qst#L230) |
| 134162 | tower black shadow | [areas/qst/lortower.qst:237](../../../areas/qst/lortower.qst#L237) |
| 134162 | sargon god negative evil | [areas/qst/lortower.qst:247](../../../areas/qst/lortower.qst#L247) |
| 134162 | key stone keystone star | [areas/qst/lortower.qst:256](../../../areas/qst/lortower.qst#L256) |
| 134167 | noble isabia elf | [areas/qst/lortower.qst:286](../../../areas/qst/lortower.qst#L286) |
| 134169 | key prison | [areas/qst/lortower.qst:299](../../../areas/qst/lortower.qst#L299) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 135014 | `braddistock` | [src/specs/specs.assign.c:343](../../../src/specs/specs.assign.c#L343) |

## Reset coverage

175 parsed reset commands: D: 10, E: 17, M: 48, O: 74, P: 26.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
