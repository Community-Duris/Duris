# Zalkapfaan, City of the Headless Horde: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence headless \
  --evidence-format markdown --output docs/reference/zone-story-audits/headless.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 2749 | `give=I:2778;receive=I:2782;disappear=0` | request: Finish the engineer's clockwork project | [areas/qst/headless.qst:11](../../../areas/qst/headless.qst#L11) |
| 2755 | `give=I:2735;receive=I:2786;disappear=0` | request: Return the ears of Murf | [areas/qst/headless.qst:25](../../../areas/qst/headless.qst#L25) |
| 2758 | `give=I:2764,I:2765;receive=I:2784;disappear=0` | request: Requisition: sea-serpent armor | [areas/qst/headless.qst:42](../../../areas/qst/headless.qst#L42) |
| 2758 | `give=I:2764,I:2771;receive=I:2783;disappear=0` | request: Requisition: xorn-hide vest | [areas/qst/headless.qst:34](../../../areas/qst/headless.qst#L34) |
| 2758 | `give=I:2764,I:2777;receive=I:2785;disappear=0` | request: Requisition: queen-xorn armor | [areas/qst/headless.qst:50](../../../areas/qst/headless.qst#L50) |
| 2777 | `give=I:2747;receive=I:2750;disappear=1` | request: Malra: the mithril access key | [areas/qst/headless.qst:59](../../../areas/qst/headless.qst#L59) |
| 2778 | `give=I:2748;receive=I:2749;disappear=1` | request: Kaan: the iron access key | [areas/qst/headless.qst:73](../../../areas/qst/headless.qst#L73) |
| 2787 | `give=I:2747,I:2748;receive=I:2780;disappear=1` | request: Zekrallin: the two-token piety return | [areas/qst/headless.qst:93](../../../areas/qst/headless.qst#L93) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 2749 | project | [areas/qst/headless.qst:2](../../../areas/qst/headless.qst#L2) |
| 2787 | piety | [areas/qst/headless.qst:87](../../../areas/qst/headless.qst#L87) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

227 parsed reset commands: D: 34, E: 72, F: 4, G: 11, M: 100, O: 5, P: 1.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
