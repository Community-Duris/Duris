# Tiamat: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence tiamat \
  --evidence-format markdown --output docs/reference/zone-story-audits/tiamat.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 19616 | `give=I:19608,I:19609,I:19610;receive=I:19603;disappear=0` | request: Assemble the ruby-encrusted key | [areas/qst/tiamat.qst:19](../../../areas/qst/tiamat.qst#L19) |
| 19616 | `give=I:19616,I:19617;receive=I:19602;disappear=0` | request: Assemble the bright key | [areas/qst/tiamat.qst:14](../../../areas/qst/tiamat.qst#L14) |
| 19616 | `give=I:19618,I:19619;receive=I:19604;disappear=0` | request: Assemble the golden key | [areas/qst/tiamat.qst:25](../../../areas/qst/tiamat.qst#L25) |
| 19616 | `give=I:19621,I:19622,I:19623;receive=I:19605;disappear=0` | request: Assemble the jeweled key | [areas/qst/tiamat.qst:30](../../../areas/qst/tiamat.qst#L30) |
| 19616 | `give=I:19626,I:19627;receive=I:19611;disappear=0` | request: Assemble the bronze key | [areas/qst/tiamat.qst:36](../../../areas/qst/tiamat.qst#L36) |
| 19616 | `give=I:19629,I:19631,I:19632;receive=I:19612;disappear=0` | request: Assemble the red key | [areas/qst/tiamat.qst:41](../../../areas/qst/tiamat.qst#L41) |
| 19616 | `give=I:19633,I:19635,I:19636;receive=I:19613;disappear=0` | request: Assemble the green key | [areas/qst/tiamat.qst:47](../../../areas/qst/tiamat.qst#L47) |
| 19616 | `give=I:19641;receive=I:19639,I:19640;disappear=0` | request: Fashion a tribute from Tiamat remains | [areas/qst/tiamat.qst:53](../../../areas/qst/tiamat.qst#L53) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 19616 | key fragment item something for me | [areas/qst/tiamat.qst:2](../../../areas/qst/tiamat.qst#L2) |
| 19616 | tiamat | [areas/qst/tiamat.qst:6](../../../areas/qst/tiamat.qst#L6) |
| 19617 | torment beg end | [areas/qst/tiamat.qst:60](../../../areas/qst/tiamat.qst#L60) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 19617 | `tiamat_human_to_rareloads` | [src/specs/specs.assign.c:280](../../../src/specs/specs.assign.c#L280) |
| mob | 19600 | `block_dir` | [src/specs/specs.assign.c:1037](../../../src/specs/specs.assign.c#L1037) |
| obj | 19638 | `zion_shield_absorb_proc` | [src/specs/specs.assign.c:2169](../../../src/specs/specs.assign.c#L2169) |

## Reset coverage

90 parsed reset commands: D: 20, F: 13, G: 27, M: 23, O: 7.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
