# Valley of Crushk: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence valley_crushk \
  --evidence-format markdown --output docs/reference/zone-story-audits/valley_crushk.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 2038 | `give=I:43143;receive=C:4000;disappear=0` | request: Claim Flazoh’s bandit bounty | [areas/qst/valley_crushk.qst:16](../../../areas/qst/valley_crushk.qst#L16) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 2038 | hi hello | [areas/qst/valley_crushk.qst:6](../../../areas/qst/valley_crushk.qst#L6) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

171 parsed reset commands: D: 20, E: 6, G: 15, M: 128, O: 2.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
