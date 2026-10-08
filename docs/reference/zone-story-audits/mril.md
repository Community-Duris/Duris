# Miaeril Village: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence mril \
  --evidence-format markdown --output docs/reference/zone-story-audits/mril.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 33700 | `give=I:33710;receive=I:33702;disappear=0` | request: Bring the hooded figure a frozen heart | [areas/qst/mril.qst:11](../../../areas/qst/mril.qst#L11) |
| 33701 | `give=I:33710;receive=I:33713;disappear=0` | request: Bring Uduku a frozen heart | [areas/qst/mril.qst:30](../../../areas/qst/mril.qst#L30) |
| 33711 | `give=I:33709,I:33709,I:33709,I:33709,I:33709;receive=I:33711;disappear=0` | request: Bring Falga five salmon | [areas/qst/mril.qst:48](../../../areas/qst/mril.qst#L48) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 33700 | hi forbidden forest unknown yeti frost giant abominable | [areas/qst/mril.qst:2](../../../areas/qst/mril.qst#L2) |
| 33701 | hi forbidden forest unknown yeti frost giant abominable | [areas/qst/mril.qst:20](../../../areas/qst/mril.qst#L20) |
| 33711 | salmon food wul hi | [areas/qst/mril.qst:39](../../../areas/qst/mril.qst#L39) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

87 parsed reset commands: D: 8, E: 11, F: 1, G: 12, M: 55.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
