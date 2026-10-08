# The BrimStone Forge: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence brimeforge \
  --evidence-format markdown --output docs/reference/zone-story-audits/brimeforge.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 131009 | `give=I:131002,I:131003,I:131004;receive=E:300000,I:131005,I:131006;disappear=0` | request: Bring the three lockets to the traveling efreeti | [areas/qst/brimeforge.qst:8](../../../areas/qst/brimeforge.qst#L8) |
| 131011 | `give=I:131019;receive=I:131012;disappear=0` | request: Bring the ancient arcane rune to the ethereal guardian | [areas/qst/brimeforge.qst:18](../../../areas/qst/brimeforge.qst#L18) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 131009 | hi hello hey howdy | [areas/qst/brimeforge.qst:2](../../../areas/qst/brimeforge.qst#L2) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

120 parsed reset commands: D: 24, E: 22, F: 3, G: 4, M: 53, O: 14.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
