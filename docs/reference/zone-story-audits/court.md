# Court of the Muse: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence court \
  --evidence-format markdown --output docs/reference/zone-story-audits/court.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 6706 | `give=I:6722;receive=E:50000,I:6717;disappear=0` | story: Winter: return the perfect snowflake | [areas/qst/court.qst:10](../../../areas/qst/court.qst#L10) |
| 6707 | `give=I:6727;receive=E:50000,I:6716;disappear=0` | story: Summer: recover the sunflower-petal pouch | [areas/qst/court.qst:27](../../../areas/qst/court.qst#L27) |
| 6708 | `give=I:6724;receive=E:50000,I:6714;disappear=0` | story: Spring: bring the perfect dew drop | [areas/qst/court.qst:37](../../../areas/qst/court.qst#L37) |
| 6714 | `give=I:6720;receive=E:50000,I:6715;disappear=0` | story: Autumn: return the undecayed red leaf | [areas/qst/court.qst:53](../../../areas/qst/court.qst#L53) |
| 6716 | `give=I:6734;receive=I:6735;disappear=0` | request: A skipping stone for the imaginary friend | [areas/qst/court.qst:67](../../../areas/qst/court.qst#L67) |
| 6718 | `give=I:6732;receive=I:6733;disappear=0` | story: A lost soul for the banshee | [areas/qst/court.qst:74](../../../areas/qst/court.qst#L74) |
| 6727 | `give=I:6702,I:6702,I:6702,I:6702,I:6702,I:6702,I:6702,I:6702,I:6702,I:6702,I:6702,I:6702;receive=I:6703;disappear=1` | story: Twelve koi scales for the fisherman | [areas/qst/court.qst:87](../../../areas/qst/court.qst#L87) |
| 6729 | `give=I:6714,I:6715,I:6716,I:6717;receive=I:6713;disappear=0` | story: Four distinct seasonal tokens for admission | [areas/qst/court.qst:114](../../../areas/qst/court.qst#L114) |
| 6735 | `give=I:6741;receive=I:6740;disappear=0` | request: Return Larissa's lost diamond wand | [areas/qst/court.qst:126](../../../areas/qst/court.qst#L126) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 6706 | hi quest snowflake favor token frost stolen | [areas/qst/court.qst:2](../../../areas/qst/court.qst#L2) |
| 6707 | quest hi sunflower token favor plotting pouch | [areas/qst/court.qst:20](../../../areas/qst/court.qst#L20) |
| 6714 | quest hi leaf token favor | [areas/qst/court.qst:47](../../../areas/qst/court.qst#L47) |
| 6716 | rocks rock quest | [areas/qst/court.qst:63](../../../areas/qst/court.qst#L63) |
| 6727 | fishing hi quest koi fish | [areas/qst/court.qst:81](../../../areas/qst/court.qst#L81) |
| 6729 | muse tokens token key | [areas/qst/court.qst:108](../../../areas/qst/court.qst#L108) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

209 parsed reset commands: D: 8, E: 23, F: 5, G: 42, M: 122, O: 9.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
