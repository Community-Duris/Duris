# Drifting Realm: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence dream \
  --evidence-format markdown --output docs/reference/zone-story-audits/dream.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 31310 | `give=I:31311,I:31311,I:31311,I:31311;receive=I:31313;disappear=0` | request: Bring four ethereal soul shards to Qin | [areas/qst/dream.qst:27](../../../areas/qst/dream.qst#L27) |
| 31310 | `give=I:31316,I:31317,I:31318,I:31319,I:31320;receive=I:31315;disappear=0` | request: Bring Qin five different crystal skulls | [areas/qst/dream.qst:46](../../../areas/qst/dream.qst#L46) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 31310 | hi help deliver boundless fantasies fantasy | [areas/qst/dream.qst:2](../../../areas/qst/dream.qst#L2) |
| 31310 | yes quest | [areas/qst/dream.qst:12](../../../areas/qst/dream.qst#L12) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

90 parsed reset commands: D: 18, E: 7, G: 7, M: 50, O: 8.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
