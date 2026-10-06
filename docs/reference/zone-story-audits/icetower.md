# Ice Tower: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence icetower \
  --evidence-format markdown --output docs/reference/zone-story-audits/icetower.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 31800 | `give=I:31821;receive=I:31822;disappear=0` | request: Return the necklace to the Utuva nomad | [areas/qst/icetower.qst:15](../../../areas/qst/icetower.qst#L15) |
| 31823 | `give=I:31815;receive=E:30000,I:31816;disappear=0` | request: Deliver the wedding ring to the husband | [areas/qst/icetower.qst:34](../../../areas/qst/icetower.qst#L34) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 31800 | tribe utuva clan angry | [areas/qst/icetower.qst:2](../../../areas/qst/icetower.qst#L2) |
| 31800 | help mage sorceress | [areas/qst/icetower.qst:9](../../../areas/qst/icetower.qst#L9) |
| 31822 | husband | [areas/qst/icetower.qst:24](../../../areas/qst/icetower.qst#L24) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

236 parsed reset commands: D: 30, E: 33, F: 2, G: 7, M: 164.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
