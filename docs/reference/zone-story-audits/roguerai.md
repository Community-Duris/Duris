# Rogue Plains: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence roguerai \
  --evidence-format markdown --output docs/reference/zone-story-audits/roguerai.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 75832 | `give=I:75847;receive=I:75846;disappear=0` | story: The mediator's lost medal | [areas/qst/roguerai.qst:16](../../../areas/qst/roguerai.qst#L16) |
| 75832 | `give=I:75852,I:75853;receive=I:75854;disappear=0` | story: Peace between the giants | [areas/qst/roguerai.qst:20](../../../areas/qst/roguerai.qst#L20) |
| 75835 | `give=I:75837,I:75843,I:75844,I:75845;receive=I:75855,I:75855;disappear=1` | story: The wandering orc's meal | [areas/qst/roguerai.qst:29](../../../areas/qst/roguerai.qst#L29) |
| 75837 | `give=I:75832;receive=I:75833;disappear=0` | story: The sage's partial soul | [areas/qst/roguerai.qst:42](../../../areas/qst/roguerai.qst#L42) |
| 75842 | `give=I:75850;receive=I:75852;disappear=0` | story: The cloud giant's promise | [areas/qst/roguerai.qst:50](../../../areas/qst/roguerai.qst#L50) |
| 75842 | `give=I:75851;receive=I:75852;disappear=0` | story: The cloud giant's promise | [areas/qst/roguerai.qst:54](../../../areas/qst/roguerai.qst#L54) |
| 75843 | `give=I:75850;receive=I:75853;disappear=0` | story: The storm giant's promise | [areas/qst/roguerai.qst:60](../../../areas/qst/roguerai.qst#L60) |
| 75843 | `give=I:75851;receive=I:75853;disappear=0` | story: The storm giant's promise | [areas/qst/roguerai.qst:64](../../../areas/qst/roguerai.qst#L64) |
| 75845 | `give=I:75833,I:75836;receive=I:75838;disappear=0` | story: The reaper's ancient magic | [areas/qst/roguerai.qst:80](../../../areas/qst/roguerai.qst#L80) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 75826 | hi hello | [areas/qst/roguerai.qst:2](../../../areas/qst/roguerai.qst#L2) |
| 75845 | hi hello | [areas/qst/roguerai.qst:70](../../../areas/qst/roguerai.qst#L70) |
| 75845 | staff | [areas/qst/roguerai.qst:75](../../../areas/qst/roguerai.qst#L75) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| obj | 75857 | `master_set` | [src/specs/specs.assign.c:1345](../../../src/specs/specs.assign.c#L1345) |

## Reset coverage

223 parsed reset commands: D: 12, E: 28, F: 7, G: 19, M: 127, O: 24, P: 4, R: 2.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
