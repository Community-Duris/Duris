# The Trakkia Mountains: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence trakkia \
  --evidence-format markdown --output docs/reference/zone-story-audits/trakkia.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 57002 | `give=I:57024;receive=I:57038;disappear=0` | request: Return the ancestral signet | [areas/qst/trakkia.qst:2](../../../areas/qst/trakkia.qst#L2) |
| 57003 | `give=I:57054;receive=I:32019,I:55323;disappear=0` | request: Return Aspuru's tormented soul | [areas/qst/trakkia.qst:24](../../../areas/qst/trakkia.qst#L24) |
| 57036 | `give=I:57044;receive=I:57053;disappear=0` | request: Bring news of the lost flock | [areas/qst/trakkia.qst:41](../../../areas/qst/trakkia.qst#L41) |
| 57058 | `give=I:57040,I:57040,I:57040,I:57040;receive=I:57041;disappear=1` | request: Gather four grangle roots | [areas/qst/trakkia.qst:52](../../../areas/qst/trakkia.qst#L52) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 57003 | hi hello | [areas/qst/trakkia.qst:12](../../../areas/qst/trakkia.qst#L12) |
| 57003 | khan aspuru | [areas/qst/trakkia.qst:18](../../../areas/qst/trakkia.qst#L18) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| room | 57068 | `pet_shops` | [src/specs/specs.assign.c:2594](../../../src/specs/specs.assign.c#L2594) |

## Reset coverage

333 parsed reset commands: D: 20, E: 12, F: 7, G: 12, M: 218, O: 53, P: 11.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
