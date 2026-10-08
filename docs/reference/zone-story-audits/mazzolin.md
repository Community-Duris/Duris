# Mazzolin: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence mazzolin \
  --evidence-format markdown --output docs/reference/zone-story-audits/mazzolin.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 21323 | `give=I:21309,I:21310,I:21311,I:21312,I:21313;receive=I:21316,I:21320;disappear=1` | request: Return the five pieces of Aeirayne’s star | [areas/qst/mazzolin.qst:15](../../../areas/qst/mazzolin.qst#L15) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 21323 | hello hi help | [areas/qst/mazzolin.qst:2](../../../areas/qst/mazzolin.qst#L2) |
| 21323 | star | [areas/qst/mazzolin.qst:10](../../../areas/qst/mazzolin.qst#L10) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

232 parsed reset commands: D: 16, E: 58, F: 11, G: 6, M: 134, O: 7.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
