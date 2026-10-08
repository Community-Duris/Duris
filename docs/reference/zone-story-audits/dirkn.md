# Dirk'nspire Stronghold: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence dirkn \
  --evidence-format markdown --output docs/reference/zone-story-audits/dirkn.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 96829 | `give=I:96802;receive=C:2570;disappear=0` | request: Bring Balith the tattered silk-paper | [areas/qst/dirkn.qst:19](../../../areas/qst/dirkn.qst#L19) |
| 96829 | `give=I:96845;receive=I:96843;disappear=0` | request: Bring Balith the sealed defense document | [areas/qst/dirkn.qst:10](../../../areas/qst/dirkn.qst#L10) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 96829 | secrets maps secret map | [areas/qst/dirkn.qst:2](../../../areas/qst/dirkn.qst#L2) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

187 parsed reset commands: D: 44, E: 30, G: 6, M: 69, O: 27, P: 11.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
